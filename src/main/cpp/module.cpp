//! robotkernel module definition
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

#include "robotkernel/kernel.h"
#include "robotkernel/module.h"
#include "robotkernel/kernel_worker.h"
#include "robotkernel/exceptions.h"
#include "robotkernel/helpers.h"
#include "robotkernel/trigger.h"
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
    ret_state(boot);
    ret_state(init);
    ret_state(preop);
    ret_state(safeop);
    ret_state(op);

    return module_state_init;
}

//! generate new trigger object
/*!
 * \param node configuration node
 */
module::external_trigger::external_trigger(const YAML::Node& node) {
    dev_name       = get_as<string>(node, "dev_name");
    prio           = 0;
    affinity_mask  = 0;
    divisor        = get_as<int>(node, "divisor", 1);
    direct_mode    = get_as<bool>(node, "direct_mode", false);

    if (node["prio"]) {
        prio = get_as<int>(node, "prio");

        if (direct_mode)
            klog(info, "external_trigger prio will not have any effect in direct mode!\n");
    }

    if(node["affinity"]) {
        const YAML::Node& affinity = node["affinity"];
        if (affinity.Type() == YAML::NodeType::Scalar)
            affinity_mask = (1 << affinity.as<int>());
        else if (affinity.Type() == YAML::NodeType::Sequence)
            for (YAML::const_iterator it = affinity.begin(); it != affinity.end(); ++it)
                affinity_mask |= (1 << it->as<int>());

        if (direct_mode)
            klog(info, "external_trigger affinity will not have any effect in direct mode!\n");
    }
}

YAML::Emitter& operator<<(YAML::Emitter& out, const module::external_trigger& t) {
    out << YAML::BeginMap;
    out << YAML::Key << "dev_name" << YAML::Value << t.dev_name;

    if (t.direct_mode)
        out << YAML::Key << "direct_mode" << YAML::Value << t.direct_mode;
    else {
        if (t.prio)
            out << YAML::Key << "prio"     << YAML::Value << t.prio;
        if (t.affinity_mask) {
            out << YAML::Key << "affinity" << YAML::Value;

            out << YAML::Flow;
            out << YAML::BeginSeq;
            for (int i = 0; i < 32; ++i)
                if (t.affinity_mask & (1 << i))
                    out << i;
            out << YAML::EndSeq;
        }
    }

    if (t.divisor != 1)
        out << YAML::Key << "divisor"  << YAML::Value << t.divisor;
    out << YAML::EndMap;

    return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const robotkernel::module& mdl) {
    out << YAML::BeginMap;
    out << YAML::Key << "name" << YAML::Value << mdl.name;
    out << YAML::Key << "so_file" << YAML::Value << mdl.file_name;

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
    mod_unconfigure(NULL), mod_set_state(NULL) {
    name             = get_as<string>(node, "name");

    currently_loading_module = this;
    
    if (node["trigger"]) {
        for (YAML::const_iterator it = node["trigger"].begin(); 
                it != node["trigger"].end(); ++it) {
            try {
                external_trigger *trigger = new external_trigger(*it);
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
    k.add_service(name, "set_state",
            service_definition_set_state,
            std::bind(&module::service_set_state, this, _1, _2));
    k.add_service(name, "get_state",
            service_definition_get_state,
            std::bind(&module::service_get_state, this, _1, _2));
    k.add_service(name, "get_config",
            service_definition_get_config,
            std::bind(&module::service_get_config, this, _1, _2));
}

void module::_init() {
    mod_configure           = (mod_configure_t)         dlsym(so_handle, "mod_configure");
    mod_unconfigure         = (mod_unconfigure_t)       dlsym(so_handle, "mod_unconfigure");
    mod_set_state           = (mod_set_state_t)         dlsym(so_handle, "mod_set_state");
    mod_get_state           = (mod_get_state_t)         dlsym(so_handle, "mod_get_state");
    mod_tick                = (mod_tick_t)              dlsym(so_handle, "mod_tick");

    if (!mod_configure)
        klog(warning, "missing mod_configure in %s\n", file_name.c_str());;
    if (!mod_unconfigure)
        klog(verbose, "missing mod_unconfigure in %s\n", file_name.c_str());
    if (!mod_set_state)
        klog(verbose, "missing mod_set_state in %s\n", file_name.c_str());
    if (!mod_get_state)
        klog(verbose, "missing mod_get_state in %s\n", file_name.c_str());
    if (!mod_tick)
        klog(verbose, "missing mod_tick in %s\n", file_name.c_str());

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
        throw str_exception("mod_handle of %s is NULL, can not proceed!\n"
                "(does module export mod_configure() function?)", file_name.c_str());

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
        klog(verbose, "%s calling unconfigure\n", name.c_str());
        mod_unconfigure(mod_handle);
        mod_handle = NULL;
    }

    kernel::get_instance()->remove_devices(name);
    kernel::get_instance()->remove_services(name);
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

    auto& k = *kernel::get_instance();
    module_state_t act_state = get_state();
    
    // get transition
    uint32_t transition = GEN_STATE(act_state, state);

#define set_state__check(to_state) {                    \
    int ret = 0;                                        \
    try {                                               \
        ret = mod_set_state(mod_handle, to_state);      \
    } catch (exception& e) {                            \
        klog(error, "set_state: %s\n", e.what());       \
    }                                                   \
    if (ret != to_state) return -1; }                          

    switch (transition) {
        case op_2_safeop:
        case op_2_preop:
        case op_2_init:
        case op_2_boot:
            // ====> stop sending commands
            if (state == module_state_safeop) {
                set_state__check(module_state_safeop);
                break;
            }
        case safeop_2_preop:
        case safeop_2_init:
        case safeop_2_boot:
            // ====> stop receiving measurements, unregistering triggers
            for (const auto& et : triggers) {
                klog(info, "%s removing module trigger %s\n",
                        name.c_str(), et->dev_name.c_str());
                
                auto t_dev = k.get_trigger(et->dev_name);
                t_dev->remove_trigger(shared_from_this());
            } 
            
            if (state == module_state_preop) {
                set_state__check(module_state_preop);
                break;
            }
        case preop_2_init:
        case preop_2_boot:
            // ====> deinit devices
        case init_2_init:
            // ====> re-/open device
            set_state__check(module_state_init);

            if (state == module_state_init)
                break;
        case init_2_boot:
            set_state__check(module_state_boot);
            break;
        case boot_2_init:
        case boot_2_preop:
        case boot_2_safeop:
        case boot_2_op:
            // ====> re-/open device
            set_state__check(module_state_init);

            if (state == module_state_init)
                break;
        case init_2_op:
        case init_2_safeop:
        case init_2_preop:
            // ====> initial devices            
            set_state__check(module_state_preop);

            if (state == module_state_preop)
                break;
        case preop_2_op:
        case preop_2_safeop:
            // ====> start receiving measurements, registering triggers
            for (const auto& et : triggers) {
                klog(info, "%s adding module trigger \"%s\"\n",
                        name.c_str(), et->dev_name.c_str());
                
                auto t_dev = k.get_trigger(et->dev_name);
                divisor = et->divisor;
                t_dev->add_trigger(shared_from_this(), et->direct_mode, et->prio, et->affinity_mask);
            }
            
            set_state__check(module_state_safeop);

            if (state == module_state_safeop)
                break;
        case safeop_2_op:
            // ====> start sending commands           
            set_state__check(module_state_op);
            
            break;
        case op_2_op:
        case safeop_2_safeop:
        case preop_2_preop:
            // ====> do nothing
            set_state__check(state);
            break;

        default:
            return -1;
    }
            
    return 0;
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
        return module_state_init;
    }

    return mod_get_state(mod_handle); 
}

//! trigger module 
/*!
*/
void module::tick() {
    if (!mod_handle)
        throw str_exception("[%s] not configured\n", name.c_str());

    if (!mod_tick)
        return;

    (*mod_tick)(mod_handle);
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
"- string: state\n"
"response:\n"
"- string: error_message\n";

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
"- string: state\n"
"- string: error_message\n";

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
"- string: config\n";

