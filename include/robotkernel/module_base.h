//! robotkernel module base
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

#ifndef ROBOTKERNEL_MODULE_BASE_H
#define ROBOTKERNEL_MODULE_BASE_H

#include <unistd.h>
#include <stdint.h>
#include <list>
#include <stdio.h>

#include "robotkernel/trigger_base.h"
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

#define MODULE_HANDLE void*

const static uint16_t module_state_init     = 0x0001;
const static uint16_t module_state_preop    = 0x0002;
const static uint16_t module_state_safeop   = 0x0004;
const static uint16_t module_state_op       = 0x0008;
const static uint16_t module_state_boot     = 0x0010;
const static uint16_t module_state_error    = 0x8000;

typedef uint16_t module_state_t;

#define GEN_STATE(from, to) \
    ((uint32_t)from << 16 | to)

#define DEFINE_STATE(from, to) \
    const static uint32_t from ## _2_ ## to = GEN_STATE(module_state_ ## from, module_state_ ## to)

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

typedef uint32_t transition_t;

// -----------------------------------------------------------------------------------
// module symbols
// -----------------------------------------------------------------------------------

extern "C" const char *state_to_string(module_state_t state);
extern "C" module_state_t string_to_state(const char *state);

//! configures module
/*!
  \param name module name
  \param config configure file
  \return handle on success, NULL otherwise
 */
typedef MODULE_HANDLE (*mod_configure_t)(const char* name, const char* config);

//! unconfigure module
/*!
  \param hdl module handle
  \return success or failure
 */
typedef int (*mod_unconfigure_t)(MODULE_HANDLE hdl);

//! set module state machine to defined state
/*!
  \param hdl module handle
  \param state requested state
  \return success or failure
 */
typedef int (*mod_set_state_t)(MODULE_HANDLE hdl, module_state_t state);

//! get module state machine state
/*!
  \param hdl module handle
  \return current state
 */
typedef module_state_t (*mod_get_state_t)(MODULE_HANDLE hdl);

//! module tick callback
/*!
 * \param hdl module handle
 */
typedef void (*mod_tick_t)(MODULE_HANDLE hdl);

// CPLUSPLUS Base Class
#ifdef __cplusplus

#define HDL_2_MODCLASS(hdl, impl, modclass)                                         \
    struct impl ## _wrapper *wr =                                                   \
        reinterpret_cast<struct impl ## _wrapper *>(hdl);                           \
    std::shared_ptr<modclass> dev = wr->sp;                                         \
    if (!dev)                                                                       \
        throw string_util::str_exception("["#impl"] invalid module "                \
                "handle to <"#modclass" *>\n"); 

#define MODULE_DEF(impl, modclass)                                                  \
struct impl ## _wrapper {                                                           \
    std::shared_ptr<modclass> sp;                                                   \
};                                                                                  \
                                                                                    \
EXPORT_C void mod_tick(MODULE_HANDLE hdl) {                                         \
    HDL_2_MODCLASS(hdl, impl, modclass)                                             \
    return dev->tick();                                                             \
}                                                                                   \
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
        throw string_util::str_exception(                                           \
                "["#impl"] error allocating memory\n");                             \
    wr->sp = std::make_shared<modclass>(name, doc);                                 \
    wr->sp->init();                                                                 \
                                                                                    \
    return (MODULE_HANDLE)wr;                                                       \
}

namespace robotkernel {

class module_base : 
    public robotkernel::log_base,  
    public robotkernel::trigger_base
{
    private:
        module_base();                  //!< prevent default construction

    public:
        const std::string name;         //!< module instance name
        module_state_t state;           //!< actual module state
        std::mutex state_mtx;
        
        //! Module construction
        /*!
         * \param[in] impl     Name of the module.
         * \param[in] name     Instance name of the module.
         */
        module_base(const std::string& impl, const std::string& name, 
                const YAML::Node& node = YAML::Node());

        //! Module destruction
        virtual ~module_base() {};

        //! Get module instance name
        /*!
         * \return Module instance name.
         */
        std::string get_name() { return name; }

        //! Optional initiazation method
        /* 
         * usefull to call shared_from_this() at construction time
         */
        virtual void init() {};
    
//        //! Get robotkernel module
//        /*!
//         * \returns Shared pointer to our robotkernel module class.
//         */
//        robotkernel::sp_module_t get_module();

        //! Module trigger implementation.
        virtual void tick() {
            throw string_util::str_exception("[%s|%s] trigger not implemented!\n",
                    impl.c_str(), name.c_str());
        }

        //*********************************************
        // STATE MACHINE FUNCTIONS
        //*********************************************

        //! Set module state machine to defined state.
        /*!
         * \param[in] state     Requested state which will be tried to switch to.
         *
         * \return success or failure
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

        //! Get module state machine state.
        /*!
         * \return Current state.
         */
        virtual module_state_t get_state() { return state; }
};

typedef std::shared_ptr<module_base> sp_module_base_t;

}; // namespace robotkernel

#endif // __cplusplus

#endif // ROBOTKERNEL_MODULE_BASE_H

