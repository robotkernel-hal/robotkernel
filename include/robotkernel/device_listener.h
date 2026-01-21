//! robotkernel device listener class
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
 * @file device_listener.h
 * @brief Abstract listener for device lifecycle events.
 *
 * Defines the interface for classes that want to be notified
 * about device additions and removals within the robotkernel
 * device registry.
 *
 * A listener registers interest in device events and receives:
 *   - `notify_add_device()`
 *   - `notify_remove_device()`
 *
 * When devices are added/removed, the runtime can call these
 * methods to let observers react (e.g., update UI, reconfigure
 * mappings, manage resource lifetimes).
 *
 * @note
 * - This class is abstract and must be implemented by concrete
 *   listeners.
 * - Use shared pointers (`sp_device_listener_t`) to manage lifetime.
 */

#ifndef ROBOTKERNEL_DEVICE_LISTENER_H
#define ROBOTKERNEL_DEVICE_LISTENER_H

#include <string>

// public header
#include "robotkernel/device.h"

namespace robotkernel {

/**
 * @class device_listener
 * @brief Receiver of device lifecycle notifications.
 *
 * Listens for device registration and deregistration events.
 * Each listener has:
 *   - an `owner`: logical owner of devices it cares about
 *   - a `name`: human/product name for the listener itself
 *
 * Derived classes implement callbacks to respond to events.
 */
class device_listener
{
public:
    /// Logical owner of devices this listener watches.
    const std::string owner;

    /// Name of the listener (for logging/identification).
    const std::string name;

public:
    /**
     * @brief Constructor.
     *
     * @param owner
     *        Logical owner or subsystem that this listener is
     *        interested in.
     *
     * @param name
     *        Identifier for this listener instance.
     */
    device_listener(const std::string& owner,
                    const std::string& name)
        : owner(owner),
          name(name)
    {
    }

    /**
     * @brief Virtual destructor.
     *
     * Declared pure virtual to make this class abstract,
     * but defined inline to ensure proper deletion of derived
     * listener objects.
     */
    virtual ~device_listener() = 0;

    /**
     * @brief Called when a device is added.
     *
     * Derived classes implement this to react when a new
     * device gets registered (e.g., allocate resources,
     * update state).
     *
     * @param req
     *        Shared pointer to the newly added device.
     */
    virtual void notify_add_device(sp_device_t req) = 0;

    /**
     * @brief Called when a device is removed.
     *
     * Derived listener can handle the removal (e.g.,
     * cleanup, reconfiguration, resource release).
     *
     * @param req
     *        Shared pointer to the removed device.
     */
    virtual void notify_remove_device(sp_device_t req) = 0;
};

// -----------------------------------------------------------------------------
// Inline definitions
// -----------------------------------------------------------------------------

/**
 * @brief Abstract base destructor definition.
 *
 * Although pure virtual, this is defined so that derived
 * destructors are called correctly when deleted via base
 * pointer.
 */
inline device_listener::~device_listener() {}

/**
 * @brief Shared pointer alias for device_listener.
 */
typedef std::shared_ptr<device_listener> sp_device_listener_t;

/**
 * @brief Map of listener name to listener pointer.
 *
 * Useful for managing registered listeners keyed by name.
 *
 * Key: listener identifier
 * Value: shared pointer to listener
 */
typedef std::map<std::pair<std::string, std::string>, sp_device_listener_t> device_listener_map_t;

}; // namespace robotkernel

#endif // ROBOTKERNEL_DEVICE_LISTENER_H

