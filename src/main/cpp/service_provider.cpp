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

//! service_provider construction
/*!
  \param node configuration node
  */
service_provider::service_provider(const YAML::Node& node) : so_file(node) {
	name = get_as<string>(node, "name");

	sp_register      = (sp_register_t)  dlsym(so_handle, "sp_register");
	sp_unregister    = (sp_unregister_t)dlsym(so_handle, "sp_unregister");
	sp_add_slave     = (sp_add_slave_t)dlsym(so_handle, "sp_add_slave");
	sp_remove_slave  = (sp_remove_slave_t)dlsym(so_handle, "sp_remove_slave");
	sp_remove_module = (sp_remove_module_t)dlsym(so_handle, "sp_remove_module");
	sp_get_sp_magic = (sp_get_sp_magic_t)dlsym(so_handle, "sp_get_sp_magic");

	if (!sp_register)
		klog(verbose, "missing sp_register in %s\n", file_name.c_str());
	if (!sp_unregister)
		klog(verbose, "missing sp_unregister in %s\n", file_name.c_str());
	if (!sp_add_slave)
		klog(verbose, "missing sp_add_slave in %s\n", file_name.c_str());
	if (!sp_remove_slave)
		klog(verbose, "missing sp_remove_slave in %s\n", file_name.c_str());
	if (!sp_remove_module)
		klog(verbose, "missing sp_remove_module in %s\n", file_name.c_str());
	if (!sp_get_sp_magic)
		klog(verbose, "missing sp_get_sp_magic in %s\n", file_name.c_str());

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
 * \param mod_name slave owning module
 * \param dev_name name of device
 * \param slave_id id in module
 * \return slv_hdl slave handle
 */
void service_provider::add_slave(std::string mod_name, 
		std::string dev_name, int slave_id) {
    if (!sp_handle)
        throw str_exception("%s not configured!\n", name.c_str());

    if (!sp_add_slave) {
        klog(error, "%s error: no sp_add_slave function\n", name.c_str());
        return; 
    }

    sp_add_slave(sp_handle, mod_name.c_str(), dev_name.c_str(), slave_id);
}

//! remove registered slave
/*!
 * \param mod_name slave owning module
 * \param slave_id id in module
 */
void service_provider::remove_slave(std::string mod_name, int slave_id) {
    if (!sp_handle)
        throw str_exception("%s not configured!\n", name.c_str());

    if (!sp_remove_slave) {
        klog(error, "%s error: no sp_remove_slave function\n", name.c_str());
        return;
    }

    sp_remove_slave(sp_handle, mod_name.c_str(), slave_id);
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

//! service provider magic 
/*!
 * \return return service provider magic string
 */
std::string service_provider::get_sp_magic() {
    if (!sp_handle)
        throw str_exception("%s not configured!\n", name.c_str());

    if (!sp_get_sp_magic) {
        klog(error, "%s error: no sp_get_sp_magic function\n", name.c_str());
        return string("");
    }

    const char *magic = sp_get_sp_magic(sp_handle);
	return string(magic);
}

