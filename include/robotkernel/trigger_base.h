//! robotkernel base class for triggers
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
 * @file trigger_base.h
 * @brief Base class for trigger callbacks in robotkernel.
 *
 * The trigger subsystem in robotkernel allows registration of callback
 * objects that are invoked each time a trigger event ("tick") occurs.
 * Classes that wish to receive these callbacks should derive from
 * `trigger_base` and implement the pure virtual function `tick()`.
 *
 * A trigger_base object also supports configuration from YAML via its
 * constructor, and can acquire/release itself from a trigger device
 * identified by name. :contentReference[oaicite:0]{index=0}
 */

#ifndef ROBOTKERNEL__TRIGGER_BASE_H
#define ROBOTKERNEL__TRIGGER_BASE_H

#include <functional>
#include <list>
#include <memory>
#include <string>
#include <yaml-cpp/yaml.h>

#include "robotkernel/helpers.h"

namespace robotkernel {

class trigger;

/**
 * @class trigger_base
 * @brief Abstract base class for objects that are notified by a trigger.
 *
 * Derive from this class to implement a callback object that can be
 * registered with a `trigger` (or a trigger device) to receive periodic
 * notifications (`tick()` calls). The base class also holds configuration
 * fields such as `divisor`, execution mode flags, and worker thread hints.
 *
 * @note
 * - The `tick()` method must be overridden in derived classes.
 * - The object can be configured from a YAML node using the second ctor.
 * - Call `aquire()` to register with the trigger device and
 *   `release()` to unregister. :contentReference[oaicite:1]{index=1}
 */
class trigger_base : public virtual shared_base
{
    public:
        /// Trigger divisor: number of ticks before invoking this callback.
        int divisor = 1;
    
        /// Internal counter updated by trigger device.
        int cnt = 0;
    
        /**
         * @brief If true, callback runs in the triggering thread.
         *
         * If false, execution may be handed off to a worker thread
         * according to `worker_prio` / `worker_affinity`. :contentReference[oaicite:2]{index=2}
         */
        bool direct_mode = true;
    
        /// Priority for worker thread execution.
        int worker_prio = 0;
    
        /// CPU affinity mask for worker thread execution.
        int worker_affinity = 0xFFFFFFFF;
    
        /// Optional device name from which this callback is sourced.
        std::string dev_name = "";
    
        /// Optional pointer to the owning device instance.
        std::shared_ptr<robotkernel::trigger> dev = nullptr;
    
    public:
        /**
         * @brief Construct with an optional trigger divisor.
         *
         * @param divisor
         *        Number of base trigger ticks between invocations. Default 1. :contentReference[oaicite:3]{index=3}
         */
        trigger_base(int divisor = 1)
            : divisor(divisor)
        {
        }
    
        /**
         * @brief Construct and configure from a YAML node.
         *
         * This constructor reads configuration fields such as:
         * - `divisor`
         * - `direct_mode`
         * - `worker_prio`
         * - `worker_affinity`
         * - `dev_name`
         *
         * @param node
         *        YAML node containing configuration. :contentReference[oaicite:4]{index=4}
         */
        trigger_base(const YAML::Node& node)
        {
            using robotkernel::helpers::get_as;
            divisor = get_as<int>(node, "divisor", 1);
            direct_mode = get_as<bool>(node, "direct_mode", true);
            worker_prio = get_as<int>(node, "worker_prio", 0);
            worker_affinity = get_as<int>(node, "worker_affinity", 0xFFFFFFFF);
            dev_name = get_as<std::string>(node, "dev_name", "");
        }
    
        /**
         * @brief Virtual destructor ensures cleanup via `release()`. :contentReference[oaicite:5]{index=5}
         */
        virtual ~trigger_base() { release(); }
    
        /**
         * @brief Pure virtual callback invoked on trigger ticks.
         *
         * Derived classes must override this to implement the actual
         * behavior when a trigger event occurs. :contentReference[oaicite:6]{index=6}
         */
        virtual void tick() = 0;
    
        /**
         * @brief Register this callback with the trigger device.
         *
         * This method looks up the trigger by `dev_name` and adds this
         * callback to it. The owning device pointer (`dev`) may be set
         * as part of registration. :contentReference[oaicite:7]{index=7}
         */
        void acquire(void);
    
        /**
         * @brief Unregister this callback from the trigger device.
         *
         * Reverse operation to `aquire()`: remove this callback from
         * the trigger device and release references. :contentReference[oaicite:8]{index=8}
         */
        void release(void);
};

/**
 * @brief Shared pointer alias for a trigger callback. :contentReference[oaicite:9]{index=9}
 */
typedef std::shared_ptr<trigger_base> sp_trigger_base_t;

/**
 * @brief List of trigger callbacks. :contentReference[oaicite:10]{index=10}
 */
typedef std::list<sp_trigger_base_t> trigger_list_t;

/**
 * @class triggerable
 * @brief Helper type wrapping a simple lambda/function into trigger_base.
 *
 * This convenience class allows binding a `std::function<void()>` to
 * a trigger callback without needing to derive your own subclass.
 */
class triggerable : public trigger_base
{
    private:
        /// Underlying function to call on tick.
        std::function<void()> cb;
    
    public:
        /**
         * @brief Construct a triggerable from a YAML node and function. :contentReference[oaicite:11]{index=11}
         *
         * @param node
         *        YAML configuration node.
         *
         * @param cb
         *        Function to invoke on `tick()`.
         */
        triggerable(const YAML::Node& node, std::function<void()> cb)
            : trigger_base(node), cb(cb)
        {
        }
    
        /**
         * @brief Invoke the underlying function. :contentReference[oaicite:12]{index=12}
         */
        virtual void tick() override { cb(); }
};

} // namespace robotkernel

#endif // ROBOTKERNEL__TRIGGER_BASE_H

