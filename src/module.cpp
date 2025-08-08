//! robotkernel module definition
/*!
 * (C) Robert Burger <robert.burger@dlr.de>
 */

// vim: set expandtab softtabstop=4 shiftwidth=4
// -*- mode: c++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil; -*- 

/*
 * This file is part of robotkernel.
 *
 * robotkernel is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 * 
 * robotkernel is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with robotkernel; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

// public headers 
#include "robotkernel/exceptions.h"
#include "robotkernel/helpers.h"
#include "robotkernel/service_definitions.h"

// private headers 
#include "kernel.h"
#include "module.h"

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

const char string_state_error[]   = "<ERROR>";
const char string_state_init[]    = "<INIT>";
const char string_state_preop[]   = "<PREOP>";
const char string_state_safeop[]  = "<SAFEOP>";
const char string_state_op[]      = "<OP>";
const char string_state_boot[]    = "<BOOT>";

module* currently_loading_module = NULL;

namespace robotkernel {

// assign generated service definitions
const std::string module::service_definition_set_state  = services::robotkernel_module_set_state_service_definition;
const std::string module::service_definition_get_state  = services::robotkernel_module_get_state_service_definition;
const std::string module::service_definition_get_config = services::robotkernel_module_get_config_service_definition;

void split_file_name(const string& str, string& path, string& file);

module_state_t decode_power_up_state(std::string tmp_power_up) {
    module_state_t power_up;

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

#undef power_up_state

    return power_up;
}
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

YAML::Emitter& operator<<(YAML::Emitter& out, const robotkernel::module& mdl) {
    out << YAML::BeginMap;
    out << YAML::Key << "name" << YAML::Value << mdl.name;
    out << YAML::Key << "so_file" << YAML::Value << mdl.file_name;

    if (mdl.config != "") {
        YAML::Node node = YAML::Load(mdl.config);
        out << YAML::Key << "config";
        out << YAML::Value << node;
    }

    if (!mdl.depends.empty()) {
        out << YAML::Key << "depends" << YAML::Value;
        out << YAML::Flow << YAML::BeginSeq;
        for (module::depend_list_t::const_iterator it = mdl.depends.begin(); 
                it != mdl.depends.end(); ++it)
            out << *it;
        out << YAML::EndSeq;
    }

    // TODO excludes

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
    
    if (node["depends"]) {
        if (node["depends"].Type() == YAML::NodeType::Scalar) {
            std::string tmp = get_as<std::string>(node, "depends");

            if (tmp == name)
                throw runtime_error(string_printf("module %s depends on itself?\n", name.c_str()));

            depends.push_back(tmp); 
        } else if (node["depends"].Type() == YAML::NodeType::Sequence) {
            for (YAML::const_iterator it = node["depends"].begin();
                    it != node["depends"].end(); ++it) {
                std::string tmp = it->as<std::string>();

                if (tmp == name)
                    throw runtime_error(string_printf("module %s depends on itself?\n", name.c_str()));

                depends.push_back(tmp); 
            }
        }
    }
    
    if (node["excludes"]) {
        if (node["excludes"].Type() == YAML::NodeType::Scalar) {
            std::string tmp = get_as<std::string>(node, "excludes");

            if (tmp == name)
                throw runtime_error(string_printf("module %s excludes on itself?\n", name.c_str()));

            excludes.push_back(tmp); 
        } else if (node["excludes"].Type() == YAML::NodeType::Sequence) {
            for (YAML::const_iterator it = node["excludes"].begin();
                    it != node["excludes"].end(); ++it) {
                std::string tmp = it->as<std::string>();

                if (tmp == name)
                    throw runtime_error(string_printf("module %s excludes on itself?\n", name.c_str()));

                excludes.push_back(tmp); 
            }
        }
    }

    string tmp_power_up = get_as<string>(node, "power_up", "init");
    power_up = decode_power_up_state(tmp_power_up);

    _init();

    currently_loading_module = NULL;

    kernel::instance.add_service(name, "set_state",
            service_definition_set_state,
            std::bind(&module::service_set_state, this, _1, _2));
    kernel::instance.add_service(name, "get_state",
            service_definition_get_state,
            std::bind(&module::service_get_state, this, _1, _2));
    kernel::instance.add_service(name, "get_config",
            service_definition_get_config,
            std::bind(&module::service_get_config, this, _1, _2));
}
        
void module::_init() {
    mod_configure           = (mod_configure_t)         dlsym(so_handle, "mod_configure");
    mod_unconfigure         = (mod_unconfigure_t)       dlsym(so_handle, "mod_unconfigure");
    mod_set_state           = (mod_set_state_t)         dlsym(so_handle, "mod_set_state");
    mod_get_state           = (mod_get_state_t)         dlsym(so_handle, "mod_get_state");

    if (!mod_configure)
        robotkernel::kernel::instance.log(warning, "missing mod_configure in %s\n", file_name.c_str());;
    if (!mod_unconfigure)
        robotkernel::kernel::instance.log(verbose, "missing mod_unconfigure in %s\n", file_name.c_str());
    if (!mod_set_state)
        robotkernel::kernel::instance.log(verbose, "missing mod_set_state in %s\n", file_name.c_str());
    if (!mod_get_state)
        robotkernel::kernel::instance.log(verbose, "missing mod_get_state in %s\n", file_name.c_str());

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
    if (mod_configure) {
        mod_handle = mod_configure(name.c_str(), config.c_str());
    }

    if (!mod_handle) {
        throw runtime_error(string_printf("mod_handle of %s is NULL, can not proceed!\n"
                "(does module export mod_configure() function?)", file_name.c_str()));
    }

    return configured();
}

//! module destruction
/*!
  destroys module
  */
module::~module() {
    robotkernel::kernel::instance.log(verbose, "%s destructing %s\n", name.c_str(), file_name.c_str());

    // unconfigure module first
    if (mod_handle && mod_unconfigure) {
        robotkernel::kernel::instance.log(verbose, "%s calling unconfigure\n", name.c_str());
        mod_unconfigure(mod_handle);
        mod_handle = NULL;
    }

    kernel::instance.remove_devices(name);
    kernel::instance.remove_services(name);
}

//! Set module state 
/*!
  \param state new model state 
  \return success or failure
  */
int module::set_state(module_state_t state) {
    if (!mod_handle)
        throw runtime_error(string_printf("%s not configured\n", name.c_str()));

    if (!mod_set_state) {
        robotkernel::kernel::instance.log(error, "%s error: no mod_set_state function\n", name.c_str());
        return -1;
    }

    module_state_t act_state = get_state();

    // get transition
    uint32_t transition = GEN_STATE(act_state, state);

#define set_state__check(to_state) {                                                                    \
    int ret = 0;                                                                                        \
    try {                                                                                               \
        robotkernel::kernel::instance.log(info, "module %s -> requesting state %s\n", name.c_str(), state_to_string(to_state));   \
        ret = mod_set_state(mod_handle, to_state);                                                      \
        robotkernel::kernel::instance.log(info, "module %s -> reached    state %s\n", name.c_str(), state_to_string(ret));          \
    } catch (exception& e) {                                                                            \
        robotkernel::kernel::instance.log(error, "set_state: %s\n", e.what());                                                       \
        robotkernel::kernel::instance.log(error,"module %s -> refused    state %s, error %s, staying in %s\n", name.c_str(),        \
                state_to_string(to_state), e.what(), state_to_string(act_state));                       \
    }                                                                                                   \
    if (ret != to_state) return -1; }                          

    if ((act_state == module_state_error) && (state == module_state_init)) {
        set_state__check(state);
        return 0;
    }
    
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
            // ====> stop receiving measurements
            set_state__check(module_state_preop);
            
            if (state == module_state_preop)
                break;
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
            // ====> start receiving measurements
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
        throw runtime_error(string_printf("%s not configured\n", name.c_str()));

    if (!mod_get_state) {
        robotkernel::kernel::instance.log(error, "%s error: no mod_get_state function\n", name.c_str()); 
        return module_state_init;
    }

    return mod_get_state(mod_handle); 
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

    // response data
    string error_message;

    try {      
        kernel::instance.set_state(name, string_to_state(state.c_str()));
    } catch (const exception& e) {
        error_message = e.what();
    }

#define SET_STATE_RESP_ERROR_MESSAGE    0
    response.resize(1);
    response[SET_STATE_RESP_ERROR_MESSAGE] = error_message;

    return 0;
}

//! get module state
/*!
 * \param request service request data
 * \parma response service response data
 * \return success
 */
int module::service_get_state(const service_arglist_t& request, 
        service_arglist_t& response) {
    
    // response data
    string state = "";
    string error_message = "";

    try {      
        module_state_t act_state = kernel::instance.get_state(name);
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


