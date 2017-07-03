//! robotkernel service_provider definition
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
#include "robotkernel/service_provider.h"
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
using namespace string_util;

//! service_provider construction
/*!
  \param node configuration node
  */
service_provider::service_provider(const YAML::Node& node) : so_file(node) {
    name = get_as<string>(node, "name");

    sp_register         = (sp_register_t)  dlsym(so_handle, "sp_register");
    sp_unregister       = (sp_unregister_t)dlsym(so_handle, "sp_unregister");
    sp_add_interface    = (sp_add_interface_t)dlsym(so_handle, "sp_add_interface");
    sp_remove_interface = (sp_remove_interface_t)dlsym(so_handle, "sp_remove_interface");
    sp_remove_module    = (sp_remove_module_t)dlsym(so_handle, "sp_remove_module");
    sp_test_interface   = (sp_test_interface_t)dlsym(so_handle, "sp_test_interface");

    if (!sp_register)
        klog(verbose, "missing sp_register in %s\n", file_name.c_str());
    if (!sp_unregister)
        klog(verbose, "missing sp_unregister in %s\n", file_name.c_str());
    if (!sp_add_interface)
        klog(verbose, "missing sp_add_interface in %s\n", file_name.c_str());
    if (!sp_remove_interface)
        klog(verbose, "missing sp_remove_interface in %s\n", file_name.c_str());
    if (!sp_remove_module)
        klog(verbose, "missing sp_remove_module in %s\n", file_name.c_str());
    if (!sp_test_interface)
        klog(verbose, "missing sp_test_interface in %s\n", file_name.c_str());

    // try to configure
    if (sp_register) {
        sp_handle = sp_register();
    }
}

//! service_provider destruction
/*!
  destroys service_provider
  */
service_provider::~service_provider() {
    klog(verbose, "service_provider destructing %s\n", file_name.c_str());

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
        throw str_exception("%s not configured!\n", name.c_str());

    if (!sp_add_interface) {
        klog(error, "%s error: no sp_add_interface function\n", name.c_str());
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
        throw str_exception("%s not configured!\n", name.c_str());

    if (!sp_remove_interface) {
        klog(error, "%s error: no sp_remove_interface function\n", name.c_str());
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
        throw str_exception("%s not configured!\n", name.c_str());

    if (!sp_remove_module) {
        klog(error, "%s error: no sp_remove_module function\n", name.c_str());
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
        throw str_exception("%s not configured!\n", name.c_str());

    if (!sp_test_interface) {
        throw str_exception("%s error: no sp_test_interface\n", name.c_str());
    }

    return sp_test_interface(sp_handle, req);
}

