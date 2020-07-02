//! robotkernel kernel
/*!
 * (C) Robert Burger <robert.burger@dlr.de>
 */

// vim: set expandtab softtabstop=4 shiftwidth=4
// -*- mode: c++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil; -*- 

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

#ifndef ROBOTKERNEL__KERNEL_H
#define ROBOTKERNEL__KERNEL_H

#include <string>
#include <mutex>
#include <functional>

#include <robotkernel/bridge.h>
#include <robotkernel/device_listener.h>
#include <robotkernel/dump_log.h>
#include <robotkernel/exceptions.h>
#include <robotkernel/log_thread.h>
#include <robotkernel/loglevel.h>
#include <robotkernel/module.h>
#include <robotkernel/process_data.h>
#include <robotkernel/rk_type.h>
#include <robotkernel/service.h>
#include <robotkernel/service_interface.h>
#include <robotkernel/service_provider.h>
#include <robotkernel/stream.h>
#include <robotkernel/trigger.h>


#define klog(...) robotkernel::kernel::get_instance()->log(__VA_ARGS__)

namespace robotkernel {
#ifdef EMACS
}
#endif

class kernel {
    private:
        //! kernel singleton instance
        static kernel *instance;

        kernel(const kernel &);             // prevent copy-construction
        kernel &operator=(const kernel &);  // prevent assignment

        loglevel                    ll;                         //!< robotkernel global loglevel
        bridge_map_t                bridge_map;                 //!< bridges map
        service_provider_map_t      service_provider_map;       //!< service_providers map
        module_map_t                module_map;                 //!< modules map
        std::recursive_mutex        module_map_mtx;             //!< module map lock
        service_map_t               services;                   //!< service list
        device_listener_map_t       dl_map;                     //!< device listeners

        device_map_t device_map;

        int trace_fd;
        bool log_to_trace_fd;

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

        //! holds all registered process data
        process_data_map_t process_data_map;

        bool do_log_to_trace_fd() { return log_to_trace_fd; }

        //! log object to trace fd
        void trace_write(const char *fmt, ...);

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
        
        //! wrapper around \link get_device \endlink
        /*!
         * \param[in] name  Trigger device name.
         * \return shared pointer to trigger device
         */
        sp_trigger_t get_trigger(const std::string& name);

        //! wrapper around \link get_device \endlink
        /*!
         * \param[in] name  process data device name.
         * \return shared pointer to process data device
         */
        sp_process_data_t get_process_data(const std::string& name);

        //! wrapper around \link get_device \endlink
        /*!
         * \param[in] name  process data device name.
         * \return shared pointer to process data device
         */
        sp_stream_t get_stream(const std::string& name);

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
        bool log_to_lttng_ust;

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
        int service_list_devices(const service_arglist_t &request,
                service_arglist_t &response);

        static const std::string service_definition_list_devices;

        //! process data information
        /*!
         * \param request service request data
         * \parma response service response data
         * \return success
         */
        int service_process_data_info(const service_arglist_t &request,
                service_arglist_t &response);

        static const std::string service_definition_process_data_info;

        //! trigger information
        /*!
         * \param request service request data
         * \parma response service response data
         * \return success
         */
        int service_trigger_info(const service_arglist_t &request,
                service_arglist_t &response);

        static const std::string service_definition_trigger_info;
        
        //! stream information
        /*!
         * \param request service request data
         * \parma response service response data
         * \return success
         */
        int service_stream_info(const service_arglist_t &request,
                service_arglist_t &response);

        static const std::string service_definition_stream_info;
        
        //! service_interface information
        /*!
         * \param request service request data
         * \parma response service response data
         * \return success
         */
        int service_service_interface_info(const service_arglist_t &request,
                service_arglist_t &response);

        static const std::string service_definition_service_interface_info;

        //! add pd injection service
        /*!
         * \param request service request data
         * \parma response service response data
         * \return success
         */
        int service_add_pd_injection(const service_arglist_t &request,
                service_arglist_t &response);

        static const std::string service_definition_add_pd_injection;

        //! delete pd injection service
        /*!
         * \param request service request data
         * \parma response service response data
         * \return success
         */
        int service_del_pd_injection(const service_arglist_t &request,
                service_arglist_t &response);
        
        static const std::string service_definition_del_pd_injection;
        
        //! list pd injections service
        /*!
         * \param request service request data
         * \parma response service response data
         * \return success
         */
        int service_list_pd_injections(const service_arglist_t &request,
                service_arglist_t &response);
        
        static const std::string service_definition_list_pd_injections;
};
        
// wrapper around \link get_device \endlink
inline sp_trigger_t kernel::get_trigger(const std::string& name) {
    return get_device<trigger>(name);
}

// wrapper around \link get_device \endlink
inline sp_process_data_t kernel::get_process_data(const std::string& name) {
    return get_device<process_data>(name);
}

// wrapper around \link get_device \endlink
inline sp_stream_t kernel::get_stream(const std::string& name) {
    return get_device<stream>(name);
}
        
// get a device by name
template <typename T>
inline std::shared_ptr<T> kernel::get_device(const std::string& dev_name) {
    if (device_map.find(dev_name) == device_map.end()) 
        throw string_util::str_exception("device %s not found\n", dev_name.c_str());

    std::shared_ptr<T> retval = std::dynamic_pointer_cast<T>(device_map[dev_name]);
    if (!retval)
        throw string_util::str_exception("device %s is not of type %s\n", 
                dev_name.c_str(), typeid(T).name());

    return retval;
};

#ifdef EMACS
{
#endif
} // namespace robotkernel

#endif // ROBOTKERNEL__KERNEL_H

