//! robotkernel module class
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

#ifndef ROBOTKERNEL__MODULE_H
#define ROBOTKERNEL__MODULE_H

#include <string>
#include <stdio.h>

// public headers
#include "robotkernel/service.h"
#include "robotkernel/module_base.h"
#include "robotkernel/trigger_base.h"

// private headers
#include "so_file.h"

#include <yaml-cpp/yaml.h>

namespace robotkernel { 
class module; 

module_state_t decode_power_up_state(std::string tmp_power_up);

};
YAML::Emitter& operator<<(YAML::Emitter& out, const robotkernel::module& mdl);

namespace robotkernel {
#ifdef EMACS
}
#endif

class kernel_worker;

//! module class
/*!
  This class opens a shared module and loads all needed symbols
 */
class module :
    public std::enable_shared_from_this<module>,
    public robotkernel::so_file,
    public robotkernel::trigger_base
{
    private:
        module();
        module(const module&);             // prevent copy-construction
        module& operator=(const module&);  // prevent assignment

    public:
        //! external module trigger
        class external_trigger {
            private:
                external_trigger();
                //! prevent copy-construction
                external_trigger(const external_trigger&);
                //! prevent assignment
                external_trigger& operator=(const external_trigger&);

            public:
                std::string dev_name;  //! name of trigger device
                int prio;              //! trigger priority
                int affinity_mask;     //! trigger affinity mask
                int divisor;           //! trigger divisor

                bool direct_mode;      //! direct or threaded
                int  direct_cnt;       //! direct mode counter
                module *direct_mdl;    //! direct mode module pointer

                //! generate new trigger object
                /*!
                 * \param node configuration node
                 */
                external_trigger(const YAML::Node& node);
        };

        typedef std::list<external_trigger *> trigger_list_t; //! trigger list
        typedef std::list<std::string> depend_list_t;         //! dependency list
        typedef std::list<std::string> exclude_list_t;         //! dependency list
        
        //! module construction
        /*!
         * \param node configuration node
         */
        module(const YAML::Node& node);

        //! module destruction
        /*!
          destroys module
          */
        ~module();

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

        //! trigger module
        /*!
        */
        void tick();

        std::string get_name();                         //!< return module name
        const depend_list_t& get_depends();             //!< return dependency list
        const module_state_t get_power_up();            //!< return power up state
        void add_depends(std::string other_module);     //!< add new dependency
        void remove_depends(std::string other_module);  //!< remove dependency

        //! set module state
        /*!
         * \param request service request data
         * \parma response service response data
         * \return success
         */
        int service_set_state(const service_arglist_t& request,
                service_arglist_t& response);
        static const std::string service_definition_set_state;

        //! get module state
        /*!
         * \param request service request data
         * \parma response service response data
         * \return success
         */
        int service_get_state(const service_arglist_t& request,
                service_arglist_t& response);
        static const std::string service_definition_get_state;

        //! get module config
        /*!
         * \param request service request data
         * \parma response service response data
         * \return success
         */
        int service_get_config(const service_arglist_t& request,
                service_arglist_t& response);
        static const std::string service_definition_get_config;

        void set_power_up(module_state_t power_up_state) {
            power_up = power_up_state;
        }
        
        const exclude_list_t& get_excludes();           //!< return exclude list
        void add_excludes(std::string other_module);    //!< add new exclude
        void remove_excludes(std::string other_module); //!< remove exclude

        friend YAML::Emitter& (::operator<<)(YAML::Emitter& out,
                const robotkernel::module& mdl);

        depend_list_t depends;          //! module dependecy list
        exclude_list_t excludes;        //! module exclude list
        trigger_list_t triggers;        //! module trigger list

    private:
        std::string name;               //! module name
        module_state_t power_up;        //! auto power up on startup

    private:
        void _init();

        MODULE_HANDLE mod_handle;       //! module handle

        //! module symbols
        mod_configure_t         mod_configure;
        mod_unconfigure_t       mod_unconfigure;
        mod_set_state_t         mod_set_state;
        mod_get_state_t         mod_get_state;
        mod_tick_t              mod_tick;
};

//! return module name
inline std::string module::get_name() {
    return name;
}

//! return dependency list
inline const module::depend_list_t& module::get_depends() {
    return depends;
}
//! add new dependency
inline void module::add_depends(std::string other_module) {
    depends.push_back(other_module);
}

inline void module::remove_depends(std::string other_module) {
    for(depend_list_t::iterator i = depends.begin();
            i != depends.end(); ) {

        if(*i == other_module) {
            depends.erase(i);
            i = depends.begin();
            continue;
        }

        ++i;
    }
}

//! return power up state
inline const module_state_t module::get_power_up() {
    return power_up;
}

//! return dependency list
inline const module::exclude_list_t& module::get_excludes() {
    return excludes;
}

//! add new exclude
inline void module::add_excludes(std::string other_module) {
    excludes.push_back(other_module);
}

inline void module::remove_excludes(std::string other_module) {
    for(exclude_list_t::iterator i = excludes.begin();
            i != excludes.end(); ) {

        if(*i == other_module) {
            excludes.erase(i);
            i = excludes.begin();
            continue;
        }

        ++i;
    }
}

typedef std::shared_ptr<module> sp_module_t;
typedef std::map<std::string, sp_module_t> module_map_t;

#ifdef EMACS
{
#endif
} // namespace robotkernel

#endif // ROBOTKERNEL__MODULE_H

