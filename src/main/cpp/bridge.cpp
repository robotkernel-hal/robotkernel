//! robotkernel bridge definition
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
#include "robotkernel/bridge.h"
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

//! bridge construction
/*!
  \param bridge_file filename of bridge
  \param node configuration node
  */
bridge::bridge(const std::string& fn, const YAML::Node& node)
    : bridge_file(fn)
{
    struct stat buf;
    
    // check if filename is absolute and exists
    bool found = (stat(bridge_file.c_str(), &buf) == 0);

    if (!found) {
        // try enviroment search path
        char *intfpathes = getenv("ROBOTKERNEL_LIBRARY_PATH");
        if (intfpathes) {
            char *pch, *str = strdup(intfpathes);
            pch = strtok(str, ":");
            while (pch != NULL) {
                string mod = string(pch) + "/" + bridge_file;
                if (stat(mod.c_str(), &buf) == 0) {
                    bridge_file = mod;
                    found = true;
                    break;
                }
                pch = strtok(NULL, ":");
            }

            free(str);
        }
    }

    if(!found)
        klog(warning, "bridge file %s not found (checked ROBOTKERNEL_BRIDGE_PATH and internal bridge path)\nwill try to dlopen() it nevertheless!!\n", 
            bridge_file.c_str());

    assert(bridge_file != "");
    
#ifdef __VXWORKS__
    klog(info, "loading bridge %s\n", 
            bridge_file.c_str());

    so_handle = dlopen(bridge_file.c_str(), 
            RTLD_GLOBAL | RTLD_NOW);
#else
    if ((so_handle = dlopen(bridge_file.c_str(), 
                    RTLD_GLOBAL | RTLD_NOW | RTLD_NOLOAD)) == NULL) {
        klog(info, "loading bridge %s\n", 
                bridge_file.c_str());

        if (access(bridge_file.c_str(), R_OK) != 0) {
            klog(error, "bridge file '%s' not readable!\n", 
                    bridge_file.c_str());
            throw str_exception("access '%s' signaled error: %s", 
                    bridge_file.c_str(), strerror(errno));
        }

        so_handle = dlopen(bridge_file.c_str(), 
                RTLD_GLOBAL | RTLD_NOW);
    }
#endif

    if (!so_handle)
        throw str_exception("dlopen signaled error: %s", strerror(errno));

    intf_register   = (intf_register_t)  dlsym(so_handle, "intf_register");
    intf_unregister = (intf_unregister_t)dlsym(so_handle, "intf_unregister");

    if (!intf_register)
        klog(verbose, "missing intf_register in %s\n", 
                bridge_file.c_str());;
    if (!intf_unregister)
        klog(verbose, "missing intf_unregister in %s\n", 
                bridge_file.c_str());

    // try to configure
    if (intf_register) {
        YAML::Emitter e;
        e << node;
        intf_handle = intf_register(e.c_str());
    }
}

//! bridge destruction
/*!
  destroys bridge
  */
bridge::~bridge() {
    klog(verbose, "bridge destructing %s\n", bridge_file.c_str());

    // unconfigure bridge first
    if (intf_handle && intf_unregister) {
        intf_unregister(intf_handle);
        intf_handle = NULL;
    }

    if (so_handle && !kernel::get_instance()->_do_not_unload_modules) {
        klog(verbose, "unloading bridge %s\n", 
                bridge_file.c_str());

        if (dlclose(so_handle) != 0)
            klog(error, "error on unloading bridge %s\n", 
                    bridge_file.c_str());
        else
            so_handle = NULL;
    }
}

