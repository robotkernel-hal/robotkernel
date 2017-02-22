#include "robotkernel/kernel.h"
#include "robotkernel/service.h"
#include "cli_server.h"

#ifndef ROBOTKERNEL_CLI_BRIDGE_H
#define ROBOTKERNEL_CLI_BRIDGE_H


#include "robotkernel/bridge_interface.h"
#include "robotkernel/rk_type.h"

namespace cli_bridge {
    
    class Client : robotkernel::CommBridgeInterface{
    private:
        robotkernel::service_t* parseRequest(std::string &msg, robotkernel::service_arglist_t &req);
        void parseArgs(robotkernel::service_t &svc, std::string &args, robotkernel::service_arglist_t &req);
        robotkernel::rk_type parseArg(std::string &args, std::string typeName, std::string paramName, size_t *sPos);
        robotkernel::rk_type parseVectorArg(std::string &args, std::string typeName, std::string paramName, size_t *sPos);
        robotkernel::rk_type parseStringArg(std::string &args, std::string &value, size_t *sPos);
    public:
        //! construct cli_bridge client
        Client();

        //! destruct cli_bridge client
        ~Client();

        void addService(const robotkernel::service_t &svc);
        void removeService(const robotkernel::service_t &svc);

        void onCliMessage(cli_bridge::CliConnection* c, char* msg, ssize_t len);
        void onCliConnect(cli_bridge::CliConnection* c);
        void onCliDisconnect(cli_bridge::CliConnection* c);

    private:
        //! Server for cli connections
        cli_bridge::CliServer cliServer;
        typedef std::map<std::string, robotkernel::service_t> ServiceMap;
        ServiceMap services;
    };

}

#endif