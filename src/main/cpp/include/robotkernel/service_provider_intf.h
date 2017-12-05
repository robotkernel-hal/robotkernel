//! robotkernel module service_provider definition
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

#ifndef ROBOTKERNEL__SERVICE_PROVIDER_INTF_H
#define ROBOTKERNEL__SERVICE_PROVIDER_INTF_H

#include <unistd.h>
#include <stdint.h>
#include <list>
#include <stdio.h>

#include "robotkernel/service_interface.h"

#define SERVICE_PROVIDER_HANDLE void*

//! service_provider register
/*!
 * \param mod_name module name to register
 * \return service_provider handle
 */
typedef SERVICE_PROVIDER_HANDLE(*sp_register_t)();

//! service_provider unregister
/*!
 * \param hdl service_provider handle
 * \return success
 */
typedef int (*sp_unregister_t)(SERVICE_PROVIDER_HANDLE hdl);

//! test service requester
/*!
 * \return true if we can handle service requester
 */
typedef bool (*sp_test_interface_t)(SERVICE_PROVIDER_HANDLE hdl,
        robotkernel::sp_service_interface_t req);

//! add slave
/*!
 * \param hdl service provider handle
 * \param req slave inteface specialization         
 */
typedef void (*sp_add_interface_t)(SERVICE_PROVIDER_HANDLE hdl, 
        robotkernel::sp_service_interface_t req);

//! remove registered slave
/*!
 * \param hdl service provider handle
 * \param req slave inteface specialization         
 * \param slave_id id in module
 */
typedef void (*sp_remove_interface_t)(SERVICE_PROVIDER_HANDLE hdl, 
        robotkernel::sp_service_interface_t req);

//! remove module
/*!
 * \param hdl service provider handle
 * \param mod_name module owning slaves
 */
typedef void (*sp_remove_module_t)(SERVICE_PROVIDER_HANDLE hdl, 
        const char *mod_name);

//! service provider magic 
/*!
 * \return return service provider magic string
 */
typedef const char* (*sp_get_sp_magic_t)(SERVICE_PROVIDER_HANDLE hdl);

#endif // ROBOTKERNEL__SERVICE_PROVIDER_INTF_H

