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

#include "robotkernel/module_intf.h"
#include "robotkernel/kernel.h"
#include "robotkernel/exceptions.h"
#include "robotkernel/helpers.h"
#include "robotkernel/log_base.h"
#include "yaml-cpp/yaml.h"

#ifdef __cplusplus
#define EXPORT_C extern "C" 
#else
#define EXPORT_C
#endif

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
#ifdef EMACS
}
#endif

class module_base : 
    public robotkernel::log_base,  
    public robotkernel::trigger_base
{
    private:
        module_base();                  //!< prevent default construction

    public:
        const std::string name;         //!< module instance name
        module_state_t state;           //!< actual module state
        
        //! Module construction
        /*!
         * \param[in] impl     Name of the module.
         * \param[in] name     Instance name of the module.
         */
        module_base(const std::string& impl, const std::string& name, 
                const YAML::Node& node = YAML::Node()) : 
            log_base(name, impl, "", node), name(name), state(module_state_init)
        { }

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
        void init() {};
    
        //! Get robotkernel module
        /*!
         * \returns Shared pointer to our robotkernel module class.
         */
        robotkernel::sp_module_t get_module() {
            kernel& k = *kernel::get_instance();
            return k.get_module(name);
        }

        //! Module trigger implementation.
        virtual void tick() {
            throw string_util::str_exception("[%s|%s] trigger not implemented!\n",
                    impl.c_str(), name.c_str());
        }

        //! Set module state machine to defined state.
        /*!
         * \param[in] state     Requested state which will be tried to switch to.
         *
         * \return success or failure
         */
        virtual int set_state(module_state_t state) {
            throw string_util::str_exception("[%s|%s] set_state not implemented!\n",
                    impl.c_str(), name.c_str());
        }

        //! Get module state machine state.
        /*!
         * \return Current state.
         */
        virtual module_state_t get_state() {
            return state;
        }

};

#ifdef EMACS
{
#endif
};

#endif // ROBOTKERNEL_MODULE_BASE_H

