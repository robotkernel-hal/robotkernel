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
using namespace robotkernel;

YAML::Node tmp = YAML::Clone(YAML::Node("dieses clone steht hier damit es vom linker beim statischen linken mit eingepackt wird"));

static void split_file_name(const string& str, string& path, string& file) {
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
        throw robotkernel::str_exception("wrong loglevel value \"%s\"",
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
            log(info, "dependent module %d is in error state, "
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
        throw str_exception("[robotkernel] ERROR: failed to swtich module %s"
                " to state %s!\n", mod_name.c_str(), state_to_string(state));

    return mdl.get_state();
}

//! return module state
/*!
 * \param mod_name name of module which state to return
 * \return module state
 */
module_state_t kernel::get_state(std::string mod_name) {
    module_map_t::const_iterator it = module_map.find(mod_name);
    if (it == module_map.end())
        throw str_exception("[robotkernel] get_state: module %s not found!\n", mod_name.c_str());

    log(verbose, "getting state of module %s\n",
            mod_name.c_str());

    module *mdl = it->second;
    return mdl->get_state();
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

    clnt = NULL;
    _ln_thread_pool_size_main = 1;

    log(info, PACKAGE_STRING "\n");

    int _major, _minor, _patch;
    sscanf(PACKAGE_VERSION, "%d.%d.%d", &_major, &_minor, &_patch);
    
    log(verbose, "major %d, minor %d, patch %d\n",
            _major, _minor, _patch);
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

    log(info, "all modules destructed... cleaning up ln client\n");


    // unregister services before we delete the client, cannot be done in destructors
    unregister_robotkernel_config_dump_log();
    unregister_robotkernel_get_dump_log();
    unregister_robotkernel_set_state();
    unregister_robotkernel_get_state();
    unregister_robotkernel_add_module();
    unregister_robotkernel_remove_module();
    unregister_robotkernel_module_list();
    unregister_robotkernel_reconfigure_module();
    unregister_robotkernel_get_states();

    if (clnt) {
        delete clnt;
    }

    log(info, "clean up finished\n");
};

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

//! initialize links and nodes client
/*!
 * \param argc command line argument counter
 * \param argv command line arguments
 */
void kernel::init_ln(int argc, char **argv) {
    // ln client stuff
    clnt = new ln::client(_name.c_str(), argc, argv);

    register_robotkernel_get_dump_log(clnt, clnt->name + ".get_dump_log");
    register_robotkernel_config_dump_log(clnt, clnt->name + ".config_dump_log");
    register_robotkernel_set_state(clnt, clnt->name + ".set_state");
    register_robotkernel_get_state(clnt, clnt->name + ".get_state");
    register_robotkernel_add_module(clnt, clnt->name + ".add_module");
    register_robotkernel_remove_module(clnt, clnt->name + ".remove_module");
    register_robotkernel_module_list(clnt, clnt->name + ".module_list");
    register_robotkernel_reconfigure_module(clnt, clnt->name + ".reconfigure_module");
    register_robotkernel_get_states(clnt, clnt->name + ".get_states");

    // handle default service group (NULL) in new "main" thread-pool
    clnt->handle_service_group_in_thread_pool(NULL, "main");
    // configure max number of threads for "main" thread-pool
    log(verbose, "setting main thread-pool size to %d\n", _ln_thread_pool_size_main);
    clnt->set_max_threads("main", _ln_thread_pool_size_main);

    // powering up modules to operational
    for (module_map_t::iterator it = module_map.begin();
            it != module_map.end(); ++it) {
        module *mdl = it->second;
        mdl->register_services();
    }
}

//! handle links and nodes service requests
void kernel::handle_ln_request(int argc, char **argv) {
    if (!clnt) {
        try {
            init_ln(argc, argv);
        } catch(exception& e) {
        }
    }

    sleep(1); // daeumchen drehen
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
        try {    
            init_ln(argc, argv);        
        } catch(exception& e) {
            klog(warning, "unable to init ln\n");

            vector<string> err_msg = split_string(string(e.what()), string("\n"));

            for (vector<string>::iterator it = err_msg.begin(); it != err_msg.end(); ++it)
                klog(warning, "[links_and_nodes] %s\n", (*it).c_str());

            klog(warning, "will try to init ln in background...\n");
        }
        return ;
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
        config_dump_log(len, 0);
    }

    _ln_thread_pool_size_main = 
        get_as<unsigned int>(doc, "ln_thread_pool_size_main", 1);
    if(_ln_thread_pool_size_main < 1) {
        klog(error, "ln_thread_pool_size_main has to be >= 1! (you gave %d)\n",
                _ln_thread_pool_size_main);
        _ln_thread_pool_size_main = 1;
    }

    _do_not_unload_modules = 
        get_as<bool>(doc, "do_not_unload_modules", false);

    try {
        init_ln(argc, argv);        
    } catch(exception& e) {
        klog(warning, "unable to init ln: %s\n", e.what());
        klog(warning, "will try to init ln in background...\n");
    }

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
                it != failed_modules.end(); ++it)
            msg += *it + ", ";
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
        const YAML::Node& node) {
    interface *iface = new interface(if_name, node);
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

//! get module
/*!
 * \param mod_name name of module
 * \return pointer to module
 */
module *kernel::get_module(const char *mod_name) {
    kernel& k = *get_instance();

    kernel::module_map_t::const_iterator it = k.module_map.find(mod_name);
    if (it == k.module_map.end())
        throw str_exception("[robotkernel] get_module: module %s not found!\n", mod_name);

    return it->second;
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
        throw str_exception("[robotkernel] state_change: module %s not found!\n", mod_name);

    module_state_t current_state = it->second->get_state();
    if (new_state == current_state)
        return 0;

    klog(info, "WARNING: module %s changed state to %d, old state %d\n",
            mod_name, new_state, current_state);

    printf("\n\n\n THIS IS UNEXPECTED !!!! \n\n\n");

    int ret = set_state(mod_name, module_state_init);

    return ret;
}

//! get dump log
/*!
 * \param req service request
 * \param svn service container
 * \return success
 */
int kernel::on_get_dump_log(ln::service_request& req, ln_service_robotkernel_get_dump_log& svc) {
    ln::string_buffer dl(&svc.resp.dump_log, dump_log());
    req.respond();
    return 0;
}

//! config dump log
/*!
 * \param req service request
 * \param svn service container
 * \return success
 */
int kernel::on_config_dump_log(ln::service_request& req, ln_service_robotkernel_config_dump_log& svc) {
    config_dump_log(svc.req.max_len, svc.req.do_ust);
    klog(verbose, "dump_log len set to %d, do_ust to %d\n", svc.req.max_len, svc.req.do_ust);

    string current_log_level;
#define loglevel_to_string(x)             \
    if (ll == x)                          \
    current_log_level = string(#x)

    loglevel_to_string(error);
    else loglevel_to_string(warning);
    else loglevel_to_string(info);
    else loglevel_to_string(verbose);

    string set_log_level(svc.req.set_log_level, svc.req.set_log_level_len);
    if(set_log_level.size()) {
        py_value *pval = eval_full(set_log_level);
        py_string *pstring = dynamic_cast<py_string *>(pval);

        if (pstring) 
            ll = string(*pstring);
    }

    ln::string_buffer current_log_level_sb(&svc.resp.current_log_level, current_log_level);
    req.respond();
    return 0;
}

//! set state
/*!
 * \param req service request
 * \param svn service container
 * \return success
 */
int kernel::on_set_state(ln::service_request& req, ln_service_robotkernel_set_state& svc) {
    try {
        string mod_name(svc.req.mod_name, svc.req.mod_name_len);
        string state(svc.req.state, svc.req.state_len);
        set_state(mod_name, string_to_state(state.c_str()));
        svc.resp.error_message_len = 0;
        req.respond();
    } catch (const exception& e) {
        klog(error, "%s\n", e.what());
        ln::string_buffer err(&svc.resp.error_message, e.what());
        req.respond();
    }
    return 0;
}

//! get state
/*!
 * \param req service request
 * \param svn service container
 * \return success
 */
int kernel::on_get_state(ln::service_request& req, ln_service_robotkernel_get_state& svc) {
    try {
        string mod_name(svc.req.mod_name, svc.req.mod_name_len);
        string state(state_to_string(get_state(mod_name)));
        std::transform(state.begin(), state.end(), state.begin(), ::tolower);
        ln::string_buffer state_sb(&svc.resp.state, state.substr(1, state.size()-2));
        req.respond();
    } catch (const exception& e) {
        klog(error, "%s\n", e.what());
        ln::string_buffer err(&svc.resp.error_message, e.what());
        req.respond();
    }
    return 0;
}

//! get states
/*!
 * \param req service request
 * \param svn service container
 * \return success
 */
int kernel::on_get_states(ln::service_request& req, ln_service_robotkernel_get_states& svc) {
    string mod_names(svc.req.mod_names, svc.req.mod_names_len);
    vector<string> module_patterns = split_string(mod_names, ",");	
    for(vector<string>::iterator i = module_patterns.begin(); i != module_patterns.end(); ++i)
        *i = strip(*i);

    set<string> seen;
    stringstream module_list;
    for(vector<string>::iterator p = module_patterns.begin(); p != module_patterns.end(); ++p) {
        for(module_map_t::iterator i = module_map.begin(); i != module_map.end(); ++i) {
            if(seen.find(i->first) != seen.end())
                continue;
            if(pattern_matches(*p, i->first)) {
                if(seen.size())
                    module_list << ", ";
                module_list << i->first;
                seen.insert(i->first);
            }
        }
    }

    ln::string_buffer module_list_sb(&svc.resp.mod_names, module_list);

    stringstream states;
    for (set<string>::iterator it = seen.begin(); it != seen.end(); ++it) {
        if(it != seen.begin())
            states << ", ";
        string state(state_to_string(module_map[*it]->get_state()));
        std::transform(state.begin(), state.end(), state.begin(), ::tolower);
        states << state.substr(1, state.size()-2);
    }
    ln::string_buffer states_sb(&svc.resp.states, states);

    req.respond();
    return 0;
}

//! add module
/*!
 * \param req service request
 * \param svn service container
 * \return success
 */
int kernel::on_add_module(ln::service_request& req, ln_service_robotkernel_add_module& svc) {
    svc.resp.error_message_len = 0;

    try {
        string config = string(svc.req.config, svc.req.config_len);
        klog(info, "got config: %s\n", config.c_str());

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
        req.respond();
    } catch(exception& e) {
        klog(error, "caught exception: %s\n", e.what());
        svc.resp.error_message = strdup(e.what());
        svc.resp.error_message_len = strlen(svc.resp.error_message);
        req.respond();
        free(svc.resp.error_message);
    }

    return 0;
}
//! remove module
/*!
 * \param req service request
 * \param svn service container
 * \return success
 */
int kernel::on_remove_module(ln::service_request& req, ln_service_robotkernel_remove_module& svc) {
    svc.resp.error_message_len = 0;

    try {
        string mod_name = string(svc.req.name, svc.req.name_len);
        module_map_t::iterator it = module_map.find(mod_name);
        if (it == module_map.end())
            throw str_exception("[robotkernel] module %s not found!\n", mod_name.c_str());

        module *mdl = it->second;
        set_state(mdl->get_name(), module_state_init);

        module_map.erase(it);
        delete mdl;

        req.respond();
    } catch (exception& e) {
        svc.resp.error_message = strdup(e.what());
        svc.resp.error_message_len = strlen(svc.resp.error_message);
        req.respond();
        free(svc.resp.error_message);
    }

    return 0;
}

//! module list
/*!
 * \param req service request
 * \param svn service container
 * \return success
 */
int kernel::on_module_list(ln::service_request& req, ln_service_robotkernel_module_list& svc) {
    svc.resp.error_message_len = 0;

    try {
        string module_list = "[ ";
        for (module_map_t::const_iterator
                it = module_map.begin(); it != module_map.end(); ++it) {
            module_list += "\"" + it->first + "\", ";
        }
        module_list += "]";
        svc.resp.modules = (char *)module_list.c_str();
        svc.resp.modules_len = strlen(svc.resp.modules);

        req.respond();
    } catch(exception& e) {
        svc.resp.error_message = strdup(e.what());
        svc.resp.error_message_len = strlen(svc.resp.error_message);
        req.respond();
        free(svc.resp.error_message);
    }
    return 0;
}


//! reconfigure module
/*!
 * \param req service request
 * \param svn service container
 * \return success
 */
int kernel::on_reconfigure_module(ln::service_request& req, ln_service_robotkernel_reconfigure_module& svc) {
    svc.resp.error_message_len = 0;

    try {
        string mod_name = string(svc.req.name, svc.req.name_len);
        module_map_t::iterator it = module_map.find(mod_name);
        if (it == module_map.end())
            throw str_exception("[robotkernel] module %s not found!\n", mod_name.c_str());

        module *mdl = it->second;
        if (get_state(mod_name) != module_state_init)
            set_state(mdl->get_name(), module_state_init);

        mdl->reconfigure();
        svc.resp.state = 0;
        req.respond();
    } catch(exception& e) {
        svc.resp.error_message = strdup(e.what());
        svc.resp.error_message_len = strlen(svc.resp.error_message);
        req.respond();
        free(svc.resp.error_message);
    }
    return 0;
}

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

