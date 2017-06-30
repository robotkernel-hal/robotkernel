//! robotkernel module bridge definition
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

#ifndef __BRIDGE_INTF_H__
#define __BRIDGE_INTF_H__

#include <unistd.h>
#include <stdint.h>
#include <list>
#include <stdio.h>

#include <robotkernel/service.h>

#define BRIDGE_HANDLE void*

//! bridge configure
/*!
 * \param name bridge name
 * \param config bridge config
 * \return bridge handle
 */
typedef BRIDGE_HANDLE (*bridge_configure_t)(const char* name, const char* config);

//! bridge unconfigure
/*!
 * \param hdl bridge handle
 */
typedef int (*bridge_unconfigure_t)(BRIDGE_HANDLE hdl);

//! create and register ln service
/*!
 * \param svc robotkernel service struct
 */
typedef void (*bridge_add_service_t)(BRIDGE_HANDLE hdl, const robotkernel::service_t &svc);

//! unregister and remove ln service 
/*!
 * \param svc robotkernel service struct
 */
typedef void (*bridge_remove_service_t)(BRIDGE_HANDLE hdl, const robotkernel::service_t &svc);

#endif // __bridge_bridge_H__

