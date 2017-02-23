#include "robotkernel/ln_bridge.h"
#include "robotkernel/helpers.h"
#include "robotkernel/service.h"

#include <functional>
#include <boost/algorithm/string/predicate.hpp>

using namespace std;
using namespace std::placeholders;

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

    robotkernel::service_provider_t *sp = 
        new robotkernel::service_provider_t();
    sp->add_service = std::bind(&ln_bridge::client::addService, this, _1);
    sp->remove_service = 
        std::bind(&ln_bridge::client::removeService, this, _1);

    robotkernel::kernel::get_instance()->add_service_provider(sp);
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
void ln_bridge::client::addService(const robotkernel::service_t& svc) {
    ln_bridge::service *ln_svc = new ln_bridge::service(*this, svc);
    service_map[svc.name] = ln_svc;
}

//! unregister and remove ln service 
/*!
 * \param svc robotkernel service struct
 */
void ln_bridge::client::removeService(
        const robotkernel::service_t& svc) {
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
        const robotkernel::service_t& svc) 
    : _clnt(clnt), _svc(svc) {
    _create_ln_message_defition(); 
        
    // create service name
    string svc_name = clnt.clnt->name + "." + _svc.name;

    // put ln message definition. this will create 
    // ~/ln_message_definitions/gen/<svc_name>
    for (map<string, string>::iterator it = sub_mds.begin(); 
            it != sub_mds.end(); ++it) {
        clnt.clnt->put_message_definition(it->first, it->second);
    }

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

typedef struct __attribute__((__packed__)) ln_vector {
    uint32_t len;
    const uint8_t *val;
} __attribute__((__packed__)) ln_vector_t;
        
int ln_bridge::service::handle(ln::service_request& req) {
    uint8_t svc[1024];
    req.set_data(&svc[0], signature.c_str());
    uint64_t adr = (uint64_t)&svc[0];

    // request arguments
    robotkernel::service_arglist_t service_request;

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
    robotkernel::service_arglist_t service_response;
    _svc.callback(service_request, service_response);

    std::list<uint8_t *> to_delete;

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
                const string& tmp_string = service_response[i++];
                ((uint32_t *)adr)[0] = (uint32_t)tmp_string.size();
                adr += 4;
                if (tmp_string.size())
                    ((const char **)adr)[0] = (const char *)tmp_string.c_str();
                else 
                    ((const char **)adr)[0] = NULL;
                adr += sizeof(char *);
            } else if (boost::starts_with(key, "vector")) {
                const size_t equals_idx = key.find_first_of('/');
                if (std::string::npos != equals_idx)
                {
                    //signature "uint32_t 4 1,[uint32_t 4 1,char* 1 1]* 8 1|uint32_t 4 1,[uint32_t 4 1,char* 1 1]* 8 1"

                    string vector = key.substr(0, equals_idx);
                    string real_key = key.substr(equals_idx + 1);
                    string ln_dt = service_datatype_to_ln(real_key);
        
                    const std::vector<rk_type> t = service_response[i++];

#define add_vector_type(type) \
    if (ln_dt == #type) {                                                                       \
        ((uint32_t *)adr)[0] = (uint32_t)t.size();                                              \
        adr += 4;                                                                               \
                                                                                                \
        for (unsigned i = 0; i < t.size(); ++i) {                                               \
            ((type *)adr)[0] = (type)t[i];                                                      \
            adr += ln_dt_size;                                                                  \
        }                                                                                       \
    }

#define add_vector_type_char(type) \
    if (ln_dt == #type) {                                                                       \
        ((uint32_t *)adr)[0] = (uint32_t)t.size();                                              \
        adr += 4;                                                                               \
                                                                                                \
        ln_vector_t* entries = new ln_vector_t[t.size()];                                       \
        to_delete.push_back((uint8_t *)entries);                                                \
                                                                                                \
        for (unsigned i = 0; i < t.size(); ++i) {                                               \
            entries[i].len = t[i].length();                                                     \
            entries[i].val = (const uint8_t *)t[i].c_str();                                     \
        }                                                                                       \
        ((ln_vector_t **)adr)[0] = entries;                                                     \
        adr += sizeof(void*);                                                                   \
    }

                    add_vector_type(uint64_t*);
                    add_vector_type(int64_t*);
                    add_vector_type(uint32_t*);
                    add_vector_type(int32_t*);
                    add_vector_type(uint16_t*);
                    add_vector_type(int16_t*);
                    add_vector_type(uint8_t*);
                    add_vector_type(int8_t*);
                    add_vector_type_char(char*);
                }
            } else if (ends_with(ln_dt, string("*"))) {
                ((uint32_t *)adr)[0] = service_response[i++];
                adr += 4;

#define push_back_type(type) \
                if (ln_dt == #type) {                                               \
                    ((type*)adr)[0] = service_response[i++]; \
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
    
    for (std::list<uint8_t *>::iterator it = to_delete.begin();
            it != to_delete.end(); ++it) {
        delete[] (*it);
    }

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
            if (boost::starts_with(key, "vector")) {
                printf("got one vector\n");
    
                const size_t equals_idx = key.find_first_of('/');
                if (std::string::npos != equals_idx)
                {
//signature "uint32_t 4 1,[uint32_t 4 1,char* 1 1]* 8 1|uint32_t 4 1,[uint32_t 4 1,char* 1 1]* 8 1"

                    string vector = key.substr(0, equals_idx);
                    string real_key = key.substr(equals_idx + 1);
                    cout << "vector: " << vector << ", datatype: " << real_key << endl;
                    
                    ss_signature << "uint32_t 4 1,[";

                    string ln_dt = service_datatype_to_ln(real_key);
                    int ln_dt_size = ln_datatype_size(ln_dt);

                    stringstream ss_sub_md;
                    ss_sub_md << ln_dt << " " << real_key << endl;
                    sub_mds[key] = ss_sub_md.str();

                    ss_md << "define " << key << " as \"gen/" << key << "\"" << endl;
                    ss_md << key << "* " << value << endl;
                    
                    if (ends_with(ln_dt, string("*")))
                        ss_signature << "uint32_t 4 1,";

                    ss_signature << ln_dt << " " << ln_dt_size << " " << "1";

                    ss_signature << "]* " << sizeof(void*) << " 1";

                }
                else
                {
                    //name = name_value;
                    cout << "error after vector" << endl;
                }
            } else {
                string ln_dt = service_datatype_to_ln(key);
                int ln_dt_size = ln_datatype_size(ln_dt);
                ss_md << ln_dt << " " << value << endl;

                if (ends_with(ln_dt, string("*")))
                    ss_signature << "uint32_t 4 1,";

                ss_signature << ln_dt << " " << ln_dt_size << " " << "1";
            }
        }
    }

    cout << "sig: " << ss_signature.str() << endl << "md: " << ss_md.str() << endl;
    signature = ss_signature.str();
    md = ss_md.str();
}

