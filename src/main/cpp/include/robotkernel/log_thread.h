//! robotkernel log thread
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

#ifndef ROBOTKERNEL__LOG_THREAD_H
#define ROBOTKERNEL__LOG_THREAD_H

#include <string>
#include "robotkernel/runnable.h"

#ifdef __VXWORKS__
#undef log
#endif

namespace robotkernel {
#ifdef EMACS
}
#endif

//! logging thread with pool
class log_thread : public runnable {
    private:
        log_thread(const log_thread&);             // prevent copy-construction
        log_thread& operator=(const log_thread&);  // prevent assignment

    public:
        struct log_pool_object {
            char buf[1024];
            size_t len;
        };
    
        unsigned int fix_modname_length;
        bool sync_logging;

        //! de-/construction
        /*!
         * \param size number of buffers in pool
         */
        log_thread(int pool_size = 100000);
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
        pthread_cond_t cond;
        pthread_mutex_t mutex;

        // log pools
        std::list<struct log_pool_object *> empty_pool;
        std::list<struct log_pool_object *> full_pool;
};

#ifdef EMACS
{
#endif
} // namespace robotkernel

#endif // ROBOTKERNEL__LOG_THREAD_H

