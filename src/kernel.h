//! robotkernel kernel
/*!
 * (C) Robert Burger <robert.burger@dlr.de>
 */

// vim: set expandtab softtabstop=4 shiftwidth=4
// -*- mode: c++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil; -*- 

/*
 * This file is part of robotkernel.
 *
 * robotkernel is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 * 
 * robotkernel is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with robotkernel; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef ROBOTKERNEL__KERNEL_H
#define ROBOTKERNEL__KERNEL_H

#include <string>
#include <mutex>
#include <functional>
#include <stdexcept>

#include <robotkernel/device_listener.h>
#include <robotkernel/exceptions.h>
#include <robotkernel/loglevel.h>
#include <robotkernel/process_data.h>
#include <robotkernel/rk_type.h>
#include <robotkernel/service.h>
#include <robotkernel/service_interface.h>
#include <robotkernel/service_definitions.h>
#include <robotkernel/stream.h>
#include <robotkernel/trigger.h>
#include <robotkernel/log_base.h>

// private headers
#include "log_thread.h"
#include "module.h"
#include "bridge.h"
#include "service_provider.h"
#include "dump_log.h"

namespace robotkernel {

class kernel :
    public log_base,
    public services::robotkernel::kernel::svc_base_config_dump_log,
    public services::robotkernel::kernel::svc_base_get_dump_log,
    public services::robotkernel::kernel::svc_base_list_devices,
    public services::robotkernel::kernel::svc_base_module_list,
    public services::robotkernel::kernel::svc_base_add_module,
    public services::robotkernel::kernel::svc_base_remove_module,
    public services::robotkernel::kernel::svc_base_reconfigure_module,
    public services::robotkernel::kernel::svc_base_process_data_info,
    public services::robotkernel::kernel::svc_base_trigger_info,
    public services::robotkernel::kernel::svc_base_stream_info,
    public services::robotkernel::kernel::svc_base_service_interface_info,
    public services::robotkernel::kernel::svc_base_add_pd_injection,
    public services::robotkernel::kernel::svc_base_del_pd_injection,
    public services::robotkernel::kernel::svc_base_list_pd_injections
{
    public:
        //! kernel singleton instance
        static kernel instance;

    private:

        kernel(const kernel &);             // prevent copy-construction
        kernel &operator=(const kernel &);  // prevent assignment

        loglevel                    ll;                         //!< robotkernel global loglevel
        bridge_map_t                bridge_map;                 //!< bridges map
        service_provider_map_t      service_provider_map;       //!< service_providers map
        module_map_t                module_map;                 //!< modules map
        std::recursive_mutex        module_map_mtx;             //!< module map lock
        service_map_t               services;                   //!< service list
        device_listener_map_t       dl_map;                     //!< device listeners

        typedef std::map<std::string, std::string> datatypes_map_t;
        datatypes_map_t datatypes_map;

        device_map_t device_map;

        int trace_fd = 0;
        bool log_to_trace_fd = false;

    protected:
        //! construction
        kernel();

        //! destruction
        ~kernel();

    public:
        int main_argc;      //!< robotkernel's main argument counter
        char **main_argv;   //!< robotkernel's main arguments

        //! holds all registered process data
        process_data_map_t process_data_map;

        bool do_log_to_trace_fd() { return log_to_trace_fd; }

        //! log object to trace fd
        void trace_write(const char *fmt, ...);
        void trace_write(const struct log_thread::log_pool_object *obj);

        //! call a robotkernel service
        /*!
         * \param[in]  name          Name of service to call.
         * \param[in]  req           Service request parameters.
         * \param[out] resp          Service response parameters.
         */
        void call_service(const std::string& name, 
                const service_arglist_t& req, service_arglist_t& resp);

        //! call a robotkernel service
        /*!
         * \param[in]  owner         Owner of service to call.
         * \param[in]  name          Name of service to call.
         * \param[in]  req           Service request parameters.
         * \param[out] resp          Service response parameters.
         */
        void call_service(const std::string& owner, const std::string& name, 
                const service_arglist_t& req, service_arglist_t& resp);

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
         * \param[in] owner     Owner of service.
         * \param[in] name      Name of service.
         */
        void remove_service(const std::string& owner, const std::string& name);

        //! remove all services from owner
        /*!
         * \param owner service owner
         */
        void remove_services(const std::string &owner);
        
        //! adds a device listener
        /*
         * \param[in] dl    device listener to add. this device listener
         *                  will be notified whenever a new device is added.
         */
        void add_device_listener(sp_device_listener_t dl);
        
        //! remove a device listener
        /*
         * \param[in] dl    device listener to reomve. this device listener
         *                  will no longer be notified when a new device is added.
         */
        void remove_device_listener(sp_device_listener_t dl);

        //! add a named device
        /*
         * \param req device to add
         */
        void add_device(sp_device_t req);

        //! remove a named device
        /*!
         * \param req device to remove
         */
        void remove_device(sp_device_t req);

        //! remove all devices from owner
        /*!
         * \param[in] owner unique owner string
         */
        void remove_devices(const std::string& owner);

        //! get a device by name
        /*!
         * \param dev_name device name
         * \return device
         */
        template <typename T>
        std::shared_ptr<T> get_device(const std::string& dev_name);
        
        //! Register a new datatype description
        /*!
         * \param[in]   name        Datatype name.
         * \param[in]   desc        Datatype description.
         *
         * \throw Exception if datatype was already found.
         */
        void add_datatype_desc(const std::string& name, const std::string& desc);

        //! get a registered datatype
        /*!
         * \param[in]   name        Datatype name.
         *
         * \throw Exception if datatype is not found.
         *
         * \return String containing datatype description.
         */
        const std::string get_datatype_desc(const std::string&name);

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

        //! loads additional modules
        /*!
         * \param[in] config    New module configuration.
         */
        void load_module(const YAML::Node& config);

        bool _do_not_unload_modules;

        std::string _name;
        std::string _internal_modpath;
        std::string _internal_intfpath;

        log_thread rk_log;
        bool log_to_lttng_ust = false;

        static std::string ll_to_string(loglevel ll);


        //! svc_get_dump_log
        /*!
         * \param[in]   req     Service request data.
         * \param[out]  resp    Service response data.
         */
        void svc_get_dump_log(
            const struct services::robotkernel::kernel::svc_req_get_dump_log& req, 
            struct services::robotkernel::kernel::svc_resp_get_dump_log& resp) override;
        
        //! svc_config_dump_log
        /*!
         * \param[in]   req     Service request data.
         * \param[out]  resp    Service response data.
         */
        void svc_config_dump_log(
            const struct services::robotkernel::kernel::svc_req_config_dump_log& req, 
            struct services::robotkernel::kernel::svc_resp_config_dump_log& resp) override;
    
        //! svc_add_module
        /*!
         * \param[in]   req     Service request data.
         * \param[out]  resp    Service response data.
         */
        void svc_add_module(
            const struct services::robotkernel::kernel::svc_req_add_module& req, 
            struct services::robotkernel::kernel::svc_resp_add_module& resp) override;
        
        //! svc_remove_module
        /*!
         * \param[in]   req     Service request data.
         * \param[out]  resp    Service response data.
         */
        void svc_remove_module(
            const struct services::robotkernel::kernel::svc_req_remove_module& req, 
            struct services::robotkernel::kernel::svc_resp_remove_module& resp) override;

        //! svc_module_list
        /*!
         * \param[in]   req     Service request data.
         * \param[out]  resp    Service response data.
         */
        void svc_module_list(
            const struct services::robotkernel::kernel::svc_req_module_list& req, 
            struct services::robotkernel::kernel::svc_resp_module_list& resp) override;

        //! svc_reconfigure_module
        /*!
         * \param[in]   req     Service request data.
         * \param[out]  resp    Service response data.
         */
        void svc_reconfigure_module(
            const struct services::robotkernel::kernel::svc_req_reconfigure_module& req, 
            struct services::robotkernel::kernel::svc_resp_reconfigure_module& resp) override;

        //! svc_list_devices
        /*!
         * \param[in]   req     Service request data.
         * \param[out]  resp    Service response data.
         */
        void svc_list_devices(
            const struct services::robotkernel::kernel::svc_req_list_devices& req, 
            struct services::robotkernel::kernel::svc_resp_list_devices& resp) override;

        //! svc_process_data_info
        /*!
         * \param[in]   req     Service request data.
         * \param[out]  resp    Service response data.
         */
        void svc_process_data_info(
            const struct services::robotkernel::kernel::svc_req_process_data_info& req, 
            struct services::robotkernel::kernel::svc_resp_process_data_info& resp) override;
        
        //! svc_trigger_info
        /*!
         * \param[in]   req     Service request data.
         * \param[out]  resp    Service response data.
         */
        void svc_trigger_info(
            const struct services::robotkernel::kernel::svc_req_trigger_info& req, 
            struct services::robotkernel::kernel::svc_resp_trigger_info& resp) override;
        
        //! svc_stream_info
        /*!
         * \param[in]   req     Service request data.
         * \param[out]  resp    Service response data.
         */
        void svc_stream_info(
            const struct services::robotkernel::kernel::svc_req_stream_info& req, 
            struct services::robotkernel::kernel::svc_resp_stream_info& resp) override;

        //! svc_service_interface_info
        /*!
         * \param[in]   req     Service request data.
         * \param[out]  resp    Service response data.
         */
        void svc_service_interface_info(
            const struct services::robotkernel::kernel::svc_req_service_interface_info& req, 
            struct services::robotkernel::kernel::svc_resp_service_interface_info& resp) override;
        
        //! svc_add_pd_injection
        /*!
         * \param[in]   req     Service request data.
         * \param[out]  resp    Service response data.
         */
        void svc_add_pd_injection(
            const struct services::robotkernel::kernel::svc_req_add_pd_injection& req, 
            struct services::robotkernel::kernel::svc_resp_add_pd_injection& resp) override;

        //! svc_del_pd_injection
        /*!
         * \param[in]   req     Service request data.
         * \param[out]  resp    Service response data.
         */
        void svc_del_pd_injection(
            const struct services::robotkernel::kernel::svc_req_del_pd_injection& req, 
            struct services::robotkernel::kernel::svc_resp_del_pd_injection& resp) override;

        //! svc_list_pd_injections
        /*!
         * \param[in]   req     Service request data.
         * \param[out]  resp    Service response data.
         */
        void svc_list_pd_injections(
            const struct services::robotkernel::kernel::svc_req_list_pd_injections& req, 
            struct services::robotkernel::kernel::svc_resp_list_pd_injections& resp) override;
};
        
// get a device by name
template <typename T>
inline std::shared_ptr<T> kernel::get_device(const std::string& dev_name) {
    if (device_map.find(dev_name) == device_map.end()) 
        throw std::runtime_error(robotkernel::string_printf("device %s not found\n", dev_name.c_str()));

    std::shared_ptr<T> retval = std::dynamic_pointer_cast<T>(device_map[dev_name]);
    if (!retval)
        throw std::runtime_error(robotkernel::string_printf("device %s is not of type %s\n", 
                dev_name.c_str(), typeid(T).name()));

    return retval;
};

} // namespace robotkernel

#endif // ROBOTKERNEL__KERNEL_H

