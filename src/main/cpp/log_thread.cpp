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

#include "robotkernel/log_thread.h"
#include "robotkernel/kernel.h"

#include "robotkernel/config.h"
#include <unistd.h>
#include <string.h>
#ifdef HAVE_SYS_SYSCALL_H
#include <sys/syscall.h>
#endif
#ifdef HAVE_PROCESS_H
#include <process.h>
#endif

using namespace robotkernel;

int gettid() {
#ifdef HAVE_GETTID
    return ::gettid();
#else
#ifdef __NR_gettid
    return syscall( __NR_gettid );
#else
    return 0;
#endif
#endif
}

//! de-/construction
/*!
 * \param size number of buffers in pool
 */
log_thread::log_thread(int pool_size) : 
    runnable(0, 0, "log_thread") 
{
    sync_logging = false;

    int i;
    for (i = 0; i < pool_size; ++i) {
        struct log_pool_object *obj = new struct log_pool_object;
        obj->len = 1024;
        empty_pool.push_back(obj);                
    }

    fix_modname_length = 20;
}

//! destruction, do clean ups
log_thread::~log_thread() {
    // stop thread
    run_flag = false; // give thread chance to exit voluntarily, without timeout
    cond.notify_all();
    tid.join();

    // clean up pools
    while (!full_pool.empty()) {
        struct log_pool_object *obj = full_pool.front();
        full_pool.pop_front();
        printf("%s", obj->buf);
        delete obj;
    }
    while (!empty_pool.empty()) {
        struct log_pool_object *obj = empty_pool.front();
        empty_pool.pop_front();
        delete obj;
    }
}

//! get empty object from log pool
/*!
 * \return empty pool object
 */
struct log_thread::log_pool_object *log_thread::get_pool_object() {
    std::unique_lock<std::mutex> lock(mtx);
    
    if (empty_pool.empty())
        return NULL;

    log_thread::log_pool_object *obj = empty_pool.front();
    empty_pool.pop_front();

    return obj;
}

//! log object to stdout and return to pool
/*!
 * \param obj object to print and to be returned to pool
 */
void log_thread::log(struct log_pool_object *obj) {
    std::unique_lock<std::mutex> lock(mtx);
    
    if(sync_logging) {
        printf("%s", obj->buf);
        empty_pool.push_back(obj);
    } else {
        full_pool.push_back(obj);
        cond.notify_all();
    }
}

//! handler function called if thread is running
void log_thread::run() {
    set_name("rk:log_thread");
    klog(verbose, "log_thread started at tid %d\n", gettid());

    std::unique_lock<std::mutex> lock(mtx);
    
    while (running()) {
        // wait for trigger, this will unlock mutex
        // other threads can now safely add/remove something from _modules
        if (cond.wait_for(lock, std::chrono::seconds(1)) == std::cv_status::timeout)
            continue;

        while (!full_pool.empty()) {
            struct log_pool_object *obj = full_pool.front();
            full_pool.pop_front();

            if(fix_modname_length == 0)
                printf("%s", obj->buf);
            else {
                char* open = strchr(obj->buf, '[');
                char* close = NULL;

                if(open)
                    close = strchr(open, ']');

                if(close) {
                    unsigned int len = close - open;
                    if(len == fix_modname_length + 1)
                        close = NULL;
                    else if(len <= fix_modname_length) {
                        // insert padding
                        printf("%-*.*s%-*.*s%s", (int)(close - obj->buf), 
                                (int)(close - obj->buf), obj->buf, 
                                fix_modname_length + 1 - len, 
                                fix_modname_length + 1 - len, "", close);
                    } else {
                        // truncate
                        printf("%-*.*s%s",
                                (int)((open + fix_modname_length + 1) - obj->buf), 
                                (int)((open + fix_modname_length + 1) - obj->buf), 
                                obj->buf, close);
                    }
                }

                if(!close)
                    // missing closing bracket or length already ok
                    printf("%s", obj->buf);
            }

            empty_pool.push_back(obj);
        }
    }

    klog(verbose, "[log_thread] stopped\n");
}

