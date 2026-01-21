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

/**
 * @file robotkernel.h
 * @brief Central API for interacting with the robotkernel framework.
 *
 * This header aggregates core robotkernel components and provides
 * high‑level functions for managing:
 *   - services
 *   - devices
 *   - device listeners
 *
 * It also exposes global accessors for runtime names and paths.
 *
 * This file is part of robotkernel (LGPLv3 or later).
 */

#ifndef ROBOTKERNEL_KERNEL_BASE_H
#define ROBOTKERNEL_KERNEL_BASE_H

#include "robotkernel/device.h"
#include "robotkernel/device_listener.h"
#include "robotkernel/service.h"
#include "robotkernel/trigger.h"
#include "robotkernel/process_data.h"
#include "robotkernel/stream.h"
#include "robotkernel/helpers.h"

namespace robotkernel {

/**
 * @brief Add a service to the robotkernel runtime.
 *
 * Register a new service that can be called by name. Services are
 * identified by a tuple of `owner` and `name`. The service definition
 * and callback handle request/response behavior.
 *
 * @param owner
 *        Logical owner or module of the service.
 *
 * @param name
 *        Unique name for this service under the given owner.
 *
 * @param service_definition
 *        YAML or textual definition of the service interface.
 *
 * @param callback
 *        Function invoked when the service is called.
 */
extern void add_service(
    const std::string &owner,
    const std::string &name,
    const std::string &service_definition,
    service_callback_t callback);

/**
 * @brief Remove a previously registered service.
 *
 * Deregisters the named service for the given owner. After removal,
 * requests to this service will fail.
 *
 * @param owner
 *        Owner of the registered service.
 *
 * @param name
 *        Name of the service to remove.
 */
extern void remove_service(
    const std::string &owner,
    const std::string &name);

/**
 * @brief Register a device listener.
 *
 * Adds a listener that will receive notifications when devices
 * are added or removed in the system.
 *
 * @param dl
 *        Shared pointer to a listener instance.
 */
extern void add_device_listener(sp_device_listener_t dl);

/**
 * @brief Remove a device listener.
 *
 * Stops sending notifications to this listener on future device
 * events.
 *
 * @param dl
 *        Shared pointer to the listener to remove.
 */
extern void remove_device_listener(sp_device_listener_t dl);

/**
 * @brief Add a device to the global registry.
 *
 * @param req
 *        Shared pointer to a device instance.
 */
extern void add_device(sp_device_t req);

/**
 * @brief Remove a device from the global registry.
 *
 * @param req
 *        Shared pointer to the device to remove.
 */
extern void remove_device(sp_device_t req);

/**
 * @brief Retrieve a device by name.
 *
 * Returns a shared pointer to the device with the given name
 * or `nullptr` if no such device exists.
 *
 * @param dev_name
 *        Unique name of the device.
 *
 * @return
 *        Shared pointer to the found device or nullptr.
 */
extern std::shared_ptr<device> get_device(const std::string &dev_name);

/**
 * @brief Type‑safe device lookup helper.
 *
 * Templated helper that attempts to cast a generic device
 * pointer to a specific derived type (`T`). Throws if the
 * cast fails.
 *
 * @tparam T
 *        The expected device type.
 *
 * @param dev_name
 *        Name of the device to look up.
 *
 * @return
 *        Shared pointer to the device of type `T`.
 *
 * @throws std::runtime_error
 *        If the device is not of type `T` or does not exist.
 */
template<typename T>
inline std::shared_ptr<T> get_device(const std::string &dev_name)
{
    std::shared_ptr<T> retval =
        std::dynamic_pointer_cast<T>(get_device(dev_name));
    if (!retval) {
        throw std::runtime_error(robotkernel::helpers::string_printf(
            "device %s is not of type %s\n",
            dev_name.c_str(),
            typeid(T).name()));
    }
    return retval;
}

/**
 * @brief Get the configured robotkernel name.
 *
 * @return Identifier string for the running kernel.
 */
extern const std::string name(void);

/**
 * @brief Get the loaded configuration file name.
 *
 * @return YAML or config file name used at startup.
 */
extern const std::string config_file(void);

/**
 * @brief Get the path of the configuration file.
 *
 * @return Full filesystem path to the configuration file.
 */
extern const std::string config_file_path(void);

/**
 * @brief Get the path of the executable.
 *
 * @return Path to the running binary.
 */
extern const std::string exec_file_path(void);

} // namespace robotkernel

#endif // ROBOTKERNEL_KERNEL_BASE_H

