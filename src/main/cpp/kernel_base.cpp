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
#include "robotkernel/kernel_base.h"

// private headers
#include "kernel.h"

using namespace robotkernel;

static kernel *_pkernel = kernel::get_instance();

//! add service to kernel
/*!
 * \param owner service owner
 * \param name service name
 * \param service_definition service definition
 * \param callback service callback
 */
void kernel_base::add_service(
        const std::string &owner,
        const std::string &name,
        const std::string &service_definition,
        service_callback_t callback)
{
    return _pkernel->add_service(owner, name, service_definition, callback);
}

//! remove on service given by name
/*!
 * \param[in] owner     Owner of service.
 * \param[in] name      Name of service.
 */
void kernel_base::remove_service(
        const std::string& owner, 
        const std::string& name)
{
    return _pkernel->remove_service(owner, name);
}

//! adds a device listener
/*
 * \param[in] dl    device listener to add. this device listener
 *                  will be notified whenever a new device is added.
 */
void kernel_base::add_device_listener(sp_device_listener_t dl)
{
    return _pkernel->add_device_listener(dl);
}

//! remove a device listener
/*
 * \param[in] dl    device listener to reomve. this device listener
 *                  will no longer be notified when a new device is added.
 */
void kernel_base::remove_device_listener(sp_device_listener_t dl)
{
    return _pkernel->remove_device_listener(dl);
}

//! add a named device
/*
 * \param req device to add
 */
void kernel_base::add_device(sp_device_t req) {
    return _pkernel->add_device(req);
}

//! remove a named device
/*!
 * \param req device to remove
 */
void kernel_base::remove_device(sp_device_t req) {
    return _pkernel->remove_device(req);
}
        
//! get a device by name
/*!
 * \param dev_name device name
 * \return device
 */
template <typename T>
std::shared_ptr<T> kernel_base::get_device(const std::string& dev_name) {
    return _pkernel->get_device<T>(dev_name);
}

