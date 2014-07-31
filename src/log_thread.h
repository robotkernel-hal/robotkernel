//! robotkernel log thread
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

#ifndef __LOG_THREAD_H__
#define __LOG_THREAD_H__

#include <string>
#include "runnable.h"

#ifdef __VXWORKS__
#undef log
#endif

namespace robotkernel {

//! logging thread with pool
class log_thread : public runnable {
    public:
        struct log_pool_object {
            char buf[1024];
            size_t len;
        };

        //! de-/construction
        /*!
         * \param size number of buffers in pool
         */
        log_thread(int pool_size = 1000);
        ~log_thread();

        //! get empty object from log pool
        /*!
         * \return empty pool object
         */
        struct log_pool_object *get_pool_object();

        //! log object to stdout and return to pool
        /*!
         * \param obj object to print and to be returned to pool
         */
        void log(struct log_pool_object *obj);

        //! handler function called if thread is running
        void run();

    private:
        // sync objects
        pthread_cond_t _cond;
        pthread_mutex_t _mutex;

        // log pools
        std::list<struct log_pool_object *> _empty_pool;
        std::list<struct log_pool_object *> _full_pool;
};

} // namespace robotkernel

#endif // __LOG_THREAD_H__

