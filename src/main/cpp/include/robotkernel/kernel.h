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
#include <robotkernel/bridge.h>
#include <robotkernel/service.h>
#include <robotkernel/service_provider.h>
#include <robotkernel/service_requester_base.h>
#include <robotkernel/process_data.h>
#include <robotkernel/trigger_base.h>

#define klog(...) robotkernel::kernel::get_instance()->log(__VA_ARGS__)

namespace robotkernel {

    class pd_provider {
    };

    class pd_requirer {
    };

    class kernel {
    public:
        typedef std::shared_ptr<module> sp_module_t;
        typedef std::shared_ptr<bridge> sp_bridge_t;
        typedef std::shared_ptr<service_provider> sp_service_provider_t;
        typedef std::shared_ptr<process_data> sp_process_data_t;
        typedef std::shared_ptr<trigger_base> sp_trigger_device_t;

    private:
        //! kernel singleton instance
        static kernel *instance;

        kernel(const kernel &);             // prevent copy-construction
        kernel &operator=(const kernel &);  // prevent assignment

        loglevel ll;    //!< robotkernel global loglevel

        // modules map
        typedef std::map<std::string, sp_module_t> module_map_t;
        module_map_t module_map;
        pthread_rwlock_t module_map_lock;
        
        // bridges map
        typedef std::map<std::string, sp_bridge_t> bridge_map_t;
        bridge_map_t bridge_map;

        // service_providers map
        typedef std::map<std::string, sp_service_provider_t> service_provider_map_t;
        service_provider_map_t service_provider_map;

        // trigger_device map
        typedef std::map<std::string, sp_trigger_device_t> trigger_device_map_t;
        trigger_device_map_t trigger_device_map;

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
        int main_argc;      //!< robotkernel's main argument counter
        char **main_argv;   //!< robotkernel's main arguments

        service_list_t service_list;
        service_requester_list_old_t service_requester_list_old;
        service_requester_list_t service_requester_list;
        bridge::cbs_list_t bridge_callbacks;
        
        //! holds all registered process data
        std::map<std::string, sp_process_data_t> process_data_map;

        //! add bridge callbacks
        /*!
         * \param cbs bridge callback to add
         */
        void add_bridge_cbs(bridge::cbs_t *cbs);

        //! remove bridge callbacks
        /*!
         * \param cbs bridge callback to remove 
         */
        void remove_bridge_cbs(bridge::cbs_t *cbs);

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

        //! remove on service given by name
        /*!
         * \param name service name
         */
        void remove_service(const std::string& name);

        //! remove all services from owner
        /*!
         * \param owner service owner
         */
        void remove_services(const std::string &owner);

        //! add service requester
        /*!
         * \param req slave inteface specialization         
         */
        void add_service_requester(sp_service_requester_t req);
        
        //! remove service requester
        /*!
         * \param req slave inteface specialization         
         */
        void remove_service_requester(sp_service_requester_t req);
        
        //! remove all service requester for given owner
        /*!
         * \param owner service requester owner      
         */
        void remove_service_requester(
                const std::string &owner);
        
        //! add process data
        /*!
         * \param req slave inteface specialization         
         */
        void add_process_data(sp_process_data_t req);
        
        //! remove process data
        /*!
         * \param req slave inteface specialization         
         */
        void remove_process_data(sp_process_data_t req);
        
        //! remove all process data for given owner
        /*!
         * \param owner process data owner       
         */
        void remove_process_data(
                const std::string &owner);

        //! get named process data
        /*!
         * \param pd_name name of process data
         * \return process data shared pointer
         */
        sp_process_data_t get_process_data(const std::string& pd_name);
        
        //! add a named trigger device
        /*!
         * \param req trigger device to add
         */
        void add_trigger_device(sp_trigger_device_t req);
        
        //! remove a named trigger device
        /*!
         * \param req trigger device to remove
         */
        void remove_trigger_device(sp_trigger_device_t req);

        //! get a trigger device by name
        /*!
         * \param trigger_name trigger device name
         * \return trigger device
         */
        sp_trigger_device_t get_trigger_device(const std::string& trigger_name);

        //! register trigger module to named trigger device
        /*!
         * \param t_dev trigger_device name
         * \param trigger_mdl module which should be triggered by t_dev
         * \param t external trigger
         */
        void trigger_register_module(const std::string& t_dev, 
                module *trigger_mdl, module::external_trigger& t);

        //! unregister trigger module from named trigger device
        /*!
         * \param t_dev trigger_device name
         * \param trigger_mdl module which was triggered by t_dev
         * \param t external trigger
         */
        void trigger_unregister_module(const std::string& t_dev, 
                module *trigger_mdl, module::external_trigger& t);

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
        void config(std::string config_file, int argc, char *argv[]);

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
        int set_state(std::string mod_name, module_state_t state, 
                std::list<std::string> caller=std::list<std::string>());

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
        std::string config_file_path;
        std::string exec_file_path;

        //! module state change
        /*!
         * \param mod_name module name which changed state
         * \param new_state new state of module
         * \retun success
         */
        int state_change(const char *mod_name, module_state_t new_state);

        //! get module
        /*!
         * \param mod_name name of module
         * \return shared pointer to module
         */
        sp_module_t get_module(const std::string& mod_name);
  
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

        //! list process data objects
        /*!
         * \param request service request data
         * \parma response service response data
         * \return success
         */
        int service_list_process_data(const service_arglist_t &request,
                                       service_arglist_t &response);

        static const std::string service_definition_list_process_data;

        //! process data information
        /*!
         * \param request service request data
         * \parma response service response data
         * \return success
         */
        int service_process_data_info(const service_arglist_t &request,
                service_arglist_t &response);

        static const std::string service_definition_process_data_info;
    };

} // namespace robotkernel

#endif // __KERNEL_H__

