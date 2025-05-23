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

#ifndef ROBOTKERNEL__TRIGGER_COLLECTOR_H
#define ROBOTKERNEL__TRIGGER_COLLECTOR_H

#include <stdint.h>
#include <time.h>

#include <functional>

namespace robotkernel {

/** 
 * Trigger callback class to call an arbitrary function from trigger tick 
 */
class trigger_cb : public robotkernel::trigger_base {
    public:
        std::function<void(void)> cb;

        trigger_cb(std::function<void(void)> cb) : cb(cb) {}

        //! trigger function
        void tick() { cb(); }
};

typedef std::shared_ptr<trigger_cb> sp_trigger_cb_t;

/**
 * Collects triggers for slaves 0..Count-1 via trigger_collect(unique slave id) within timeout and calls
 * callback when all are collected. For example this is used to ensure that data from different process
 * data devices were received since order may vary.
 */
class trigger_collector {
    public:
        trigger_collector() : 
            callback(nullptr), count(-1), timeout(1.) { };

        trigger_collector(std::function<void(void)> callback) : 
            callback(callback), count(-1), timeout(1.) { };

        trigger_collector(int count, double timeout, std::function<void(void)> callback)
        { reinit(count, timeout, callback); };

        virtual ~trigger_collector() { }
        
        void reinit(const int count, const double timeout, std::function<void(void)> callback) {
            this->count = count;
            this->timeout = timeout;
            this->callback = callback;

            timestamps.resize(count);

            for (int i = 0; i < count; i++) {
                timestamps[i] = 0;
                receive_timeout = 0;
            }
        }

        void trigger_collect(uint32_t slave_id) {
            double ts = get_timestamp();
            if (receive_timeout < ts) {
                // timeout
                for (int i = 0; i < count; i++) {
                    timestamps[i] = 0;
                }

                receive_timeout = ts + timeout;
            }
            
            timestamps[slave_id] = ts;

            for (int i = 0; i < count; i++) {
                if (0 == timestamps[i]) {
                    return;
                }
            }

            if (callback) { callback(); }
            receive_timeout = 0;
        }

        double get_timestamp() {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            return (double) ts.tv_sec + (ts.tv_nsec / 1e9);
        }

    protected:
        std::function<void(void)> callback;

        int count;
        std::vector<double> timestamps;
        double timeout;
        double receive_timeout;

};

typedef std::shared_ptr<trigger_collector> sp_trigger_collector_t;

}; // namespace robotkernel
 
#endif // ROBOTKERNEL__TRIGGER_COLLECTOR_H

