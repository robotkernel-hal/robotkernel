//! robotkernel trigger class
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
 * @file trigger.h
 * @brief Trigger mechanism for periodic and event-based callbacks.
 *
 * The `trigger` class allows registration of callback objects
 * that are invoked at a specified rate. It also includes a
 * helper waiter (`trigger_waiter`) that can block until the
 * next trigger event occurs.
 *
 * This mechanism is widely used for scheduling periodic tasks
 * in device drivers, bridges, services, or other runtime modules.
 *
 * @note
 * - A trigger manages a set of callbacks that inherit from
 *   `trigger_base`.
 * - Callbacks may be handled directly or via worker threads
 *   depending on configuration.
 * - The `wait()` call may throw on timeout.
 *
 * See also `trigger_worker.h` for worker-based callback dispatch.
 */

#ifndef ROBOTKERNEL__TRIGGER_H
#define ROBOTKERNEL__TRIGGER_H

#include <string>
#include <mutex>
#include <stdexcept>

// public headers
#include "robotkernel/runnable.h"
#include "robotkernel/device.h"
#include "robotkernel/trigger_base.h"
#include "robotkernel/trigger_worker.h"

namespace robotkernel {

/**
 * @class trigger_waiter
 * @brief Helper class that waits on a condition variable until a trigger occurs.
 *
 * This class can be used when a component needs to block until the
 * next trigger. The `tick()` method wakes all waiting threads,
 * and `wait()` sleeps until either a wake occurs or a timeout expires.
 */
class trigger_waiter : public trigger_base
{
    public:
        /// Condition variable used to wake waiting threads.
        std::condition_variable cond;
    
        /// Mutex protecting the condition variable.
        std::mutex mtx;
    
        /// Construct a trigger waiter.
        trigger_waiter() {}
    
        /// Default destructor.
        ~trigger_waiter() {}
    
        /**
         * @brief Notify all waiting threads.
         *
         * Called by the triggering mechanism on each tick to wake
         * up any waiters.
         */
        void tick()
        {
            std::unique_lock<std::mutex> lock(mtx);
            cond.notify_all();
        }
    
        /**
         * @brief Block until the next trigger tick or timeout.
         *
         * @param timeout Maximum time to wait, in seconds.
         * @throws std::runtime_error If the wait times out.
         */
        void wait(double timeout)
        {
            std::unique_lock<std::mutex> lock(mtx);
            wait(timeout, lock);
        }
    
        /**
         * @brief Block on the condition with an existing lock.
         *
         * @param timeout Maximum time to wait, in seconds.
         * @param lock Held mutex lock during wait.
         * @throws std::runtime_error If the wait times out.
         */
        void wait(double timeout, std::unique_lock<std::mutex>& lock)
        {
            using namespace std::chrono;
            if (cond.wait_for(lock, nanoseconds((uint64_t)(timeout * 1e9)))
                == std::cv_status::timeout) {
                throw std::runtime_error("timeout waiting for trigger");
            }
        }
};

/// Shared pointer type for trigger_waiter.
typedef std::shared_ptr<trigger_waiter> sp_trigger_waiter_t;


/**
 * @class trigger
 * @brief Central trigger object that schedules callback execution.
 *
 * A trigger object manages one or more callback handlers derived
 * from `trigger_base`. Each callback is scheduled at the trigger's
 * configured rate and may be executed either directly or via
 * worker threads depending on the callback configuration.
 *
 * Derived classes or users can adjust the trigger rate at runtime.
 */
class trigger : public device
{
    private:
        /// Disable copy construction.
        trigger(const trigger&);
    
        /// Disable assignment.
        trigger& operator=(const trigger&);
    
        /// Mutex protecting access to the trigger list.
        std::mutex list_mtx;
    
        /// Map of registered trigger callbacks.
        ///
        /// Sorted by priority so that we can do a simple
        /// priority scheduling.
        std::map<int, trigger_list_t> triggers;
    
        /// Worker threads for handling callbacks.
        trigger_workers_t workers;
    
    protected:
        /// Trigger rate in Hz (events per second).
        double rate;
    
    public:
        /**
         * @brief Construct a trigger with a given rate.
         *
         * @param owner Logical owner name (e.g. bridge/module).
         * @param name Name of the trigger.
         * @param rate Frequency of trigger in Hertz (Hz). 0 means manual.
         */
        trigger(const std::string& owner,
                const std::string& name,
                double rate = 0.);
    
        /// Virtual destructor.
        virtual ~trigger();
    
        /**
         * @brief Add a trigger callback.
         *
         * Registers a callback object with this trigger. The callback
         * will be invoked on each tick at the configured rate.
         *
         * @param trigger Shared pointer to callback object.
         */
        void add_trigger(sp_trigger_base_t trigger)
        {
            add_trigger(trigger,
                        trigger->direct_mode,
                        trigger->worker_prio,
                        trigger->worker_affinity);
        }
    
        /**
         * @brief Add a trigger callback with custom execution parameters.
         *
         * @param trigger       Callback object to register.
         * @param direct_mode   Whether to run the callback in the trigger thread.
         * @param worker_prio   Thread priority for worker execution or prio scheduling.
         * @param worker_affinity CPU affinity for worker thread.
         */
        void add_trigger(sp_trigger_base_t trigger,
                         bool direct_mode,
                         int worker_prio,
                         int worker_affinity);
    
        /**
         * @brief Remove a previously registered trigger callback.
         *
         * @param trigger Callback object to unregister.
         */
        void remove_trigger(sp_trigger_base_t trigger);
    
        /**
         * @brief Get the configured trigger rate.
         *
         * @return The rate in Hertz.
         */
        double get_rate() const { return rate; }
    
        /**
         * @brief Set a new trigger rate.
         *
         * Derived classes may override this to implement
         * specific rate constraints or scheduling behavior.
         *
         * @param new_rate New trigger frequency in Hertz.
         */
        virtual void set_rate(double new_rate);
    
        /**
         * @brief Invoke all registered callbacks.
         *
         * Called internally by the trigger scheduler (timer or loop).
         * Executes all callbacks at this tick.
         */
        void do_trigger();
    
        /**
         * @brief Block until the next trigger event or timeout.
         *
         * @param timeout Maximum wait time in seconds.
         */
        void wait(double timeout);
};

/// Shared pointer type for trigger.
typedef std::shared_ptr<trigger> sp_trigger_t;

/// Map from trigger names to shared trigger pointers.
typedef std::map<std::string, sp_trigger_t> trigger_map_t;

} // namespace robotkernel

#endif // ROBOTKERNEL__TRIGGER_H

