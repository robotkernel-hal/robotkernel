//! robotkernel device base class
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
 * @file device.h
 * @brief Base class for robotkernel devices.
 *
 * This header defines the abstract base class representing a
 * generic *device* in the robotkernel system.
 * Each device has a logical owner, a unique device name, and
 * an instance suffix; together these form a unique identifier.
 *
 * The class is lightweight and intended to be subclassed by
 * concrete device implementations in robotkernel modules.
 *
 * @note
 * - Instances of device should normally be managed with
 *   std::shared_ptr (alias `sp_device_t`).
 * - `device_map_t` provides a convenient map from ID to
 *   shared device pointers.
 */

#ifndef ROBOTKERNEL__DEVICE_H
#define ROBOTKERNEL__DEVICE_H

#include <map>
#include <memory>
#include <string>

namespace robotkernel {

/**
 * @brief Abstract base class for a generic robotkernel device.
 *
 * A device in robotkernel is identified by three components:
 *   - the logical `owner` (module or subsystem),
 *   - the `device_name` (type or class of the device),
 *   - the `suffix` (instance identifier).
 *
 * The unique ID string is formed as:
 *   <owner>.<device_name>.<suffix>
 *
 * Derived classes can add specific behavior or interfaces as
 * needed by the particular robotkernel subsystem.
 */
class device
{
    public:
        /// Logical owner of the device (e.g., subsystem name).
        const std::string owner;

        /// Device type or class name.
        const std::string device_name;

        /// Instance identifier (suffix) for the device.
        const std::string suffix;

    public:
        /**
         * @brief Construct a device base.
         *
         * @param owner
         *        Name of the logical owner or module.
         * @param device_name
         *        Device type or general name.
         * @param suffix
         *        Unique instance suffix for this device.
         */
        device(const std::string& owner,
                const std::string& device_name,
                const std::string& suffix)
            : owner(owner),
            device_name(device_name),
            suffix(suffix)
        {
        }

        /**
         * @brief Virtual destructor.
         *
         * Ensures proper cleanup of derived device types.
         */
        virtual ~device() = default;

        /**
         * @brief Form the unique device identifier string.
         *
         * Combines the owner, device name, and suffix separated
         * by dots. This can be used as a key in maps or for logging.
         *
         * @return A fully qualified device ID.
         */
        std::string id() const;
};

/**
 * @brief Inline implementation of id().
 *
 * Concatenates:
 *   owner + "." + device_name + "." + suffix
 */
inline std::string device::id() const
{
    return owner + std::string(".")
        + device_name + std::string(".")
        + suffix;
}

/**
 * @brief Shared pointer alias for device.
 *
 * Use this to hold references to device objects polymorphically:
 * @code
 * sp_device_t d = std::make_shared<derived_device>(...);
 * @endcode
 */
typedef std::shared_ptr<device> sp_device_t;

/**
 * @brief Map of device ID to shared device pointer.
 *
 * Useful when managing collections of devices keyed by
 * their unique ID.
 */
typedef std::map<std::string, sp_device_t> device_map_t;

} // namespace robotkernel

#endif // ROBOTKERNEL__DEVICE_H

