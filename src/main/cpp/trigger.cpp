//! robotkernel base class for trigger devices
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

#include "robotkernel/trigger.h"
#include "robotkernel/trigger_worker.h"
#include "robotkernel/kernel.h"

#include "string_util/string_util.h"

#include <condition_variable>

using namespace std;
using namespace robotkernel;
using namespace string_util;
        
// construction
trigger::trigger(const std::string& owner, const std::string& name, double rate) 
    : device(owner, name, "trigger"), rate(rate)
{
}

//! destruction
trigger::~trigger() {
    {
        std::unique_lock<std::mutex> lock(list_mtx);
        triggers.clear();
    }

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

    std::unique_lock<std::mutex> lock(list_mtx);

    if (direct_mode) {
        triggers.push_back(trigger);
        return;
    }

    if (workers.find(k) == workers.end()) {
        // create new worker thread
        workers[k] = make_shared<trigger_worker>(worker_prio, worker_affinity, trigger->divisor);
        triggers.push_back(workers[k]);
    }

    workers[k]->add_trigger(trigger);
}

//! remove a trigger callback function
/*!
 * \param obj trigger object to trigger callback
 */
void trigger::remove_trigger(sp_trigger_base_t trigger) {
    std::unique_lock<std::mutex> lock(list_mtx);

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
}

//! wait blocking for next trigger
void trigger::wait(double timeout) {
    std::shared_ptr<trigger_waiter> waiter = make_shared<trigger_waiter>();
    add_trigger(waiter);
    
    try {
        waiter->wait(timeout);
    } catch (str_exception& e) {
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
    std::unique_lock<std::mutex> lock(list_mtx);

    for (auto& t : triggers) {
        if (((++t->cnt) % t->divisor) == 0) {
            t->cnt = 0;

            t->tick();
        }
    }
}

