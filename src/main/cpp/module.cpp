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

#include "robotkernel/kernel.h"
#include "robotkernel/module.h"
#include "robotkernel/kernel_worker.h"
#include "robotkernel/exceptions.h"
#include "robotkernel/helpers.h"
#include <sys/stat.h>
#include <dlfcn.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <algorithm>
#include <locale> 

#ifndef __VXWORKS__
#include <sys/types.h>
#include <libgen.h>
#include <dirent.h>
#endif

using namespace std;
using namespace std::placeholders;
using namespace robotkernel;
using namespace string_util;

const char string_state_error[]   = "<ERROR>";
const char string_state_config[]  = "<CONFIG>";
const char string_state_init[]    = "<INIT>";
const char string_state_preop[]   = "<PREOP>";
const char string_state_safeop[]  = "<SAFEOP>";
const char string_state_op[]      = "<OP>";
const char string_state_boot[]    = "<BOOT>";

module* currently_loading_module = NULL;

namespace robotkernel {
void split_file_name(const string& str, string& path, string& file);
}

const char *state_to_string(module_state_t state) {
    switch (state) {
        default:
        case module_state_config:
            return string_state_config;
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
    std::transform(state.begin(), state.end(), state.begin(), 
            (int (*)(int))tolower);
#define ret_state(a) if(state == #a) return module_state_ ## a;
    ret_state(error);
    ret_state(config);
    ret_state(boot);
    ret_state(init);
    ret_state(preop);
    ret_state(safeop);
    ret_state(op);

    return module_state_config;
}

//! generate new trigger object
/*!
 * \param node configuration node
 */
module::external_trigger::external_trigger(const YAML::Node& node) {
    _mod_name       = get_as<string>(node, "mod_name");
    _clk_id         = get_as<int>(node, "clk_id");
    _prio           = 0;
    _affinity_mask  = 0;
    _divisor        = get_as<int>(node, "divisor", 1);
    _direct_mode    = get_as<bool>(node, "direct_mode", false);

    if (node["prio"]) {
        _prio = get_as<int>(node, "prio");

        if (_direct_mode)
            klog(info, "external_trigger prio will not have any effect in direct mode!\n");
    }

    if(node["affinity"]) {
        const YAML::Node& affinity = node["affinity"];
        if (affinity.Type() == YAML::NodeType::Scalar)
            _affinity_mask = (1 << affinity.as<int>());
        else if (affinity.Type() == YAML::NodeType::Sequence)
            for (YAML::const_iterator it = affinity.begin(); it != affinity.end(); ++it)
                _affinity_mask |= (1 << it->as<int>());

        if (_direct_mode)
            klog(info, "external_trigger affinity will not have any effect in direct mode!\n");
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

YAML::Emitter& operator<<(YAML::Emitter& out, const robotkernel::module& mdl) {
    out << YAML::BeginMap;
    out << YAML::Key << "name" << YAML::Value << mdl.name;
    out << YAML::Key << "file_name" << YAML::Value << mdl.file_name;

    YAML::Node node = YAML::Load(mdl.config);
    out << YAML::Key << "config";
    out << YAML::Value << node;

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
 * \param node configuration node
 */
module::module(const YAML::Node& node)
    : so_file(node), mod_handle(NULL), mod_configure(NULL), 
    mod_unconfigure(NULL), mod_read(NULL), mod_write(NULL), 
    mod_set_state(NULL) {
    name             = get_as<string>(node, "name");

    currently_loading_module = this;
    
    if (node["trigger"]) {
        for (YAML::const_iterator it = node["trigger"].begin(); 
                it != node["trigger"].end(); ++it) {
            try {
                external_trigger *trigger = new external_trigger(*it);
                depends.push_back(trigger->_mod_name);
                triggers.push_back(trigger);
            } catch (YAML::Exception& e) {
                YAML::Emitter t;
                t << node["trigger"];
                klog(error, "%s exception creating external trigger: %s\n"
                        "got config string: \n====\n%s\n====\n",
                        name.c_str(), e.what(), t.c_str());
                throw;
            }
        }
    }

    if (node["depends"]) {
        if (node["depends"].Type() == YAML::NodeType::Scalar) {
            std::string tmp = get_as<std::string>(node, "depends");

            if (tmp == name)
                throw str_exception("module %s depends on itself?\n", name.c_str());

            depends.push_back(tmp); 
        } else if (node["depends"].Type() == YAML::NodeType::Sequence) {
            for (YAML::const_iterator it = node["depends"].begin();
                    it != node["depends"].end(); ++it) {
                std::string tmp = it->as<std::string>();

                if (tmp == name)
                    throw str_exception("module %s depends on itself?\n", name.c_str());

                depends.push_back(tmp); 
            }
        }
    }

    string tmp_power_up = get_as<string>(node, "power_up", "init");

    std::transform(tmp_power_up.begin(), tmp_power_up.end(), 
            tmp_power_up.begin(), (int (*)(int))tolower);
#define power_up_state(a) \
    if (tmp_power_up == #a) \
        power_up = module_state_##a

    power_up_state(init);
    else power_up_state(preop);
    else power_up_state(safeop);
    else power_up_state(op);
    else power_up_state(boot);
    else if (tmp_power_up == "true") power_up = module_state_op;
    else power_up = module_state_init;

    _init();

    currently_loading_module = NULL;

    kernel& k = *kernel::get_instance();
    k.add_service(name, name + ".set_state",
            service_definition_set_state,
            std::bind(&module::service_set_state, this, _1, _2));
    k.add_service(name, name + ".get_state",
            service_definition_get_state,
            std::bind(&module::service_get_state, this, _1, _2));
    k.add_service(name, name + ".get_config",
            service_definition_get_config,
            std::bind(&module::service_get_config, this, _1, _2));
    k.add_service(name, name + ".get_feat",
            service_definition_get_feat,
            std::bind(&module::service_get_feat, this, _1, _2));
}

void module::_init() {
    mod_configure           = (mod_configure_t)         dlsym(so_handle, "mod_configure");
    mod_unconfigure         = (mod_unconfigure_t)       dlsym(so_handle, "mod_unconfigure");
    mod_read                = (mod_read_t)              dlsym(so_handle, "mod_read");
    mod_write               = (mod_write_t)             dlsym(so_handle, "mod_write");
    mod_set_state           = (mod_set_state_t)         dlsym(so_handle, "mod_set_state");
    mod_get_state           = (mod_get_state_t)         dlsym(so_handle, "mod_get_state");
    mod_request             = (mod_request_t)           dlsym(so_handle, "mod_request");
    mod_trigger             = (mod_trigger_t)           dlsym(so_handle, "mod_trigger");
    mod_trigger_slave_id    = (mod_trigger_slave_id_t)  dlsym(so_handle, "mod_trigger_slave_id");

    if (!mod_configure)
        klog(warning, "missing mod_configure in %s\n", file_name.c_str());;
    if (!mod_unconfigure)
        klog(verbose, "missing mod_unconfigure in %s\n", file_name.c_str());
    if (!mod_read)
        klog(verbose, "missing mod_read in %s\n", file_name.c_str());
    if (!mod_write)
        klog(verbose, "missing mod_write in %s\n", file_name.c_str());
    if (!mod_set_state)
        klog(verbose, "missing mod_set_state in %s\n", file_name.c_str());
    if (!mod_get_state)
        klog(verbose, "missing mod_get_state in %s\n", file_name.c_str());
    if (!mod_request)
        klog(verbose, "missing mod_request in %s\n", file_name.c_str());
    if (!mod_trigger)
        klog(verbose, "missing mod_trigger in %s\n", file_name.c_str());
    if (!mod_trigger_slave_id)
        klog(verbose, "missing mod_trigger_slave_id in %s\n", file_name.c_str());

    // try to configure
    reconfigure();
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

    if(!mod_handle)
        throw str_exception("mod_handle of %s is NULL, can not proceed!\n(does module export mod_configure() function?)", file_name.c_str());

    // try to reach init state
    set_state(module_state_init);

    return configured();
}

//! module destruction
/*!
  destroys module
  */
module::~module() {
    klog(info, "%s destructing %s\n", name.c_str(), file_name.c_str());

    while (!triggers.empty()) {
        external_trigger *trigger = triggers.front();
        triggers.pop_front();

        delete trigger;
    }

    // unconfigure module first
    if (mod_handle && mod_unconfigure) {
        mod_unconfigure(mod_handle);
        mod_handle = NULL;
    }

	kernel::get_instance()->remove_service_requester(name);
    kernel::get_instance()->remove_services(name);
}

//! Read process data from module
/*!
  \param buf buffer to store cyclic data
  \param busize size of buffer 
  \return size of read bytes 
  */
size_t module::read(char* buf, size_t bufsize) {
    if (!mod_handle)
        throw str_exception("%s not configured!\n", name.c_str());

    if (!mod_read) {
        klog(error, "%s error: no mod_read function\n", name.c_str());
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
        throw str_exception("%s not configured!\n", name.c_str());

    if (!mod_write) {
        klog(error, "%s error: no mod_write function\n", name.c_str());
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
        throw str_exception("%s not configured\n", name.c_str());

    if (!mod_set_state) {
        klog(error, "%s error: no mod_set_state function\n", name.c_str());
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
                klog(info, "%s removing module trigger %s\n",
                        name.c_str(), (*it)->_mod_name.c_str());

                kernel& k = *kernel::get_instance();
                k.trigger_unregister_module((*it)->_mod_name, this, **it);
            }

            klog(info, "%s setting state from %s to %s\n", name.c_str(), 
                    state_to_string(get_state()), state_to_string(state));

            ret = mod_set_state(mod_handle, state);
            break;
        case module_state_preop:
            if (act_state != module_state_init) {
                if ((ret = set_state(module_state_init)) == -1)
                    return ret;
            }

            klog(info, "%s setting state from %s to %s\n", name.c_str(), 
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
                klog(info, "%s adding module trigger %s\n",
                        name.c_str(), (*it)->_mod_name.c_str());

                kernel& k = *kernel::get_instance();
                k.trigger_register_module((*it)->_mod_name, this, **it);
            }

            klog(info, "%s setting state from %s to %s\n", name.c_str(), 
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

            klog(info, "%s setting state from %s to %s\n", name.c_str(), 
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

            klog(info, "%s setting state from %s to %s\n", name.c_str(), 
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
        throw str_exception("%s not configured\n", name.c_str());

    if (!mod_get_state) {
        klog(error, "%s error: no mod_get_state function\n", name.c_str()); 
        return module_state_config;
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
//    if(reqcode == MOD_REQUEST_GET_CFG_PATH) {
//        char** ret = (char**)ptr;
//        *ret = (char*)config_file_path.c_str();
//        return 0;
//    }
    
    if (!mod_handle)
        throw str_exception("[%s] not configured\n", name.c_str());

    if (!mod_request)
        return 0;

    return mod_request(mod_handle, reqcode, ptr);
}

//! register trigger module 
/*!
 * \param mdl module to register
 */
void module::trigger_register_module(module *mdl, external_trigger& t) {
    // let mdl know, that it is triggered by us
    char *triggered_by = strdup(name.c_str());
    mdl->request(MOD_REQUEST_TRIGGERED_BY, &triggered_by);
    free(triggered_by);

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
        klog(info, "%s new kernel worker from %s, clk_id %d, prio %d, divisor %d\n",
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
        klog(verbose, "%s kernel worker from %s, clk_id %d, prio %d not found\n",
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

//! trigger module's slave id
/*!
 * \param slave_id slave id to trigger
 */
void module::trigger(uint32_t slave_id) {
    if (!mod_handle)
        throw str_exception("[%s] not configured\n", name.c_str());

    if (!mod_trigger_slave_id)
        return;

    (*mod_trigger_slave_id)(mod_handle, slave_id);
}

//! trigger module 
/*!
*/
void module::trigger() {
    if (!mod_handle)
        throw str_exception("[%s] not configured\n", name.c_str());

    if (!mod_trigger)
        return;

    (*mod_trigger)(mod_handle);
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

//! set module state
/*!
 * \param request service request data
 * \parma response service response data
 * \return success
 */
int module::service_set_state(const service_arglist_t& request, 
        service_arglist_t& response) {
    // request data
#define SET_STATE_REQ_STATE     0
    string state = request[SET_STATE_REQ_STATE];
    kernel& k = *kernel::get_instance();

    // response data
    string error_message;

    try {      
        k.set_state(name, string_to_state(state.c_str()));
    } catch (const exception& e) {
        error_message = e.what();
    }

#define SET_STATE_RESP_ERROR_MESSAGE    0
    response.resize(1);
    response[SET_STATE_RESP_ERROR_MESSAGE] = error_message;

    return 0;
}

const std::string module::service_definition_set_state = 
    "request:\n"
    "    string: state\n"
    "response:\n"
    "    string: error_message\n";

//! get module state
/*!
 * \param request service request data
 * \parma response service response data
 * \return success
 */
int module::service_get_state(const service_arglist_t& request, 
        service_arglist_t& response) {
    kernel& k = *kernel::get_instance();
    
    // response data
    string state = "";
    string error_message = "";

    try {      
        module_state_t act_state = k.get_state(name);
        state = state_to_string(act_state);
    } catch (const exception& e) {
        error_message = e.what();
    }

#define GET_STATE_RESP_STATE            0
#define GET_STATE_RESP_ERROR_MESSAGE    1
    response.resize(2);
    response[GET_STATE_RESP_STATE]          = state;
    response[GET_STATE_RESP_ERROR_MESSAGE]  = error_message;

    return 0;
}

const std::string module::service_definition_get_state = 
    "response:\n"
    "    string: state\n"
    "    string: error_message\n";

//! get module config
/*!
 * \param request service request data
 * \parma response service response data
 * \return success
 */
int module::service_get_config(const service_arglist_t& request, 
        service_arglist_t& response) {
    YAML::Emitter out;
    out << *this;

#define GET_CONFIG_RESP_CONFIG  0
    response.resize(1);
    response[GET_CONFIG_RESP_CONFIG] = string(out.c_str());

    return 0;
}

const std::string module::service_definition_get_config =
    "response:\n"
    "    string: config\n";

//! get module feat
/*!
 * \param request service request data
 * \parma response service response data
 * \return success
 */
int module::service_get_feat(const service_arglist_t& request, 
        service_arglist_t& response) {
    int32_t feat = 0;
    this->request(MOD_REQUEST_GET_MODULE_FEAT, &feat);

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

#define GET_FEAT_RESP_FEAT      0
#define GET_FEAT_RESP_REATURES  1
    response.resize(2);
    response[GET_FEAT_RESP_FEAT]     = feat;
    response[GET_FEAT_RESP_REATURES] = features;

    return 0;
}

const std::string module::service_definition_get_feat = 
    "response:\n"
    "    int32_t: feat\n"
    "    string: features\n";

