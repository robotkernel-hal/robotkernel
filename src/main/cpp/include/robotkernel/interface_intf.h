//! robotkernel module interface definition
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

#ifndef __INTERFACE_INTF_H__
#define __INTERFACE_INTF_H__

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

#endif // __INTERFACE_INTF_H__

