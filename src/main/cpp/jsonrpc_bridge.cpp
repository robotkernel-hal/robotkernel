#include "robotkernel/jsonrpc_bridge.h"
#include "robotkernel/rk_type.h"
#include "robotkernel/helpers.h"
#include "robotkernel/service.h"

#include <functional>
#include <algorithm> 
#include <functional> 
#include <cctype>
#include "string_util/string_util.h"
#include <locale>

using namespace std;
using namespace std::placeholders;
using namespace robotkernel;



namespace jsonrpc_bridge {
    
    const string TYPENAME_STRING = string("string");
    const string TYPENAME_INT8   = string("int8_t");
    const string TYPENAME_INT16  = string("int16_t");
    const string TYPENAME_INT32  = string("int32_t");
    const string TYPENAME_INT64  = string("int64_t");
    const string TYPENAME_UINT8  = string("uint8_t");
    const string TYPENAME_UINT16 = string("uint16_t");
    const string TYPENAME_UINT32 = string("uint32_t");
    const string TYPENAME_UINT64 = string("uint64_t");
    const string TYPENAME_FLOAT  = string("float");
    const string TYPENAME_DOUBLE = string("double");


    static int getPort(){
        char *portStr = getenv("RK_jsonrpc_PORT");
        return portStr ? atoi(portStr) : 5094;
    }
    
    
    client::client() : server(std::string("127.0.0.1"), 8086) {
        robotkernel::service_provider_t *sp = new robotkernel::service_provider_t();
        sp->add_service = std::bind(&jsonrpc_bridge::client::add_service, this, _1);
        sp->remove_service = std::bind(&jsonrpc_bridge::client::remove_service, this, _1);
        robotkernel::kernel::get_instance()->add_service_provider(sp);

        if (!networking::init()) {
            throw str_exception("jsonrpc: networking initialization failed");
        }

        if (!server.Bind()) {
            throw str_exception("jsonrpc: bind failed");
        }

        if (!server.Listen()) {
            throw str_exception("listen failed");
        }

        start();
    }


    client::~client() {
        stop();
    }

    void client::run() {
        while (running()) {
            server.WaitMessage(1000);
        }
    }
    
    bool client::on_service(const Json::Value& root, Json::Value& response) {
        std::cout << "Notification: " << root << std::endl;
        response = Json::Value::nullRef;

        response["jsonrpc"] = "2.0";
        response["id"] = root["id"];
        response["result"] = "success";

        std::string method = root["method"].asString();

        if (services.find(method) != services.end()) {
            service_t *svc = &services[method];
            service_arglist_t req;
    
            YAML::Node message_definition = YAML::Load(svc->service_definition);
            if (message_definition["request"]) {
                const YAML::Node& request = message_definition["request"];

                for (YAML::const_iterator it = request.begin(); 
                        it != request.end(); ++it) {
                    string key   = it->first.as<string>();
                    string value = it->second.as<string>();

                    if (key == "string")
                        req.push_back(root[value].asString());
                    else if (key == "uint8_t")
                        req.push_back((uint8_t)root[value].asUInt());
                    else if (key == "uint16_t")
                        req.push_back((uint16_t)root[value].asUInt());
                    else if (key == "uint32_t")
                        req.push_back((uint32_t)root[value].asUInt());
                    else if (key == "uint64_t")
                        req.push_back((uint64_t)root[value].asUInt64());
                    else if (key == "int8_t")
                        req.push_back((int8_t)root[value].asInt());
                    else if (key == "int16_t")
                        req.push_back((int16_t)root[value].asInt());
                    else if (key == "int32_t")
                        req.push_back((int32_t)root[value].asInt());
                    else if (key == "int64_t")
                        req.push_back((int64_t)root[value].asInt64());
                }
            }

            service_arglist_t resp;
            
            if (svc->callback(req, resp) != 0) {
            }
    
            if (message_definition["response"]) {
                const YAML::Node& md_response = message_definition["response"];
                int i = 0;

                for (YAML::const_iterator it = md_response.begin(); 
                        it != md_response.end(); ++it) {
                    string key   = it->first.as<string>();
                    string value = it->second.as<string>();
                    
                    if (key == "string") {
                        const string& tmp_string = resp[i++];
                        response[value] = tmp_string;
                    } else if (key == "uint8_t")
                        response[value] = (uint8_t)(resp[i++]);
                    else if (key == "uint16_t")
                        response[value] = (uint16_t)(resp[i++]);
                    else if (key == "uint32_t")
                        response[value] = (uint32_t)(resp[i++]);
                    else if (key == "uint64_t")
                        response[value] = (Json::UInt64)(resp[i++]);
                    else if (key == "int8_t")
                        response[value] = (int8_t)(resp[i++]);
                    else if (key == "int16_t")
                        response[value] = (int16_t)(resp[i++]);
                    else if (key == "int32_t")
                        response[value] = (int32_t)(resp[i++]);
                    else if (key == "int64_t")
                        response[value] = (Json::Int64)(resp[i++]);
                }
            }
        }

        return true;
    }

    void client::add_service(const robotkernel::service_t &svc) {
        klog(info, "jsonrpc_bridge: got service: %s\n", svc.name.c_str());
        services[svc.name] = svc;

        Json::Value root;
        Json::Value req, resp;
        YAML::Node message_definition = YAML::Load(svc.service_definition);
        
        if (message_definition["request"]) {
            const YAML::Node& request = message_definition["request"];

            for (YAML::const_iterator it = request.begin(); 
                    it != request.end(); ++it) {
                string key   = it->first.as<string>();
                string value = it->second.as<string>();

                req[value] = key;
            }
        }

        if (message_definition["response"]) {
            const YAML::Node& response = message_definition["response"];

            for (YAML::const_iterator it = response.begin(); 
                    it != response.end(); ++it) {
                string key   = it->first.as<string>();
                string value = it->second.as<string>();

                resp[value] = key;
            }
        }

        root["request"] = req;
        root["response"] = resp;
         
        server.AddMethod(new Json::Rpc::RpcMethod<client>(*this, &client::on_service,
                    svc.name, root));
    }


    void client::remove_service(const robotkernel::service_t &svc) {
        server.DeleteMethod(svc.name);
        services.erase(svc.name);
    }
}

