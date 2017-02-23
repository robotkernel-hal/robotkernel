//! robotkernel interface class
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

#ifndef __INTERFACE_H__
#define __INTERFACE_H__

#include <string>
#include <stdio.h>
#include "robotkernel/interface_intf.h"
#include "yaml-cpp/yaml.h"

namespace robotkernel {

//! interface class
/*!
  This class opens a shared interface and loads all needed symbols
 */
class interface {
    private:
        interface();
        interface(const interface&);             // prevent copy-construction
        interface& operator=(const interface&);  // prevent assignment

    public:
        //! interface construction
        /*!
          \param interface_file filename of interface
          \param node configuration node
          */
        interface(const std::string& interface_file, const YAML::Node& node, void* sp_interface);

        //! interface destruction
        /*!
          destroys interface
          */
        ~interface();

        std::string interface_file;     //! interface shared object file name
        std::string mod_name;           //! module name
        std::string dev_name;           //! device name
        int offset;                     //! module offset
    
    private:
        void* so_handle;                //! dlopen handle
        INTERFACE_HANDLE intf_handle;   //! interface handle

        //! interface symbols
        intf_register_t   intf_register;
        intf_unregister_t intf_unregister;
};

} // namespace robotkernel

#endif // __INTERFACE_H__

