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
    
    
    Client::Client() : server(std::string("127.0.0.1"), 8086) {
        robotkernel::service_provider_t *sp = new robotkernel::service_provider_t();
        sp->add_service = std::bind(&jsonrpc_bridge::Client::addService, this, _1);
        sp->remove_service = std::bind(&jsonrpc_bridge::Client::removeService, this, _1);
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

//        cliServer.onConnectHandler    = std::bind(&jsonrpc_bridge::Client::onCliConnect, this, _1);
//        cliServer.onDisconnectHandler = std::bind(&jsonrpc_bridge::Client::onCliDisconnect, this, _1);
//        cliServer.start();
    }


    Client::~Client() {
//        cliServer.stop();
    }

    void Client::run() {
        while (running()) {
            server.WaitMessage(1000);
        }
    }
    
    
//    static std::string escape(std::string in) {
//        const int N = 3;
//        const string escapeStrings[N] = {"\n", "\r", "\t"};
//        const string replaceStrings[N] = {"\\n", "\\r", "\\t"};
//
//        for(int i=0; i<N; ++i){
//            in = string_replace(in, escapeStrings[i], replaceStrings[i]);
//        }
//        return in;
//    }
//    
//    
//    rk_type parseArg(string& args, string key, string value, unsigned long* sPos){
//        while(*sPos < args.length() && args[*sPos] == ' '){
//            (*sPos)++;
//        }
//
//        if(*sPos >= args.length()){
//            throw str_exception("Too few arguments: Missing %s", value.c_str());
//        }
//
//        if(key == TYPENAME_STRING) {
//            if(args[*sPos] != '"'){
//                throw str_exception("Parse error for argument: %s -> Strings must be enclosed with quotation marks", value.c_str());
//            }
//            unsigned long strStart = (*sPos)++;
//            bool escape = false;
//            while(*sPos < args.length()){
//                char c = args[*sPos];
//                if (c == '"' && !escape) {
//                    string arg = args.substr(strStart+1, (*sPos)-1);
//                    rk_type rkType(arg);
//                    return rkType;
//                }
//                escape = c == '\\' && !escape;
//                (*sPos)++;
//            }
//        } else {
//            throw str_exception("Unsupported type <%s> (Not implemented yet)", key.c_str());
//        }
//        
//    }
//    
//    
//    void parseArgs(service_t &svc, std::string &args, service_arglist_t& req){
//        YAML::Node message_definition = YAML::Load(svc.service_definition);
//        if (!message_definition["request"]) {
//            return;
//        }
//        
//        const YAML::Node &request = message_definition["request"];
//        unsigned long sPos = 0;
//        for (YAML::const_iterator it = request.begin(); it != request.end(); ++it) {
//            string key   = it->first.as<string>();
//            string value = it->second.as<string>();
//
//            rk_type x = parseArg(args, key, value, &sPos);
//            req.push_back(x);
//        }
//    }
//    
//    
//    service_t* Client::parseRequest(string &msg, service_arglist_t &req) {
//        unsigned long delim = msg.find(" ");
//        std::string param0 = msg.substr(0, delim);
//        ServiceMap::iterator svcIt = services.find(param0);
//        if(svcIt == services.end()){
//            return NULL;
//        }
//        service_t &svc = svcIt->second;
//        
//        string args = (delim == string::npos ? std::string("") : msg.substr(delim+1));
//        parseArgs(svc, args, req);
//        
//        return &svc;
//    }
//    
//    
//    string parseResponse(service_t* svc, service_arglist_t& resp){
//        stringstream response;
//        response << "OK" << endl;
//        
//        YAML::Node message_definition = YAML::Load(svc->service_definition);
//        const YAML::Node &mdResp = message_definition["response"];
//        
//        if (mdResp) {
//            auto mdIt = mdResp.begin();
//            for (auto it = resp.begin(); it != resp.end() && mdIt != mdResp.end(); ++it, ++mdIt) {
//                response << mdIt->second.as<string>() << ": " << it->toString() << endl;
//            }
//        }
//        
//        return response.str();
//    }
//    
//    
//    void Client::onCliMessage(jsonrpc_bridge::CliConnection* c, char* buf, ssize_t len){
//        string msg(buf, (unsigned long) (buf[len-1] == '\n' ? len-1 : len));
//        msg = strip(msg);
//        if(msg.empty()){
//            return;
//        }
//        try {
//            service_arglist_t req;
//            service_t *svc = parseRequest(msg, req);
//            string result;
//            if(!svc){
//                if(msg == string("!list")) {
//                    result += "\n";
//                    for (ServiceMap::iterator it = services.begin(); it != services.end(); it++) {
//                        service_t &s = it->second;
//                        result += std::string("[") + s.name + std::string("]\n") + s.service_definition + "\n";
//                    }
//                } else if(msg == string("!help")) {
//                    result += "\n";
//                    result += "'!help': Print this help.\n";
//                    result += "'!list': Get a list of available services.\n";
//                } else {
//                    result = std::string("ERR: Command or service not found: '") + escape(msg) + string("'\n Use !help to get CLI instructions.\n");
//                }
//            } else {
//                klog(info, "Calling: %s %s", svc->name.c_str(), svc->service_definition.c_str());
//                
//                service_arglist_t resp;
//                if (svc->callback(req, resp) != 0) {
//                    throw str_exception("CliBridge: Internal error in service call. See previous messages in error log for details.");
//                }
//                
//                result = parseResponse(svc, resp);
//            }
//
//            c->write(result.c_str(), result.length());
//        } catch(str_exception e){
//            const string& err = format_string("Exception in service call: %s\n%s\n", msg.c_str(), e.what());
//            klog(warning, "CliBridge: %s", err.c_str());
//            c->write(err.c_str(), err.length());
//        }
//    }
//    
//    
//    void Client::onCliConnect(jsonrpc_bridge::CliConnection* c){
//        c->messageHandler = std::bind(&jsonrpc_bridge::Client::onCliMessage, this, _1, _2, _3);
//    }
//
//
//    void Client::onCliDisconnect(jsonrpc_bridge::CliConnection* c){
//    }
    
    bool Client::on_service(const Json::Value& root, Json::Value& response) {
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

    void Client::addService(const robotkernel::service_t &svc) {
        klog(info, "got service: %s\n", svc.name.c_str());
        services[svc.name] = svc;

        server.AddMethod(new Json::Rpc::RpcMethod<Client>(*this, &Client::on_service,
                    svc.name));

    }


    void Client::removeService(const robotkernel::service_t &svc) {
        services.erase(svc.name);
    }
}

