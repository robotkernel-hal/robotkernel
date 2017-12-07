//! robotkernel kernel worker
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

#include <stdio.h>
#include <sstream>
#include "robotkernel/kernel_worker.h"
#include "robotkernel/kernel.h"

using namespace string_util;
using namespace robotkernel;
using namespace std;

kernel_worker::kernel_worker(int prio, int affinity_mask) :
    runnable(prio, affinity_mask, format_string("kernel_worker.prio_%d."
                "affinity_mask_%d", prio, affinity_mask))
{
    // start worker thread
    start();
    klog(info, "[kernel_worker] created with prio %d\n", prio);
};
        
//! destruction
kernel_worker::~kernel_worker() {
    // stop worker thread
    stop();
    klog(info, "[kernel_worker] destructed\n");
}

//! add module to trigger
/*!
 * \param mdl module to add
 */
void kernel_worker::add_module(module *mdl) {
    std::unique_lock<std::mutex> lock(mtx);

    // push to module list
    modules.push_back(mdl);

    klog(info, "[kernel_worker] added module %s\n", mdl->get_name().c_str());
}

//! remove module to trigger
/*!
 * \param mdl module to remove
 * \return true if worker module list is empty
 */
bool kernel_worker::remove_module(module *mdl) {
    std::unique_lock<std::mutex> lock(mtx);
    
    // remove from module list
    modules.remove(mdl);
    
    klog(info, "[kernel_worker] removed module %s\n", mdl->get_name().c_str());

    return (modules.size() == 0);
}

//! handler function called if thread is running
void kernel_worker::run() {
    klog(info, "[kernel_worker] running worker thread\n");

    // lock mutex cause we access _modules
    std::unique_lock<std::mutex> lock(mtx);
    
    stringstream name;
    name << "rk:kernel_worker ";
    for (module_list_t::iterator it = modules.begin();
            it != modules.end(); ++it)
        name << (*it)->get_name();
    set_name(name.str());

    while (running()) {
        // wait for trigger, this will unlock mutex
        // other threads can now safely add/remove something from _modules
        if (cond.wait_for(lock, std::chrono::seconds(1)) == std::cv_status::timeout)
            continue;

        for (module_list_t::iterator it = modules.begin();
                it != modules.end(); ++it) {
            (*it)->tick();
        }
    }

    klog(info, "[kernel_worker] finished worker thread\n");
}

