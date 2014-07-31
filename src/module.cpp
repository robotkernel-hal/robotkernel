//! robotkernel module definition
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

#include "kernel.h"
#include "module.h"
#include "kernel_worker.h"
#include "exceptions.h"
#include "helpers.h"
#include <sys/stat.h>
#include <dlfcn.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <algorithm>

using namespace std;
using namespace robotkernel;

const char string_state_error[]   = "<ERROR>";
const char string_state_unknown[] = "<UNKNOWN>";
const char string_state_init[]    = "<INIT>";
const char string_state_preop[]   = "<PREOP>";
const char string_state_safeop[]  = "<SAFEOP>";
const char string_state_op[]      = "<OP>";
const char string_state_boot[]    = "<BOOT>";

const char *state_to_string(module_state_t state) {
    switch (state) {
        default:
        case module_state_unknown:
            return string_state_unknown;
        case module_state_error:
            return string_state_error;
        case module_state_init:
            return string_state_init;
        case module_state_preop:
            return string_state_preop;
        case module_state_safeop:
            return string_state_safeop;
        case module_state_op:
            return string_state_op;
        case module_state_boot:
            return string_state_boot;
    }
}
module_state_t string_to_state(const char* state_ptr) {
    string state(state_ptr);
    std::transform(state.begin(), state.end(), state.begin(), ::tolower);
#define ret_state(a) if(state == #a) return module_state_ ## a;
    ret_state(error);
    ret_state(unknown);
    ret_state(boot);
    ret_state(init);
    ret_state(preop);
    ret_state(safeop);
    ret_state(op);
}

//! generate new trigger object
/*!
 * \param node configuration node
 */
module::external_trigger::external_trigger(const YAML::Node& node) {
    _mod_name       = node["mod_name"].to<std::string>();
    _clk_id         = node["clk_id"].to<int>();
    _prio           = 0;
    _affinity_mask  = 0;
    _divisor        = 1;
    _direct_mode    = false;

    if (node.FindValue("direct_mode"))
        _direct_mode = node["direct_mode"].to<bool>();    

    if (node.FindValue("divisor"))
        _divisor = node["divisor"].to<int>();
    
    if (node.FindValue("prio")) {
        _prio = node["prio"].to<int>();

        if (_direct_mode)
            klog(info, "[external_trigger] prio will not have any effect in direct mode!\n");
    }

    if(node.FindValue("affinity")) {
        const YAML::Node& affinity = node["affinity"];
        if (affinity.Type() == YAML::NodeType::Scalar)
            _affinity_mask = (1 << affinity.to<int>());
        else if (affinity.Type() == YAML::NodeType::Sequence)
            for (YAML::Iterator it = affinity.begin(); it != affinity.end(); ++it)
                _affinity_mask |= (1 << it->to<int>());
        
        if (_direct_mode)
            klog(info, "[external_trigger] affinity will not have any effect in direct mode!\n");
    }
}
            
YAML::Emitter& operator<<(YAML::Emitter& out, const module::external_trigger& t) {
    out << YAML::BeginMap;
    out << YAML::Key << "mod_name" << YAML::Value << t._mod_name;
    out << YAML::Key << "clk_id"   << YAML::Value << t._clk_id;

    if (t._direct_mode)
        out << YAML::Key << "direct_mode" << YAML::Value << t._direct_mode;
    else {
        out << YAML::Key << "prio"     << YAML::Value << t._prio;
        out << YAML::Key << "affinity" << YAML::Value;

        out << YAML::Flow;
        out << YAML::BeginSeq;
        for (int i = 0; i < 32; ++i)
            if (t._affinity_mask & (1 << i))
                out << i;
        out << YAML::EndSeq;
    }

    out << YAML::Key << "divisor"  << YAML::Value << t._divisor;
    out << YAML::EndMap;

    return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const module& mdl) {
    out << YAML::BeginMap;
    out << YAML::Key << "name" << YAML::Value << mdl.name;
    out << YAML::Key << "module_file" << YAML::Value << mdl.module_file;

    stringstream stream(mdl.config);
    YAML::Parser parser(stream);
    YAML::Node node;
    if (parser.GetNextDocument(node)) {
        out << YAML::Key << "config";
        out << YAML::Value << node;
    }

    if (!mdl.triggers.empty()) {
        out << YAML::Key << "trigger";
        out << YAML::Value << YAML::BeginSeq;
        
        for (module::trigger_list_t::const_iterator it = mdl.triggers.begin(); 
                it != mdl.triggers.end(); ++it)
            out << **it;

        out << YAML::EndSeq;
    }

    if (!mdl.depends.empty()) {
        out << YAML::Key << "depends" << YAML::Value;
        out << YAML::Flow << YAML::BeginSeq;
        for (module::depend_list_t::const_iterator it = mdl.depends.begin(); 
                it != mdl.depends.end(); ++it)
            out << *it;
        out << YAML::EndSeq;
    }

    out << YAML::EndMap;
    return out;
}

//! module construction
/*!
  \param mod_name name of shared object to load
  \param module_file filename of module
  \param config module configuration string
  \param trigger configuration
  */
module::module(std::string mod_name, std::string module_file, std::string config, 
        std::string trigger, std::string depends) :
    mod_handle(NULL), name(mod_name), module_file(module_file), config(config), 
    mod_configure(NULL), mod_unconfigure(NULL), mod_read(NULL), mod_write(NULL), 
    mod_set_state(NULL) {
    
    stringstream stream(trigger);
    YAML::Parser parser(stream);
    YAML::Node trigger_node;
    if (parser.GetNextDocument(trigger_node)) {
        for (YAML::Iterator it = trigger_node.begin(); 
                it != trigger_node.end(); ++it) {
            try {
                external_trigger *newtrigger = new external_trigger(*it);
                this->depends.push_back(newtrigger->_mod_name);
                triggers.push_back(newtrigger);                
            } catch (YAML::Exception& e) {
                klog(error, "[module] exception creating external trigger: %s\n", e.what());
                klog(error, "got config string: \n====\n%s\n====\n", trigger.c_str());
                throw;
            }
        }
    }

    _init();
}

//! module construction
/*!
 * \param node configuration node
 */
module::module(const YAML::Node& node, string config_path)
:    mod_handle(NULL), mod_configure(NULL), 
    mod_unconfigure(NULL), mod_read(NULL), mod_write(NULL), 
    mod_set_state(NULL) {
    name = node["name"].to<string>();
    module_file = node["module_file"].to<string>();
    config_file_path = config_path;
    
    const YAML::Node *config_file_node = node.FindValue("config_file");
    const YAML::Node *config_node = node.FindValue("config");
    if (config_file_node) {
        string file_name = (*config_file_node).to<string>();
        // check for absolute/relative path
        if (file_name[0] != '/') {
            // relative path to config file
            file_name = config_path + "/" + file_name;
        }
            
        klog(info, "[module] config file \"%s\" for module %s\n", 
                file_name.c_str(), name.c_str());

        ifstream t(file_name.c_str());
        stringstream buffer;
        buffer << t.rdbuf();
        config = buffer.str();
    } else if (config_node) {
        YAML::Emitter t;
        t << (*config_node);
        config = string(t.c_str());
    }

    const YAML::Node *trigger = node.FindValue("trigger");
    if (trigger) {
        for (YAML::Iterator it = trigger->begin(); 
                it != trigger->end(); ++it) {
            try {
                external_trigger *trigger = new external_trigger(*it);
                depends.push_back(trigger->_mod_name);
                triggers.push_back(trigger);
            } catch (YAML::Exception& e) {
                YAML::Emitter t;
                t << (*trigger);
                klog(error, "[module] exception creating external trigger: %s\n", e.what());
                klog(error, "got config string: \n====\n%s\n====\n", t.c_str());
                throw;
            }
        }
    }

    const YAML::Node *deps = node.FindValue("depends");
    if (deps) {
        if (deps->Type() == YAML::NodeType::Scalar) {
            std::string tmp = deps->to<std::string>();

            if (tmp == name)
                throw str_exception("module %s depends on itself?\n", name.c_str());

            depends.push_back(tmp); 
        } else if (deps->Type() == YAML::NodeType::Sequence) {
            for (YAML::Iterator it = deps->begin();
                    it != deps->end(); ++it) {
                std::string tmp = it->to<std::string>();
            
                if (tmp == name)
                    throw str_exception("module %s depends on itself?\n", name.c_str());

                depends.push_back(tmp); 
            }
        }
    }

    const YAML::Node *power_up_node = node.FindValue("power_up");
    if (power_up_node) {
        power_up = power_up_node->to<bool>();
    } else {
        power_up = false;
    }

//    const YAML::Node *clocks = node.FindValue("clocks");
    _init();
}

void module::_init() {
    struct stat buf;
    if (stat(module_file.c_str(), &buf) != 0) {
        // try path relative to config file first
        string mod = config_file_path + "/" + module_file;
        if (stat(mod.c_str(), &buf) != 0) {
            bool found = false;

            // no in relative path, try enviroment search path
            char *modpathes = getenv("ROBOTKERNEL_MODULE_PATH");
            if (modpathes) {
                char *pch, *str = strdup(modpathes);
                pch = strtok(str, ":");
                while (pch != NULL) {
                    mod = string(pch) + "/" + module_file;
                    if (stat(mod.c_str(), &buf) == 0) {
                        module_file = mod;
                        found = true;
                        break;
                    }
                    pch = strtok(NULL, ":");
                }

                free(str);
            }

            if (!found) {
                // try path from robotkernel release
                string mod = kernel::get_instance()->_internal_modpath + "/" + module_file;
                if (stat(mod.c_str(), &buf) == 0) {
                    module_file = mod;
                    found = true;
                }
            }
        } else
            module_file = mod;
    }

    klog(info, "[module] loading module %s\n", module_file.c_str());

    if (access(module_file.c_str(), R_OK) != 0) {
#ifdef __VXWORKS__
        // special case on vxworks, because it may return ENOTSUP, the we try to open anyway !
#else
        klog(error, "[module] module file name not given as absolute filename, either set\n"
                "         ROBOTKERNEL_MODULE_PATH environment variable or specify absolut path!\n");
        klog(error, "[module] access signaled error: %s\n", strerror(errno));
        return;
#endif
    }

    so_handle = dlopen(module_file.c_str(), 
            RTLD_GLOBAL |
            RTLD_NOW
            );

    if (!so_handle) {
        klog(error, "[module] dlopen signaled error opening module:\n");
        klog(error, "%s\n", dlerror());;
        return;
    }

    mod_configure   = (mod_configure_t)  dlsym(so_handle, "mod_configure");
    mod_unconfigure = (mod_unconfigure_t)dlsym(so_handle, "mod_unconfigure");
    mod_read        = (mod_read_t)       dlsym(so_handle, "mod_read");
    mod_write       = (mod_write_t)      dlsym(so_handle, "mod_write");
    mod_set_state   = (mod_set_state_t)  dlsym(so_handle, "mod_set_state");
    mod_get_state   = (mod_get_state_t)  dlsym(so_handle, "mod_get_state");
    mod_request     = (mod_request_t)    dlsym(so_handle, "mod_request");
    mod_trigger     = (mod_trigger_t)    dlsym(so_handle, "mod_trigger");

    if (!mod_configure)
        klog(verbose, "[module] missing mod_configure in %s\n", module_file.c_str());;
    if (!mod_unconfigure)
        klog(verbose, "[module] missing mod_unconfigure in %s\n", module_file.c_str());
    if (!mod_read)
        klog(verbose, "[module] missing mod_read in %s\n", module_file.c_str());
    if (!mod_write)
        klog(verbose, "[module] missing mod_write in %s\n", module_file.c_str());
    if (!mod_set_state)
        klog(verbose, "[module] missing mod_set_state in %s\n", module_file.c_str());
    if (!mod_get_state)
        klog(verbose, "[module] missing mod_get_state in %s\n", module_file.c_str());
    if (!mod_request)
        klog(verbose, "[module] missing mod_request in %s\n", module_file.c_str());
    if (!mod_trigger)
        klog(verbose, "[module] missing mod_trigger in %s\n", module_file.c_str());

    // try to configure
    reconfigure();
}

//! register services
void module::register_services() {
    request(MOD_REQUEST_REGISTER_SERVICES, NULL);

    kernel& k = *kernel::get_instance();
    if (k.clnt) {
        register_get_config(k.clnt, k.clnt->name + "." + name + ".get_config");
        register_get_feat(k.clnt, k.clnt->name + "." + name + ".get_feat");
    }
}

//! module reconfiguration
/*! 
  \param sConfig configuration string for module
  \return success or failure
  */
bool module::reconfigure() {
    // unconfigure module first
    if (mod_handle && mod_unconfigure) {
        mod_unconfigure(mod_handle);
        mod_handle = NULL;
    }

    // try to configure
    if (mod_configure)
        mod_handle = mod_configure(name.c_str(), config.c_str());

    register_services();

    return configured();
}

//! module destruction
/*!
  destroys module
  */
module::~module() {
    klog(info, "[module] destructing %s\n", module_file.c_str());

    while (!triggers.empty()) {
        external_trigger *trigger = triggers.front();
        triggers.pop_front();

        delete trigger;
    }

    while (!interfaces.empty()) {
        interface *iface = interfaces.front();
        interfaces.pop_front();

        delete iface;
    }

    // unconfigure module first
    if (mod_handle && mod_unconfigure) {
        mod_unconfigure(mod_handle);
        mod_handle = NULL;
    }

    if (so_handle) {
        klog(info, "[module] unloading module %s\n", module_file.c_str());
        if (dlclose(so_handle) != 0)
            klog(error, "[module] error on unloading module %s\n", module_file.c_str());
        else
            so_handle = NULL;
    }
}

//! Read process data from module
/*!
  \param buf buffer to store cyclic data
  \param busize size of buffer 
  \return size of read bytes 
  */
size_t module::read(char* buf, size_t bufsize) {
    if (!mod_handle)
        throw str_exception("[module] %s not configured\n", name.c_str());

    if (!mod_read) {
        klog(error, "[module] error: no mod_read function\n");
        return 0;
    }

    return mod_read(mod_handle, buf, bufsize); 
}

//! Write process data to module
/*!
  \param buf buffer with new cyclic data
  \param busize size of buffer 
  \return size of written bytes 
  */
size_t module::write(char* buf, size_t bufsize) {
    if (!mod_handle)
        throw str_exception("[module] %s not configured\n", name.c_str());

    if (!mod_write) {
        klog(error, "[module] error: no mod_write function\n");
        return 0;
    }

    return mod_write(mod_handle, buf, bufsize);
}

//! Set module state 
/*!
  \param state new model state 
  \return success or failure
  */
int module::set_state(module_state_t state) {
    if (!mod_handle)
        throw str_exception("[module] %s not configured\n", name.c_str());

    if (!mod_set_state) {
        klog(error, "[module] error: no mod_set_state function\n");
        return -1;
    }

    module_state_t act_state = get_state();

    if (act_state == state)
        return act_state;

    int ret = -1;
    switch (state) {
        case module_state_init:
            for (trigger_list_t::iterator it = triggers.begin();
                    it != triggers.end(); ++it) {
                klog(info, "[module] removing module trigger %s for module %s\n",
                        (*it)->_mod_name.c_str(), name.c_str());

                kernel& k = *kernel::get_instance();
                kernel::module_map_t::iterator module_it = k.module_map.find((*it)->_mod_name);
                if (module_it == k.module_map.end())
                    throw str_exception("[module] %s not found\n", (*it)->_mod_name.c_str());

                module *mdl2 = module_it->second;
                mdl2->trigger_unregister_module(this, **it);
            }

            klog(info, "[module] setting state of %s from %s to %s\n", name.c_str(), 
                state_to_string(get_state()), state_to_string(state));

            ret = mod_set_state(mod_handle, state);
            break;
        case module_state_preop:
            if (act_state != module_state_init) {
                if ((ret = set_state(module_state_init)) == -1)
                    return ret;
            }
            
            klog(info, "[module] setting state of %s from %s to %s\n", name.c_str(), 
                state_to_string(get_state()), state_to_string(state));

            ret = mod_set_state(mod_handle, state);
            break;
        case module_state_safeop:
            if (act_state != module_state_preop) {
                if ((ret = set_state(module_state_preop)) == -1)
                    return ret;
            }

            for (trigger_list_t::iterator it = triggers.begin();
                    it != triggers.end(); ++it) {
                klog(info, "[module] adding module trigger %s for module %s\n",
                        (*it)->_mod_name.c_str(), name.c_str());

                kernel& k = *kernel::get_instance();
                kernel::module_map_t::iterator module_it = k.module_map.find((*it)->_mod_name);
                if (module_it == k.module_map.end())
                    throw str_exception("[module] %s not found\n", (*it)->_mod_name.c_str());

                module *mdl2 = module_it->second;
                mdl2->trigger_register_module(this, **it);
            }

            klog(info, "[module] setting state of %s from %s to %s\n", name.c_str(), 
                state_to_string(get_state()), state_to_string(state));

            if ((ret = mod_set_state(mod_handle, state)) == -1)
                return ret;

            act_state = get_state();
            if (act_state != module_state_safeop)
                return -1;

            break;
        case module_state_op:
            if (act_state != module_state_safeop) {
                if (set_state(module_state_safeop) == -1)
                    return ret;
            }

            klog(info, "[module] setting state of %s from %s to %s\n", name.c_str(), 
                state_to_string(get_state()), state_to_string(state));

            if ((ret = mod_set_state(mod_handle, state)) == -1)
                return ret;
            
            act_state = get_state();
            if (act_state != module_state_op)
                return -1;
            
            break;
        case module_state_boot:
            if (act_state != module_state_init) {
                if (set_state(module_state_init) == -1)
                    return ret;
            }

            klog(info, "[module] setting state of %s from %s to %s\n", name.c_str(), 
                state_to_string(get_state()), state_to_string(state));

            ret = mod_set_state(mod_handle, state);
            break;
        default:
            break;
    }

    return ret;
}

//! Get module state
/*!
  \return current module state 
  */
module_state_t module::get_state() {
    if (!mod_handle)
        throw str_exception("[module] %s not configured\n", name.c_str());

    if (!mod_get_state) {
        klog(error, "[module] error: no mod_get_state function\n"); 
        return module_state_unknown;
    }

    return mod_get_state(mod_handle); 
}

//! Send a request to module
/*! 
  \param reqcode request code
  \param ptr pointer to request structure
  \return success or failure
 */
int module::request(int reqcode, void* ptr) {
    if (!mod_handle)
        throw str_exception("[module] %s not configured\n", name.c_str());

    if (!mod_request)
        return 0;

    return mod_request(mod_handle, reqcode, ptr);
}

//! register trigger module 
/*!
 * \param mdl module to register
 */
void module::trigger_register_module(module *mdl, external_trigger& t) {
    if (t._direct_mode) {
        t._direct_cnt       = 0;
        t._direct_mdl       = mdl;

        set_trigger_cb_t cb = set_trigger_cb_t();
        cb.cb               = module::trigger_wrapper;
        cb.hdl              = &t;
        cb.clk_id           = t._clk_id;
        request(MOD_REQUEST_SET_TRIGGER_CB, &cb);        
        return;
    }

    worker_key k = { t._clk_id, t._prio, t._affinity_mask, t._divisor };
    worker_map_t::iterator it = _worker.find(k); 
    if (it == _worker.end()) {
        klog(info, "[module] new kernel worker for %s from %s, clk_id %d, prio %d, divisor %d\n",
                name.c_str(), mdl->name.c_str(), t._clk_id, t._prio, t._divisor);

        // create new worker thread
        kernel_worker *w = new kernel_worker(t._prio, t._affinity_mask, t._divisor);
        _worker[k] = w;

        set_trigger_cb_t cb = set_trigger_cb_t();
        cb.cb               = w->kernel_worker_trigger;
        cb.hdl              = w;
        cb.clk_id           = t._clk_id;
        request(MOD_REQUEST_SET_TRIGGER_CB, &cb);        
    }

    kernel_worker *w = _worker[k];
    w->add_module(mdl);
}
        
//! unregister trigger module 
/*!
 * \param mdl module to register
 */
void module::trigger_unregister_module(module *mdl, external_trigger& t) {
    if (t._direct_mode) {
        set_trigger_cb_t cb = set_trigger_cb_t();
        cb.cb               = module::trigger_wrapper;
        cb.hdl              = &t;
        cb.clk_id           = t._clk_id;
        request(MOD_REQUEST_UNSET_TRIGGER_CB, &cb);        
        return;
    }

    worker_key k = { t._clk_id, t._prio, t._affinity_mask, t._divisor };
    worker_map_t::iterator it = _worker.find(k); 
    if (it == _worker.end()) {
        klog(verbose , "[module] kernel worker for %s from %s, clk_id %d, prio %d not found\n",
                name.c_str(), mdl->name.c_str(), t._clk_id, t._prio);
        return;
    }

    kernel_worker *w = _worker[k];
    if (w->remove_module(mdl)) {
        set_trigger_cb_t cb = set_trigger_cb_t();
        cb.cb               = w->kernel_worker_trigger;
        cb.hdl              = w;
        cb.clk_id           = t._clk_id;
        request(MOD_REQUEST_UNSET_TRIGGER_CB, &cb);        

        _worker.erase(it);
        delete w;
    }
}

//! trigger module 
/*!
*/
void module::trigger() {
    if (!mod_handle)
        throw str_exception("[module] %s not configured\n", name.c_str());

    if (!mod_trigger)
        return;

    (*mod_trigger)(mod_handle);
}

//! service callbacks
int module::on_get_config(ln::service_request& req, 
        ln_service_robotkernel_module_get_config& svc) {
    YAML::Emitter out;
    out << *this;
    svc.resp.config = strdup(out.c_str());
    svc.resp.config_len = strlen(out.c_str());
    
    req.respond();

    free(svc.resp.config);
    return 0;
}

int module::on_get_feat(ln::service_request& req, 
        ln_service_robotkernel_module_get_feat& svc) {
    int feat = 0;
    request(MOD_REQUEST_GET_MODULE_FEAT, &feat);

    string features = "[ ";

    if (feat & MODULE_FEAT_PD)
        features += "'pd', ";
    if (feat & MODULE_FEAT_READ)
        features += "'read', ";
    if (feat & MODULE_FEAT_WRITE)
        features += "'write', ";
    if (feat & MODULE_FEAT_TRIGGER)
        features += "'trigger', ";
    if (feat & MODULE_FEAT_CANOPEN)
        features += "'canopen', ";
    if (feat & MODULE_FEAT_ETHERCAT)
        features += "'ethercat', ";
    if (feat & MODULE_FEAT_SERCOS)
        features += "'sercos', ";
    features += " ]";

    svc.resp.feat = feat;
    svc.resp.features = strdup(features.c_str());
    svc.resp.features_len = features.length();    
    req.respond();
    free(svc.resp.features);
    return 0;
}

bool module::worker_key::operator<(const worker_key& a) const {
    if (prio < a.prio)
        return true;
    if (prio > a.prio)
        return false;

    if (affinity < a.affinity)
        return true;
    if (affinity > a.affinity)
        return false;

    if (clk_id < a.clk_id)
        return true;
    if (clk_id > a.clk_id)
        return false;

    return (int) divisor < (int)a.divisor;
}

