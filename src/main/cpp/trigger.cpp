//! robotkernel base class for trigger devices
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

#include "robotkernel/trigger.h"
#include "robotkernel/trigger_worker.h"
#include "robotkernel/kernel.h"

#include "string_util/string_util.h"

using namespace std;
using namespace robotkernel;
using namespace string_util;
        
// construction
trigger::trigger(const std::string& owner, const std::string& name, double rate) 
    : device(owner, name, "trigger"), rate(rate)
{
    pthread_mutex_init(&list_lock, NULL);
}

//! destruction
trigger::~trigger() {
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
void trigger::add_trigger(sp_trigger_base_t trigger, 
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

    workers[k]->add_trigger(trigger);
    
func_exit:
    pthread_mutex_unlock(&list_lock);
}

//! remove a trigger callback function
/*!
 * \param obj trigger object to trigger callback
 */
void trigger::remove_trigger(sp_trigger_base_t trigger) {
    pthread_mutex_lock(&list_lock);

    triggers.remove(trigger);

    for (auto it = workers.begin(); it != workers.end(); ) {
        it->second->remove_trigger(trigger);
        auto act_it = it++;

        if (!act_it->second->size()) {
            auto w = act_it->second;
            triggers.remove(w);
            workers.erase(act_it);
        }
    }

    pthread_mutex_unlock(&list_lock);
}

//! trigger waiter class
class trigger_waiter : 
    public trigger_base 
{
    public:
        pthread_cond_t cond;
        pthread_mutex_t mutex;

        trigger_waiter() {
            pthread_cond_init(&cond, NULL);
            pthread_mutex_init(&mutex, NULL);
        }

        ~trigger_waiter() {
            pthread_cond_destroy(&cond);
            pthread_mutex_destroy(&mutex);
        }

        void tick() {
            pthread_cond_broadcast(&cond);
        }

        void wait(double timeout) {
            struct timespec abstime;
            clock_gettime(CLOCK_MONOTONIC, &abstime);
            
            uint64_t nsec = (uint64_t)(timeout * 1000000000) % 1000000000;
            uint64_t sec  = timeout;

            abstime.tv_nsec += nsec;
            if (abstime.tv_nsec > 1000000000) {
                abstime.tv_nsec -= 1000000000;
                sec++;
            }
            abstime.tv_sec += sec;

            int ret = pthread_cond_timedwait(&cond, &mutex, &abstime);
            if (ret)
                throw errno_exception("error occured waiting for trigger: ");
        }
};

//! wait blocking for next trigger
void trigger::wait(double timeout) {
    std::shared_ptr<trigger_waiter> waiter = make_shared<trigger_waiter>();
    add_trigger(waiter);
    
    try {
        waiter->wait(timeout);
    } catch (exception& e) {
        remove_trigger(waiter);
        throw e;
    }
    
    remove_trigger(waiter);
}

//! set rate of trigger device
/*!
 * set the rate of the current trigger
 * overload in derived trigger class
 *
 * \param new_rate new trigger rate to set
 */
void trigger::set_rate(double new_rate) {
    throw str_exception("setting rate not permitted!");
}

//! trigger all modules in list
void trigger::trigger_modules() {
    pthread_mutex_lock(&list_lock);
    for (auto& t : triggers) {
        if (((++t->cnt) % t->divisor) == 0) {
            t->cnt = 0;

            t->tick();
        }
    }

    pthread_mutex_unlock(&list_lock);
}

