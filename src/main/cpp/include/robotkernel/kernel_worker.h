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

#ifndef ROBOTKERNEL__KERNEL_WORKER_H
#define ROBOTKERNEL__KERNEL_WORKER_H

#include <string>
#include <condition_variable>
#include <mutex>
#include <stdio.h>
#include <list>
#include "robotkernel/module.h"
#include "robotkernel/runnable.h"

namespace robotkernel {
#ifdef EMACS
}
#endif

//! kernel worker thread
/*!
 */
class kernel_worker : public runnable {
    private:
        kernel_worker();
        kernel_worker(const kernel_worker&);             // prevent copy-construction
        kernel_worker& operator=(const kernel_worker&);  // prevent assignment

    public:
        //! default construction
        kernel_worker(int prio = 60, int affinity_mask = 0xFF);

        //! destruction
        ~kernel_worker();

        //! add module to trigger
        /*!
         * \param mdl module to add
         */
        void add_module(module *mdl);

        //! remove module to trigger
        /*!
         * \param mdl module to remove
         * \return true if worker module list is empty
         */
        bool remove_module(module *mdl);

        //! trigger wrapper
        static void trigger_wrapper(void *ptr) {
            ((kernel_worker *)ptr)->tick();
        };

        //! trigger worker
        void tick();

        //! handler function called if thread is running
        void run();

    private:
        typedef std::list<module *> module_list_t;

        module_list_t modules;  //!< list of module triggered by worker

        std::mutex              mtx;  //!< ipc lock
        std::condition_variable cond; //!< ipc condition
};

//! trigger callback
/*!
 * \param hdl module handle
 */
inline void kernel_worker::tick() {
    // trigger worker thread
    cond.notify_one();
}

#ifdef EMACS
{
#endif
} // namespace robotkernel

#endif // ROBOTKERNEL__KERNEL_WORKER_H

