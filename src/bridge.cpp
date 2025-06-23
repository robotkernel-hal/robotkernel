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

#include "robotkernel/exceptions.h"
#include "robotkernel/helpers.h"

#include "kernel.h"
#include "bridge.h"

#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>

#include <string_util/string_util.h>

using namespace std;
using namespace robotkernel;
using namespace string_util;

//! bridge construction
/*!
 * \param file_name filename of bridge
 * \param node configuration node
 */
bridge::bridge(const YAML::Node& node) : so_file(node) 
{
    name = get_as<string>(node, "name");

    get_symbol(bridge_configure);
    get_symbol(bridge_unconfigure);
    get_symbol(bridge_add_service);
    get_symbol(bridge_remove_service);

    // try to configure
    bridge_handle = bridge_configure(name.c_str(), config.c_str());
}

//! bridge destruction
/*!
 * destroys bridge
 */
bridge::~bridge() {
    // unconfigure bridge first
    bridge_unconfigure(bridge_handle);
    bridge_handle = NULL;
}

// create and register ln service
void bridge::add_service(const robotkernel::service_t &svc) {
    bridge_add_service(bridge_handle, svc);
}

// unregister and remove ln service 
void bridge::remove_service(const robotkernel::service_t &svc) {
    bridge_remove_service(bridge_handle, svc);
}

