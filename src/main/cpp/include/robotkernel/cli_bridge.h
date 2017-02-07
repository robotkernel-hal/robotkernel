#include "robotkernel/kernel.h"
#include "robotkernel/service.h"
#include "cli_server.h"

#ifndef ROBOTKERNEL_CLI_BRIDGE_H
#define ROBOTKERNEL_CLI_BRIDGE_H


#include "robotkernel/bridge_interface.h"

namespace cli_bridge {

    class Client : robotkernel::CommBridgeInterface{
    private:
        robotkernel::service_t* parseRequest(std::string &msg, robotkernel::service_arglist_t &req);
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