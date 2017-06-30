//! robotkernel trigger woker class
/*!
 * author: Robert Burger <robert.burger@dlr.de>
 */

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

#include "robotkernel/trigger_worker.h"
#include "robotkernel/kernel.h"

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

trigger_worker::trigger_worker(int prio, int affinity_mask, int divisor) 
    : runnable(prio, affinity_mask), trigger_base(divisor) {
    pthread_cond_init(&cond, NULL);
    pthread_mutex_init(&mutex, NULL);

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

    pthread_mutex_lock(&mutex);
    triggers.push_back(trigger);
    pthread_mutex_unlock(&mutex);
}

//! remove trigger from worker
/*!
 * \param trigger trigger to remove
 */
void trigger_worker::remove_trigger(sp_trigger_base_t trigger) {
    // remove from module list
    pthread_mutex_lock(&mutex);
    triggers.remove(trigger);
    pthread_mutex_unlock(&mutex);
}

//! handler function called if thread is running
void trigger_worker::run() {
    klog(info, "[trigger_worker] running worker thread\n");

    // lock mutex cause we access _modules
    pthread_mutex_lock(&mutex);
    
    while (running()) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec++;

        // wait for trigger, this will unlock mutex
        // other threads can now safely add/remove something from triggers
        int ret = pthread_cond_timedwait(&cond, &mutex, &ts);

        for (const auto& t : triggers)
            t->trigger();
    }
        
    // unlock mutex cause we have accessed _modules
    pthread_mutex_unlock(&mutex);

    klog(info, "[trigger_worker] finished worker thread\n");
}

//! trigger worker
void trigger_worker::trigger() {
    pthread_mutex_lock(&mutex);
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
}

