//! robotkernel interface definition
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

#include <fstream>
#include <iostream>
#include <algorithm>
#include <string_util/string_util.h>
#include "robotkernel/kernel.h"
#include "robotkernel/helpers.h"
#include "robotkernel/exceptions.h"
#include "robotkernel/module_base.h"
#include "robotkernel/cmd_delay.h"
#include "yaml-cpp/yaml.h"

using namespace std;
using namespace std::placeholders;
using namespace robotkernel;

extern module* currently_loading_module; // in module.cpp

YAML::Node tmp = YAML::Clone(YAML::Node("dieses clone steht hier damit es vom "
            "linker beim statischen linken mit eingepackt wird"));

namespace robotkernel {
void split_file_name(const string& str, string& path, string& file) {
    size_t found;
    found = str.find_last_of("/\\");
    if (found == string::npos) {
        path = "";
        file = str;
    } else {
        path = str.substr(0,found);
        file = str.substr(found+1);
    }
}
}

loglevel& loglevel::operator=(const std::string& ll_string) {
    if (ll_string == "error")
        value = error;
    else if (ll_string == "warning")
        value = warning;
    else if (ll_string == "info")
        value = info;
    else if (ll_string == "verbose")
        value = verbose;
    else
        throw str_exception("wrong loglevel value \"%s\"",
                ll_string.c_str());

    return *this;
}

const static string ROBOT_KERNEL = "[robotkernel] ";

//! kernel singleton instance
kernel *kernel::instance = NULL;

//! set state of module
/*!
 * \param mod_name name of module
 * \param state new module state
 * \return state
 */
int kernel::set_state(std::string mod_name, module_state_t state) {
    module_map_t::const_iterator it = module_map.find(mod_name);
    if (it == module_map.end())
        throw str_exception("[robotkernel] set_state: module %s not "
                "found!\n", mod_name.c_str());

    module& mdl = *(it->second);
    if (mdl.get_state() == state)
        return state;

    // iterate through dependencies
    for (module::depend_list_t::const_iterator d_it = mdl.get_depends().begin();
            d_it != mdl.get_depends().end(); ++d_it) {
        const std::string& d_mod_name = *d_it;
        module_state_t dep_mod_state = get_state(d_mod_name);
        if ((dep_mod_state == module_state_error) && (state == module_state_init))
            continue;

        if (dep_mod_state == module_state_error) {
            log(info, "dependent module %s is in error state, "
                    "cannot power up module %s\n", d_mod_name.c_str(), 
                    mod_name.c_str());

            return mdl.get_state();
        }

        if (dep_mod_state < state) {
            log(info, "powering up %s module dependencies %s\n",
                    mod_name.c_str(), d_mod_name.c_str());

            set_state(d_mod_name, state);
        }
    }

    // check if other module depend on this one
    for (module_map_t::iterator m_it = module_map.begin();
            m_it != module_map.end(); ++m_it) {
        module& mdl2 = *(m_it->second);
        if (mdl2.get_name() == mod_name)
            // skip this one
            continue;

        // iterate through dependencies
        for (module::depend_list_t::const_iterator d_it = mdl2.get_depends().begin();
                d_it != mdl2.get_depends().end(); ++d_it) {
            const std::string& d_mod_name = *d_it;
            if (d_mod_name != mod_name)
                continue;

            if (get_state(mdl2.get_name()) > state) {
                log(info, "%s depends on %s -> setting state to init\n",
                        mdl2.get_name().c_str(), mod_name.c_str());
                set_state(mdl2.get_name(), module_state_init);
                break;
            } else
                log(verbose, "%s depend on %s but is always in a lower"
                        " state\n", mdl2.get_name().c_str(), mod_name.c_str());
        }
    }

    log(info, "setting state of %s to %s\n",
            mod_name.c_str(), state_to_string(state));

    if (mdl.set_state(state) == -1)
        // throw exception
        throw str_exception("[robotkernel] ERROR: failed to switch module %s"
                " to state %s!", mod_name.c_str(), state_to_string(state));

    return mdl.get_state();
}

//! return module state
/*!
 * \param mod_name name of module which state to return
 * \return module state
 */
module_state_t kernel::_internal_get_state(std::string mod_name) {
    module_map_t::const_iterator it = module_map.find(mod_name);
    if (it == module_map.end())
        throw str_exception("[robotkernel] get_state: module %s not found!\n", mod_name.c_str());

    log(verbose, "getting state of module %s\n",
            mod_name.c_str());

    module *mdl = it->second;
    return mdl->get_state();
}

//! return module state
/*!
 * \param mod_name name of module which state to return
 * \return actual module state
 */
module_state_t kernel::get_state(std::string mod_name) {
    module_state_t _state;

    pthread_mutex_lock(&module_map_lock);
    _state = _internal_get_state(mod_name);
    pthread_mutex_unlock(&module_map_lock);

    return _state;
}

//! add service to kernel
/*!
 * \param owner service owner
 * \param name service name
 * \param service_definition service definition
 * \param callback service callback
 */
void kernel::add_service(
        const std::string& owner,
        const std::string& name, 
        const std::string& service_definition, 
        service_callback_t callback) {
    
    service_t *svc          = new service_t();
    svc->owner              = owner;
    svc->name               = name;
    svc->service_definition = service_definition;
    svc->callback           = callback;
    service_list[svc->name] = svc;
    
    for (service_providers_list_t::iterator it = service_providers.begin();
            it != service_providers.end(); ++it) {
        (*it)->add_service(*svc);
    }
}
        
//! remove all services from owner
/*!
 * \param owner service owner
 */
void kernel::remove_services(const std::string& owner) {
    for (service_list_t::iterator it = service_list.begin(); 
            it != service_list.end(); ++it) {
        if (it->second->owner != owner)
            continue;

        for (service_providers_list_t::iterator 
                it_prov = service_providers.begin(); 
                it_prov != service_providers.end();
                ++it_prov) {
            (*it_prov)->remove_service(*(it->second));
        }

        delete it->second;
    }
}

//! construction
/*!
 * \param configfile config file name
 */
kernel::kernel() {
    ll = info;
    _name = "robotkernel";
    char *ln_program_name = getenv("LN_PROGRAM_NAME");
    if (ln_program_name)
        _name = string(ln_program_name);

    log(info, PACKAGE_STRING "\n");

    int _major, _minor, _patch;
    sscanf(PACKAGE_VERSION, "%d.%d.%d", &_major, &_minor, &_patch);
    
    log(verbose, "major %d, minor %d, patch %d\n",
            _major, _minor, _patch);

    pthread_mutex_init(&module_map_lock, NULL);

    add_service(_name, "get_dump_log", service_definition_get_dump_log,
            std::bind(&kernel::service_get_dump_log, this, _1, _2));
    add_service(_name, "config_dump_log", service_definition_config_dump_log,
            std::bind(&kernel::service_config_dump_log, this, _1, _2));
    add_service(_name, "module_list", service_definition_module_list,
            std::bind(&kernel::service_module_list, this, _1, _2));
    add_service(_name, "reconfigure_module", service_definition_reconfigure_module,
            std::bind(&kernel::service_reconfigure_module, this, _1, _2));
    add_service(_name, "add_module", service_definition_add_module,
            std::bind(&kernel::service_add_module, this, _1, _2));
    add_service(_name, "remove_module", service_definition_remove_module,
            std::bind(&kernel::service_remove_module, this, _1, _2));
}

//! destruction
kernel::~kernel() {
    log(info, "destructing...\n");

    module_map_t::iterator it;
    while ((it = module_map.begin()) != module_map.end()) {
        module *mdl = it->second;
        try {
            set_state(mdl->get_name(), module_state_init);
        } catch (const std::exception& e) {
            // ignore this on destruction
        }

        module_map.erase(it);
        delete mdl;
    }

    pthread_mutex_destroy(&module_map_lock);

    log(info, "clean up finished\n");
}

//! get kernel singleton instance
/*!
 * \return kernel singleton instance
 */
kernel * kernel::get_instance() {
    if (!instance) {
        instance = new kernel();
        instance->rk_log.start();
    }

    return instance;
}

//! destroy singleton instance
void kernel::destroy_instance() {
    if (instance) {
        delete instance;

        instance = NULL;
    }
}

#ifdef __VXWORKS__
#define realpath(a, b) strdup(a)
#endif

#include <sys/stat.h>

//! construction
/*!
 * \param configfile config file name
 */
void kernel::config(std::string config_file, int argc, char **argv) {
    char *real_exec_file = realpath(argv[0], NULL);
    string path, file;
    if (!real_exec_file)
        throw str_exception("supplied exec file \"%s\" not found!", argv[0]);

    split_file_name(string(real_exec_file), path, file);
    log(verbose, "got exec path %s\n", path.c_str());
    log(info, "searching interfaces and modules ....\n");
    free(real_exec_file);

    string base_path, sub_path;
    split_file_name(path, base_path, sub_path);
    if (sub_path == string("bin")) {
        struct stat buf;
        stringstream _modpath, _intfpath;
        _modpath << base_path << "/lib/robotkernel/modules";
        if (stat(_modpath.str().c_str(), &buf) == 0)
            _internal_modpath = _modpath.str();
        _intfpath << base_path << "/lib/robotkernel/interfaces";
        if (stat(_intfpath.str().c_str(), &buf) == 0)
            _internal_intfpath = _intfpath.str();
    } else {
        // assume rmpm release 
        string base_path_2, arch = sub_path;
        split_file_name(base_path, base_path_2, sub_path);

        if (sub_path == string("bin")) {
            struct stat buf;
            stringstream _modpath, _intfpath;
            _modpath << base_path_2 << "/lib/" << arch << "/robotkernel/modules";
            if (stat(_modpath.str().c_str(), &buf) == 0)
                _internal_modpath = _modpath.str();
            _intfpath << base_path_2 << "/lib/" << arch << "/robotkernel/interfaces";
            if (stat(_intfpath.str().c_str(), &buf) == 0)
                _internal_intfpath = _intfpath.str();
        } else {
            log(info, "unable to determine internal modules/interfaces path!\n");
        }
    }

    if (_internal_modpath != string(""))
        log(info, "found modules path %s\n", _internal_modpath.c_str());
    if (_internal_intfpath != string(""))
        log(info, "found interfaces path %s\n", _internal_intfpath.c_str());

    if (config_file.compare("") == 0) {
        return;
    }

    char *real_config_file = realpath(config_file.c_str(), NULL);
    if (!real_config_file)
        throw str_exception("supplied config file \"%s\" not found!", config_file.c_str());

    split_file_name(string(real_config_file), path, file);

    log(info, "got config file path: %s, file name %s\n",
            path.c_str(), file.c_str());

    this->config_file = string(real_config_file);
    free(real_config_file);
    YAML::Node doc = YAML::LoadFile(this->config_file);

    // search for name
    char *ln_program_name = getenv("LN_PROGRAM_NAME");
    if (doc["name"] && !ln_program_name)
        _name = get_as<string>(doc, "name");

    // search for loglevel
    ll = get_as<string>(doc, "loglevel", "info");

    rk_log.fix_modname_length = 
        get_as<unsigned int>(doc, "log_fix_modname_length", 20);

    rk_log.sync_logging = 
        get_as<bool>(doc, "sync_logging", false);

    // search for log level
    if (doc["max_dump_log_len"]) {
        unsigned int len = 
            get_as<unsigned int>(doc, "max_dump_log_len");
        dump_log_set_len(len, 0);
    }

    _do_not_unload_modules = 
        get_as<bool>(doc, "do_not_unload_modules", false);

    // creating modules specified in config file
    const YAML::Node& modules = doc["modules"];
    for (YAML::const_iterator it = modules.begin(); it != modules.end(); ++it) {
        module* mdl;
        try {
            mdl = new module(*it, path);
        }
        catch(const exception& e) {
            throw str_exception("exception while instantiating module %s:\n%s",
                                get_as<string>(*it, "name", "<no name specified>").c_str(),
                                e.what());
        }
        if (!mdl->configured()) {
            string name = mdl->get_name();
            delete mdl;
            throw str_exception("[robotkernel] module %s not configured!\n", name.c_str());
        }

        if (module_map.find(mdl->get_name()) != module_map.end()) {
            string name = mdl->get_name();
            delete mdl;
            throw str_exception("[robotkernel] duplicate module name: %s\n", name.c_str());
        }

        // add to module map
        klog(info, "adding [%s]\n", mdl->get_name().c_str());
        module_map[mdl->get_name()] = mdl;
    }
}

//! powering up modules
bool kernel::power_up() {
    std::list<string> failed_modules;

    // powering up modules to operational
    for (module_map_t::iterator it = module_map.begin();
            it != module_map.end(); ++it) {
        module& mdl = *(it->second);

        module_state_t target_state = mdl.get_power_up(),
                       current_state = mdl.get_state();

        if (current_state == module_state_error) {
            log(info, "module '%s' in in error state, not "
                    "powering up!\n", mdl.get_name().c_str());
            failed_modules.push_back(mdl.get_name());
            continue;
        }

        if ((current_state >= target_state) && (target_state != module_state_boot))
            continue;

        try {
            log(info, "powering up '%s' to state %s\n", 
                    mdl.get_name().c_str(), state_to_string(target_state));
            set_state(mdl.get_name(), target_state);
        } catch (exception& e) {
            log(error, "caught exception: %s\n", e.what());
            failed_modules.push_back(mdl.get_name());
        }
    }

    if (!failed_modules.empty()) {
        string msg = "modules failed to switch to OP: ";

        for (std::list<string>::iterator it = failed_modules.begin();
             it != failed_modules.end(); ++it) {
            if(it != failed_modules.begin())
                msg += ", ";
            msg += *it;
        }
        log(error, "%s\n", msg.c_str());
        return false;
    }

    return true;
}

//! powering down modules
void kernel::power_down() {
    // powering up modules to operational
    for (module_map_t::iterator it = module_map.begin();
            it != module_map.end(); ++it) {
        module& mdl = *(it->second);

        try {
            set_state(mdl.get_name(), module_state_init);
        } catch (exception& e) {
            log(error, "caught exception: %s\n", e.what());
        }
    }
}

//! checks if all modules are at requested state
/*!
 * \param mod_name module name to check
 * \param state requested state
 *
 * \return true if we are in right  state
 */
bool kernel::state_check(std::string mod_name, module_state_t state) {
    kernel::module_map_t::const_iterator it = module_map.find(mod_name);
    if (it == module_map.end())
        throw str_exception("[robotkernel] state_check: module %s not found!\n", mod_name.c_str());

    module& mdl = *(it->second);
    return (mdl.get_state() == state);
}

bool kernel::state_check() {
    kernel::module_map_t::const_iterator it;
    for (it = module_map.begin(); it != module_map.end(); ++it) {
        module_state_t state = it->second->get_state();
        if (state == module_state_error) {
            klog(error, "module %s signaled error, switching "
                    "to init\n", it->first.c_str());
            set_state(it->first.c_str(), module_state_init);
        }
    }

    return true;
}

//! kernel register interface callback
/*!
 * \param mod_name module name to send request to
 * \param reqcode request code
 * \param ptr request specifix pointer
 * \return request status code
 */
int kernel::request_cb(const char *mod_name, int reqcode, void *ptr) {
    kernel& k = *get_instance();

    if (mod_name == NULL)
        return k.request(reqcode, ptr);

    if (currently_loading_module && mod_name == currently_loading_module->get_name())
        return currently_loading_module->request(reqcode, ptr);
    
    kernel::module_map_t::const_iterator it = k.module_map.find(mod_name);
    if (it == k.module_map.end())
        throw str_exception("[robotkernel] request_cb: module %s not found!\n", mod_name);

    return it->second->request(reqcode, ptr);
}
        
//! kernel register interface callback
/*!
 * \param if_name interface name to register
 * \param node configuration node
 * \return interface id or -1
 */
kernel::interface_id_t kernel::register_interface_cb(const char *if_name, 
        const YAML::Node& node, void* sp_interface) {
    interface *iface = new interface(if_name, node, sp_interface);
    if (!iface)
        return (interface_id_t)NULL;
    
    return (interface_id_t)iface;
}

//! kernel unregister interface callback
/*!
 * \param interface_id interface id
 * \return N/A
 */
void kernel::unregister_interface_cb(interface_id_t interface_id) {
    interface *iface = (interface *)interface_id;
    if (!iface)
        return;

    delete iface;
}

//! Send a request to kernel
/*!
  \param reqcode request code
  \param ptr pointer to request structure
  \return success or failure
  */
int kernel::request(int reqcode, void* ptr) {
    klog(error, "request %d\n", reqcode);

    return 0;
}

//! module state change
/*!
 * \param mod_name module name which changed state
 * \param new_state new state of module
 * \retun success
 */
int kernel::state_change(const char *mod_name, module_state_t new_state) {
    return 0;
    module_map_t::const_iterator it = module_map.find(mod_name);
    if (it == module_map.end())
        throw str_exception("[robotkernel] state_change: module %s not "
                "found!\n", mod_name);

    module_state_t current_state = it->second->get_state();
    if (new_state == current_state)
        return 0;

    klog(info, "WARNING: module %s changed state to %d, old state %d\n",
            mod_name, new_state, current_state);

    printf("\n\n\n THIS IS UNEXPECTED !!!! \n\n\n");

    int ret = set_state(mod_name, module_state_init);

    return ret;
}
        
//! register trigger module to module named mod_name
/*!
 * \param mod_name module name
 * \param trigger_mdl module which should be triggered by mod_name
 * \param t external trigger
 */
void kernel::trigger_register_module(const std::string& mod_name, 
        module *trigger_mdl, module::external_trigger& t) {

    pthread_mutex_lock(&module_map_lock);

    module_map_t::iterator module_it = module_map.find(mod_name);
    if (module_it == module_map.end()) {
        pthread_mutex_unlock(&module_map_lock);

        throw str_exception("%s not found\n", 
                mod_name.c_str());
    }

    module *mdl = module_it->second;
    mdl->trigger_register_module(trigger_mdl, t);

    pthread_mutex_unlock(&module_map_lock);
}

//! unregister trigger module from module named mod_name
/*!
 * \param mod_name module name
 * \param trigger_mdl module which was triggered by mod_name
 * \param t external trigger
 */
void kernel::trigger_unregister_module(const std::string& mod_name, 
        module *trigger_mdl, module::external_trigger& t) {

    pthread_mutex_lock(&module_map_lock);

    module_map_t::iterator module_it = module_map.find(mod_name);
    if (module_it == module_map.end()) {
        pthread_mutex_unlock(&module_map_lock);

        throw str_exception("%s not found\n", 
                mod_name.c_str());
    }

    module *mdl = module_it->second;
    mdl->trigger_unregister_module(trigger_mdl, t);

    pthread_mutex_unlock(&module_map_lock);
}

//! get dump log
/*!
 * \param request service request data
 * \parma response service response data
 * \return success
 */
int kernel::service_get_dump_log(const service_arglist_t& request, 
        service_arglist_t& response) {
#define GET_DUMP_LOG_RESP_LOG   0
    response.resize(1);
    response[0] = dump_log_dump();

    return 0;
}

const std::string kernel::service_definition_get_dump_log = 
    "response:\n"
    "    string: log\n";

//! config dump log
/*!
 * \param request service request data
 * \parma response service response data
 * \return success
 */
int kernel::service_config_dump_log(const service_arglist_t& request, 
        service_arglist_t& response) {
    // request data
#define CONFIG_DUMP_LOG_REQ_MAX_LEN         0
#define CONFIG_DUMP_LOG_REQ_DO_UST          1
#define CONFIG_DUMP_LOG_REQ_SET_LOGLEVEL    2
    uint32_t max_len        = request[CONFIG_DUMP_LOG_REQ_MAX_LEN]; 
    uint8_t  do_ust         = request[CONFIG_DUMP_LOG_REQ_DO_UST];
    string   set_loglevel   = request[CONFIG_DUMP_LOG_REQ_SET_LOGLEVEL];

    // response data
    string current_loglevel = "";
    string error_message    = "";

    dump_log_set_len(max_len, do_ust);
    klog(info, "dump_log len set to %d, do_ust to %d\n", max_len, do_ust);

#define loglevel_to_string(x)             \
    if (ll == x)                          \
    current_loglevel = string(#x)

    loglevel_to_string(error);
    else loglevel_to_string(warning);
    else loglevel_to_string(info);
    else loglevel_to_string(verbose);

    if(set_loglevel.size()) {
        py_value *pval = eval_full(set_loglevel);
        py_string *pstring = dynamic_cast<py_string *>(pval);

        if (pstring) 
            ll = string(*pstring);
    }

#define CONFIG_DUMP_LOG_RESP_CURRENT_LOGLEVEL   0
#define CONFIG_DUMP_LOG_RESP_ERROR_MESSAGE      1
    response.resize(2);
    response[CONFIG_DUMP_LOG_RESP_CURRENT_LOGLEVEL] = current_loglevel;
    response[CONFIG_DUMP_LOG_RESP_ERROR_MESSAGE]    = error_message;

    return 0;
}

const std::string kernel::service_definition_config_dump_log = 
    "request:\n"
    "    uint32_t: max_len\n"
    "    uint8_t: do_ust\n"
    "    string: set_loglevel\n"
    "response:\n"
    "    string: current_loglevel\n"
    "    string: error_message\n";

//! add module
/*!
 * \param request service request data
 * \parma response service response data
 * \return success
 */
int kernel::service_add_module(const service_arglist_t& request, 
        service_arglist_t& response) {
    // request data
#define ADD_MODULE_REQ_CONFIG   0
    string config = request[ADD_MODULE_REQ_CONFIG];

    //response data
    string error_message = "";
    
    klog(info, "got config: %s\n", config.c_str());

    try {
        YAML::Node node = YAML::Load(config);
        module *mdl = new module(node, "");

        if (module_map.find(mdl->get_name()) != module_map.end()) {
            string name = mdl->get_name();
            delete mdl;
            throw str_exception("[robotkernel] duplicate module name: %s\n", name.c_str());
        }

        if (!mdl->configured()) {
            string name = mdl->get_name();
            delete mdl;
            throw str_exception("[robotkernel] module %s not configured!\n", name.c_str());
        }

        // add to module map
        module_map[mdl->get_name()] = mdl;
    } catch(exception& e) {
        error_message = e.what();
    }

#define ADD_MODULE_RESP_ERROR_MESSAGE   0
    response.resize(1);
    response[ADD_MODULE_RESP_ERROR_MESSAGE] = error_message;

    return 0;
}

const std::string kernel::service_definition_add_module = 
    "request:\n"
    "    string: config\n"
    "response:\n"
    "    string: error_message\n";

//! remove module
/*!
 * \param request service request data
 * \parma response service response data
 * \return success
 */
int kernel::service_remove_module(const service_arglist_t& request, 
        service_arglist_t& response) {
    // request data
#define REMOVE_MODULE_REQ_MOD_NAME  0
    string mod_name = request[REMOVE_MODULE_REQ_MOD_NAME];

    //response data
    string error_message = "";

    try {
        module_map_t::iterator it = module_map.find(mod_name);
        if (it == module_map.end())
            throw str_exception("[robotkernel] module %s not found!\n", 
                    mod_name.c_str());

        module *mdl = it->second;
        set_state(mdl->get_name(), module_state_init);

        module_map.erase(it);
        delete mdl;
    } catch (exception& e) {
        error_message = e.what();
    }

#define REMOVE_MODULE_RESP_ERROR_MESSAGE    0
    response[REMOVE_MODULE_RESP_ERROR_MESSAGE] = error_message;

    return 0;
}

const std::string kernel::service_definition_remove_module = 
    "request:\n"
    "    string: mod_name\n"
    "response:\n"
    "    string: error_message\n";

//! module list
/*!
 * \param request service request data
 * \parma response service response data
 * \return success
 */
int kernel::service_module_list(const service_arglist_t& request, 
        service_arglist_t& response) {
    //response data
    std::vector<rk_type> modules(0);   
    string error_message = "";

    try {        
        int i = 0;
        modules.resize(module_map.size());

        for (module_map_t::const_iterator
                it = module_map.begin(); it != module_map.end(); ++it) {
            modules[i++] = it->first;
        }
    } catch(exception& e) {
        error_message = e.what();
    }
    
#define MODULE_LIST_RESP_MODULES        0
#define MODULE_LIST_RESP_ERROR_MESSAGE  1
    response.resize(2);
    response[MODULE_LIST_RESP_MODULES]       = modules;
    response[MODULE_LIST_RESP_ERROR_MESSAGE] = error_message;
    
    return 0;
}

const std::string kernel::service_definition_module_list = 
    "response:\n"
    "    vector/string: modules\n"
    "    string: error_message\n";

//! reconfigure module
/*!
 * \param request service request data
 * \parma response service response data
 * \return success
 */
int kernel::service_reconfigure_module(const service_arglist_t& request, 
        service_arglist_t& response) {
    // request data
#define RECONFIGURE_MODULE_REQ_MOD_NAME 0
    string mod_name = request[RECONFIGURE_MODULE_REQ_MOD_NAME]; 

    //response data
    uint64_t state = 0;
    string error_message = "";

    try {
        module_map_t::iterator it = module_map.find(mod_name);
        if (it == module_map.end())
            throw str_exception("[robotkernel] module %s not found!\n", 
                    mod_name.c_str());

        module *mdl = it->second;
        if (get_state(mod_name) != module_state_init)
            set_state(mdl->get_name(), module_state_init);

        mdl->reconfigure();
    } catch(exception& e) {
        error_message = e.what();
    }
    
#define RECONFIGURE_MODULE_RESP_STATE           0
#define RECONFIGURE_MODULE_RESP_ERROR_MESSAGE   1
    response.resize(2);
    response[RECONFIGURE_MODULE_RESP_STATE]         = state;
    response[RECONFIGURE_MODULE_RESP_ERROR_MESSAGE] = error_message;

    return 0;
}

const std::string kernel::service_definition_reconfigure_module = 
    "request:\n"
    "    string: mod_name\n"
    "response:\n"
    "    uint64_t: state\n"
    "    string: error_message\n";

std::string kernel::ll_to_string(loglevel ll) {
    if (ll == error)    return "ERR ";
    if (ll == warning)  return "WARN";
    if (ll == info)     return "INFO";
    if (ll == verbose)  return "VERB";

    return "????";
}

#include "robotkernel/dump_log.h"

void kernel::log(loglevel lvl, const char *format, ...) {  
    struct log_thread::log_pool_object *obj;
    
    char buf[1024];
    int bufpos = 0;
    bufpos += snprintf(buf+bufpos, sizeof(buf)+bufpos, "[%s|robotkernel] ", 
            _name.c_str());

    // format argument list    
    va_list args;
    va_start(args, format);
    vsnprintf(buf+bufpos, sizeof(buf)-bufpos, format, args);
    va_end(args);
    
    if (ll < lvl)
        goto log_exit;

    if ((obj = rk_log.get_pool_object()) != NULL) {
        // only ifempty log pool avaliable!
        struct tm timeinfo;
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        double timestamp = (double)ts.tv_sec + (ts.tv_nsec / 1e9);
        time_t seconds = (time_t)timestamp;
        double mseconds = (timestamp - (double)seconds) * 1000.;
        localtime_r(&seconds, &timeinfo);
        strftime(obj->buf, sizeof(obj->buf), "%F %T", &timeinfo);


        int len = strlen(obj->buf);
        snprintf(obj->buf + len, sizeof(obj->buf) - len, ".%03.0f ", mseconds);
        len = strlen(obj->buf);

        snprintf(obj->buf + len, sizeof(obj->buf) - len, "%s %s", 
                ll_to_string(lvl).c_str(), buf);
        rk_log.log(obj);
    }
    
log_exit:
    ::dump_log(buf);
}

