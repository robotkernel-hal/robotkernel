#include <list>
#include <string>

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
const std::string log_base::service_definition_configure_loglevel = 
"name: log_base/configure_loglevel\n"
"request:\n"
"- string: set_loglevel\n"
"response:\n"
"- string: current_loglevel\n"
"- string: error_message\n";

// module
const std::string module::service_definition_set_state = 
"name: module/set_state\n"
"request:\n"
"- string: state\n"
"response:\n"
"- string: error_message\n";

const std::string module::service_definition_get_state = 
"name: module/get_state\n"
"response:\n"
"- string: state\n"
"- string: error_message\n";

const std::string module::service_definition_get_config =
"name: module/get_config\n"
"response:\n"
"- string: config\n";

// kernel
const std::string kernel::service_definition_get_dump_log = 
"name: kernel/get_dump_log\n"
"response:\n"
"- string: log\n";

const std::string kernel::service_definition_config_dump_log = 
"name: kernel/config_dump_log\n"
"request:\n"
"- uint32_t: max_len\n"
"- uint8_t: do_ust\n"
"- string: set_loglevel\n"
"response:\n"
"- string: current_loglevel\n"
"- string: error_message\n";

const std::string kernel::service_definition_add_module = 
"name: kernel/add_module\n"
"request:\n"
"- string: config\n"
"response:\n"
"- string: error_message\n";

const std::string kernel::service_definition_remove_module = 
"name: kernel/remove_module\n"
"request:\n"
"- string: mod_name\n"
"response:\n"
"- string: error_message\n";

const std::string kernel::service_definition_module_list = 
"name: kernel/module_list\n"
"response:\n"
"- vector/string: modules\n"
"- string: error_message\n";

const std::string kernel::service_definition_reconfigure_module = 
"name: kernel/reconfigure_module\n"
"request:\n"
"- string: mod_name\n"
"response:\n"
"- uint64_t: state\n"
"- string: error_message\n";

const std::string kernel::service_definition_list_devices = 
"name: kernel/list_devices\n"
"response:\n"
"- vector/string: devices\n"
"- string: error_message\n";

const std::string kernel::service_definition_process_data_info = 
"name: kernel/process_data_info\n"
"request:\n"
"- string: name\n"
"response:\n"
"- string: owner\n"
"- string: definition\n"
"- string: clk_device\n"
"- int32_t: length\n"
"- string: provider\n"
"- string: consumer\n"
"- string: error_message\n";

const std::string kernel::service_definition_trigger_info = 
"name: kernel/trigger_info\n"
"request:\n"
"- string: name\n"
"response:\n"
"- string: owner\n"
"- double: rate\n"
"- string: error_message\n";

const std::string kernel::service_definition_stream_info = 
"name: kernel/stream_info\n"
"request:\n"
"- string: name\n"
"response:\n"
"- string: owner\n"
"- string: error_message\n";

const std::string kernel::service_definition_service_interface_info = 
"name: kernel/service_interface_info\n"
"request:\n"
"- string: name\n"
"response:\n"
"- string: owner\n"
"- string: error_message\n";

const std::string kernel::service_definition_add_pd_injection = 
"name: kernel/add_pd_injection\n"
"request:\n"
"- string: pd_dev\n"
"- string: field_name\n"
"- string: value\n"
"- string: bitmask\n"
"response:\n"
"- uint64_t: injection_hash\n"
"- string: error_message\n";

const std::string kernel::service_definition_del_pd_injection = 
"name: kernel/del_pd_injection\n"
"request:\n"
"- string: pd_dev\n"
"- uint64_t: injection_hash\n"
"response:\n"
"- string: error_message\n";

const std::string kernel::service_definition_list_pd_injections = 
"name: kernel/list_pd_injections\n"
"response:\n"
"- vector/string: pd_dev\n"
"- vector/string: field_name\n"
"- vector/string: value\n"
"- vector/string: bitmask\n"
"- string: error_message\n";

extern "C" void get_sd(std::list<std::string>& sd_list) {
    sd_list.push_back(log_base::service_definition_configure_loglevel);
    sd_list.push_back(module::service_definition_set_state);
    sd_list.push_back(module::service_definition_get_state);
    sd_list.push_back(module::service_definition_get_config);
    sd_list.push_back(kernel::service_definition_get_dump_log);
    sd_list.push_back(kernel::service_definition_config_dump_log);
    sd_list.push_back(kernel::service_definition_add_module);
    sd_list.push_back(kernel::service_definition_remove_module);
    sd_list.push_back(kernel::service_definition_module_list);
    sd_list.push_back(kernel::service_definition_reconfigure_module);
    sd_list.push_back(kernel::service_definition_list_devices);
    sd_list.push_back(kernel::service_definition_process_data_info);
    sd_list.push_back(kernel::service_definition_trigger_info);
    sd_list.push_back(kernel::service_definition_stream_info);
    sd_list.push_back(kernel::service_definition_service_interface_info);
    sd_list.push_back(kernel::service_definition_add_pd_injection);
    sd_list.push_back(kernel::service_definition_del_pd_injection);
    sd_list.push_back(kernel::service_definition_list_pd_injections);
}
