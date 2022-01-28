#include <list>
#include <string>

#include "robotkernel/service_definitions.h"

namespace robotkernel {
    class log_base {
        public:
            static const std::string service_definition_configure_loglevel;
    };

    class module {
        public:
            static const std::string service_definition_set_state;
            static const std::string service_definition_get_state;
            static const std::string service_definition_get_config;
    };

    class kernel {
        public:
            static const std::string service_definition_get_dump_log;
            static const std::string service_definition_config_dump_log;
            static const std::string service_definition_add_module;
            static const std::string service_definition_remove_module;
            static const std::string service_definition_module_list;
            static const std::string service_definition_reconfigure_module;
            static const std::string service_definition_list_devices;
            static const std::string service_definition_process_data_info;
            static const std::string service_definition_trigger_info;
            static const std::string service_definition_stream_info;
            static const std::string service_definition_service_interface_info;
            static const std::string service_definition_add_pd_injection;
            static const std::string service_definition_del_pd_injection;
            static const std::string service_definition_list_pd_injections;
    };
};

using namespace robotkernel;

// log_base
const std::string log_base::service_definition_configure_loglevel = log_base_configure_loglevel_service_definition;

// module
const std::string module::service_definition_set_state = module_set_state_service_definition;
const std::string module::service_definition_get_state = module_get_state_service_definition;
const std::string module::service_definition_get_config = module_get_config_service_definition;

// kernel
const std::string kernel::service_definition_get_dump_log = kernel_get_dump_log_service_definition;
const std::string kernel::service_definition_config_dump_log = kernel_config_dump_log_service_definition;
const std::string kernel::service_definition_add_module = kernel_add_module_service_definition;
const std::string kernel::service_definition_remove_module = kernel_remove_module_service_definition;
const std::string kernel::service_definition_module_list = kernel_module_list_service_definition;
const std::string kernel::service_definition_reconfigure_module = kernel_reconfigure_module_service_definition;
const std::string kernel::service_definition_list_devices = kernel_list_devices_service_definition;
const std::string kernel::service_definition_process_data_info = kernel_process_data_info_service_definition;
const std::string kernel::service_definition_trigger_info = kernel_trigger_info_service_definition;
const std::string kernel::service_definition_stream_info = kernel_stream_info_service_definition;
const std::string kernel::service_definition_service_interface_info = kernel_service_interface_info_service_definition;
const std::string kernel::service_definition_add_pd_injection = kernel_add_pd_injection_service_definition;
const std::string kernel::service_definition_del_pd_injection = kernel_del_pd_injection_service_definition;
const std::string kernel::service_definition_list_pd_injections = kernel_list_pd_injections_service_definition;
