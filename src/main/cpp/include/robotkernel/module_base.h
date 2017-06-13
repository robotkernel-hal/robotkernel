//! robotkernel module base
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

#ifndef __ROBOTKERNEL_MODULE_BASE_H__
#define __ROBOTKERNEL_MODULE_BASE_H__

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

#define HDL_2_MODCLASS(hdl, instance_name, modclass)                                \
    struct instance_name ## _wrapper *wr =                                          \
        reinterpret_cast<struct instance_name ## _wrapper *>(hdl);                  \
    std::shared_ptr<modclass> dev = wr->sp;                                         \
    if (!dev)                                                                       \
        throw string_util::str_exception("["#instance_name"] invalid module "       \
                "handle to <"#modclass" *>\n"); 

#define MODULE_DEF(instance_name, modclass)                                         \
struct instance_name ## _wrapper {                                                  \
    std::shared_ptr<modclass> sp;                                                   \
};                                                                                  \
                                                                                    \
EXPORT_C size_t mod_read(MODULE_HANDLE hdl, void* buf, size_t bufsize) {            \
    HDL_2_MODCLASS(hdl, instance_name, modclass)                                    \
    return dev->read(buf, bufsize);                                                 \
}                                                                                   \
                                                                                    \
EXPORT_C size_t mod_write(MODULE_HANDLE hdl, void* buf, size_t bufsize) {           \
    HDL_2_MODCLASS(hdl, instance_name, modclass)                                    \
    return dev->write(buf, bufsize);                                                \
}                                                                                   \
                                                                                    \
EXPORT_C void mod_trigger(MODULE_HANDLE hdl) {                                      \
    HDL_2_MODCLASS(hdl, instance_name, modclass)                                    \
    return dev->trigger();                                                          \
}                                                                                   \
                                                                                    \
EXPORT_C void mod_trigger_slave_id(MODULE_HANDLE hdl, uint32_t slave_id) {          \
    HDL_2_MODCLASS(hdl, instance_name, modclass)                                    \
    return dev->trigger_slave_id(slave_id);                                         \
}                                                                                   \
                                                                                    \
EXPORT_C size_t mod_set_state(MODULE_HANDLE hdl, module_state_t state) {            \
    HDL_2_MODCLASS(hdl, instance_name, modclass)                                    \
    return dev->set_state(state);                                                   \
}                                                                                   \
                                                                                    \
EXPORT_C module_state_t mod_get_state(MODULE_HANDLE hdl) {                          \
    HDL_2_MODCLASS(hdl, instance_name, modclass)                                    \
    return dev->get_state();                                                        \
}                                                                                   \
                                                                                    \
EXPORT_C int mod_unconfigure(MODULE_HANDLE hdl) {                                   \
    HDL_2_MODCLASS(hdl, instance_name, modclass)                                    \
    delete wr;                                                                      \
    return 0;                                                                       \
}                                                                                   \
                                                                                    \
EXPORT_C MODULE_HANDLE mod_configure(const char* name, const char* config) {        \
    struct instance_name ## _wrapper *wr;                                           \
    YAML::Node doc = YAML::Load(config);                                            \
                                                                                    \
    wr = new struct instance_name ## _wrapper();                                    \
    if (!wr)                                                                        \
        throw string_util::str_exception(                                           \
                "["#instance_name"] error allocating memory\n");                    \
    wr->sp = std::make_shared<modclass>(name, doc);                                 \
                                                                                    \
    return (MODULE_HANDLE)wr;                                                       \
}

namespace robotkernel {

class module_base : 
    public log_base//,  
//    public std::enable_shared_from_this<module_base>
{
    private:
        module_base();          //!< prevent default construction

    public:
        module_state_t state;       //!< actual module state
        
        //! construction
        /*!
         * \param instance_name module name
         * \param name instance name
         */
        module_base(const std::string& instance_name, const std::string& name, 
                const YAML::Node& node = YAML::Node());

        virtual ~module_base();

        std::string get_name() { return name; }
    
        //! get robotkernel module
        /*!
         * \returns shared pointer to our robotkernel module class
         */
        robotkernel::sp_module_t get_module();

        //! cyclic process data read
        /*!
         * \param buf process data buffer
         * \param bufsize size of process data buffer
         * \return size of read bytes
         */
        virtual size_t read(void* buf, size_t bufsize) {
            throw string_util::str_exception("[%s|%s] read not implemented!\n", 
                    instance_name.c_str(), name.c_str());
        }
        
        //! cyclic process data write
        /*!
         * \param buf process data buffer
         * \param bufsize size of process data buffer
         * \return size of written bytes
         */
        virtual size_t write(void* buf, size_t bufsize) {
            throw string_util::str_exception("[%s|%s] write not implemented!\n",
                    instance_name.c_str(), name.c_str());
        }
        
        //! module trigger callback
        virtual void trigger() {
            throw string_util::str_exception("[%s|%s] trigger not implemented!\n",
                    instance_name.c_str(), name.c_str());
        }

        //! module trigger callback
        /*!
         * \param slave_id slave id to trigger
         */
        virtual void trigger_slave_id(uint32_t slave_id) {
            throw string_util::str_exception("[%s|%s] trigger slave_id not implemented!\n",
                    instance_name.c_str(), name.c_str());
        }

        //! set module state machine to defined state
        /*!
         * \param state requested state
         * \return success or failure
         */
        virtual int set_state(module_state_t state) {
            throw string_util::str_exception("[%s|%s] set_state not implemented!\n",
                    instance_name.c_str(), name.c_str());
        }

        //! get module state machine state
        /*!
         * \return current state
         */
        virtual module_state_t get_state() {
            return state;
        }

};

};

#endif // __ROBOTKERNEL_MODULE_BASE_H__

