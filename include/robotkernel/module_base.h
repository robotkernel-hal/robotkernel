//! robotkernel module base
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

/**
 * @file module_base.h
 * @brief Abstract base class for robotkernel modules.
 *
 * Defines the interface and lifecycle for robotkernel modules.
 * Modules are loadable plugin components that:
 *   - register services
 *   - add/remove devices
 *   - listen for device events
 *   - interact with the robotkernel runtime
 *
 * A module is typically instantiated via a C bridge entrypoint
 * (`BRIDGE_DEF()`) and binds into the robotkernel service/device
 * registries.
 */

#ifndef ROBOTKERNEL_MODULE_BASE_H
#define ROBOTKERNEL_MODULE_BASE_H

#include <unistd.h>
#include <stdint.h>
#include <list>
#include <stdio.h>
#include <stdexcept>

#include "robotkernel/exceptions.h"
#include "robotkernel/helpers.h"
#include "robotkernel/log_base.h"
#include "robotkernel/service_definitions.h"
#include "yaml-cpp/yaml.h"

#ifdef __cplusplus
#define EXPORT_C extern "C" 
#else
#define EXPORT_C
#endif

/**
 * @brief Opaque handle used by the C API.
 *
 * Internally this wraps a C++ shared_ptr to the module instance.
 */
#define MODULE_HANDLE void*

/**
 * @name Module state flags
 * @{
 */

/**
 * @brief Initial state after construction.
 */
const static uint16_t module_state_init     = 0x0001;

/**
 * @brief Pre-operational state.
 */
const static uint16_t module_state_preop    = 0x0002;

/**
 * @brief Safe-operational state.
 */
const static uint16_t module_state_safeop   = 0x0004;

/**
 * @brief Fully operational state.
 */
const static uint16_t module_state_op       = 0x0008;

/**
 * @brief Boot state (initial loading).
 */ 
const static uint16_t module_state_boot     = 0x0010;

/**
 * @brief Error flag (ORed with other states).
 */
const static uint16_t module_state_error    = 0x8000;

/** @} */

/**
 * @brief Module state type.
 */

typedef uint16_t module_state_t;

/**
 * @brief Combine two module states into a transition key.
 *
 * Used internally to identify valid transitions.
 */
#define GEN_STATE(from, to) \
    ((uint32_t)(from & ~module_state_error) << 16 | to)

/**
 * @brief Define a symbolic transition constant.
 */
#define DEFINE_STATE(from, to) \
    const static uint32_t from ## _2_ ## to = GEN_STATE(module_state_ ## from, module_state_ ## to)

/* All legal state transitions */
DEFINE_STATE(boot, boot);
DEFINE_STATE(boot, init);
DEFINE_STATE(boot, preop);
DEFINE_STATE(boot, safeop);
DEFINE_STATE(boot, op);

DEFINE_STATE(init, boot);
DEFINE_STATE(init, init);
DEFINE_STATE(init, preop);
DEFINE_STATE(init, safeop);
DEFINE_STATE(init, op);

DEFINE_STATE(preop, boot);
DEFINE_STATE(preop, init);
DEFINE_STATE(preop, preop);
DEFINE_STATE(preop, safeop);
DEFINE_STATE(preop, op);

DEFINE_STATE(safeop, boot);
DEFINE_STATE(safeop, init);
DEFINE_STATE(safeop, preop);
DEFINE_STATE(safeop, safeop);
DEFINE_STATE(safeop, op);

DEFINE_STATE(op, boot);
DEFINE_STATE(op, init);
DEFINE_STATE(op, preop);
DEFINE_STATE(op, safeop);
DEFINE_STATE(op, op);

/**
 * @brief Encoded state transition.
 */
typedef uint32_t transition_t;

// -----------------------------------------------------------------------------------
// module symbols
// -----------------------------------------------------------------------------------

/**
 * @brief Convert a module state to string.
 *
 * @param state 
 *        Module state value.
 *
 * @return Null-terminated string representation.
 */
extern "C" const char *state_to_string(module_state_t state);

/**
 * @brief Convert a string to a module state.
 *
 * @param state 
 *        String representation.
 *
 * @return Corresponding module state value.
 */
extern "C" module_state_t string_to_state(const char *state);

/**
 * @brief Configures module.
 * 
 * @param name 
 *        module name
 * @param config 
 *        configure file
 *
 * @return handle on success, NULL otherwise
 */
typedef MODULE_HANDLE (*mod_configure_t)(const char* name, const char* config);

/**
 * @brief Unconfigure module.
 * 
 * @param hdl 
 *        module handle
 *
 * @return success or failure
 */
typedef int (*mod_unconfigure_t)(MODULE_HANDLE hdl);

/**
 * @brief Set module state machine to defined state.
 * 
 * @param hdl 
 *        module handle
 * @param state 
 *        requested state
 *
 * @return success or failure
 */
typedef int (*mod_set_state_t)(MODULE_HANDLE hdl, module_state_t state);

/**
 * @brief Get module state machine state.
 *
 * @param hdl 
 *        module handle
 *
 * @return current state
 */
typedef module_state_t (*mod_get_state_t)(MODULE_HANDLE hdl);


// CPLUSPLUS Base Class
#ifdef __cplusplus

/**
 * @brief Cast C handle back to C++ module instance.
 *
 * Used internally by MODULE_DEF().
 */
#define HDL_2_MODCLASS(hdl, impl, modclass)                                         \
    struct impl ## _wrapper *wr =                                                   \
        reinterpret_cast<struct impl ## _wrapper *>(hdl);                           \
    std::shared_ptr<modclass> dev = wr->sp;                                         \
    if (!dev)                                                                       \
        throw std::runtime_error(robotkernel::helpers::string_printf("["#impl"] "   \
                    "invalid module handle to <"#modclass" *>\n")); 

/**
 * @brief Export a module as a C-loadable plugin.
 *
 * Generates:
 *  - mod_configure()
 *  - mod_unconfigure()
 *  - mod_set_state()
 *  - mod_get_state()
 *
 * @param impl     Implementation name
 * @param modclass C++ module class
 */
#define MODULE_DEF(impl, modclass)                                                  \
struct impl ## _wrapper {                                                           \
    std::shared_ptr<modclass> sp;                                                   \
};                                                                                  \
                                                                                    \
EXPORT_C size_t mod_set_state(MODULE_HANDLE hdl, module_state_t state) {            \
    HDL_2_MODCLASS(hdl, impl, modclass)                                             \
    return dev->set_state(state);                                                   \
}                                                                                   \
                                                                                    \
EXPORT_C module_state_t mod_get_state(MODULE_HANDLE hdl) {                          \
    HDL_2_MODCLASS(hdl, impl, modclass)                                             \
    return dev->get_state();                                                        \
}                                                                                   \
                                                                                    \
EXPORT_C int mod_unconfigure(MODULE_HANDLE hdl) {                                   \
    HDL_2_MODCLASS(hdl, impl, modclass)                                             \
    wr->sp->deinit();                                                               \
    wr->sp = nullptr;                                                               \
    delete wr;                                                                      \
    return 0;                                                                       \
}                                                                                   \
                                                                                    \
EXPORT_C MODULE_HANDLE mod_configure(const char* name, const char* config) {        \
    struct impl ## _wrapper *wr;                                                    \
    YAML::Node doc = YAML::Load(config);                                            \
                                                                                    \
    wr = new struct impl ## _wrapper();                                             \
    if (!wr)                                                                        \
        throw std::runtime_error(                                                   \
                robotkernel::helpers::string_printf(                                \
                "["#impl"] error allocating memory\n"));                            \
    wr->sp = std::make_shared<modclass>(name, doc);                                 \
    wr->sp->init();                                                                 \
                                                                                    \
    return (MODULE_HANDLE)wr;                                                       \
}

namespace robotkernel {

/**
 * @class module_base
 * @brief Abstract interface for modules in robotkernel.
 *
 * A module provides a defined lifecycle:
 *   - construction(): initialize from a YAML node
 *   - init():         post-configuration setup
 *   - set_state():    module state machine calls
 *   - deinit():       clean shutdown
 *   - destruction():  freeing resources
 *
 * During these phases, modules register services, devices,
 * and device listeners with the robotkernel runtime.
 *
 * Modules should be created and configured via the module
 * entrypoints using `MODULE_DEF()`.
 */
class module_base : 
    public robotkernel::log_base
{
    private:
        // Prevent default construction
        module_base();                  

    public:
        /// The configured instance name of this module.
        const std::string name;

        /// The active state of this module.
        module_state_t state;

        /// Synchronizes access to the state variable.
        std::mutex state_mtx;

        /**
         * @brief Construct a module instance.
         *
         * @param impl  
         *        String identifying the implementation type.
         * @param name  
         *        Instance name from configuration.
         * @param node  
         *        YAML configuration node (defaults to empty).
         */
        module_base(const std::string& impl,
                const std::string& name,
                const YAML::Node& node = YAML::Node());

        /**
         * @brief Virtual destructor.
         */
        virtual ~module_base() {};

        /**
         * @brief Get module instance name.
         *
         * @return Module instance name.
         */
        std::string get_name() { return name; }

        /**
         * @brief Optional initiazation method
         * 
         * usefull to call shared_from_this() at construction time
         */
        virtual void init() {};
        
        /**
         * @biref Optional de-initiazation method
         * 
         * usefull to call shared_from_this() at construction time
         */
        virtual void deinit() {};
    
        //*********************************************
        // STATE MACHINE FUNCTIONS
        //*********************************************

        /**
         * @brief Set module state machine to defined state.
         *
         * @param[in] state     Requested state which will be tried to switch to.
         *
         * @return success or failure
         */
        virtual int set_state(module_state_t state);

        //! State transition from OP to SAFEOP
        virtual void set_state_op_2_safeop() {}

        //! State transition from SAFEOP to PREOP
        virtual void set_state_safeop_2_preop() {}

        //! State transition from PREOP to INIT
        virtual void set_state_preop_2_init() {}

        //! State transition from INIT to PREOP
        virtual void set_state_init_2_preop() {}

        //! State transition from PREOP to SAFEOP
        virtual void set_state_preop_2_safeop() {}

        //! State transition from SAFEOP to OP
        virtual void set_state_safeop_2_op() {}

        /**
         * @brief Get module state machine state.
         *
         * @return Current state.
         */
        virtual module_state_t get_state() { return state; }

    protected:
        /// Set error flag
        void set_error(void) { state |= module_state_error; }
        /// Clear error flag
        void reset_error(void) { state &= ~module_state_error; }
        /// Check error flag
        bool is_error(void) { return (state & module_state_error); }
};

typedef std::shared_ptr<module_base> sp_module_base_t;

}; // namespace robotkernel

#endif // __cplusplus

#endif // ROBOTKERNEL_MODULE_BASE_H

