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

using namespace robotkernel;

//! add a trigger callback function
/*!
 * \param cb trigger callback
 * \return true on success, false if no ring is present or
 *         callback function pointer is NULL
 */
bool trigger_base::add_trigger_module(set_trigger_cb_t& cb) {
    if (cb.cb == NULL) {
        klog(info, "could not register, callback is NULL\n");
        return false;
    }
    
    pthread_mutex_lock(&list_lock);
    trigger_cbs.push_back(cb);
    pthread_mutex_unlock(&list_lock);
    
    return true;
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
    
    for (cb_list_t::iterator it = trigger_cbs.begin();
            it != trigger_cbs.end(); ++it) {
        if (clk_id == -1 || clk_id == it->clk_id)
            (*(it->cb))(it->hdl);
    }

    pthread_mutex_unlock(&list_lock);
}

