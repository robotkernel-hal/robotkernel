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
#include <functional>

#include <robotkernel/rk_type.h>
#include <robotkernel/loglevel.h>
#include <robotkernel/log_thread.h>
#include <robotkernel/dump_log.h>
#include <robotkernel/exceptions.h>
#include <robotkernel/module.h>
#include <robotkernel/service.h>

#define klog(...) robotkernel::kernel::get_instance()->log(__VA_ARGS__)

namespace robotkernel {

    class kernel {
    private:
        //! kernel singleton instance
        static kernel *instance;

        kernel(const kernel &);             // prevent copy-construction
        kernel &operator=(const kernel &);  // prevent assignment

        loglevel ll;    //!< robotkernel global loglevel

        // modules map
        typedef std::map<std::string, module *> module_map_t;
        module_map_t module_map;
        pthread_mutex_t module_map_lock;

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
        service_list_t service_list;
        service_providers_list_t service_providers;
        //! add a service provider
        /*!
         * \param svc_provider service provider to add
         */
        void add_service_provider(service_provider_t *svc_provider);

        //! add service to kernel
        /*!
         * \param owner service owner
         * \param name service name
         * \param service_definition service definition
         * \param callback service callback
         */
        void add_service(
                const std::string &owner,
                const std::string &name,
                const std::string &service_definition,
                service_callback_t callback);

        //! remove all services from owner
        /*!
         * \param owner service owner
         */
        void remove_services(const std::string &owner);

        //! register trigger module to module named mod_name
        /*!
         * \param mod_name module name
         * \param trigger_mdl module which should be triggered by mod_name
         * \param t external trigger
         */
        void trigger_register_module(const std::string &mod_name,
                                     module *trigger_mdl, module::external_trigger &t);

        //! unregister trigger module from module named mod_name
        /*!
         * \param mod_name module name
         * \param trigger_mdl module which was triggered by mod_name
         * \param t external trigger
         */
        void trigger_unregister_module(const std::string &mod_name,
                                       module *trigger_mdl, module::external_trigger &t);

        //! get kernel singleton instance
        /*!
         * \return kernel singleton instance
         */
        static kernel *get_instance();

        //! destroy singleton instance
        static void destroy_instance();

        const loglevel get_loglevel() const { return ll; }

        void set_loglevel(loglevel ll) { this->ll = ll; }

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

        typedef void *interface_id_t;

        //! kernel register interface callback
        /*!
         * \param if_name interface name to register
         * \param node configuration node
         * \return interface id or -1
         */
        static interface_id_t register_interface_cb(const char *if_name,
                                                    const YAML::Node &node,
                                                    void* sp_interface);

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
        int request(int reqcode, void *ptr);

        bool _do_not_unload_modules;

        std::string _name;
        std::string _internal_modpath;
        std::string _internal_intfpath;

        log_thread rk_log;

        static std::string ll_to_string(loglevel ll);

        // write to logfile
        void log(loglevel ll, const char *format, ...);

        //! get dump log
        /*!
         * \param request service request data
         * \parma response service response data
         * \return success
         */
        int service_get_dump_log(const service_arglist_t &request,
                                 service_arglist_t &response);

        static const std::string service_definition_get_dump_log;

        //! config dump log
        /*!
         * \param request service request data
         * \parma response service response data
         * \return success
         */
        int service_config_dump_log(const service_arglist_t &request,
                                    service_arglist_t &response);

        static const std::string service_definition_config_dump_log;

        //! add module
        /*!
         * \param request service request data
         * \parma response service response data
         * \return success
         */
        int service_add_module(const service_arglist_t &request,
                               service_arglist_t &response);

        static const std::string service_definition_add_module;

        //! remove module
        /*!
         * \param request service request data
         * \parma response service response data
         * \return success
         */
        int service_remove_module(const service_arglist_t &request,
                                  service_arglist_t &response);

        static const std::string service_definition_remove_module;

        //! module list
        /*!
         * \param request service request data
         * \parma response service response data
         * \return success
         */
        int service_module_list(const service_arglist_t &request,
                                service_arglist_t &response);

        static const std::string service_definition_module_list;

        //! reconfigure module
        /*!
         * \param request service request data
         * \parma response service response data
         * \return success
         */
        int service_reconfigure_module(const service_arglist_t &request,
                                       service_arglist_t &response);

        static const std::string service_definition_reconfigure_module;
    };

} // namespace robotkernel

#endif // __KERNEL_H__

