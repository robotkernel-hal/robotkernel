//! robotkernel kernel
/*!
 * @brief This file defines the `kernel` class, which is the core component of the robotkernel.
 *
 * The `kernel` class manages modules, services, devices, and other resources required for the robotkernel to function.
 * It provides interfaces for adding, removing, and configuring modules, as well as for registering and calling services.
 * The `kernel` class also handles device management, including adding, removing, and retrieving devices.
 *
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

/*!
 * @brief The main kernel class for the robotkernel system.
 *
 * This class manages modules, services, devices, and other core functionalities.
 * It inherits from several service base classes to provide a comprehensive set of services.
 */
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
        /*! @brief The singleton instance of the kernel.
         *
         *  This allows global access to the kernel's functionality.
         */
        static kernel instance;

    private:

        kernel(const kernel &);             // prevent copy-construction
        kernel &operator=(const kernel &);  // prevent assignment

        loglevel                    ll;                         /*!< @brief robotkernel global loglevel */
        bridge_map_t                bridge_map;                 /*!< @brief Map of bridges, used for inter-module communication. */
        service_provider_map_t      service_provider_map;       /*!< @brief Map of service providers. */
        module_map_t                module_map;                 /*!< @brief Map of loaded modules. */
        std::recursive_mutex        module_map_mtx;             /*!< @brief Mutex to protect access to the module map. */
        service_map_t               service_map;                /*!< @brief Map of registered services. */
        std::recursive_mutex        service_map_mtx;            /*!< @brief Mutex to protect access to the service map. */
        device_listener_map_t       dl_map;                     /*!< @brief Map of device listeners. */
        std::recursive_mutex        device_map_mtx;             /*!< @brief Mutex to protect access to the device map. */
        device_map_t                device_map;                 /*!< @brief Map of the devices. */

        typedef std::map<std::string, std::string> string_map_t;
        string_map_t datatypes_map;
        string_map_t service_definitions_map;
        string_map_t pd_definitions_map;

        int trace_fd = 0;
        bool log_to_trace_fd = false;

    protected:
        /*! @brief Default constructor (protected to enforce singleton pattern).*/
        kernel();

        /*! @brief Default destructor.*/
        ~kernel();

    public:
        int main_argc;      /*!< @brief robotkernel's main argument counter */
        char **main_argv;   /*!< @brief robotkernel's main arguments */

        /*! @brief Returns a boolean indicating whether logging to the trace file descriptor is enabled.
         *  @return true if logging to trace fd is enabled, false otherwise.
         */
        bool do_log_to_trace_fd() { return log_to_trace_fd; }

        /*! @brief Logs a formatted string to the trace file descriptor.
         *
         *  @param[in] fmt The format string, similar to printf.
         *  @param[in] ... Variable arguments to be formatted.
         */
        void trace_write(const char *fmt, ...);

        /*! @brief Logs a log pool object to the trace file descriptor.
         *
         *  @param[in] obj Pointer to the log pool object to be written.
         */
        void trace_write(const struct log_thread::log_pool_object *obj);

        /*! @brief Calls a robotkernel service.
         *
         *  This method allows invoking a service registered within the robotkernel.
         *  @param[in]  name          Name of service to call.
         *  @param[in]  req           Service request parameters.
         *  @param[out] resp          Service response parameters.
         */
        void call_service(const std::string& name, const YAML::Node& req, YAML::Node& resp);

        /*! @brief Calls a robotkernel service, specifying the owner.
         *
         *  This overloaded method allows invoking a service registered within the robotkernel,
         *  specifying the owner of the service.
         *  @param[in]  owner         Owner of service to call.
         *  @param[in]  name          Name of service to call.
         *  @param[in]  req           Service request parameters.
         *  @param[out] resp          Service response parameters.
         */
        void call_service(const std::string& owner, const std::string& name, const YAML::Node& req, YAML::Node& resp);

        /*! @brief Adds a service to the kernel.
         *
         *  This method registers a new service within the robotkernel, making it available for other modules to call.
         *  @param[in] owner service owner
         *  @param[in] name service name
         *  @param[in] service_definition service definition
         *  @param[in] callback service callback
         */
        void add_service(
                const std::string &owner,
                const std::string &name,
                const std::string &service_definition,
                service_callback_t callback);

        /*! @brief Removes a service from the kernel.
         *
         *  This method unregisters a service from the robotkernel, making it unavailable for other modules to call.
         *  @param[in] owner     Owner of service.
         *  @param[in] name      Name of service.
         */
        void remove_service(const std::string& owner, const std::string& name);

        /*! @brief Removes all services owned by a specific owner.
         *
         *  This method unregisters all services associated with a given owner from the robotkernel.
         *  @param[in] owner service owner
         */
        void remove_services(const std::string &owner);

        /*! @brief Adds a device listener.
         *
         *  This method registers a device listener, which will be notified when new devices are added to the kernel.
         *  @param[in] dl    device listener to add. this device listener
         *                  will be notified whenever a new device is added.
         */
        void add_device_listener(sp_device_listener_t dl);

        /*! @brief Removes a device listener.
         *
         *  This method unregisters a device listener, preventing it from receiving notifications when new devices are added.
         *  @param[in] dl    device listener to reomve. this device listener
         *                  will no longer be notified when a new device is added.
         */
        void remove_device_listener(sp_device_listener_t dl);

        /*! @brief Adds a named device.
         *
         *  This method registers a new device with the kernel, making it available for use by other modules.
         *  @param[in] req device to add
         */
        void add_device(sp_device_t req);

        /*! @brief Removes a named device.
         *
         *  This method unregisters a device from the kernel, making it unavailable for use by other modules.
         *  @param[in] req device to remove
         */
        void remove_device(sp_device_t req);

        /*! @brief Removes all devices owned by a specific owner.
         *
         *  This method unregisters all devices associated with a given owner from the robotkernel.
         *  @param[in] owner unique owner string
         */
        void remove_devices(const std::string& owner);

        /*! @brief Gets a device by its name.
         *  @param[in] dev_name device name
         *  @return device
         */
        template <typename T>
        std::shared_ptr<T> get_device(const std::string& dev_name);
        
        /*! @brief Registers a new pd definition.
         *  @param[in]   name        Datatype name.
         *  @param[in]   desc        Datatype definition.
         *
         *  @throw Exception if pd was already found.
         */
        void add_pd_definition(const std::string& name, const std::string& desc);

        /*! @brief Gets a registered pd definition.
         *  @param[in]   name        Datatype name.
         *
         *  @throw Exception if pd is not found.
         *
         *  @return String containing pd definition.
         */
        const std::string get_pd_definition(const std::string&name);

        //! Remove a pd definition
        /*!
         * \param[in]   name        Datatype name.
         */
        void remove_pd_definition(const std::string& name);

        /*! @brief Registers a new datatype definition.
         *  @param[in]   name        Datatype name.
         *  @param[in]   desc        Datatype definition.
         *
         *  @throw Exception if datatype was already found.
         */
        void add_datatype_definition(const std::string& name, const std::string& desc);

        /*! @brief Gets a registered datatype definition.
         *  @param[in]   name        Datatype name.
         *
         *  @throw Exception if datatype is not found.
         *
         *  @return String containing datatype definition.
         */
        const std::string get_datatype_definition(const std::string&name);

        //! Remove a datatype definition
        /*!
         * \param[in]   name        Datatype name.
         */
        void remove_datatype_definition(const std::string& name);

        /*! @brief Registers a new service definition.
         *  @param[in]   name        Service name.
         *  @param[in]   desc        Service definition.
         *
         *  @throw Exception if service was already found.
         */
        void add_service_definition(const std::string& name, const std::string& desc);

        /*! @brief Gets a registered service definition.
         *  @param[in]   name        Service name.
         *
         *  @throw Exception if service is not found.
         *
         *  @return String containing service definition.
         */
        const std::string get_service_definition(const std::string&name);

        /*! @brief Remove a service definition.
         *  @param[in]   name        Service name.
         */
        void remove_service_definition(const std::string& name);

        /*! @brief Configures the kernel from a configuration file.
         *  @param[in] configfile config file name
         */
        void config(std::string config_file, int argc, char *argv[]);

        /*! @brief Powers up all modules.
         *  @return success
         */
        bool power_up();

        /*! @brief Powers down all modules.
         */
        void power_down();

        /*! @brief Sets the state of a module.
         *  @param[in] mod_name name of module
         *  @param[in] state new module state
         *  @return state
         */
        int set_state(std::string mod_name, module_state_t state, 
                std::list<std::string> caller=std::list<std::string>());

        /*! @brief Returns the state of a module.
         *  @param[in] mod_name name of module which state to return
         *  @return module state
         */
        module_state_t get_state(std::string mod_name);

        /*! @brief Checks if a module is in the requested state.
         *  @param[in] mod_name module name to check
         *  @param[in] state requested state
         *
         *  @return true if we are in right  state
         */
        bool state_check(std::string mod_name, module_state_t state);

        /*! @brief Checks if all modules are in their target states.
         *  @return true if all modules are in their target states.
         */
        bool state_check();

        // config file name
        std::string config_file;
        std::string config_file_path;
        std::string exec_file_path;

        /*! @brief Handles a module state change.
         *  @param[in] mod_name module name which changed state
         *  @param[in] new_state new state of module
         *  @retun success
         */
        int state_change(const char *mod_name, module_state_t new_state);

        /*! @brief Gets a module by its name.
         *  @param[in] mod_name name of module
         *  @return shared pointer to module
         */
        sp_module_t get_module(const std::string& mod_name);

        /*! @brief Loads additional modules from a configuration.
         *  @param[in] config    New module configuration.
         */
        void load_module(const YAML::Node& config);

        bool _do_not_unload_modules;

        std::string _name;
        std::string _internal_modpath;
        std::string _internal_intfpath;

        log_thread rk_log;
        bool log_to_lttng_ust = false;

        static std::string ll_to_string(loglevel ll);


        /*! @brief Service implementation for getting the dump log.
         *  @param[in]   req     Service request data.
         *  @param[out]  resp    Service response data.
         */
        void svc_get_dump_log(
            const struct services::robotkernel::kernel::svc_req_get_dump_log& req, 
            struct services::robotkernel::kernel::svc_resp_get_dump_log& resp) override;
        
        /*! @brief Service implementation for configuring the dump log.
         *  @param[in]   req     Service request data.
         *  @param[out]  resp    Service response data.
         */
        void svc_config_dump_log(
            const struct services::robotkernel::kernel::svc_req_config_dump_log& req, 
            struct services::robotkernel::kernel::svc_resp_config_dump_log& resp) override;
    
        /*! @brief Service implementation for adding a module.
         *  @param[in]   req     Service request data.
         *  @param[out]  resp    Service response data.
         */
        void svc_add_module(
            const struct services::robotkernel::kernel::svc_req_add_module& req, 
            struct services::robotkernel::kernel::svc_resp_add_module& resp) override;
        
        /*! @brief Service implementation for removing a module.
         *  @param[in]   req     Service request data.
         *  @param[out]  resp    Service response data.
         */
        void svc_remove_module(
            const struct services::robotkernel::kernel::svc_req_remove_module& req, 
            struct services::robotkernel::kernel::svc_resp_remove_module& resp) override;

        /*! @brief Service implementation for listing modules.
         *  @param[in]   req     Service request data.
         *  @param[out]  resp    Service response data.
         */
        void svc_module_list(
            const struct services::robotkernel::kernel::svc_req_module_list& req, 
            struct services::robotkernel::kernel::svc_resp_module_list& resp) override;

        /*! @brief Service implementation for reconfiguring a module.
         *  @param[in]   req     Service request data.
         *  @param[out]  resp    Service response data.
         */
        void svc_reconfigure_module(
            const struct services::robotkernel::kernel::svc_req_reconfigure_module& req, 
            struct services::robotkernel::kernel::svc_resp_reconfigure_module& resp) override;

        /*! @brief Service implementation for listing devices.
         *  @param[in]   req     Service request data.
         *  @param[out]  resp    Service response data.
         */
        void svc_list_devices(
            const struct services::robotkernel::kernel::svc_req_list_devices& req, 
            struct services::robotkernel::kernel::svc_resp_list_devices& resp) override;

        /*! @brief Service implementation for providing process data information.
         *  @param[in]   req     Service request data.
         *  @param[out]  resp    Service response data.
         */
        void svc_process_data_info(
            const struct services::robotkernel::kernel::svc_req_process_data_info& req, 
            struct services::robotkernel::kernel::svc_resp_process_data_info& resp) override;
        
        /*! @brief Service implementation for providing trigger information.
         *  @param[in]   req     Service request data.
         *  @param[out]  resp    Service response data.
         */
        void svc_trigger_info(
            const struct services::robotkernel::kernel::svc_req_trigger_info& req, 
            struct services::robotkernel::kernel::svc_resp_trigger_info& resp) override;
        
        /*! @brief Service implementation for providing stream information.
         *  @param[in]   req     Service request data.
         *  @param[out]  resp    Service response data.
         */
        void svc_stream_info(
            const struct services::robotkernel::kernel::svc_req_stream_info& req, 
            struct services::robotkernel::kernel::svc_resp_stream_info& resp) override;

        /*! @brief Service implementation for providing service interface information.
         *  @param[in]   req     Service request data.
         *  @param[out]  resp    Service response data.
         */
        void svc_service_interface_info(
            const struct services::robotkernel::kernel::svc_req_service_interface_info& req, 
            struct services::robotkernel::kernel::svc_resp_service_interface_info& resp) override;
        
        /*! @brief Service implementation for adding process data injection.
         *  @param[in]   req     Service request data.
         *  @param[out]  resp    Service response data.
         */
        void svc_add_pd_injection(
            const struct services::robotkernel::kernel::svc_req_add_pd_injection& req, 
            struct services::robotkernel::kernel::svc_resp_add_pd_injection& resp) override;

        /*! @brief Service implementation for deleting process data injection.
         *  @param[in]   req     Service request data.
         *  @param[out]  resp    Service response data.
         */
        void svc_del_pd_injection(
            const struct services::robotkernel::kernel::svc_req_del_pd_injection& req, 
            struct services::robotkernel::kernel::svc_resp_del_pd_injection& resp) override;

        /*! @brief Service implementation for listing process data injections.
         *  @param[in]   req     Service request data.
         *  @param[out]  resp    Service response data.
         */
        void svc_list_pd_injections(
            const struct services::robotkernel::kernel::svc_req_list_pd_injections& req, 
            struct services::robotkernel::kernel::svc_resp_list_pd_injections& resp) override;
};
        
/*! @brief Gets a device by name.
 *  @param[in] dev_name name of the device
 *  @return shared pointer to the device
 */
template <typename T>
inline std::shared_ptr<T> kernel::get_device(const std::string& dev_name) {
    std::unique_lock<std::recursive_mutex> lock(device_map_mtx);

    if (device_map.find(dev_name) == device_map.end()) 
        throw std::runtime_error(robotkernel::helpers::string_printf(
                    "device %s not found\n", dev_name.c_str()));

    std::shared_ptr<T> retval = std::dynamic_pointer_cast<T>(device_map[dev_name]);
    if (!retval)
        throw std::runtime_error(robotkernel::helpers::string_printf(
                    "device %s is not of type %s\n", dev_name.c_str(), typeid(T).name()));

    return retval;
};

} // namespace robotkernel

#endif // ROBOTKERNEL__KERNEL_H
