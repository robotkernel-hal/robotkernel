//! robotkernel interface definition
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

#include "robotkernel/kernel.h"
#include "robotkernel/interface.h"
#include "robotkernel/kernel_worker.h"
#include "robotkernel/exceptions.h"
#include "robotkernel/helpers.h"
#include <sys/stat.h>
#include <dlfcn.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <robotkernel/service_provider_base.h>

using namespace std;
using namespace robotkernel;

//! interface construction
/*!
  \param node configuration node
  */
interface::interface(const YAML::Node& node, void* sp_interface)
    : so_file(node) {
    intf_register   = (intf_register_t)  dlsym(so_handle, "intf_register");
    intf_unregister = (intf_unregister_t)dlsym(so_handle, "intf_unregister");

    if (!intf_register)
        klog(verbose, "missing intf_register in %s\n", file_name.c_str());
    if (!intf_unregister)
        klog(verbose, "missing intf_unregister in %s\n", file_name.c_str());

    // try to configure
    if (intf_register) {
        intf_handle = intf_register();
        ServiceProvider* sp = static_cast<ServiceProvider*>(intf_handle);

        if (!sp) {
            klog(warning, "passed object not of type ServiceProvider*\n");
        } else {
            sp->init(node, sp_interface);
        }
    }
}

//! interface destruction
/*!
  destroys interface
  */
interface::~interface() {
    klog(verbose, "interface destructing %s\n", file_name.c_str());

    // unconfigure interface first
    if (intf_handle && intf_unregister) {
        intf_unregister(intf_handle);
        intf_handle = NULL;
    }
}

