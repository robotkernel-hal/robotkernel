#include "robotkernel/kernel.h"

#include "ln.h"
#include "ln_cppwrapper.h"

namespace ln_bridge {

class service;

class client {
    public:
        //! construct ln_bridge client
        client();

        //! destruct ln_bridge client
        ~client();

        //! create and register ln service
        /*!
         * \param svc robotkernel service struct
         */
        void add_service(const robotkernel::kernel::service_t& svc);

        //! unregister and remove ln service 
        /*!
         * \param svc robotkernel service struct
         */
        void remove_service(
            const robotkernel::kernel::service_t& svc);
        
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
                const robotkernel::kernel::service_t& svc);

        //! destruct ln_bridge service
        ~service();

        void _create_ln_message_defition();

        ln_bridge::client& _clnt;
        const robotkernel::kernel::service_t& _svc;
        ln::service *_ln_service;
        std::string md;
        std::string signature;

        int handle(ln::service_request& req);

        static int service_cb(ln::client&, ln::service_request& req, 
                void* user_data) {
            service *self = (service *)user_data;
            return self->handle(req);
            //            ln_service_config_dump_log_base* self = (ln_service_config_dump_log_base*)user_data;
            //ln_service_robotkernel_config_dump_log svc;
        }
};
        
}

