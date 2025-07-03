//! robotkernel kernel base
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

// public headers
#include "robotkernel/robotkernel.h"

// private headers
#include "kernel.h"

//! add service to kernel
/*!
 * \param owner service owner
 * \param name service name
 * \param service_definition service definition
 * \param callback service callback
 */
void robotkernel::add_service(
        const std::string &owner,
        const std::string &name,
        const std::string &service_definition,
        service_callback_t callback)
{
    return robotkernel::kernel::instance.add_service(owner, name, service_definition, callback);
}

//! remove on service given by name
/*!
 * \param[in] owner     Owner of service.
 * \param[in] name      Name of service.
 */
void robotkernel::remove_service(
        const std::string& owner, 
        const std::string& name)
{
    return robotkernel::kernel::instance.remove_service(owner, name);
}

//! adds a device listener
/*
 * \param[in] dl    device listener to add. this device listener
 *                  will be notified whenever a new device is added.
 */
void robotkernel::add_device_listener(sp_device_listener_t dl)
{
    return robotkernel::kernel::instance.add_device_listener(dl);
}

//! remove a device listener
/*
 * \param[in] dl    device listener to reomve. this device listener
 *                  will no longer be notified when a new device is added.
 */
void robotkernel::remove_device_listener(sp_device_listener_t dl)
{
    return robotkernel::kernel::instance.remove_device_listener(dl);
}

//! add a named device
/*
 * \param req device to add
 */
void robotkernel::add_device(sp_device_t req) {
    return robotkernel::kernel::instance.add_device(req);
}

//! remove a named device
/*!
 * \param req device to remove
 */
void robotkernel::remove_device(sp_device_t req) {
    return robotkernel::kernel::instance.remove_device(req);
}
        
//! get a device by name
/*!
 * \param dev_name device name
 * \return device
 */
std::shared_ptr<robotkernel::device> robotkernel::get_device(const std::string& dev_name) {
    return robotkernel::kernel::instance.get_device<robotkernel::device>(dev_name);
}

