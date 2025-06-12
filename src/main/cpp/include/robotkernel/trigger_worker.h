//! robotkernel trigger worker class
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

#ifndef ROBOTKERNEL__TRIGGER_WORKER_H
#define ROBOTKERNEL__TRIGGER_WORKER_H

#include <string>
#include <mutex>
#include <condition_variable>

// public header
#include "robotkernel/runnable.h"
#include "robotkernel/trigger_base.h"

namespace robotkernel {

//! trigger worker thread
class trigger_worker : 
    public robotkernel::runnable, 
    public robotkernel::trigger_base 
{
    private:
        trigger_worker();
        trigger_worker(const trigger_worker&);             // prevent copy-construction
        trigger_worker& operator=(const trigger_worker&);  // prevent assignment

    public:
        struct worker_key {
            int prio;
            int affinity;
            int divisor;

            bool operator<(const worker_key& a) const;
        };

        //! default construction
        trigger_worker(int prio = 60, int affinity_mask = 0xFF, int divisor=1);

        //! destruction
        ~trigger_worker();

        //! add trigger to worker
        /*!
         * \param trigger trigger to add
         */
        void add_trigger(sp_trigger_base_t trigger);

        //! remove trigger from worker
        /*!
         * \param trigger trigger to remove
         */
        void remove_trigger(sp_trigger_base_t trigger);

        //! trigger worker
        void tick();

        //! handler function called if thread is running
        void run();

        //! return size of triggers
        size_t size() { return triggers.size(); }

    private:
        trigger_list_t triggers;

        std::condition_variable cond;
        std::mutex              mtx;
};
        
typedef std::shared_ptr<trigger_worker> sp_trigger_worker_t;
typedef std::map<trigger_worker::worker_key, sp_trigger_worker_t> trigger_workers_t;

}; // namespace robotkernel

#endif // ROBOTKERNEL__TRIGGER_WORKER_H

