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
#include "yaml-cpp/yaml.h"

#ifdef __cplusplus
#define EXPORT_C extern "C" 
#else
#define EXPORT_C
#endif

#define HDL_2_MODCLASS(hdl, modname, modclass)                                      \
    modclass *dev = reinterpret_cast<modclass *>(hdl);                              \
    if (!dev)                                                                       \
        throw robotkernel::str_exception("["#modname"] invalid module "             \
                "handle to <"#modclass" *>\n"); 

#define MODULE_DEF(modname, modclass)                                               \
                                                                                    \
EXPORT_C size_t mod_read(MODULE_HANDLE hdl, void* buf, size_t bufsize) {            \
    HDL_2_MODCLASS(hdl, modname, modclass)                                          \
    return dev->read(buf, bufsize);                                                 \
}                                                                                   \
                                                                                    \
EXPORT_C size_t mod_write(MODULE_HANDLE hdl, void* buf, size_t bufsize) {           \
    HDL_2_MODCLASS(hdl, modname, modclass)                                          \
    return dev->write(buf, bufsize);                                                \
}                                                                                   \
                                                                                    \
EXPORT_C int mod_request(MODULE_HANDLE hdl, int reqcode, void *arg) {               \
    HDL_2_MODCLASS(hdl, modname, modclass)                                          \
    return dev->request(reqcode, arg);                                              \
}                                                                                   \
                                                                                    \
EXPORT_C void mod_trigger(MODULE_HANDLE hdl) {                                      \
    HDL_2_MODCLASS(hdl, modname, modclass)                                          \
    return dev->trigger();                                                          \
}                                                                                   \
                                                                                    \
EXPORT_C size_t mod_set_state(MODULE_HANDLE hdl, module_state_t state) {            \
    HDL_2_MODCLASS(hdl, modname, modclass)                                          \
    return dev->set_state(state);                                                   \
}                                                                                   \
                                                                                    \
EXPORT_C module_state_t mod_get_state(MODULE_HANDLE hdl) {                          \
    HDL_2_MODCLASS(hdl, modname, modclass)                                          \
    return dev->get_state();                                                        \
}                                                                                   \
                                                                                    \
EXPORT_C int mod_unconfigure(MODULE_HANDLE hdl) {                                   \
    HDL_2_MODCLASS(hdl, modname, modclass)                                          \
    delete dev;                                                                     \
    return 0;                                                                       \
}                                                                                   \
                                                                                    \
EXPORT_C MODULE_HANDLE mod_configure(const char* name, const char* config) {        \
    modclass *dev;                                                                  \
    std::stringstream stream(config);                                               \
    YAML::Parser parser(stream);                                                    \
    YAML::Node doc;                                                                 \
                                                                                    \
    if (!parser.GetNextDocument(doc))                                               \
        throw robotkernel::str_exception("["#modname"] error parsing config\n");    \
                                                                                    \
    dev = new modclass(name, doc);                                                  \
    if (!dev)                                                                       \
        throw robotkernel::str_exception("["#modname"] error allocating memory\n"); \
                                                                                    \
    return (MODULE_HANDLE)dev;                                                      \
}

namespace robotkernel {

class module_base {
    private:
        module_base();         //!< prevent default construction

    protected:
        std::string modname;     //!< module name
        std::string name;        //!< instance name
        module_state_t state;    //!< actual module state

        enum {
            module, kernel
        } logmode;

        int ll_bits;

    public:
        //! construction
        /*!
         * \param modname module name
         * \param name instance name
         */
        module_base(const std::string& modname, const std::string& name) {
            this->modname = modname;
            this->name = name;
            this->state = module_state_init;
            this->logmode = kernel;
            this->ll_bits = 0;
        }
        
        //! construction
        /*!
         * \param modname module name
         * \param name instance name
         */
        module_base(const std::string& modname, const std::string& name, 
                const YAML::Node& node) {
            this->modname = modname;
            this->name = name;
            this->state = module_state_init;
            this->logmode = kernel;
            this->ll_bits = 0;

            // search for loglevel
            const YAML::Node *ll_node = node.FindValue("loglevel");
            if (ll_node) {
                this->logmode = module;
                ll_bits = 0;

#define loglevel_if(ll, x) \
                if ((x) == std::string(#ll)) \
                ll_bits |= ( 1 << (ll-1));
#define loglevel_add(x)                 \
                loglevel_if(error, x)          \
                else loglevel_if(warning, x)   \
                else loglevel_if(info, x)      \
                else loglevel_if(verbose, x) \
                else loglevel_if(module_error, x)          \
                else loglevel_if(module_warning, x)   \
                else loglevel_if(module_info, x)      \
                else loglevel_if(module_verbose, x) \
                else loglevel_if(interface_error, x)          \
                else loglevel_if(interface_warning, x)   \
                else loglevel_if(interface_info, x)      \
                else loglevel_if(interface_verbose, x)

                if (ll_node->Type() == YAML::NodeType::Scalar) {
                    try {
                        // try to read mask directly
                        ll_bits = ll_node->to<int>();
                    } catch (YAML::Exception& e) {
                        loglevel_add(ll_node->to<std::string>());
                    }
                } else
                    for (YAML::Iterator it = ll_node->begin(); it != ll_node->end(); ++it) {
                        loglevel_add(it->to<std::string>());
                    }
#undef loglevel_if
#undef loglevel_add
            }
        }

        //! cyclic process data read
        /*!
         * \param buf process data buffer
         * \param bufsize size of process data buffer
         * \return size of read bytes
         */
        virtual size_t read(void* buf, size_t bufsize) {
            throw robotkernel::str_exception("[%s|%s] read not implemented!\n", 
                    modname.c_str(), name.c_str());
        }
        
        //! cyclic process data write
        /*!
         * \param buf process data buffer
         * \param bufsize size of process data buffer
         * \return size of written bytes
         */
        virtual size_t write(void* buf, size_t bufsize) {
            throw robotkernel::str_exception("[%s|%s] write not implemented!\n",
                    modname.c_str(), name.c_str());
        }
        
        //! module trigger callback
        virtual void trigger() {
            throw robotkernel::str_exception("[%s|%s] trigger not implemented!\n",
                    modname.c_str(), name.c_str());
        }

        //! module trigger callback
        /*!
         * \param slave_id slave id to trigger
         */
        virtual void trigger_slave_id(uint32_t slave_id) {
            throw robotkernel::str_exception("[%s|%s] trigger slave_id not implemented!\n",
                    modname.c_str(), name.c_str());
        }

        //! set module state machine to defined state
        /*!
         * \param state requested state
         * \return success or failure
         */
        virtual int set_state(module_state_t state) {
            throw robotkernel::str_exception("[%s|%s] set_state not implemented!\n",
                    modname.c_str(), name.c_str());
        }

        //! get module state machine state
        /*!
         * \return current state
         */
        virtual module_state_t get_state() {
            return state;
        }

        //! send a request to module
        /*!
         * \param reqcode request code
         * \param ptr pointer to request structure
         * \return success or failure
         */
        virtual int request(int reqcode, void* ptr) {
            throw robotkernel::str_exception("[%s|%s] request not implemented!\n",
                    modname.c_str(), name.c_str());
        }

        //! log to kernel logging facility
        void log(robotkernel::loglevel lvl, const char *format, ...);
};

};

#endif // __ROBOTKERNEL_MODULE_BASE_H__

