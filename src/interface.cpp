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
  \param interface_file filename of interface
  \param node configuration node
  */
interface::interface(const std::string& interface_file, const YAML::Node& node) {
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

#ifdef __VXWORKS__
    klog(info, "loading interface %s\n", 
            this->interface_file.c_str());

    so_handle = dlopen(this->interface_file.c_str(), 
            RTLD_GLOBAL | RTLD_NOW);
#else
    if ((so_handle = dlopen(this->interface_file.c_str(), 
                    RTLD_GLOBAL | RTLD_NOW | RTLD_NOLOAD)) == NULL) {
        klog(info, "loading interface %s\n", 
                this->interface_file.c_str());

        if (access(this->interface_file.c_str(), R_OK) != 0) {
            klog(error, "interface file '%s' not readable!\n", 
                    this->interface_file.c_str());
            throw str_exception("access '%s' signaled error: %s", 
                    this->interface_file.c_str(), strerror(errno));
        }

        so_handle = dlopen(this->interface_file.c_str(), 
                RTLD_GLOBAL | RTLD_NOW);
    }
#endif

    if (!so_handle)
        throw str_exception("dlopen signaled error: %s", strerror(errno));

    intf_register   = (intf_register_t)  dlsym(so_handle, "intf_register");
    intf_unregister = (intf_unregister_t)dlsym(so_handle, "intf_unregister");

    if (!intf_register)
        klog(verbose, "missing intf_register in %s\n", 
                this->interface_file.c_str());;
    if (!intf_unregister)
        klog(verbose, "missing intf_unregister in %s\n", 
                this->interface_file.c_str());

    // try to configure
    if (intf_register) {
        YAML::Emitter e;
        e << node;
        intf_handle = intf_register(e.c_str());
    }
}

//! interface destruction
/*!
  destroys interface
  */
interface::~interface() {
    klog(verbose, "interface destructing %s\n", interface_file.c_str());

    // unconfigure interface first
    if (intf_handle && intf_unregister) {
        intf_unregister(intf_handle);
        intf_handle = NULL;
    }

    if (so_handle && !kernel::get_instance()->_do_not_unload_modules) {
        klog(verbose, "unloading interface %s\n", 
                interface_file.c_str());

        if (dlclose(so_handle) != 0)
            klog(error, "error on unloading interface %s\n", 
                    interface_file.c_str());
        else
            so_handle = NULL;
    }
}

