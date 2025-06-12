//! robotkernel trigger woker class
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

// public headers
#include "robotkernel/trigger_worker.h"

// private headers
#include "kernel.h"

#include "string_util/string_util.h"

using namespace std;
using namespace robotkernel;
using namespace string_util;
        
bool trigger_worker::worker_key::operator<(const worker_key& a) const {
    if (prio < a.prio)
        return true;
    if (prio > a.prio)
        return false;

    if (affinity < a.affinity)
        return true;
    if (affinity > a.affinity)
        return false;

    if (divisor < a.divisor)
        return true;
    if (divisor > a.divisor)
        return false;

    return (int) divisor < (int)a.divisor;
}

trigger_worker::trigger_worker(int prio, int affinity_mask, int divisor) :
    runnable(prio, affinity_mask, format_string("trigger_worker.prio_%d."
                "affinity_mask_%d.divisor_%d", prio, affinity_mask, divisor)), 
    trigger_base(divisor) 
{
    // start worker thread
    start();
    klog(info, "[trigger_worker] created with prio %d\n", prio);
};
        
//! destruction
trigger_worker::~trigger_worker() {
    // stop worker thread
    stop();
    klog(info, "[trigger_worker] destructed\n");
}

//! add trigger to worker
/*!
 * \param trigger trigger to add
 */
void trigger_worker::add_trigger(sp_trigger_base_t trigger) {
    // push to module list
    for (const auto& t : triggers) {
        if (t == trigger) 
            throw str_exception("there was a try to register a trigger twice!");
    }

    std::unique_lock<std::mutex> lock(mtx);
    triggers.push_back(trigger);
}

//! remove trigger from worker
/*!
 * \param trigger trigger to remove
 */
void trigger_worker::remove_trigger(sp_trigger_base_t trigger) {
    // remove from module list
    std::unique_lock<std::mutex> lock(mtx);
    triggers.remove(trigger);
}

//! handler function called if thread is running
void trigger_worker::run() {
    klog(info, "[trigger_worker] running worker thread\n");
    
    // lock mutex cause we access _modules
    std::unique_lock<std::mutex> lock(mtx);

    while (running()) {
        // wait for trigger, this will unlock mutex
        // other threads can now safely add/remove something from triggers
        if (cond.wait_for(lock, std::chrono::seconds(1)) == std::cv_status::timeout)
            continue;

        for (const auto& t : triggers)
            t->tick();
    }
        
    klog(info, "[trigger_worker] finished worker thread\n");
}

//! trigger worker
void trigger_worker::tick() {
    // there's no need to lock 'mtx' here, it's only used to protect trigger list
    cond.notify_all();
}

