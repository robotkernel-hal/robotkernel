//! robotkernel module class
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

#ifndef __MODULE_H__
#define __MODULE_H__

#include <string>
#include <stdio.h>
#include "ln_kernel_messages.h"
#include "module_intf.h"
#include "interface.h"
#include "yaml-cpp/yaml.h"

namespace robotkernel {

class kernel_worker;

//! module class
/*!
  This class opens a shared module and loads all needed symbols
 */
class module :
    public ln_service_get_config_base,
    public ln_service_get_feat_base {

    public:
        //! external module trigger
        class external_trigger {
            public:
                std::string _mod_name;  //! name of trigger module
                int _clk_id;            //! trigger module clock id
                int _prio;              //! trigger priority
                int _affinity_mask;     //! trigger affinity mask
                int _divisor;           //! trigger divisor

                bool _direct_mode;      //! direct or threaded
                int  _direct_cnt;       //! direct mode counter
                module *_direct_mdl;    //! direct mode module pointer

                //! generate new trigger object
                /*!
                 * \param node configuration node
                 */
                external_trigger(const YAML::Node& node);
        };

        typedef std::list<external_trigger *> trigger_list_t;     //! trigger list
        typedef std::list<std::string> depend_list_t;           //! dependency list
        typedef std::list<interface *> intf_list_t;        //! interface list

        //! module construction
        /*!
          \param mod_name name of shared object to load
          \param module_file filename of module
          \param config module configuration string
          \param trigger configuration
          */
        module(std::string mod_name, std::string module_file, std::string config,
               std::string trigger, std::string depends);

        //! module construction
        /*!
         * \param node configuration node
         */
        module(const YAML::Node& node, std::string config_path);

        //! module destruction
        /*!
          destroys module
          */
        ~module();

        //! register services
        void register_services();

        //! module reconfiguration
        /*!
          \param sConfig configuration file for module
          \return success or failure
          */
        bool reconfigure();

        //! Return if module if correctly configured
        /*!
          \return module handle is valid
          */
        bool configured() {
            return mod_handle != NULL;
        }

        //! Read process data from module
        /*!
          \param buf buffer to store cyclic data
          \param busize size of buffer
          \return size of read bytes
          */
        size_t read(char* buf, size_t bufsize);

        //! Write process data to module
        /*!
          \param buf buffer with new cyclic data
          \param busize size of buffer
          \return size of written bytes
          */
        size_t write(char* buf, size_t bufsize);

        //! Set module state
        /*!
          \param state new model state
          \return success or failure
          */
        int set_state(module_state_t state);

        //! Get module state
        /*!
          \return current module state
          */
        module_state_t get_state();

        //! Send a request to module
        /*!
          \param reqcode request code
          \param ptr pointer to request structure
          \return success or failure
          */
        int request(int reqcode, void* ptr);

        //! register trigger module
        /*!
         * \param mdl module to register
         */
        void trigger_register_module(module *mdl, external_trigger& t);

        //! unregister trigger module
        /*!
         * \param mdl module to register
         */
        void trigger_unregister_module(module *mdl, external_trigger& t);

        static void trigger_wrapper(void *ptr) {
            external_trigger *t = (external_trigger *)ptr;
        
            if ((++t->_direct_cnt%t->_divisor != 0))
                return;

            t->_direct_mdl->trigger();
        };

        //! trigger module
        /*!
        */
        void trigger();

        std::string name;               //! module name
        std::string module_file;        //! module shared object file name
        std::string config;             //! module config string
        std::string config_file_path;   //! module config file path
        bool power_up;                  //! auto power up on startup
        depend_list_t depends;          //! module dependecy list
        trigger_list_t triggers;        //! module trigger list
        intf_list_t interfaces;

        //! service callbacks
        int on_get_config(ln::service_request& req, 
                ln_service_robotkernel_module_get_config& svc);
        int on_get_feat(ln::service_request& req, 
                ln_service_robotkernel_module_get_feat& svc);

    private:
        void _init();

        void* so_handle;                //! dlopen handle
        MODULE_HANDLE mod_handle;       //! module handle

        struct worker_key {
            int prio;
            int affinity;
            int clk_id;
            int divisor;

            bool operator<(const worker_key& a) const;
        };

        //! kernel worker
        typedef std::map<worker_key, kernel_worker *> worker_map_t;
        worker_map_t _worker;

        //! module symbols
        mod_configure_t   mod_configure;
        mod_unconfigure_t mod_unconfigure;
        mod_read_t        mod_read;
        mod_write_t       mod_write;
        mod_set_state_t   mod_set_state;
        mod_get_state_t   mod_get_state;
        mod_request_t     mod_request;
        mod_trigger_t     mod_trigger;
};

} // namespace robotkernel

#endif // __MODULE_H__

