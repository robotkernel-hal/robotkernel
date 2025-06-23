//! robotkernel interface definition
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

#include <sys/mman.h>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <string_util/string_util.h>

// public headers
#include "robotkernel/helpers.h"
#include "robotkernel/module_base.h"
#include "robotkernel/bridge_base.h"
#include "robotkernel/config.h"
#include "robotkernel/service_definitions.h"

// private headers
#include "kernel.h"
#include "rkc_loader.h"

#include "yaml-cpp/yaml.h"
#include "sys/stat.h"
#include "fcntl.h"

using namespace std;
using namespace std::placeholders;
using namespace robotkernel;
using namespace string_util;

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

//! log object to trace fd
void kernel::trace_write(const char *fmt, ...) {
    va_list ap;
    char buf[256];
    int n;

    if (trace_fd < 0)
        return;

    va_start(ap, fmt);
    n = vsnprintf(buf, 256, fmt, ap);
    va_end(ap);

    int local_ret = write(trace_fd, buf, n);
    (void)local_ret;
}
        
void kernel::trace_write(const struct log_thread::log_pool_object *obj) {
    if (trace_fd < 0)
        return;

    int local_ret = write(trace_fd, obj->buf, obj->len);
    (void)local_ret;
}

//! kernel singleton instance
kernel kernel::instance;

//! set state of module
/*!
 * \param mod_name name of module
 * \param state new module state
 * \return state
 */
int kernel::set_state(std::string mod_name, module_state_t state, 
        std::list<std::string> caller) {
    module_map_t::const_iterator it = module_map.find(mod_name);
    if (it == module_map.end())
        throw str_exception("[robotkernel] set_state: module %s not "
                "found!\n", mod_name.c_str());

    sp_module_t mdl = it->second;
    if (mdl->get_state() == state)
        return state;

    if (state == module_state_boot) {
        set_state(mod_name, module_state_init);
    }

    // iterate through dependencies
    for (const auto& d_mod_name : mdl->get_depends()) {
        if (std::find(caller.begin(), caller.end(), d_mod_name) != caller.end())
            continue; // do not recurse any further

        module_state_t dep_mod_state = get_state(d_mod_name);
        if ((dep_mod_state == module_state_error) && (state == module_state_init))
            continue;

        if (dep_mod_state == module_state_error) {
            log(info, "dependent module %s is in error state, "
                    "cannot power up module %s\n", d_mod_name.c_str(), 
                    mod_name.c_str());

            return mdl->get_state();
        }

        if ((dep_mod_state < state) || (dep_mod_state == module_state_boot)) {
            log(info, "powering up %s module dependencies %s\n",
                    mod_name.c_str(), d_mod_name.c_str());

            caller.push_back(mod_name);
            set_state(d_mod_name, state, caller);
        }
    }

    // check if other module depend on this one
    for (auto& kv : module_map) {
        sp_module_t mdl2 = kv.second;
        if (mdl2->get_name() == mod_name)
            // skip this one
            continue;

        // iterate through dependencies
        for (const auto& d_mod_name : mdl2->get_depends()) {
            if (std::find(caller.begin(), caller.end(), d_mod_name) != caller.end())
                continue; // do not recurse any further
            if (d_mod_name != mod_name)
                continue; // not dependent to us

            if (get_state(mdl2->get_name()) > state) {
                log(info, "%s depends on %s -> setting state to init\n",
                        mdl2->get_name().c_str(), mod_name.c_str());
                caller.push_back(mod_name);
                set_state(mdl2->get_name(), module_state_init, caller);
                break;
            } else
                log(verbose, "%s depend on %s but is always in a lower"
                        " state\n", mdl2->get_name().c_str(), mod_name.c_str());
        }
    }

    if (state != module_state_init) {
        for (const auto& e_mod_name : mdl->get_excludes()) {
            if (std::find(caller.begin(), caller.end(), e_mod_name) != caller.end())
                continue; // do not recurse any further

            module_state_t exc_mod_state = get_state(e_mod_name);
            if (exc_mod_state == module_state_init) {
                continue;
            }

            log(info, "switching excluded module \"%s\" to init\n", e_mod_name.c_str());
            set_state(e_mod_name, module_state_init, caller);
        }
    }

    log(info, "setting state of %s to %s\n",
            mod_name.c_str(), state_to_string(state));

    if (mdl->set_state(state) == -1)
        // throw exception
        throw str_exception("[robotkernel] ERROR: failed to switch module %s"
                " to state %s!", mod_name.c_str(), state_to_string(state));

    return mdl->get_state();
}

//! return module state
/*!
 * \param mod_name name of module which state to return
 * \return actual module state
 */
module_state_t kernel::get_state(std::string mod_name) {
    module_map_t::const_iterator it = module_map.find(mod_name);
    if (it == module_map.end())
        throw str_exception("[robotkernel] get_state: module %s not found!\n", mod_name.c_str());

    log(verbose, "getting state of module %s\n",
            mod_name.c_str());

    sp_module_t mdl = it->second;
    return mdl->get_state();
}

//! call a robotkernel service
/*!
 * \param[in]  name          Name of service to call.
 * \param[in]  req           Service request parameters.
 * \param[out] resp          Service response parameters.
 */
void kernel::call_service(const std::string& name, 
        const service_arglist_t& req, service_arglist_t& resp) {
    for (auto& kv : services) {
        string svc_name = kv.first.first + "." + kv.first.second;

        if (svc_name != name) 
            continue;
    
        service_t *svc = kv.second;
        svc->callback(req, resp);
        return;
    }
        
    throw str_exception("service \"%s\" not found!\n", name.c_str());
}

//! call a robotkernel service
/*!
 * \param[in]  owner         Owner of service to call.
 * \param[in]  name          Name of service to call.
 * \param[in]  req           Service request parameters.
 * \param[out] resp          Service response parameters.
 */
void kernel::call_service(const std::string& owner, const std::string& name, 
        const service_arglist_t& req, service_arglist_t& resp) {
    
    service_map_t::iterator it;
    if ((it = services.find(std::make_pair(owner, name))) == services.end()) {
        throw str_exception("service \"%s.%s\" not found!\n", owner.c_str(), name.c_str());
    }
    
    for (const auto& kv : bridge_map)
        kv.second->remove_service(*(it->second));

    service_t *svc = it->second;
    svc->callback(req, resp);
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
    
    if (services.find(std::make_pair(owner, name)) != services.end()) {
        log(warning, "SKIPPING service (already in) owner \"%s\", name \"%s\", service_definition:\n%s\n", 
                owner.c_str(), name.c_str(), service_definition.c_str());
        return;
    }

    log(verbose, "adding service owner \"%s\", name \"%s\", service_definition:\n%s\n", 
            owner.c_str(), name.c_str(), service_definition.c_str());

    service_t *svc          = new service_t();
    svc->owner              = owner;
    svc->name               = name;
    svc->service_definition = service_definition;
    svc->callback           = callback;
    services[std::make_pair(owner, name)] = svc;

    for (const auto& kv : bridge_map)
        kv.second->add_service(*svc);
}

//! remove on service given by name
/*!
 * \param[in] owner     Owner of service.
 * \param[in] name      Name of service.
 */
void kernel::remove_service(const std::string& owner, const std::string& name) {
    service_map_t::iterator it;
    if ((it = services.find(std::make_pair(owner, name))) == services.end())
        return; // service not found
    
    for (const auto& kv : bridge_map)
        kv.second->remove_service(*(it->second));

    delete it->second;
    services.erase(it);
}

//! remove all services from owner
/*!
 * \param owner service owner
 */
void kernel::remove_services(const std::string& owner) {
    // remove all slaves from service providers
    for (service_provider_map_t::iterator it = service_provider_map.begin();
            it != service_provider_map.end(); ++it) {
        it->second->remove_module(owner);
    }

    for (service_map_t::iterator it = services.begin(); 
            it != services.end(); ) {
        const auto& svc = *(it->second);

        if (svc.owner != owner) {
            ++it;
            continue;
        }

        log(verbose, "removing service %s.%s\n", svc.owner.c_str(), svc.name.c_str());
    
        for (const auto& kv : bridge_map)
            kv.second->remove_service(svc);


        delete it->second;
        it = services.erase(it);
    }
}

//! construction
/*!
 * \param configfile config file name
 */
kernel::kernel() :
    log_base(info)
{
    _name = "robotkernel";

    rk_log.start();
}
        
//! destruction
kernel::~kernel() {
    log(info, "destructing...\n");

    log(info, "removing modules\n");

    // first step: set all modules to init
    for (const auto& kv : module_map) {
        sp_module_t mdl = kv.second;
    
        try {
            set_state(mdl->get_name(), module_state_init);
        } catch (const std::exception& e) {
            // ignore this on destruction
            log(warning, "got exception from set_state: %s\n", e.what());
        }
    }

    // second step: erase module now, this will call module destructor
    module_map_t::iterator it;
    while ((it = module_map.begin()) != module_map.end()) {
        std::unique_lock<std::recursive_mutex> lock(module_map_mtx);
        module_map.erase(it);
    }
    
    log(info, "removing bridges\n");
    bridge_map_t::iterator bit;
    while ((bit = bridge_map.begin()) != bridge_map.end()) {
        sp_bridge_t bridge = bit->second;
        bridge_map.erase(bit);

        log(verbose, "    bridge %s\n", bridge->name.c_str());
    }
    
    log(info, "removing service providers\n");
    service_provider_map_t::iterator sit;
    while ((sit = service_provider_map.begin()) != service_provider_map.end()) {
        sp_service_provider_t sp = sit->second;
        service_provider_map.erase(sit);

        log(verbose, "    service_provider %s\n", sp->name.c_str());
    }

    // remove services
    log(verbose, "removing services\n");
    service_map_t::iterator slit;
    while ((slit = services.begin()) != services.end()) {
        const auto& svc = *(slit->second);

        log(verbose, "    service %s.%s\n", svc.owner.c_str(), svc.name.c_str());
    
        for (const auto& kv : bridge_map)
            kv.second->remove_service(svc);

        delete slit->second;
        services.erase(slit);
    }


    // remove process data
    log(verbose, "removing process data\n");
    std::map<std::string, sp_process_data_t>::iterator pdit;
    while ((pdit = process_data_map.begin()) != process_data_map.end()) {
        log(verbose, "    process_data %d\n", pdit->first.c_str());
        process_data_map.erase(pdit);
    }

    log(info, "clean up finished\n");
    dump_log_free();
}

//! Register a new datatype description
/*!
 * \param[in]   name        Datatype name.
 * \param[in]   desc        Datatype description.
 *
 * \throw Exception if datatype was already found.
 */
void kernel::add_datatype_desc(const std::string& name, const std::string& desc) {
    datatypes_map_t::iterator dtm_it = datatypes_map.find(name);

    if (dtm_it != datatypes_map.end()) {
        if ((*dtm_it).second.compare(desc) != 0) {
            throw str_exception("registering datatype %s was not successfull, already found "
                    "with different content!", name.c_str());
        }

        return; // description is the same as alread in.
    }

    datatypes_map[name] = desc;
}

//! get a registered datatype
/*!
 * \param[in]   name        Datatype name.
 *
 * \throw Exception if datatype is not found.
 *
 * \return String containing datatype description.
 */
const std::string kernel::get_datatype_desc(const std::string&name) {
    datatypes_map_t::iterator dtm_it = datatypes_map.find(name);

    if (dtm_it == datatypes_map.end()) {
        throw str_exception("getting datatype %s was not successfull, not found!", name.c_str());
    }

    return dtm_it->second;
}

#ifdef __VXWORKS__
#define realpath(a, b) strdup(a)
#endif

#include <sys/stat.h>

//! construction
/*!
 * \param configfile config file name
 */
void kernel::config(std::string config_file, int argc, char *argv[]) {
    main_argc = argc; 
    main_argv = argv;

    char *real_exec_file = realpath("/proc/self/exe", NULL);
    string file;
    if (!real_exec_file)
        throw str_exception("supplied exec file \"%s\" not found: %s", argv[0], 
                strerror(errno));

    split_file_name(string(real_exec_file), exec_file_path, file);
    log(verbose, "got exec path %s\n", exec_file_path.c_str());
    log(verbose, "searching interfaces and modules ....\n");
    free(real_exec_file);

    string base_path, sub_path;
    split_file_name(exec_file_path, base_path, sub_path);
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
        log(verbose, "found modules path %s\n", _internal_modpath.c_str());
    if (_internal_intfpath != string(""))
        log(verbose, "found interfaces path %s\n", _internal_intfpath.c_str());

    if (config_file.compare("") == 0) {
        return;
    }

    char *real_config_file = realpath(config_file.c_str(), NULL);
    if (!real_config_file)
        throw str_exception("supplied config file \"%s\" not found!", config_file.c_str());

    split_file_name(string(real_config_file), config_file_path, file);

    log(verbose, "got config file path: %s, file name %s\n",
            config_file_path.c_str(), file.c_str());

    this->config_file = string(real_config_file);
    free(real_config_file);
    YAML::Node doc = rkc_load_file(this->config_file);

    // search for name
    char *ln_program_name = getenv("LN_PROGRAM_NAME");
    if (ln_program_name) {
        unsetenv("LN_PROGRAM_NAME");
    }
    _name = get_as<string>(doc, "name");
    log_base::name = _name;

    log(info, "Robotkernel " PACKAGE_VERSION "\n");

    // locking all current and future memory to keep it hold in 
    // memory without swapping
    mlockall(MCL_CURRENT | MCL_FUTURE);

    // search for loglevel
    ll = get_as<string>(doc, "loglevel", "info");

    log_to_trace_fd = get_as<bool>(doc, "log_to_trace_fd", false);
    if (do_log_to_trace_fd()) {
        trace_fd = open("/sys/kernel/debug/tracing/trace_marker", O_WRONLY);
        if (trace_fd == -1) 
            throw errno_exception_tb("cannot open trace_marker\n");
    }
    
    log_to_lttng_ust = get_as<bool>(doc, "log_to_lttng_ust", false);

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
        sp_module_t mdl;
        try {
            mdl = make_shared<module>(*it);
        }
        catch(const exception& e) {
            throw str_exception("exception while instantiating module %s:\n%s",
                                get_as<string>(*it, "name", "<no name specified>").c_str(),
                                e.what());
        }
        if (!mdl->configured()) {
            string name = mdl->get_name();
            throw str_exception("[robotkernel] module %s not configured!\n", name.c_str());
        }

        if (mdl->get_name() == _name){
            string name = mdl->get_name();
            throw str_exception("[robotkernel] module name \"%s\" matches robotkernel name\n", name.c_str());
        }

        {
            std::unique_lock<std::recursive_mutex> lock(module_map_mtx);

            if (module_map.find(mdl->get_name()) != module_map.end()) {
                string name = mdl->get_name();
                throw str_exception("[robotkernel] duplicate module name: %s\n", name.c_str());
            }
        }

        // add to module map
        log(verbose, "adding [%s]\n", mdl->get_name().c_str());
        module_map[mdl->get_name()] = mdl;
        
        mdl->set_state(module_state_init);
    }

    const YAML::Node& bridges = doc["bridges"];
    for (YAML::const_iterator it = bridges.begin(); it != bridges.end(); ++it) {
        sp_bridge_t brdg;
        try {
            brdg = make_shared<bridge>(*it);
        }
        catch(const exception& e) {
            throw str_exception("exception while instantiating bridge %s:\n%s",
                                get_as<string>(*it, "name", "<no name specified>").c_str(),
                                e.what());
        }

        if (brdg->name == _name){
            string name = brdg->name;
            throw str_exception("[robotkernel] bridge name \"%s\" matches robotkernel name\n", name.c_str());
        }

        if (bridge_map.find(brdg->name) != bridge_map.end()) {
            throw str_exception("[robotkernel] duplicate module name: %s\n", 
                    brdg->name.c_str());
        }

        // add to module map
        log(verbose, "adding [%s]\n", brdg->name.c_str());
        bridge_map[brdg->name] = brdg;

        for (const auto& kv : services)
            brdg->add_service(*(kv.second));
    }
    
    const YAML::Node& service_providers = doc["service_providers"];
    for (YAML::const_iterator it = service_providers.begin(); it != service_providers.end(); ++it) {
        sp_service_provider_t sp;
        try {
            sp = make_shared<service_provider>(*it);
        }
        catch(const exception& e) {
            throw str_exception("exception while instantiating service_provider %s:\n%s",
                                get_as<string>(*it, "name", "<no name specified>").c_str(),
                                e.what());
        }

        if (sp->name == _name){
            string name = sp->name;
            throw str_exception("[robotkernel] service_provider name \"%s\" matches robotkernel name\n", name.c_str());
        }

        if (service_provider_map.find(sp->name) != service_provider_map.end()) {
            throw str_exception("[robotkernel] duplicate module name: %s\n", 
                    sp->name.c_str());
        }

        // add to module map
        log(verbose, "adding [%s]\n", sp->name.c_str());
        service_provider_map[sp->name] = sp;
    }

    add_svc_get_dump_log(_name, "get_dump_log");
    add_svc_config_dump_log(_name, "config_dump_log");
    add_svc_module_list(_name, "module_list");
    add_svc_reconfigure_module(_name, "reconfigure_module");
    add_svc_add_module(_name, "add_module");
    add_svc_remove_module(_name, "remove_module");
    add_svc_list_devices(_name, "list_devices");
    add_svc_process_data_info(_name, "process_data_info");
    add_svc_trigger_info(_name, "trigger_info");
    add_svc_stream_info(_name, "stream_info");
    add_svc_service_interface_info(_name, "service_interface_info");
    add_svc_add_pd_injection(_name, "add_pd_injection");
    add_svc_del_pd_injection(_name, "del_pd_injection");
    add_svc_list_pd_injections(_name, "list_pd_injections");
    add_svc_configure_loglevel(_name, "configure_loglevel");
}

//! powering up modules
bool kernel::power_up() {
    std::unique_lock<std::recursive_mutex> lock(module_map_mtx);
    std::list<string> failed_modules;

    // powering up modules to operational
    for (module_map_t::iterator it = module_map.begin();
            it != module_map.end(); ++it) {
        sp_module_t mdl = it->second;

        module_state_t target_state = mdl->get_power_up(),
                       current_state = mdl->get_state();

        if (current_state == module_state_error) {
            log(info, "module '%s' in in error state, not "
                    "powering up!\n", mdl->get_name().c_str());
            failed_modules.push_back(mdl->get_name());
            continue;
        }

        if ((current_state >= target_state) && (target_state != module_state_boot))
            continue;

        try {
            log(info, "powering up '%s' to state %s\n", 
                    mdl->get_name().c_str(), state_to_string(target_state));
            set_state(mdl->get_name(), target_state);
        } catch (exception& e) {
            log(error, "caught exception: %s\n", e.what());
            failed_modules.push_back(mdl->get_name());
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
    std::unique_lock<std::recursive_mutex> lock(module_map_mtx);
    
    // powering up modules to operational
    for (module_map_t::iterator it = module_map.begin();
            it != module_map.end(); ++it) {
        sp_module_t mdl = it->second;

        try {
            set_state(mdl->get_name(), module_state_init);
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
    std::unique_lock<std::recursive_mutex> lock(module_map_mtx);

    module_map_t::const_iterator it = module_map.find(mod_name);
    if (it == module_map.end())
        throw str_exception("[robotkernel] state_check: module %s not found!\n", mod_name.c_str());

    sp_module_t mdl = it->second;
    return (mdl->get_state() == state);
}

bool kernel::state_check() {
    std::unique_lock<std::recursive_mutex> lock(module_map_mtx);

    for (auto& kv : module_map) {
        module_state_t state = kv.second->get_state();
        if (state == module_state_error) {
            log(error, "module %s signaled error, switching "
                    "to init\n", kv.first.c_str());
            set_state(kv.first.c_str(), module_state_init);
        }
    }

    return true;
}

//! get module
/*!
 * \param mod_name name of module
 * \return shared pointer to module
 */
sp_module_t kernel::get_module(const std::string& mod_name) {
    module_map_t::const_iterator it = module_map.find(mod_name);
    if (it == module_map.end())
        throw str_exception("[robotkernel] get_module: module %s not found!\n", 
                mod_name.c_str());

    return it->second;
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

    log(info, "WARNING: module %s changed state to %d, old state %d\n",
            mod_name, new_state, current_state);

    printf("\n\n\n THIS IS UNEXPECTED !!!! \n\n\n");

    int ret = set_state(mod_name, module_state_init);

    return ret;
}
        
// adds a device listener
void kernel::add_device_listener(sp_device_listener_t dl) {
    auto key = std::make_pair(dl->owner, dl->name);
    if (dl_map.find(key) != dl_map.end()) {
        log(warning, "duplicate device listener! owner %s, name %s\n", 
                dl->owner.c_str(), dl->name.c_str());
        return;
    }

    dl_map[key] = dl;
   
    for (const auto& kv : device_map)
        dl->notify_add_device(kv.second);
}

// remove a device listener
void kernel::remove_device_listener(sp_device_listener_t dl) {
    auto key = std::make_pair(dl->owner, dl->name);
    auto it = dl_map.find(key);
    if (it == dl_map.end()) {
        log(warning, "cannot remove device listener (does not exists)! owner %s, name %s\n", 
                dl->owner.c_str(), dl->name.c_str());
        return;
    }

    for (const auto& kv : device_map)
        dl_map[key]->notify_remove_device(kv.second);

    dl_map[key] = nullptr;
    dl_map.erase(it);
}

// add a named device
void kernel::add_device(sp_device_t req) {
    auto map_index = req->id();
    if (device_map.find(map_index) != device_map.end()) {
        log(warning, "duplicate regiser of device \"%s\", ignoring new device!\n", map_index.c_str());
        return; // already in
    }

    log(verbose, "registered device \"%s\"\n", map_index.c_str());
    device_map[map_index] = req;

    for (const auto& kv : dl_map) 
        kv.second->notify_add_device(req);
};
        
// remove a named device
void kernel::remove_device(sp_device_t req) {
    auto map_index = req->id();

    log(verbose, "removing device %s\n", map_index.c_str());

    for (const auto& kv : dl_map) 
        kv.second->notify_remove_device(req);

    auto it = device_map.find(map_index);
    if (it == device_map.end())
        return; // no device with name found

    device_map[map_index] = nullptr;
    device_map.erase(it);
};

// remove all devices from owner
void kernel::remove_devices(const std::string& owner) {
    for (auto it = device_map.begin(); it != device_map.end(); ) {
        if (it->second->owner == owner) {
            log(verbose, "removing device %s\n", it->second->id().c_str());
            it = device_map.erase(it);
        } else
            ++it;
    }
}

//! loads additional modules
/*!
 * \param[in] config    New module configuration.
 */
void kernel::load_module(const YAML::Node& config) {
    string name = get_as<string>(config, "name");

    if (module_map.find(name) != module_map.end()) {
        throw str_exception("[robotkernel] duplicate module name: %s\n", name.c_str());
    }

    sp_module_t mdl = make_shared<module>(config);

    if (!mdl->configured()) {
        throw str_exception("[robotkernel] module %s not configured!\n", name.c_str());
    }

    // add to module map
    module_map[mdl->get_name()] = mdl;
        
    mdl->set_state(module_state_init);
}

//! svc_get_dump_log
/*!
 * \param[in]   req     Service request data.
 * \param[out]  resp    Service response data.
 */
void kernel::svc_get_dump_log(
        const struct services::robotkernel::kernel::svc_req_get_dump_log& req, 
        struct services::robotkernel::kernel::svc_resp_get_dump_log& resp) 
{
    resp.log = dump_log_dump();
}

//! svc_config_dump_log
/*!
 * \param[in]   req     Service request data.
 * \param[out]  resp    Service response data.
 */
void kernel::svc_config_dump_log(
        const struct services::robotkernel::kernel::svc_req_config_dump_log& req, 
        struct services::robotkernel::kernel::svc_resp_config_dump_log& resp) 
{
    // response data
    resp.current_loglevel = "";
    resp.error_message    = "";

    dump_log_set_len(req.max_len, req.do_ust);
    log(info, "dump_log len set to %d, do_ust to %d\n", req.max_len, req.do_ust);

#define loglevel_to_string(x)             \
    if (ll == x)                          \
    resp.current_loglevel = string(#x)

    loglevel_to_string(error);
    else loglevel_to_string(warning);
    else loglevel_to_string(info);
    else loglevel_to_string(verbose);

    if(req.set_loglevel.size()) {
        py_value *pval = eval_full(req.set_loglevel);
        py_string *pstring = dynamic_cast<py_string *>(pval);

        if (pstring) 
            ll = string(*pstring);
    }
}

//! svc_add_module
/*!
 * \param[in]   req     Service request data.
 * \param[out]  resp    Service response data.
 */
void kernel::svc_add_module(
        const struct services::robotkernel::kernel::svc_req_add_module& req, 
        struct services::robotkernel::kernel::svc_resp_add_module& resp) 
{
    try {
        YAML::Node node = YAML::Load(req.config);
        string mod_name = get_as<string>(node, "name");
        string so_file  = get_as<string>(node, "so_file");

        log(info, "adding module \"%s\" as \"%s\"\n", so_file.c_str(), mod_name.c_str());
        load_module(node);
        log(info, "module \"%s\" added\n", mod_name.c_str());
    } catch(exception& e) {
        resp.error_message = e.what();
        log(error, "error adding module \"%s\": %s\n", resp.error_message.c_str());
    }

}

//! svc_remove_module
/*!
 * \param[in]   req     Service request data.
 * \param[out]  resp    Service response data.
 */
void kernel::svc_remove_module(
        const struct services::robotkernel::kernel::svc_req_remove_module& req, 
        struct services::robotkernel::kernel::svc_resp_remove_module& resp)
{
    try {
        log(info, "removing module \"%s\"\n", req.mod_name.c_str());

        std::unique_lock<std::recursive_mutex> lock(module_map_mtx);
        module_map_t::iterator it = module_map.find(req.mod_name);
        if (it == module_map.end())
            throw str_exception("[robotkernel] module %s not found!\n", 
                    req.mod_name.c_str());

        sp_module_t mdl = it->second;
        set_state(mdl->get_name(), module_state_init);

        module_map.erase(it);

        log(info, "module \"%s\" removed\n", req.mod_name.c_str());
    } catch (exception& e) {
        resp.error_message = e.what();
        log(error, "error removing module \"%s\": %s\n", resp.error_message.c_str());
    }
}

//! svc_module_list
/*!
 * \param[in]   req     Service request data.
 * \param[out]  resp    Service response data.
 */
void kernel::svc_module_list(
        const struct services::robotkernel::kernel::svc_req_module_list& req, 
        struct services::robotkernel::kernel::svc_resp_module_list& resp) 
{
    //response data
    resp.error_message = "";

    try {        
        std::unique_lock<std::recursive_mutex> lock(module_map_mtx);

        for (const auto& kv : module_map) 
            resp.modules.push_back(kv.first);
    } catch(exception& e) {
        resp.error_message = e.what();
    }
}

//! svc_reconfigure_module
/*!
 * \param[in]   req     Service request data.
 * \param[out]  resp    Service response data.
 */
void kernel::svc_reconfigure_module(
        const struct services::robotkernel::kernel::svc_req_reconfigure_module& req, 
        struct services::robotkernel::kernel::svc_resp_reconfigure_module& resp)
{
    //response data
    resp.state = 0;
    resp.error_message = "";

    try {
        module_map_mtx.lock();

        module_map_t::iterator it = module_map.find(req.mod_name);
        if (it == module_map.end()) {
            module_map_mtx.unlock();
            throw str_exception("[robotkernel] module %s not found!\n", 
                    req.mod_name.c_str());
        }

        sp_module_t mdl = it->second;

        module_map_mtx.unlock();

        if (get_state(req.mod_name) != module_state_init)
            set_state(mdl->get_name(), module_state_init);

        mdl->reconfigure();
    } catch(exception& e) {
        resp.error_message = e.what();
    }
}

std::string kernel::ll_to_string(loglevel ll) {
    if (ll == error)    return "ERR ";
    if (ll == warning)  return "WARN";
    if (ll == info)     return "INFO";
    if (ll == verbose)  return "VERB";

    return "????";
}

//! svc_list_devices
/*!
 * \param[in]   req     Service request data.
 * \param[out]  resp    Service response data.
 */
void kernel::svc_list_devices(
        const struct services::robotkernel::kernel::svc_req_list_devices& req, 
        struct services::robotkernel::kernel::svc_resp_list_devices& resp) 
{
    for (const auto& kv : device_map) {
        resp.devices.push_back(kv.first);
    }

    resp.error_message = "";
}

//! svc_process_data_info
/*!
 * \param[in]   req     Service request data.
 * \param[out]  resp    Service response data.
 */
void kernel::svc_process_data_info(
        const struct services::robotkernel::kernel::svc_req_process_data_info& req, 
        struct services::robotkernel::kernel::svc_resp_process_data_info& resp) 
{
    resp.error_message = "";

    if (device_map.find(req.name) != device_map.end()) {
        const auto& pd = std::dynamic_pointer_cast<process_data>(device_map[req.name]);

        if (pd) {
            resp.owner      = pd->owner;
            resp.definition = pd->process_data_definition;
            resp.clk_device = pd->trigger_dev ? pd->trigger_dev->id() : "";
            if (pd->provider)
                resp.provider  = pd->provider->name;
            if (pd->consumer)
                resp.consumer  = pd->consumer->name;
            resp.length    = pd->length;
        } else 
            resp.error_message = 
                format_string("device with name \"%s\" is not a process data device!", req.name.c_str());
    } else
        resp.error_message = 
            format_string("process data device with name \"%s\" not found!", req.name.c_str());
}

//! svc_trigger_info
/*!
 * \param[in]   req     Service request data.
 * \param[out]  resp    Service response data.
 */
void kernel::svc_trigger_info(
        const struct services::robotkernel::kernel::svc_req_trigger_info& req, 
        struct services::robotkernel::kernel::svc_resp_trigger_info& resp)
{
    //response data
    resp.owner = "";
    resp.rate = 0.;
    resp.error_message = "";

    if (device_map.find(req.name) != device_map.end()) {
        const auto& dev = std::dynamic_pointer_cast<trigger>(device_map[req.name]);

        if (dev) {
            resp.owner     = dev->owner;
            resp.rate      = dev->get_rate();
        } else 
            resp.error_message = 
                format_string("device with name \"%s\" is not a trigger device!", req.name.c_str());
    } else
        resp.error_message = 
            format_string("device with name \"%s\" not found!", req.name.c_str());
}

//! svc_stream_info
/*!
 * \param[in]   req     Service request data.
 * \param[out]  resp    Service response data.
 */
void kernel::svc_stream_info(
        const struct services::robotkernel::kernel::svc_req_stream_info& req, 
        struct services::robotkernel::kernel::svc_resp_stream_info& resp)
{
    //response data
    resp.owner = "";
    resp.error_message = "";

    if (device_map.find(req.name) != device_map.end()) {
        const auto& dev = std::dynamic_pointer_cast<stream>(device_map[req.name]);

        if (dev) {
            resp.owner     = dev->owner;
        } else 
            resp.error_message = 
                format_string("device with name \"%s\" is not a stream device!", req.name.c_str());
    } else
        resp.error_message = 
            format_string("device with name \"%s\" not found!", req.name.c_str());
}

//! svc_service_interface_info
/*!
 * \param[in]   req     Service request data.
 * \param[out]  resp    Service response data.
 */
void kernel::svc_service_interface_info(
        const struct services::robotkernel::kernel::svc_req_service_interface_info& req, 
        struct services::robotkernel::kernel::svc_resp_service_interface_info& resp)
{
    //response data
    resp.owner = "";
    resp.error_message = "";

    if (device_map.find(req.name) != device_map.end()) {
        const auto& dev = std::dynamic_pointer_cast<service_interface>(device_map[req.name]);

        if (dev) {
            resp.owner     = dev->owner;
        } else 
            resp.error_message = 
                format_string("device with name \"%s\" is not a service_interface device!", req.name.c_str());
    } else
        resp.error_message = 
            format_string("device with name \"%s\" not found!", req.name.c_str());
}

//! svc_add_pd_injection
/*!
 * \param[in]   req     Service request data.
 * \param[out]  resp    Service response data.
 */
void kernel::svc_add_pd_injection(
        const struct services::robotkernel::kernel::svc_req_add_pd_injection& req, 
        struct services::robotkernel::kernel::svc_resp_add_pd_injection& resp)
{
    // response data
    resp.error_message = "";

    try {
        auto pd = get_device<process_data>(req.pd_dev);
        
        std::shared_ptr<pd_injection_base> retval = 
            std::dynamic_pointer_cast<pd_injection_base>(pd);

        if (retval) {
            pd_entry_t e(pd, req.field_name, req.value, req.bitmask);
            retval->add_injection(e);
        } else {
            resp.error_message = format_string("pd device %s does not support injection!", req.pd_dev.c_str());
        }
    } catch (std::exception& exc) {
        resp.error_message = exc.what();
    }
}

//! svc_del_pd_injection
/*!
 * \param[in]   req     Service request data.
 * \param[out]  resp    Service response data.
 */
void kernel::svc_del_pd_injection(
        const struct services::robotkernel::kernel::svc_req_del_pd_injection& req, 
        struct services::robotkernel::kernel::svc_resp_del_pd_injection& resp)
{
    // response data
    resp.error_message = "";

    try {
        auto pd = get_device<process_data>(req.pd_dev);
        
        std::shared_ptr<pd_injection_base> retval = 
            std::dynamic_pointer_cast<pd_injection_base>(pd);

        if (retval) {
            retval->del_injection(req.field_name);
        }
    } catch (std::exception& exc) {
        resp.error_message = exc.what();
    }
}

//! svc_list_pd_injections
/*!
 * \param[in]   req     Service request data.
 * \param[out]  resp    Service response data.
 */
void kernel::svc_list_pd_injections(
        const struct services::robotkernel::kernel::svc_req_list_pd_injections& req, 
        struct services::robotkernel::kernel::svc_resp_list_pd_injections& resp)
{
    resp.error_message = "";

    for (const auto& kv : device_map) {
        std::shared_ptr<pd_injection_base> retval = 
            std::dynamic_pointer_cast<pd_injection_base>(kv.second);

        if (retval) {
            for (const auto& kv_inj : retval->pd_injections) {
                resp.pd_dev.push_back(kv.second->id());
                resp.field_name.push_back(kv_inj.second.field_name);
                resp.value.push_back(kv_inj.second.value_string);
                resp.bitmask.push_back(kv_inj.second.bitmask_string);
            }
        }
    }
}

//! convert buffer to hex string
std::string robotkernel::hex_string(const void *data, size_t len) {
    char hex_buf[4];
    std::stringstream ss;
    
    for (size_t i = 0; i < len; ++i) {
        uint8_t c = ((uint8_t*) data)[i];
        snprintf(hex_buf, sizeof(hex_buf), "%02X ", c);
        ss << hex_buf;
    }

    ss << "\n";

    return ss.str();
}

