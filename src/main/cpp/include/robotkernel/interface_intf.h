//! robotkernel module interface definition
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

#ifndef ROBOTKERNEL__INTERFACE_INTF_H
#define ROBOTKERNEL__INTERFACE_INTF_H

#include <unistd.h>
#include <stdint.h>
#include <list>
#include <stdio.h>

#define INTERFACE_HANDLE void*

//! interface register
/*!
 * \param mod_name module name to register
 * \return interface handle
 */
typedef INTERFACE_HANDLE (*intf_register_t)();

//! interface unregister
/*!
 * \param hdl interface handle
 */
typedef int (*intf_unregister_t)(INTERFACE_HANDLE hdl);

#endif // ROBOTKERNEL__INTERFACE_INTF_H

