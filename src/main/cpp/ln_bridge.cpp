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
        
//! construct ln_bridge client
ln_bridge::client::client() {
    char *argv[] = { 
        (char *)"test", (char *)"-ln_manager", (char *)"rmc-lx0141:54411" };
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

    YAML::Node message;
    message["request"] = YAML::Node();

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
                uint32_t str_len = ((uint32_t *)adr)[0];
                adr += 4;
                char *str_adr = ((char **)adr)[0];
                adr += sizeof(char *);

                message["request"][value] = string(str_adr, str_len);
            } else {
                if (ln_dt == "uint32_t") 
                    message["request"][value] = ((uint32_t *)adr)[0];
                if (ln_dt == "int32_t") 
                    message["request"][value] = ((int32_t *)adr)[0];
                if (ln_dt == "uint16_t") 
                    message["request"][value] = ((uint16_t *)adr)[0];
                if (ln_dt == "int16_t") 
                    message["request"][value] = ((int16_t *)adr)[0];
                if (ln_dt == "uint8_t") 
                    message["request"][value] = ((uint8_t *)adr)[0];
                if (ln_dt == "int8_t") 
                    message["request"][value] = ((int8_t *)adr)[0];
                adr += ln_dt_size;
            }
        }
    }

    // call robotkernel service
    _svc.callback(message);

    if (message_definition["response"]) {
        const YAML::Node& response = message_definition["response"];

        for (YAML::const_iterator it = response.begin(); 
                it != response.end(); ++it) {

            string key   = it->first.as<string>();
            string value = it->second.as<string>();

            string ln_dt = service_datatype_to_ln(key);
            //int ln_dt_size = ln_datatype_size(ln_dt);

            if (ln_dt == "char*") {
                ((uint32_t *)adr)[0] = get_as<string>(message["response"], 
                    value).size();

                adr += 4;
                ((const char **)adr)[0] = get_as<string>(message["response"], 
                    value).c_str();

                adr += sizeof(char *);
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

            if (ln_dt == "char*")
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

            if (ln_dt == "char*")
                ss_signature << "uint32_t 4 1,";

            ss_signature << ln_dt << " " << ln_dt_size << " " << "1";
        }
    }

    signature = ss_signature.str();
    md = ss_md.str();
}
