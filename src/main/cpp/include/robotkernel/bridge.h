//! robotkernel bridge class
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

#ifndef __ROBOTKERNEL_BRIDGE_H__
#define __ROBOTKERNEL_BRIDGE_H__

#include <string>
#include <stdio.h>
#include "robotkernel/bridge_intf.h"
#include "yaml-cpp/yaml.h"

namespace robotkernel {

//! bridge class
/*!
  This class opens a shared bridge and loads all needed symbols
 */
class bridge {
    private:
        bridge();
        bridge(const bridge&);             // prevent copy-construction
        bridge& operator=(const bridge&);  // prevent assignment

    public:
        //! bridge construction
        /*!
          \param bridge_file filename of bridge
          \param node configuration node
          */
        bridge(const std::string& bridge_file, const YAML::Node& node);

        //! bridge destruction
        /*!
          destroys bridge
          */
        ~bridge();

        std::string bridge_file;        //! bridge shared object file name
    
    private:
        void* so_handle;                //! dlopen handle
        BRIDGE_HANDLE intf_handle;   //! bridge handle

        //! bridge symbols
        intf_register_t   intf_register;
        intf_unregister_t intf_unregister;
};

} // namespace robotkernel

#endif // __ROBOTKERNEL_BRIDGE_H__

