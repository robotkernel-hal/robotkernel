//! robotkernel bridge class
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
 * along with robotkernel. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ROBOTKERNEL_BRIDGE_H
#define ROBOTKERNEL_BRIDGE_H

#include <string>
#include <stdio.h>
#include "robotkernel/bridge_intf.h"
#include "robotkernel/so_file.h"
#include "robotkernel/service.h"
#include "yaml-cpp/yaml.h"

namespace robotkernel {
#ifdef EMACS
}
#endif

//! bridge class
/*!
  This class opens a shared bridge and loads all needed symbols
  */
class bridge : public so_file {
    private:
        bridge();
        bridge(const bridge&);              // prevent copy-construction
        bridge& operator=(const bridge&);   // prevent assignment

    public:
        //! bridge construction
        /*!
         * \param[in] node  YAML node with bridge configuration.
         */
        bridge(const YAML::Node& node);

        //! bridge destruction
        /*!
         * destroys bridge
         */
        ~bridge();
        
        //! create and register ln service
        /*!
         * \param[in] svc   Service which should be added to bridge.
         */
        void add_service(const robotkernel::service_t &svc);

        //! unregister and remove ln service 
        /*!
         * \param[in] svc   Service which should be removed from bridge.
         */
        void remove_service(const robotkernel::service_t &svc);

        std::string name;                                   //!< bridge name

    private:
        BRIDGE_HANDLE bridge_handle;                        //!< bridge handle

        //! bridge symbols
        bridge_configure_t bridge_configure;                //!< configure bridge
        bridge_unconfigure_t bridge_unconfigure;            //!< unconfigure bridge
        bridge_add_service_t bridge_add_service;            //!< add service to bridge
        bridge_remove_service_t bridge_remove_service;      //!< remove service to bridge
};

typedef std::shared_ptr<bridge> sp_bridge_t;
typedef std::map<std::string, sp_bridge_t> bridge_map_t;

#ifdef EMACS
{
#endif
} // namespace robotkernel

#endif // ROBOTKERNEL_BRIDGE_H

