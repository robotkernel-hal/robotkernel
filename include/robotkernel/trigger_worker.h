//! robotkernel trigger worker class
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
 * @file trigger_worker.h
 * @brief Worker thread support for trigger callbacks.
 *
 * This header defines the `trigger_worker` class, which is a
 * combination of a runnable thread and a trigger callback handler.
 * It receives tick events from a trigger object and dispatches
 * them within a worker thread context.
 *
 * A worker can host multiple callbacks and schedules their
 * execution when a trigger fires.
 *
 * @note
 * - This class derives from both `runnable` and `trigger_base`.
 * - It is safe to use with `trigger` objects configured
 *   to dispatch callbacks in worker mode.
 * - Thread priority and CPU affinity can be configured.
 */

#ifndef ROBOTKERNEL__TRIGGER_WORKER_H
#define ROBOTKERNEL__TRIGGER_WORKER_H

#include <condition_variable>
#include <mutex>
#include <map>
#include <memory>

// public headers
#include "robotkernel/runnable.h"
#include "robotkernel/trigger_base.h"

namespace robotkernel {

/**
 * @class trigger_worker
 * @brief Worker thread that executes trigger callbacks.
 *
 * This class combines a thread of execution (`runnable`) with
 * a set of registered trigger callbacks (`trigger_base`). It
 * waits on a condition variable, and on each tick it runs
 * all associated callbacks in the context of a worker thread.
 *
 * Worker threads are typically created and managed by a
 * `trigger` object when callbacks are configured for
 * asynchronous execution.
 */
class trigger_worker : public robotkernel::runnable,
                       public robotkernel::trigger_base
{
    private:
        // Disable default and copy construction + assignment
        trigger_worker();
        trigger_worker(const trigger_worker&);
        trigger_worker& operator=(const trigger_worker&);
    
    public:
        /**
         * @brief Key used to identify worker threads.
         *
         * Worker threads are keyed by:
         * - Scheduling priority (`prio`)
         * - CPU affinity mask (`affinity`)
         * - Divisor for scheduling (placeholder for future use)
         */
        struct worker_key
        {
            int prio;      ///< Thread priority
            int affinity;  ///< CPU affinity mask
            int divisor;   ///< Scheduling divisor (unused)
            
            /**
             * @brief Compare two keys for ordering in maps.
             *
             * Worker keys are ordered lexicographically by
             * (prio, affinity, divisor).
             */
            bool operator<(const worker_key& a) const;
        };
    
        /**
         * @brief Construct a worker thread.
         *
         * @param prio
         *        Thread scheduling priority (platform dependent).
         *
         * @param affinity_mask
         *        CPU affinity mask indicating allowed CPUs.
         *
         * @param divisor
         *        Used for grouping or rate division (future).
         */
        trigger_worker(int prio = 60,
                       int affinity_mask = 0xFF,
                       int divisor = 1);
    
        /**
         * @brief Virtual destructor.
         *
         * Stops the worker and cleans up any state.
         */
        ~trigger_worker();
    
        /**
         * @brief Add a trigger callback to this worker.
         *
         * @param trigger
         *        Shared pointer to a callback object derived from
         *        `trigger_base`.
         */
        void add_trigger(sp_trigger_base_t trigger);
    
        /**
         * @brief Remove a previously added trigger callback.
         *
         * @param trigger
         *        Callback to remove.
         */
        void remove_trigger(sp_trigger_base_t trigger);
    
        /**
         * @brief Notify this worker of a tick event.
         *
         * Called by a `trigger` object when it fires. This method
         * wakes the worker thread to execute callbacks.
         */
        virtual void tick() override;
    
        /**
         * @brief Entry point for the worker thread.
         *
         * This is the function executed when the worker runs.
         * It waits for ticks and executes all associated callbacks.
         */
        virtual void run() override;
    
        /**
         * @brief Number of registered callbacks.
         *
         * @return Size of the internal callback list.
         */
        size_t size() { return triggers.size(); }
    
    private:
        trigger_list_t triggers;               ///< Registered callbacks
        std::condition_variable cond;          ///< Condition to sleep/wake the thread
        std::mutex mtx;                        ///< Mutex for synchronization
};

/// Shared pointer alias for trigger_worker.
typedef std::shared_ptr<trigger_worker> sp_trigger_worker_t;

/// Map of worker_key to worker pointer.
typedef std::map<trigger_worker::worker_key, sp_trigger_worker_t> trigger_workers_t;

}; // namespace robotkernel

#endif // ROBOTKERNEL__TRIGGER_WORKER_H

