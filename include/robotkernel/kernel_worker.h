//! robotkernel kernel
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

#ifndef __KERNEL_WORKER_H__
#define __KERNEL_WORKER_H__

#include <string>
#include <pthread.h>
#include <stdio.h>
#include <list>
#include "robotkernel/module.h"
#include "robotkernel/runnable.h"

namespace robotkernel {

//! kernel worker thread
/*!
*/
class kernel_worker : public runnable {
    public:
        //! list of module triggered by worker
        typedef std::list<module *> module_list_t;
        module_list_t _modules;

        //! default construction
        kernel_worker(int prio = 60, int affinity_mask = 0xFF, int divisor = 1);

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

        //! trigger callback
        /*!
         * \param hdl module handle
         */
        static void kernel_worker_trigger(MODULE_HANDLE hdl);

        //! handler function called if thread is running
        void run();

        //! divisor
        int _divisor;
        int _divisor_cnt;

        //! ipc memebers
        pthread_mutex_t _mutex;
        pthread_cond_t _cond;

        int cnt;
};

//! trigger callback
/*!
 * \param hdl module handle
 */
inline void kernel_worker::kernel_worker_trigger(MODULE_HANDLE hdl) {
    // hdl is our this pointer
    kernel_worker *worker = (kernel_worker *)hdl;

    // trigger worker thread
    pthread_cond_signal(&worker->_cond);
}

} // namespace robotkernel

#endif // __KERNEL_WORKER_H__

