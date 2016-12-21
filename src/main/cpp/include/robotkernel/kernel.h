//! robotkernel kernel
/*!
 * author: Robert Burger
 *
 * $Id$
 */

/*
 * This file is part of robotkernel.
 *
 * robotkernel is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * robotkernel is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with robotkernel.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __KERNEL_H__
#define __KERNEL_H__

#include <string>

#include <boost/function.hpp>
#include <boost/bind.hpp>

#include "robotkernel/module.h"
#include "robotkernel/log_thread.h"
#include "robotkernel/dump_log.h"
//#include "robotkernel/ln_kernel_messages.h"
//
//#include "ln.h"
//#include "ln_cppwrapper.h"

#define klog(...) robotkernel::kernel::get_instance()->log(__VA_ARGS__)

namespace robotkernel {
    
enum level { 
    error = 1,
    warning = 2,
    info = 3,
    verbose = 4, 
};
    
class loglevel {
    public:
        level value;
    
        loglevel() { value = info; }
        loglevel(const level& l) { value = l; };
        loglevel(const loglevel& ll)
        { this->value = ll.value; }

        loglevel& operator=(const loglevel& ll)
        { this->value = ll.value; return *this; }
        loglevel& operator=(const std::string& ll_string);
        bool operator==(const loglevel& ll)
        { return (this->value == ll.value); }
        bool operator==(const level& l)
        { return (this->value == l); }
        bool operator>(const loglevel& ll)
        { return (this->value > ll.value); }
        bool operator>(const level& l)
        { return (this->value > l); }
        bool operator<(const loglevel& ll)
        { return (this->value < ll.value); }
        bool operator<(const level& l)
        { return (this->value < l); }
        operator std::string() const
        { 
            switch (value) {
                case error:
                    return std::string("error");
                case warning:
                    return std::string("warning");
                case info:
                    return std::string("info");
                case verbose:
                    return std::string("verbose");
            }

            return std::string("unknown");
        }
};
//
//enum loglevel {
//    error = 1,
//    warning = 2,
//    info = 3,
//    verbose = 4, 
//};

class kernel {
//:
//    public ln_service_set_state_base,
//    public ln_service_get_state_base,
//    public ln_service_get_states_base,
//    public ln_service_add_module_base,
//    public ln_service_remove_module_base,
//    public ln_service_module_list_base,
//    public ln_service_reconfigure_module_base,
//    public ln_service_get_dump_log_base,
//    public ln_service_config_dump_log_base {
    private:
        //! kernel singleton instance
        static kernel *instance;
        
        kernel(const kernel&);             // prevent copy-construction
        kernel& operator=(const kernel&);  // prevent assignment
        
        loglevel ll;    //!< robotkernel global loglevel
        
        // modules map
        typedef std::map<std::string, module *> module_map_t;
        module_map_t module_map;
        pthread_t module_map_lock;
        
        //! return module state
        /*!
         * \param mod_name name of module which state to return
         * \return module state
         */
        module_state_t _internal_get_state(std::string mod_name);
    protected:
        //! construction
        /*!
         */
        kernel();

        //! destruction
        ~kernel();

    public:
        typedef boost::function<int(YAML::Node&)> service_callback_t;

        typedef struct service {
            std::string owner;
            std::string name;
            std::string service_definition;
            service_callback_t callback;
        } service_t;

        typedef std::map<std::string, service_t *> service_list_t;
        service_list_t service_list;

        typedef boost::function<void(const robotkernel::kernel::service_t& svc)>
            service_provider_cb_t;

        typedef struct service_provider {
            service_provider_cb_t add_service;
            service_provider_cb_t remove_service;
        } service_provider_t;

        typedef std::list<service_provider_t *> service_providers_list_t;
        service_providers_list_t service_providers;

        //! add service to kernel
        /*!
         * \param owner service owner
         * \param name service name
         * \param service_definition service definition
         * \param callback service callback
         */
        void add_service(
                const std::string& owner,
                const std::string& name, 
                const std::string& service_definition, 
                service_callback_t callback);

        //! remove all services from owner
        /*!
         * \param owner service owner
         */
        void remove_services(const std::string& owner);

        //! get kernel singleton instance
        /*!
         * \return kernel singleton instance
         */
        static kernel * get_instance();

        //! destroy singleton instance
        static void destroy_instance();

        const loglevel get_loglevel() const { return ll; }
        void set_loglevel(loglevel ll) { this->ll = ll; }

        //! initialize links and nodes client
        /*!
         * \param argc command line argument counter
         * \param argv command line arguments
         */
        void init_ln(int argc, char **argv);

        //! handle links and nodes service requests
        void handle_ln_request(int argc, char **argv);

        //! construction
        /*!
         * \param configfile config file name
         */
        void config(std::string config_file, int argc, char **argv);

        //! powering up modules
        bool power_up();

        //! powering down modules
        void power_down();

        //! set state of module
        /*!
         * \param mod_name name of module
         * \param state new module state
         * \return state
         */
        int set_state(std::string mod_name, module_state_t state);

        //! return module state
        /*!
         * \param mod_name name of module which state to return
         * \return module state
         */
        module_state_t get_state(std::string mod_name);

        //! checks if all modules are at requested state
        /*!
         * \param mod_name module name to check
         * \param state requested state
         *
         * \return true if we are in right  state
         */
        bool state_check(std::string mod_name, module_state_t state);
        bool state_check();

        // config file name
        std::string config_file;

        //! kernel request callback
        /*!
         * \param mod_name module name to send request to
         * \param reqcode request code
         * \param ptr request specifix pointer
         * \return request status code
         */
        static int request_cb(const char *mod_name, int reqcode, void *ptr);

        typedef void* interface_id_t;

        //! kernel register interface callback
        /*!
         * \param if_name interface name to register
         * \param node configuration node
         * \return interface id or -1
         */
        static interface_id_t register_interface_cb(const char *if_name, 
                const YAML::Node& node);

        //! kernel unregister interface callback
        /*!
         * \param interface_id interface id
         * \return N/A
         */
        static void unregister_interface_cb(interface_id_t interface_id);

        //! module state change
        /*!
         * \param mod_name module name which changed state
         * \param new_state new state of module
         * \retun success
         */
        int state_change(const char *mod_name, module_state_t new_state);

        //! Send a request to kernel
        /*!
          \param reqcode request code
          \param ptr pointer to request structure
          \return success or failure
          */
        int request(int reqcode, void* ptr);

        //! get module
        /*!
         * \param mod_name name of module
         * \return pointer to module
         */
        module *get_module(const char *mod_name);

        //! links and nodes client
//        ln::client *clnt;
        unsigned int _ln_thread_pool_size_main;
        bool _do_not_unload_modules;

        std::string _name;
        std::string _internal_modpath;
        std::string _internal_intfpath;

        log_thread rk_log;

        static std::string ll_to_string(loglevel ll);

        // write to logfile
        void log(loglevel ll, const char *format, ...);

        std::string dump_log() {
            return dump_log_dump();
        }
        void config_dump_log(unsigned int max_len, unsigned int do_ust=0) {
            dump_log_set_len(max_len, do_ust);
        }
        
        //! get dump log
        /*!
         * \param message service message
         * \return success
         */
        int service_get_dump_log(YAML::Node& message);
        static const std::string service_definition_get_dump_log;

        //! get dump log
        /*!
         * \param req service request
         * \param svn service container
         * \return success
         */
//        virtual int on_get_dump_log(ln::service_request& req, 
//                ln_service_robotkernel_get_dump_log& svc);
        
        //! config dump log
        /*!
         * \param message service message
         * \return success
         */
        int service_config_dump_log(YAML::Node& message);
        static const std::string service_definition_config_dump_log;

        //! config dump log
        /*!
         * \param req service request
         * \param svn service container
         * \return success
         */
//        virtual int on_config_dump_log(ln::service_request& req,
//                ln_service_robotkernel_config_dump_log& svc);

        //! set state
        /*!
         * \param req service request
         * \param svn service container
         * \return success
         */
//        virtual int on_set_state(ln::service_request& req, 
//                ln_service_robotkernel_set_state& svc);

        //! get state
        /*!
         * \param req service request
         * \param svn service container
         * \return success
         */
//        virtual int on_get_state(ln::service_request& req,
//                ln_service_robotkernel_get_state& svc);
        
        //! get states
        /*!
         * \param req service request
         * \param svn service container
         * \return success
         */
//        virtual int on_get_states(ln::service_request& req, 
//                ln_service_robotkernel_get_states& svc);
        
        //! add module
        /*!
         * \param req service request
         * \param svn service container
         * \return success
         */
//        virtual int on_add_module(ln::service_request& req,
//                ln_service_robotkernel_add_module& svc);

        //! add module
        /*!
         * \param message service message
         * \return success
         */
        int service_add_module(YAML::Node& message);
        static const std::string service_definition_add_module;
        
        //! remove module
        /*!
         * \param req service request
         * \param svn service container
         * \return success
         */
//        virtual int on_remove_module(ln::service_request& req,
//                ln_service_robotkernel_remove_module& svc);
        
        //! service remove module
        /*!
         * \param message service message
         * \return success
         */
        int service_remove_module(YAML::Node& message);
        static const std::string service_definition_remove_module;
        
        //! module list
        /*!
         * \param req service request
         * \param svn service container
         * \return success
         */
//        virtual int on_module_list(ln::service_request& req,
//                ln_service_robotkernel_module_list& svc);

        //! module list
        /*!
         * \param message service message
         * \return success
         */
        int service_module_list(YAML::Node& message);
        static const std::string service_definition_module_list;
        
        //! reconfigure module
        /*!
         * \param req service request
         * \param svn service container
         * \return success
         */
//        virtual int on_reconfigure_module(ln::service_request& req,
//                ln_service_robotkernel_reconfigure_module& svc);
};

} // namespace robotkernel

#endif // __KERNEL_H__

