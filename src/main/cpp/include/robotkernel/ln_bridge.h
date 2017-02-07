#include "robotkernel/kernel.h"
#include "robotkernel/service.h"

#include "ln.h"
#include "ln_cppwrapper.h"
#include "robotkernel/bridge_interface.h"

namespace ln_bridge {

class service;

class client : robotkernel::CommBridgeInterface{
    public:
        //! construct ln_bridge client
        client();

        //! destruct ln_bridge client
        ~client();

        //! create and register ln service
        /*!
         * \param svc robotkernel service struct
         */
        void addService(const robotkernel::service_t& svc);

        //! unregister and remove ln service 
        /*!
         * \param svc robotkernel service struct
         */
        void removeService(const robotkernel::service_t& svc);
        
    public:
        //! links-and-nodes client handle
        ln::client *clnt;

        //! links-and-nodes services map
        typedef std::map<std::string, ln_bridge::service *> service_map_t;
        service_map_t service_map;
};

class service {
    public:
        //! construct ln_bridge service
        /*!
         * \param clnt ln_bridge client
         * \param svc robotkernel service
         */
        service(ln_bridge::client& clnt, 
                const robotkernel::service_t& svc);

        //! destruct ln_bridge service
        ~service();

        void _create_ln_message_defition();

        ln_bridge::client& _clnt;
        const robotkernel::service_t& _svc;
        ln::service *_ln_service;
        std::string md;
        std::map<std::string, std::string> sub_mds;
        std::string signature;

        int handle(ln::service_request& req);

        static int service_cb(ln::client&, ln::service_request& req, 
                void* user_data) {
            service *self = (service *)user_data;
            return self->handle(req);
        }
};
        
}

