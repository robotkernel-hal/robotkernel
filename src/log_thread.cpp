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

#include "robotkernel/log_thread.h"

using namespace robotkernel;

//! de-/construction
/*!
 * \param size number of buffers in pool
 */
log_thread::log_thread(int pool_size) : runnable(0, 0) {
    pthread_mutex_init(&_mutex, NULL);
    pthread_cond_init(&_cond, NULL);

    int i;
    for (i = 0; i < pool_size; ++i) {
        struct log_pool_object *obj = new struct log_pool_object;
        obj->len = 1024;
        _empty_pool.push_back(obj);                
    }

    // start log thread
    start();
}

//! destruction, do clean ups
log_thread::~log_thread() {
    // stop thread
    stop();

    // clean up pools
    while (!_full_pool.empty()) {
        struct log_pool_object *obj = _full_pool.front();
        _full_pool.pop_front();
        printf("%s", obj->buf);
        delete obj;
    }
    while (!_empty_pool.empty()) {
        struct log_pool_object *obj = _empty_pool.front();
        _empty_pool.pop_front();
        delete obj;
    }
    
    // destroy sync objects
    pthread_cond_destroy(&_cond);
    pthread_mutex_destroy(&_mutex);
}

//! get empty object from log pool
/*!
 * \return empty pool object
 */
struct log_thread::log_pool_object *log_thread::get_pool_object() {
    if (_empty_pool.empty())
        return NULL;

    pthread_mutex_lock(&_mutex);
    struct log_pool_object *obj = _empty_pool.front();
    _empty_pool.pop_front();
    pthread_mutex_unlock(&_mutex);

    return obj;
}

//! log object to stdout and return to pool
/*!
 * \param obj object to print and to be returned to pool
 */
void log_thread::log(struct log_pool_object *obj) {
    pthread_mutex_lock(&_mutex);
    _full_pool.push_back(obj);
    pthread_cond_signal(&_cond);
    pthread_mutex_unlock(&_mutex);
}

//! handler function called if thread is running
void log_thread::run() {
    while (_running) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_nsec += 1E7;
        if (ts.tv_nsec > 1E9) {
            ts.tv_nsec = ts.tv_nsec % (long int)1E9;
            ts.tv_sec++;
        }

        pthread_mutex_lock(&_mutex);
        // wait for trigger, this will unlock mutex
        // other threads can now safely add/remove something from _modules
        int ret = pthread_cond_timedwait(&_cond, &_mutex, &ts);
        pthread_mutex_unlock(&_mutex);
        if (ret != 0)
            continue;


        while (!_full_pool.empty()) {
            pthread_mutex_lock(&_mutex);
            struct log_pool_object *obj = _full_pool.front();
            _full_pool.pop_front();

            printf("%s", obj->buf);

            _empty_pool.push_back(obj);
            pthread_mutex_unlock(&_mutex);
        }
    }
}

