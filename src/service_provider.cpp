//! robotkernel service_provider definition
/*!
 * (C) Robert Burger <robert.burger@dlr.de>
 */

// vim: set expandtab softtabstop=4 shiftwidth=4
// -*- mode: c++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil; -*- 

/*
 * This file is part of robotkernel.
 *
 * robotkernel is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 * 
 * robotkernel is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with robotkernel; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

// public headers
#include "robotkernel/exceptions.h"
#include "robotkernel/helpers.h"

// private headers
#include "kernel.h"
#include "service_provider.h"

#include <sys/stat.h>
#include <dlfcn.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <robotkernel/service_provider_base.h>

using namespace std;
using namespace robotkernel;
using namespace robotkernel::helpers;

//! service_provider construction
/*!
  \param node configuration node
  */
service_provider::service_provider(const YAML::Node& node) : so_file(node) {
    name = get_as<string>(node, "name");

    get_symbol(sp_register, so_handle, "sp_register");
    get_symbol(sp_unregister, so_handle, "sp_unregister");
    get_symbol(sp_add_interface, so_handle, "sp_add_interface");
    get_symbol(sp_remove_interface, so_handle, "sp_remove_interface");
    get_symbol(sp_remove_module, so_handle, "sp_remove_module");
    get_symbol(sp_test_interface, so_handle, "sp_test_interface");

    // try to configure
    if (sp_register) {
        sp_handle = sp_register(name.c_str());
    }
}

//! service_provider destruction
/*!
  destroys service_provider
  */
service_provider::~service_provider() {
    robotkernel::kernel::instance.log(verbose, "service_provider destructing %s\n", file_name.c_str());

    // unconfigure service_provider first
    if (sp_handle && sp_unregister) {
        sp_unregister(sp_handle);
        sp_handle = NULL;
    }
}

//! add slave
/*!
 * \param req slave inteface specialization         
 */
void service_provider::add_interface(sp_service_interface_t req) {
    if (!sp_handle)
        throw runtime_error(string_printf("%s not configured!\n", name.c_str()));

    if (!sp_add_interface) {
        robotkernel::kernel::instance.log(error, "%s error: no sp_add_interface function\n", name.c_str());
        return; 
    }

    if (test_interface(req))
        sp_add_interface(sp_handle, req);
}

//! remove registered slave
/*!
 * \param req slave inteface specialization         
 */
void service_provider::remove_interface(sp_service_interface_t req) {
    if (!sp_handle)
        throw runtime_error(string_printf("%s not configured!\n", name.c_str()));

    if (!sp_remove_interface) {
        robotkernel::kernel::instance.log(error, "%s error: no sp_remove_interface function\n", name.c_str());
        return;
    }

    sp_remove_interface(sp_handle, req);
}

//! remove all slaves from module
/*!
 * \param mod_name module owning slaves
 */
void service_provider::remove_module(std::string mod_name) {
    if (!sp_handle)
        throw runtime_error(string_printf("%s not configured!\n", name.c_str()));

    if (!sp_remove_module) {
        robotkernel::kernel::instance.log(error, "%s error: no sp_remove_module function\n", name.c_str());
        return;
    }

    sp_remove_module(sp_handle, mod_name.c_str());
}

//! test slave service requester 
/*!
 * \param return true if we can handle it
 */
bool service_provider::test_interface(sp_service_interface_t req) {
    if (!sp_handle)
        throw runtime_error(string_printf("%s not configured!\n", name.c_str()));

    if (!sp_test_interface) {
        throw runtime_error(string_printf("%s error: no sp_test_interface\n", name.c_str()));
    }

    return sp_test_interface(sp_handle, req);
}

