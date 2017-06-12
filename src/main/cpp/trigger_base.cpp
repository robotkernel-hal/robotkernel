//! robotkernel base class for triggers
/*!
 * author: Robert Burger
 *
 * $Id$
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

using namespace robotkernel;
using namespace string_util;
        
bool trigger_base::worker_key::operator<(const worker_key& a) const {
    if (prio < a.prio)
        return true;
    if (prio > a.prio)
        return false;

    if (affinity < a.affinity)
        return true;
    if (affinity > a.affinity)
        return false;

    return (int) divisor < (int)a.divisor;
}

//! construction
trigger_base::trigger_base(const std::string& trigger_dev_name, double rate) 
    : trigger_dev_name(trigger_dev_name), rate(rate)
{
    pthread_mutex_init(&list_lock, NULL);
}

//! destruction
trigger_base::~trigger_base() {
    pthread_mutex_destroy(&list_lock);

    pthread_mutex_lock(&list_lock);
    trigger_cbs.clear();
    pthread_mutex_unlock(&list_lock);

    for (auto& kv : workers) {
        delete kv.second;
    }
}

//! set rate of trigger device
/*!
 * set the rate of the current trigger
 * overload in derived trigger class
 *
 * \param new_rate new trigger rate to set
 */
void trigger_base::set_rate(double new_rate) {
    throw str_exception("setting rate not permitted!");
}

//! add a trigger callback function
/*!
 * \param cb trigger callback
 */
void trigger_base::add_trigger_module(set_trigger_cb_t& cb) {
    if (cb.cb == NULL) 
        throw str_exception("could not register, callback is NULL\n");
    
    pthread_mutex_lock(&list_lock);
    trigger_cbs.push_back(cb);
    pthread_mutex_unlock(&list_lock);
}

//! add a module to trigger device
/*!
 * \param mdl module to add
 * \param trigger trigger options
 */
void trigger_base::add_trigger_modules(module *mdl, const module::external_trigger& trigger) {
    if (trigger.direct_mode) {
        set_trigger_cb_t cb = set_trigger_cb_t();
        cb.cb               = module::trigger_wrapper;
        cb.hdl              = mdl;
        cb.divisor          = trigger.divisor;
        cb.cnt              = 0;
        
        return add_trigger_module(cb);
    }

    worker_key k = { trigger.prio, trigger.affinity_mask, trigger.divisor };
    if (workers.find(k) == workers.end()) {
        // create new worker thread
        kernel_worker *w = new kernel_worker(trigger.prio, trigger.affinity_mask);
        workers[k] = w;

        set_trigger_cb_t cb = set_trigger_cb_t();
        cb.cb               = kernel_worker::trigger_wrapper;
        cb.hdl              = w;
        cb.divisor          = trigger.divisor;
        cb.cnt              = 0;

        add_trigger_module(cb);
    }

    kernel_worker *w = workers[k];
    w->add_module(mdl);
}

//! remove a module to trigger device
/*!
 * \param mdl module to remove
 * \param trigger trigger options
 */
void trigger_base::remove_trigger_modules(module *mdl, 
        const module::external_trigger& trigger) {
    pthread_mutex_lock(&list_lock);

    if (trigger.direct_mode) {
        for (auto it = trigger_cbs.begin(); it != trigger_cbs.end(); ++it) {
            if (((*it).hdl == mdl) && ((*it).divisor == trigger.divisor)) {
                trigger_cbs.erase(it);
                pthread_mutex_unlock(&list_lock);
                return;
            }
        }

        throw str_exception("could not unregister trigger for module %s, "
                "none registered before!", mdl->get_name().c_str());
    }

    worker_key k = { trigger.prio, trigger.affinity_mask, trigger.divisor };
    if (workers.find(k) == workers.end())
        throw str_exception("could not unregister trigger for module %s, "
                "no worker exists!", mdl->get_name().c_str());

    kernel_worker *w = workers[k];
    w->remove_module(mdl);

    pthread_mutex_unlock(&list_lock);
}

//! remove a trigger callback function
/*!
 * \param cb trigger callback
 * \return true on success, false if no ring is present or
 *         callback function pointer is NULL
 */
bool trigger_base::remove_trigger_module(set_trigger_cb_t& cb) {
    if (cb.cb == NULL) {
        klog(info, "could not unregister, callback is NULL\n");
        return false;
    }

    bool found = false;

    pthread_mutex_lock(&list_lock);
    
    for (cb_list_t::iterator it = trigger_cbs.begin(); 
            it != trigger_cbs.end(); ++it) {
        if (it->cb == cb.cb && it->hdl == cb.hdl) {
            trigger_cbs.erase(it);
            found = true;
            break;
        }
    }
    
    pthread_mutex_unlock(&list_lock);

    if(!found) {
        klog(info, "could not unregister, callback %p with handle "
                "%p not in our trigger-list of length %d\n", cb.cb, cb.hdl);
        return false;
    }

    return true;
}

//! trigger all modules in list
/*!
 * \param clk_id clock id to trigger, -1 all clocks
 */
void trigger_base::trigger_modules(int clk_id) {
    pthread_mutex_lock(&list_lock);
    
    for (auto& trigger : trigger_cbs)
        if (((++trigger.cnt) % trigger.divisor) == 0) {
            trigger.cnt = 0;

            (*(trigger.cb))(trigger.hdl);
        }

    pthread_mutex_unlock(&list_lock);
}

int trigger_base::request(int reqcode, void* ptr) {
    int ret = 0;

    switch (reqcode) {
        case MOD_REQUEST_SET_TRIGGER_CB: {
            set_trigger_cb_t *cb = (set_trigger_cb_t *)ptr;
            add_trigger_module(*cb);
            break;
        }
        case MOD_REQUEST_UNSET_TRIGGER_CB: {
            set_trigger_cb_t *cb = (set_trigger_cb_t *)ptr;
            remove_trigger_module(*cb);
            break;
        }
        default:
            ret = -1;
            break;
    }

    return ret;
}

