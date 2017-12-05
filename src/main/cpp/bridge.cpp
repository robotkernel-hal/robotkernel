//! robotkernel bridge definition
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

#include <string_util/string_util.h>

using namespace std;
using namespace robotkernel;
using namespace string_util;

//! bridge construction
/*!
 * \param file_name filename of bridge
 * \param node configuration node
 */
bridge::bridge(const YAML::Node& node) : so_file(node) {
    name = get_as<string>(node, "name");

    klog(info, "bridge constructing %s -> %s\n", 
            name.c_str(), file_name.c_str());

    bridge_configure      = (bridge_configure_t)     dlsym(so_handle, "bridge_configure");
    bridge_unconfigure    = (bridge_unconfigure_t)   dlsym(so_handle, "bridge_unconfigure");
    bridge_add_service    = (bridge_add_service_t)   dlsym(so_handle, "bridge_add_service");
    bridge_remove_service = (bridge_remove_service_t)dlsym(so_handle, "bridge_remove_service");

    if (!bridge_configure)
        klog(warning, "missing bridge_configure in %s\n", file_name.c_str());;
    if (!bridge_unconfigure)
        klog(warning, "missing bridge_unconfigure in %s\n", file_name.c_str());

    // try to configure
    if (bridge_configure) {
        bridge_handle = bridge_configure(name.c_str(), config.c_str());
    }
}

//! bridge destruction
/*!
 * destroys bridge
 */
bridge::~bridge() {
    klog(info, "bridge destructing %s\n", file_name.c_str());

    // unconfigure bridge first
    if (bridge_handle && bridge_unconfigure) {
        bridge_unconfigure(bridge_handle);
        bridge_handle = NULL;
    }
}

// create and register ln service
void bridge::add_service(const robotkernel::service_t &svc) {
    if (!bridge_handle)
        throw str_exception("%s not configured!\n", name.c_str());

    if (!bridge_add_service) {
        klog(error, "%s error: no bridge_add_service function\n", name.c_str());
        return;
    }

    bridge_add_service(bridge_handle, svc);
}

// unregister and remove ln service 
void bridge::remove_service(const robotkernel::service_t &svc) {
    if (!bridge_handle)
        throw str_exception("%s not configured!\n", name.c_str());

    if (!bridge_remove_service) {
        klog(error, "%s error: no bridge_remove_service function\n", name.c_str());
        return;
    }

    bridge_remove_service(bridge_handle, svc);
}

