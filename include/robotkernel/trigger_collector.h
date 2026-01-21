//! robotkernel class for triggers
/*!
 * (C) Robert Burger <robert.burger@dlr.de>
 * (C) Sergey Tarasenko 
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
 * @file trigger_collector.h
 * @brief Trigger collector to synchronize multiple trigger sources.
 *
 * This file defines helpers for combining and synchronizing
 * multiple trigger events. This is useful when you have several
 * periodic data sources (e.g., process data from multiple slaves)
 * and want to ensure that a callback is only invoked once all
 * expected triggers have occurred within a timeout window.
 *
 * See also trigger and trigger_worker for the core trigger
 * dispatching mechanisms. :contentReference[oaicite:0]{index=0}
 */

#ifndef ROBOTKERNEL__TRIGGER_COLLECTOR_H
#define ROBOTKERNEL__TRIGGER_COLLECTOR_H

#include <functional>
#include <memory>
#include <vector>
#include <ctime>

namespace robotkernel {

/**
 * @class trigger_cb
 * @brief Simple trigger callback that wraps a function object.
 *
 * This is a convenience class that implements the trigger_base
 * interface by calling a user-supplied `std::function` on each
 * trigger tick. It allows arbitrary callables to act as
 * trigger callbacks. :contentReference[oaicite:1]{index=1}
 */
class trigger_cb : public robotkernel::trigger_base
{
    public:
        /// The callback invoked on each tick.
        std::function<void()> cb;
    
        /**
         * @brief Construct a trigger callback.
         * @param cb
         *        Function called on each tick.
         */
        trigger_cb(std::function<void()> cb) : cb(cb) {}
    
        /**
         * @brief Trigger function called by the trigger engine.
         *
         * Executes the stored callback function.
         */
        void tick() override { cb(); }
};

/// Shared pointer alias for a trigger callback wrapper.
typedef std::shared_ptr<trigger_cb> sp_trigger_cb_t;

/**
 * @class trigger_collector
 * @brief Collects trigger events from multiple sources.
 *
 * A `trigger_collector` waits until it has observed trigger calls
 * from all expected sources (e.g., slave IDs 0‥count-1) within a
 * specified timeout window. Once all active producers have
 * triggered, a combined callback is invoked.
 *
 * This is especially useful in systems where the arrival order
 * of multiple periodic data streams is indeterminate. :contentReference[oaicite:2]{index=2}
 */
class trigger_collector
{
    public:
        /**
         * @brief Default constructor.
         *
         * Leaves the collector uninitialized.
         */
        trigger_collector()
            : callback(nullptr),
              count(-1),
              timeout(1.0),
              receive_timeout(0.0)
        {}
    
        /**
         * @brief Construct with only a callback.
         *
         * The actual count/timeout must be set later via `reinit()`.
         *
         * @param callback
         *        Function invoked once all triggers have been collected.
         */
        trigger_collector(std::function<void()> callback)
            : callback(callback),
              count(-1),
              timeout(1.0),
              receive_timeout(0.0)
        {}
    
        /**
         * @brief Construct and initialize immediately.
         *
         * @param count
         *        Number of sources (e.g., slave IDs).
         *
         * @param timeout
         *        Timeout in seconds.
         *
         * @param callback
         *        Callback invoked when all active triggers have been seen.
         */
        trigger_collector(int count,
                          double timeout,
                          std::function<void()> callback)
        {
            reinit(count, timeout, callback);
        }
    
        virtual ~trigger_collector() {}
    
        /**
         * @brief Reinitialize the collector.
         *
         * Sets the expected number of sources, the timeout
         * duration, and the callback. Also resets internal state.
         *
         * @param count
         *        Total number of trigger sources.
         *
         * @param timeout
         *        Timeout window (in seconds).
         *
         * @param callback
         *        Called when all active triggers have been received.
         */
        void reinit(const int count,
                    const double timeout,
                    std::function<void()> callback)
        {
            this->count  = count;
            this->timeout = timeout;
            this->callback = callback;
    
            timestamps.resize(count);
            active_mask.resize(count);
            for (int i = 0; i < count; i++) {
                timestamps[i]    = 0.0;
                active_mask[i]   = true;
            }
            receive_timeout = 0.0;
        }
    
        /**
         * @brief Update the active mask.
         *
         * Only triggers from indices marked `true` in the mask
         * contribute toward the collection condition.
         *
         * @param active_mask A vector of booleans indicating active sources.
         */
        void set_active_mask(const std::vector<bool>& active_mask)
        {
            for (unsigned i = 0;
                 (i < active_mask.size()) && (i < this->active_mask.size());
                 ++i) {
                this->active_mask[i] = active_mask[i];
            }
        }
    
        /**
         * @brief Record a trigger for a specific source.
         *
         * If a full set of active triggers is observed within the
         * timeout, invoke the stored callback.
         *
         * @param slave_id Zero-based source index.
         */
        void trigger_collect(uint32_t slave_id)
        {
            double ts = get_timestamp();
    
            // Timeout occurred: reset timestamps
            if (receive_timeout < ts) {
                for (int i = 0; i < count; i++) {
                    timestamps[i] = 0.0;
                }
                receive_timeout = ts + timeout;
            }
    
            timestamps[slave_id] = ts;
    
            // Check whether all active sources have triggered
            for (int i = 0; i < count; i++) {
                if (active_mask[i] && (timestamps[i] == 0.0)) {
                    return;
                }
            }
    
            // All active triggers received: call callback
            if (callback) {
                callback();
            }
    
            // Reset so next cycle begins
            receive_timeout = 0.0;
        }
    
        /**
         * @brief Get the current time as a double (seconds).
         *
         * Uses `clock_gettime(CLOCK_REALTIME)`.
         *
         * @return Floating-point timestamp in seconds.
         */
        double get_timestamp()
        {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            return double(ts.tv_sec) + (double(ts.tv_nsec) / 1e9);
        }
    
    protected:
        /// Callback invoked when a full set of triggers is collected.
        std::function<void()> callback;
    
        /// Expected number of trigger sources.
        int count;
    
        /// Last trigger timestamps for each source.
        std::vector<double> timestamps;
    
        /// Mask identifying active sources (some positions may be ignored).
        std::vector<bool> active_mask;
    
        /// Timeout window duration in seconds.
        double timeout;
    
        /// Timestamp at which the current collection window expires.
        double receive_timeout;
};

/// Shared pointer alias for trigger_collector.
typedef std::shared_ptr<trigger_collector> sp_trigger_collector_t;

}; // namespace robotkernel
 
#endif // ROBOTKERNEL__TRIGGER_COLLECTOR_H

