//! robotkernel base class for triggers
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

#include "robotkernel/trigger_base.h"
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

//! add trigger_callback to worker
/*!
 * \param trigger trigger callback to add
 */
void trigger_worker::add_trigger_callback(sp_trigger_base_t trigger) {
    // push to module list
    for (const auto& t : triggers) {
        if (t == trigger) 
            throw str_exception("there was a try to register a trigger twice!");
    }

    pthread_mutex_lock(&mutex);
    triggers.push_back(trigger);
    pthread_mutex_unlock(&mutex);
}

//! remove trigger_callback from worker
/*!
 * \param trigger trigger callback to remove
 */
void trigger_worker::remove_trigger_callback(sp_trigger_base_t trigger) {
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

//! construction
trigger_device::trigger_device(const std::string& trigger_dev_name, double rate) 
    : trigger_dev_name(trigger_dev_name), rate(rate)
{
    pthread_mutex_init(&list_lock, NULL);
}

//! destruction
trigger_device::~trigger_device() {
    pthread_mutex_destroy(&list_lock);

    pthread_mutex_lock(&list_lock);
    triggers.clear();
    pthread_mutex_unlock(&list_lock);

    for (auto& kv : workers) {
        kv.second.reset();
    }
}

//! add a trigger callback function
/*!
 * \param cb trigger callback
 * \param divisor rate divisor
 * \return trigger object to newly inserted callback
 */
void trigger_device::add_trigger_callback(sp_trigger_base_t trigger, 
        bool direct_mode, int worker_prio, int worker_affinity) {
    trigger_worker::worker_key k = { worker_prio, worker_affinity, trigger->divisor };

    pthread_mutex_lock(&list_lock);

    if (direct_mode) {
        triggers.push_back(trigger);
        goto func_exit;
    }

    if (workers.find(k) == workers.end()) {
        // create new worker thread
        workers[k] = make_shared<trigger_worker>(worker_prio, worker_affinity, trigger->divisor);
        triggers.push_back(workers[k]);
    }

    workers[k]->add_trigger_callback(trigger);
    
func_exit:
    pthread_mutex_unlock(&list_lock);
}

//! remove a trigger callback function
/*!
 * \param obj trigger object to trigger callback
 */
void trigger_device::remove_trigger_callback(sp_trigger_base_t trigger) {
    pthread_mutex_lock(&list_lock);

    triggers.remove(trigger);

    for (auto it = workers.begin(); it != workers.end(); ) {
        it->second->remove_trigger_callback(it->second);
        auto act_it = it++;

        printf("worker size is %d\n", act_it->second->size());
        if (!act_it->second->size())
            workers.erase(act_it);
    }

    pthread_mutex_unlock(&list_lock);
}

//! set rate of trigger device
/*!
 * set the rate of the current trigger
 * overload in derived trigger class
 *
 * \param new_rate new trigger rate to set
 */
void trigger_device::set_rate(double new_rate) {
    throw str_exception("setting rate not permitted!");
}

//! trigger all modules in list
void trigger_device::trigger_modules() {
    pthread_mutex_lock(&list_lock);
    for (auto& t : triggers) {
        if (((++t->cnt) % t->divisor) == 0) {
            t->cnt = 0;

            t->trigger();
        }
    }

    pthread_mutex_unlock(&list_lock);
}

