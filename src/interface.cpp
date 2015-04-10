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

using namespace std;
using namespace robotkernel;

//! interface construction
/*!
  \param mod_name name of shared object to load
  \param interface_file filename of interface
  \param config interface configuration string
  \param trigger configuration
  */
interface::interface(const std::string& interface_file, 
        const std::string& mod_name, const std::string& dev_name, const int& offset) :
    interface_file(interface_file), mod_name(mod_name), dev_name(dev_name), 
    offset(offset), intf_handle(NULL) { 
    
    struct stat buf;
    if (stat(interface_file.c_str(), &buf) != 0) {
        bool found = false;

        // try enviroment search path
        char *intfpathes = getenv("ROBOTKERNEL_INTERFACE_PATH");
        if (intfpathes) {
            char *pch, *str = strdup(intfpathes);
            pch = strtok(str, ":");
            while (pch != NULL) {
                string mod = string(pch) + "/" + interface_file;
                if (stat(mod.c_str(), &buf) == 0) {
                    this->interface_file = mod;
                    found = true;
                    break;
                }
                pch = strtok(NULL, ":");
            }

            free(str);
        }

        if (!found) {
            // try path from robotkernel release
            string mod = kernel::get_instance()->_internal_intfpath + "/" + interface_file;
            if (stat(mod.c_str(), &buf) == 0) {
                this->interface_file = mod;
                found = true;
            }
        }
    }

    klog(interface_info, "[interface] loading interface %s\n", this->interface_file.c_str());

    if (access(this->interface_file.c_str(), R_OK) != 0) {
#ifdef __VXWORKS__
        // special case on vxworks, because it may return ENOTSUP, the we try to open anyway !
#else
        klog(interface_error, "[interface] interface file '%s' not readable!\n", this->interface_file.c_str());
        throw str_exception("[interface] access '%s' signaled error: %s", this->interface_file.c_str(), strerror(errno));
#endif
    }

    so_handle = dlopen(this->interface_file.c_str(), 
            RTLD_GLOBAL |
            RTLD_NOW
            );

    if (!so_handle)
        throw str_exception("[interface] dlopen signaled error: %s", strerror(errno));

    intf_register   = (intf_register_t)  dlsym(so_handle, "intf_register");
    intf_unregister = (intf_unregister_t)dlsym(so_handle, "intf_unregister");

    if (!intf_register)
        klog(interface_verbose, "[interface] missing intf_register in %s\n", this->interface_file.c_str());;
    if (!intf_unregister)
        klog(interface_verbose, "[interface] missing intf_unregister in %s\n", this->interface_file.c_str());

    // try to configure
    if (intf_register)
        intf_handle = intf_register(mod_name.c_str(), dev_name.c_str(), offset);
}

//! interface destruction
/*!
  destroys interface
  */
interface::~interface() {
    klog(interface_info, "[interface] destructing %s\n", interface_file.c_str());

    // unconfigure interface first
    if (intf_handle && intf_unregister) {
        intf_unregister(intf_handle);
        intf_handle = NULL;
    }

    if (so_handle && !kernel::get_instance()->_do_not_unload_modules) {
        klog(interface_info, "[interface] unloading interface %s\n", interface_file.c_str());
        if (dlclose(so_handle) != 0)
            klog(interface_error, "[interface] error on unloading interface %s\n", interface_file.c_str());
        else
            so_handle = NULL;
    }
}

