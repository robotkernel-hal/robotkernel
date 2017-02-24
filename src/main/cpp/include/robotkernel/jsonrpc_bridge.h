#ifndef __ROBOTKERNEL_JSONRPC_BRIDGE_H__
#define __ROBOTKERNEL_JSONRPC_BRIDGE_H__

#include "robotkernel/kernel.h"
#include "robotkernel/service.h"
#include "robotkernel/bridge_interface.h"
#include "robotkernel/runnable.h"

#include "jsonrpc-cpp/jsonrpc.h"

namespace jsonrpc_bridge {

    class Client : robotkernel::CommBridgeInterface, robotkernel::runnable {
    private:
        robotkernel::service_t* parseRequest(std::string &msg, robotkernel::service_arglist_t &req);

    public:
        //! construct jsonrpc_bridge client
        Client();

        //! destruct jsonrpc_bridge client
        ~Client();

        void addService(const robotkernel::service_t &svc);
        void removeService(const robotkernel::service_t &svc);

//        void onCliMessage(jsonrpc_bridge::CliConnection* c, char* msg, ssize_t len);
//        void onCliConnect(jsonrpc_bridge::CliConnection* c);
//        void onCliDisconnect(jsonrpc_bridge::CliConnection* c);

        void run();             //!< handler function called if 

        bool on_service(const Json::Value& root, Json::Value& response);

    private:
        Json::Rpc::TcpServer server;
        //! Server for cli connections
//        jsonrpc_bridge::CliServer cliServer;
        typedef std::map<std::string, robotkernel::service_t> ServiceMap;
        ServiceMap services;
    };

}

#endif // __ROBOTKERNEL_JSONRPC_BRIDGE_H__

