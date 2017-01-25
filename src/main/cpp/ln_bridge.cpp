#include "robotkernel/ln_bridge.h"
#include "robotkernel/helpers.h"

using namespace std;

static string service_datatype_to_ln(string datatype) {
    if (datatype == "string")
        return string("char*");

    return datatype;
}

static std::pair<std::string, int> ln_datatype_size_data[] = {
    std::make_pair("int64_t", 8),
    std::make_pair("uint64_t", 8),
    std::make_pair("int32_t", 4),
    std::make_pair("uint32_t", 4),
    std::make_pair("int16_t", 2),
    std::make_pair("uint16_t", 2),
    std::make_pair("int8_t", 1),
    std::make_pair("uint8_t", 1),
    std::make_pair("int64_t*", 8),
    std::make_pair("uint64_t*", 8),
    std::make_pair("int32_t*", 4),
    std::make_pair("uint32_t*", 4),
    std::make_pair("int16_t*", 2),
    std::make_pair("uint16_t*", 2),
    std::make_pair("int8_t*", 1),
    std::make_pair("uint8_t*", 1),
    std::make_pair("char*", 1)
};

static std::map<std::string, int> ln_datatype_size_map(
        ln_datatype_size_data,
        ln_datatype_size_data + sizeof(ln_datatype_size_data) / 
        sizeof(ln_datatype_size_data[0]));

static int ln_datatype_size(const std::string& ln_datatype) {
    if (ln_datatype_size_map.find(ln_datatype) != 
                ln_datatype_size_map.end())
        return ln_datatype_size_map[ln_datatype];

    return 0;
}

static bool ends_with(const string& a, const string& b) {
    if (b.size() > a.size()) return false;
    return std::equal(a.begin() + a.size() - b.size(), a.end(), b.begin());
}
        
//! construct ln_bridge client
ln_bridge::client::client() {
    char *argv[] = { 
        (char *)"test", (char *)"-ln_manager", (char *)"192.168.131.1:54411" };
    clnt = new ln::client("hallo", 3, argv);
    clnt->handle_service_group_in_thread_pool(NULL, "main");
    clnt->set_max_threads("main", 2);

    robotkernel::kernel::service_provider_t *sp = 
        new robotkernel::kernel::service_provider_t();
    sp->add_service = boost::bind(&ln_bridge::client::add_service, this, _1);
    sp->remove_service = 
        boost::bind(&ln_bridge::client::remove_service, this, _1);

    robotkernel::kernel::get_instance()->service_providers.push_back(sp);
}

//! destruct ln_bridge client
ln_bridge::client::~client() {
    service_map_t::iterator it;

    for (it = service_map.begin(); it != service_map.end(); ++it)
        delete it->second;

    service_map.clear();
}

//! create and register ln service
/*!
 * \param svc robotkernel service struct
 */
void ln_bridge::client::add_service(const robotkernel::kernel::service_t& svc) {
    ln_bridge::service *ln_svc = new ln_bridge::service(*this, svc);
    service_map[svc.name] = ln_svc;
}

//! unregister and remove ln service 
/*!
 * \param svc robotkernel service struct
 */
void ln_bridge::client::remove_service(
        const robotkernel::kernel::service_t& svc) {
    service_map_t::iterator it;

    if ((it = service_map.find(svc.name)) != service_map.end()) {
        ln_bridge::service *ln_svc = it->second;
        service_map.erase(it);
        delete ln_svc;
    }
}

//! construct ln_bridge service
/*!
 * \param clnt ln_bridge client
 * \param svc robotkernel service
 */
ln_bridge::service::service(ln_bridge::client& clnt, 
        const robotkernel::kernel::service_t& svc) 
    : _clnt(clnt), _svc(svc) {
    _create_ln_message_defition(); 
        
    // create service name
    string svc_name = clnt.clnt->name + "." + _svc.name;

    // put ln message definition. this will create 
    // ~/ln_message_definitions/gen/<svc_name>
    clnt.clnt->put_message_definition(svc_name, md);

    // get ln service provider
    _ln_service = clnt.clnt->get_service_provider(
            svc_name, string("gen/" + svc_name), signature);

    // set handler and register
    _ln_service->set_handler(&ln_bridge::service::service_cb, this);
    _ln_service->do_register(NULL);
}; 
        
//! destruct ln_bridge service
ln_bridge::service::~service() {
    if (_ln_service) {
		_clnt.clnt->unregister_service_provider(_ln_service);
        _ln_service = NULL;
    }
}
        
int ln_bridge::service::handle(ln::service_request& req) {
    uint8_t svc[1024];
    req.set_data(&svc[0], signature.c_str());
    uint64_t adr = (uint64_t)&svc[0];

    // request arguments
    robotkernel::kernel::service_arglist_t service_request;

    YAML::Node message_definition = YAML::Load(_svc.service_definition);
    if (message_definition["request"]) {
        const YAML::Node& request = message_definition["request"];

        for (YAML::const_iterator it = request.begin(); 
                it != request.end(); ++it) {
            string key   = it->first.as<string>();
            string value = it->second.as<string>();

            string ln_dt = service_datatype_to_ln(key);
            int ln_dt_size = ln_datatype_size(ln_dt);

            if (ln_dt == "char*") {
                uint32_t tmp_len = ((uint32_t *)adr)[0];
                adr += 4;
                char *tmp_adr = ((char **)adr)[0];
                adr += sizeof(char*);
                
                service_request.push_back(string(tmp_adr, tmp_len));
            } else if (ends_with(ln_dt, string("*"))) {               
                service_request.push_back(((uint32_t *)adr)[0]);    //<! array length
                adr += 4;                
#define push_back_type(type) \
                if (ln_dt == #type) {                               \
                    service_request.push_back(((type*)adr)[0]);     \
                    adr += sizeof(type);                            \
                }

                push_back_type(uint64_t*);
                push_back_type(int64_t*);
                push_back_type(uint32_t*);
                push_back_type(int32_t*);
                push_back_type(uint16_t*);
                push_back_type(int16_t*);
                push_back_type(uint8_t*);
                push_back_type(int8_t*);
            } else {
                push_back_type(uint64_t);
                push_back_type(int64_t);
                push_back_type(uint32_t);
                push_back_type(int32_t);
                push_back_type(uint16_t);
                push_back_type(int16_t);
                push_back_type(uint8_t);
                push_back_type(int8_t);
#undef push_back_type
            }
        }
    }

    // call robotkernel service
    robotkernel::kernel::service_arglist_t service_response;
    _svc.callback(service_request, service_response);

    if (message_definition["response"]) {
        const YAML::Node& response = message_definition["response"];
        int i = 0;

        for (YAML::const_iterator it = response.begin(); 
                it != response.end(); ++it) {
            string key   = it->first.as<string>();
            string value = it->second.as<string>();

            string ln_dt = service_datatype_to_ln(key);
            int ln_dt_size = ln_datatype_size(ln_dt);

            if (ln_dt == "char*") {
                const string& tmp_string = boost::any_cast<string>(service_response[i++]);
                ((uint32_t *)adr)[0] = (uint32_t)tmp_string.size();
                adr += 4;
                if (tmp_string.size())
                    ((const char **)adr)[0] = (const char *)tmp_string.c_str();
                else 
                    ((const char **)adr)[0] = NULL;
                adr += sizeof(char *);
            } else if (ends_with(ln_dt, string("*"))) {
                ((uint32_t *)adr)[0] = boost::any_cast<uint32_t>(service_response[i++]);
                adr += 4;

#define push_back_type(type) \
                if (ln_dt == #type) {                                               \
                    ((type*)adr)[0] = boost::any_cast<type>(service_response[i++]); \
                    adr += sizeof(type);                                            \
                }

                push_back_type(uint64_t*);
                push_back_type(int64_t*);
                push_back_type(uint32_t*);
                push_back_type(int32_t*);
                push_back_type(uint16_t*);
                push_back_type(int16_t*);
                push_back_type(uint8_t*);
                push_back_type(int8_t*);
            } else {
                push_back_type(uint64_t);
                push_back_type(int64_t);
                push_back_type(uint32_t);
                push_back_type(int32_t);
                push_back_type(uint16_t);
                push_back_type(int16_t);
                push_back_type(uint8_t);
                push_back_type(int8_t);
            }
        }
    }

    req.respond();

    return 0;
}

void ln_bridge::service::_create_ln_message_defition() {
    std::stringstream ss_md, ss_signature;
    YAML::Node message_definition = YAML::Load(_svc.service_definition);
    ss_md << "service" << endl;

    if (message_definition["request"]) {
        ss_md << "request" << endl;

        const YAML::Node& request = message_definition["request"];
        for (YAML::const_iterator it = request.begin(); 
                it != request.end(); ++it) {
            if (it != request.begin())
                ss_signature << ",";

            string key   = it->first.as<string>();
            string value = it->second.as<string>();

            string ln_dt = service_datatype_to_ln(key);
            int ln_dt_size = ln_datatype_size(ln_dt);
            ss_md << ln_dt << " " << value << endl;

            if (ends_with(ln_dt, string("*")))
                ss_signature << "uint32_t 4 1,";

            ss_signature << ln_dt << " " << ln_dt_size << " " << "1";
        }
    }

    ss_signature << "|";

    if (message_definition["response"]) {
        ss_md << "response" << endl;

        const YAML::Node& response = message_definition["response"];
        for (YAML::const_iterator it = response.begin(); 
                it != response.end(); ++it) {
            if (it != response.begin())
                ss_signature << ",";

            string key   = it->first.as<string>();
            string value = it->second.as<string>();

            string ln_dt = service_datatype_to_ln(key);
            int ln_dt_size = ln_datatype_size(ln_dt);
            ss_md << ln_dt << " " << value << endl;

            if (ends_with(ln_dt, string("*")))
                ss_signature << "uint32_t 4 1,";

            ss_signature << ln_dt << " " << ln_dt_size << " " << "1";
        }
    }

    signature = ss_signature.str();
    md = ss_md.str();
}

