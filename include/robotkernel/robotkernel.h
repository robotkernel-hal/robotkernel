//! robotkernel module base
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

#ifndef ROBOTKERNEL_KERNEL_BASE_H
#define ROBOTKERNEL_KERNEL_BASE_H

#include "robotkernel/device.h"
#include "robotkernel/device_listener.h"
#include "robotkernel/service.h"
#include "robotkernel/helpers.h"

namespace robotkernel {

//! add service to kernel
/*!
 * \param owner service owner
 * \param name service name
 * \param service_definition service definition
 * \param callback service callback
 */
extern void add_service(
        const std::string &owner,
        const std::string &name,
        const std::string &service_definition,
        service_callback_t callback);

//! remove on service given by name
/*!
 * \param[in] owner     Owner of service.
 * \param[in] name      Name of service.
 */
extern void remove_service(
        const std::string& owner, 
        const std::string& name);

//! adds a device listener
/*
 * \param[in] dl    device listener to add. this device listener
 *                  will be notified whenever a new device is added.
 */
extern void add_device_listener(sp_device_listener_t dl);

//! remove a device listener
/*
 * \param[in] dl    device listener to reomve. this device listener
 *                  will no longer be notified when a new device is added.
 */
extern void remove_device_listener(sp_device_listener_t dl);

//! add a named device
/*
 * \param req device to add
 */
extern void add_device(sp_device_t req);

//! remove a named device
/*!
 * \param req device to remove
 */
extern void remove_device(sp_device_t req);

//! get a device by name
/*!
 * \param dev_name device name
 * \return device
 */
extern std::shared_ptr<device> get_device(const std::string& dev_name);

//! get a device by name
/*!
 * \param dev_name device name
 * \return device
 */
template <typename T>
inline std::shared_ptr<T> get_device(const std::string& dev_name) {
    std::shared_ptr<T> retval = std::dynamic_pointer_cast<T>(get_device(dev_name));
    if (!retval)
        throw std::runtime_error(robotkernel::string_printf("device %s is not of type %s\n", 
                dev_name.c_str(), typeid(T).name()));

    return retval;
};

}; // namespace robotkernel;

#endif // ROBOTKERNEL_KERNEL_BASE_H


