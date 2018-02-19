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
#include "robotkernel/kernel.h"
#include "robotkernel/helpers.h"
#include "robotkernel/exceptions.h"
#include "robotkernel/module_base.h"
#include "robotkernel/bridge_base.h"
#include "robotkernel/config.h"
#include "yaml-cpp/yaml.h"

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

//! kernel singleton instance
kernel *kernel::instance = NULL;

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
kernel::kernel() {
    ll = info;
    _name = "robotkernel";

    log(info, PACKAGE_STRING "\n");

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
    add_service(_name, "list_devices", service_definition_list_devices,
            std::bind(&kernel::service_list_devices, this, _1, _2));
    add_service(_name, "process_data_info", service_definition_process_data_info,
            std::bind(&kernel::service_process_data_info, this, _1, _2));
    add_service(_name, "trigger_info", service_definition_trigger_info,
            std::bind(&kernel::service_trigger_info, this, _1, _2));
    add_service(_name, "stream_info", service_definition_stream_info,
            std::bind(&kernel::service_stream_info, this, _1, _2));
    add_service(_name, "service_interface_info", service_definition_service_interface_info,
            std::bind(&kernel::service_service_interface_info, this, _1, _2));
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
    log(info, "searching interfaces and modules ....\n");
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
        log(info, "found modules path %s\n", _internal_modpath.c_str());
    if (_internal_intfpath != string(""))
        log(info, "found interfaces path %s\n", _internal_intfpath.c_str());

    if (config_file.compare("") == 0) {
        return;
    }

    char *real_config_file = realpath(config_file.c_str(), NULL);
    if (!real_config_file)
        throw str_exception("supplied config file \"%s\" not found!", config_file.c_str());

    split_file_name(string(real_config_file), config_file_path, file);

    log(info, "got config file path: %s, file name %s\n",
            config_file_path.c_str(), file.c_str());

    this->config_file = string(real_config_file);
    free(real_config_file);
    YAML::Node doc = YAML::LoadFile(this->config_file);

    // search for name
    char *ln_program_name = getenv("LN_PROGRAM_NAME");
    if (doc["name"] && !ln_program_name)
        _name = get_as<string>(doc, "name");

    // locking all current and future memory to keep it hold in 
    // memory without swapping
    mlockall(MCL_CURRENT | MCL_FUTURE);

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

        {
            std::unique_lock<std::recursive_mutex> lock(module_map_mtx);

            if (module_map.find(mdl->get_name()) != module_map.end()) {
                string name = mdl->get_name();
                throw str_exception("[robotkernel] duplicate module name: %s\n", name.c_str());
            }
        }

        // add to module map
        klog(info, "adding [%s]\n", mdl->get_name().c_str());
        module_map[mdl->get_name()] = mdl;
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

        if (bridge_map.find(brdg->name) != bridge_map.end()) {
            throw str_exception("[robotkernel] duplicate module name: %s\n", 
                    brdg->name.c_str());
        }

        // add to module map
        klog(info, "adding [%s]\n", brdg->name.c_str());
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

        if (service_provider_map.find(sp->name) != service_provider_map.end()) {
            throw str_exception("[robotkernel] duplicate module name: %s\n", 
                    sp->name.c_str());
        }

        // add to module map
        klog(info, "adding [%s]\n", sp->name.c_str());
        service_provider_map[sp->name] = sp;
    }
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
            klog(error, "module %s signaled error, switching "
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
    std::unique_lock<std::recursive_mutex> lock(module_map_mtx);

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

    klog(info, "WARNING: module %s changed state to %d, old state %d\n",
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
    if (device_map.find(map_index) != device_map.end())
        return; // already in

    for (const auto& kv : dl_map) 
        kv.second->notify_add_device(req);

    log(verbose, "registered device \"%s\"\n", map_index.c_str());
    device_map[map_index] = req;
};
        
// remove a named device
void kernel::remove_device(sp_device_t req) {
    auto map_index = req->id();

    klog(verbose, "removing device %s\n", map_index.c_str());

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
            klog(verbose, "removing device %s\n", it->second->id().c_str());
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
    sp_module_t mdl = make_shared<module>(config);

    if (module_map.find(mdl->get_name()) != module_map.end()) {
        string name = mdl->get_name();
        throw str_exception("[robotkernel] duplicate module name: %s\n", name.c_str());
    }

    if (!mdl->configured()) {
        string name = mdl->get_name();
        throw str_exception("[robotkernel] module %s not configured!\n", name.c_str());
    }

    // add to module map
    module_map[mdl->get_name()] = mdl;
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
"- string: log\n";

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
"- uint32_t: max_len\n"
"- uint8_t: do_ust\n"
"- string: set_loglevel\n"
"response:\n"
"- string: current_loglevel\n"
"- string: error_message\n";

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
        load_module(node);
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
"- string: config\n"
"response:\n"
"- string: error_message\n";

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
        std::unique_lock<std::recursive_mutex> lock(module_map_mtx);
        module_map_t::iterator it = module_map.find(mod_name);
        if (it == module_map.end())
            throw str_exception("[robotkernel] module %s not found!\n", 
                    mod_name.c_str());

        sp_module_t mdl = it->second;
        set_state(mdl->get_name(), module_state_init);

        module_map.erase(it);
    } catch (exception& e) {
        error_message = e.what();
    }

#define REMOVE_MODULE_RESP_ERROR_MESSAGE    0
    response.resize(1);
    response[REMOVE_MODULE_RESP_ERROR_MESSAGE] = error_message;

    return 0;
}

const std::string kernel::service_definition_remove_module = 
"request:\n"
"- string: mod_name\n"
"response:\n"
"- string: error_message\n";

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
        std::unique_lock<std::recursive_mutex> lock(module_map_mtx);

        for (const auto& kv : module_map) 
            modules.push_back(kv.first);
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
"- vector/string: modules\n"
"- string: error_message\n";

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
        module_map_mtx.lock();

        module_map_t::iterator it = module_map.find(mod_name);
        if (it == module_map.end()) {
            module_map_mtx.unlock();
            throw str_exception("[robotkernel] module %s not found!\n", 
                    mod_name.c_str());
        }

        sp_module_t mdl = it->second;

        module_map_mtx.unlock();

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
"- string: mod_name\n"
"response:\n"
"- uint64_t: state\n"
"- string: error_message\n";

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


//! list process data objects
/*!
 * \param request service request data
 * \parma response service response data
 * \return success
 */
int kernel::service_list_devices(const service_arglist_t &request,
        service_arglist_t &response) {
    //response data
    std::vector<rk_type> devices(0);
    string error_message = "";

    //for (const auto& pd : devices_map)
    //    devicess.push_back(pd.first);
    for (const auto& kv : device_map)
        devices.push_back(kv.first);

#define LIST_DEVICES_RESP_DEVICESS   0
#define LIST_DEVICES_RESP_ERROR_MESSAGE   1
    response.resize(2);
    response[LIST_DEVICES_RESP_DEVICESS] = devices;
    response[LIST_DEVICES_RESP_ERROR_MESSAGE] = error_message;

    return 0;
}

const std::string kernel::service_definition_list_devices = 
"response:\n"
"- vector/string: devices\n"
"- string: error_message\n";

//! process data information
/*!
 * \param request service request data
 * \parma response service response data
 * \return success
 */
int kernel::service_process_data_info(const service_arglist_t &request,
        service_arglist_t &response) {
    // request data
#define DEVICES_INFO_REQ_NAME 0
    string name = request[DEVICES_INFO_REQ_NAME]; 

    //response data
    string owner = "";
    string definiton = "";
    string provider = "";
    string consumer = "";
    int32_t length = 0;
    string error_message = "";

    if (device_map.find(name) != device_map.end()) {
        const auto& pd = std::dynamic_pointer_cast<process_data>(device_map[name]);

        if (pd) {
            owner     = pd->owner;
            definiton = pd->process_data_definition;
            if (pd->provider)
                provider  = pd->provider->provider_name;
            if (pd->consumer)
                consumer  = pd->consumer->consumer_name;
            length    = pd->length;
        } else 
            error_message = 
                format_string("device with name \"%s\" is not a process data device!", name.c_str());
    } else
        error_message = 
            format_string("process data device with name \"%s\" not found!", name.c_str());

#define DEVICES_INFO_RESP_OWNER            0
#define DEVICES_INFO_RESP_DEFINITION       1
#define DEVICES_INFO_RESP_LENGTH           2
#define DEVICES_INFO_RESP_PROVIDER         3
#define DEVICES_INFO_RESP_CONSUMER         4
#define DEVICES_INFO_RESP_ERROR_MESSAGE    5
    response.resize(6);
    response[DEVICES_INFO_RESP_OWNER]          = owner;
    response[DEVICES_INFO_RESP_DEFINITION]     = definiton;
    response[DEVICES_INFO_RESP_LENGTH]         = length;
    response[DEVICES_INFO_RESP_PROVIDER]       = provider;
    response[DEVICES_INFO_RESP_CONSUMER]       = consumer;
    response[DEVICES_INFO_RESP_ERROR_MESSAGE]  = error_message;

    return 0;
}

const std::string kernel::service_definition_process_data_info = 
"request:\n"
"- string: name\n"
"response:\n"
"- string: owner\n"
"- string: definition\n"
"- int32_t: length\n"
"- string: provider\n"
"- string: consumer\n"
"- string: error_message\n";

//! trigger information
/*!
 * \param request service request data
 * \parma response service response data
 * \return success
 */
int kernel::service_trigger_info(const service_arglist_t &request,
        service_arglist_t &response) {
    // request data
#define TRIGGER_INFO_REQ_NAME 0
    string name = request[TRIGGER_INFO_REQ_NAME]; 

    //response data
    string owner = "";
    double rate = 0.;
    string error_message = "";

    if (device_map.find(name) != device_map.end()) {
        const auto& dev = std::dynamic_pointer_cast<trigger>(device_map[name]);

        if (dev) {
            owner     = dev->owner;
            rate      = dev->get_rate();
        } else 
            error_message = 
                format_string("device with name \"%s\" is not a trigger device!", name.c_str());
    } else
        error_message = 
            format_string("device with name \"%s\" not found!", name.c_str());

#define TRIGGER_INFO_RESP_OWNER            0
#define TRIGGER_INFO_RESP_RATE             1
#define TRIGGER_INFO_RESP_ERROR_MESSAGE    2
    response.resize(3);
    response[TRIGGER_INFO_RESP_OWNER]          = owner;
    response[TRIGGER_INFO_RESP_RATE]           = rate;
    response[TRIGGER_INFO_RESP_ERROR_MESSAGE]  = error_message;

    return 0;
}

const std::string kernel::service_definition_trigger_info = 
"request:\n"
"- string: name\n"
"response:\n"
"- string: owner\n"
"- double: rate\n"
"- string: error_message\n";

//! stream information
/*!
 * \param request service request data
 * \parma response service response data
 * \return success
 */
int kernel::service_stream_info(const service_arglist_t &request,
        service_arglist_t &response) {
    // request data
#define stream_INFO_REQ_NAME 0
    string name = request[stream_INFO_REQ_NAME]; 

    //response data
    string owner = "";
    double rate = 0.;
    string error_message = "";

    if (device_map.find(name) != device_map.end()) {
        const auto& dev = std::dynamic_pointer_cast<stream>(device_map[name]);

        if (dev) {
            owner     = dev->owner;
        } else 
            error_message = 
                format_string("device with name \"%s\" is not a stream device!", name.c_str());
    } else
        error_message = 
            format_string("device with name \"%s\" not found!", name.c_str());

#define stream_INFO_RESP_OWNER            0
#define stream_INFO_RESP_ERROR_MESSAGE    1
    response.resize(2);
    response[stream_INFO_RESP_OWNER]          = owner;
    response[stream_INFO_RESP_ERROR_MESSAGE]  = error_message;

    return 0;
}

const std::string kernel::service_definition_stream_info = 
"request:\n"
"- string: name\n"
"response:\n"
"- string: owner\n"
"- string: error_message\n";

//! service_interface information
/*!
 * \param request service request data
 * \parma response service response data
 * \return success
 */
int kernel::service_service_interface_info(const service_arglist_t &request,
        service_arglist_t &response) {
    // request data
#define service_interface_INFO_REQ_NAME 0
    string name = request[service_interface_INFO_REQ_NAME]; 

    //response data
    string owner = "";
    double rate = 0.;
    string error_message = "";

    if (device_map.find(name) != device_map.end()) {
        const auto& dev = std::dynamic_pointer_cast<service_interface>(device_map[name]);

        if (dev) {
            owner     = dev->owner;
        } else 
            error_message = 
                format_string("device with name \"%s\" is not a service_interface device!", name.c_str());
    } else
        error_message = 
            format_string("device with name \"%s\" not found!", name.c_str());

#define service_interface_INFO_RESP_OWNER            0
#define service_interface_INFO_RESP_ERROR_MESSAGE    1
    response.resize(2);
    response[service_interface_INFO_RESP_OWNER]          = owner;
    response[service_interface_INFO_RESP_ERROR_MESSAGE]  = error_message;

    return 0;
}

const std::string kernel::service_definition_service_interface_info = 
"request:\n"
"- string: name\n"
"response:\n"
"- string: owner\n"
"- string: error_message\n";

