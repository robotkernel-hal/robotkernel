#ifndef __ROBOTKERNEL_JSONRPC_BRIDGE_H__
#define __ROBOTKERNEL_JSONRPC_BRIDGE_H__

#include "robotkernel/kernel.h"
#include "robotkernel/service.h"
#include "robotkernel/bridge_interface.h"
#include "robotkernel/runnable.h"

#include "jsonrpc-cpp/jsonrpc.h"

namespace jsonrpc_bridge {
    class client : robotkernel::runnable {
        public:
            //! construct jsonrpc_bridge client
            client();

            //! destruct jsonrpc_bridge client
            ~client();

            //! callback to add a service
            /*!
             * \param svc service to add
             */
            void add_service(const robotkernel::service_t &svc);

            //! callback to remove a service
            /*!
             * \param svc service to remove
             */
            void remove_service(const robotkernel::service_t &svc);

            void run();             //!< handler function called if 

            //! service callback
            /*!
             * \param root request json node
             * \param response response json node
             * \return success
             */
            bool on_service(const Json::Value& root, Json::Value& response);

        private:
            //! json rpc server
            Json::Rpc::TcpServer server;

            //! service map
            typedef std::map<std::string, robotkernel::service_t> service_map_t;
            service_map_t services;
    };
}

#endif // __ROBOTKERNEL_JSONRPC_BRIDGE_H__

