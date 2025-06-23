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

// public headers
#include "robotkernel/config.h"

// private headers
#include "log_thread.h"
#include "kernel.h"

#include <unistd.h>
#include <string.h>
#ifdef HAVE_SYS_SYSCALL_H
#include <sys/syscall.h>
#endif
#ifdef HAVE_PROCESS_H
#include <process.h>
#endif

using namespace robotkernel;

int _gettid() {
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

    clock_gettime(CLOCK_REALTIME, &obj->ts);
    return obj;
}

static bool log_thread_value = false;

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

        if (!log_thread_value) {
            log_thread_value = true;
            cond.notify_all();
        }
    }
}

const static std::string ANSI_RESET = "\u001B[0m";
const static std::string ANSI_BLACK = "\u001B[30m";
const static std::string ANSI_RED = "\u001B[31m";
const static std::string ANSI_GREEN = "\u001B[32m";
const static std::string ANSI_YELLOW = "\u001B[33m";
const static std::string ANSI_BLUE = "\u001B[34m";
const static std::string ANSI_PURPLE = "\u001B[35m";
const static std::string ANSI_CYAN = "\u001B[36m";
const static std::string ANSI_WHITE = "\u001B[37m";

//! handler function called if thread is running
void log_thread::run() {
    set_name("rk:log_thread");
    kernel::instance.log(verbose, "log_thread started at tid %d\n", _gettid());
    char tmp_buf[2*1024];

    std::unique_lock<std::mutex> lock(mtx);
    
    while (running()) {
        // wait for trigger, this will unlock mutex
        // other threads can now safely add/remove something from _modules
        if (!log_thread_value) {
            if (cond.wait_for(lock, std::chrono::seconds(1)) == std::cv_status::timeout) {
                continue;
            }
        }

        log_thread_value = false;

        while (!full_pool.empty()) {
            struct log_pool_object *obj = full_pool.front();
            full_pool.pop_front();
            lock.unlock();

            struct tm timeinfo;
            double timestamp = (double)obj->ts.tv_sec + (obj->ts.tv_nsec / 1e9);
            time_t seconds = (time_t)timestamp;
            double mseconds = (timestamp - (double)seconds) * 1000.;
            localtime_r(&seconds, &timeinfo);
            strftime(&tmp_buf[0], sizeof(tmp_buf), "%F %T", &timeinfo);


            int len = strlen(&tmp_buf[0]);
            snprintf(&tmp_buf[len], sizeof(tmp_buf) - len, ".%03.0f ", mseconds);
            len = strlen(&tmp_buf[0]);
            snprintf(&tmp_buf[len], sizeof(tmp_buf) - len, "%s %s", 
                    kernel::instance.ll_to_string(obj->lvl).c_str(), obj->buf);

            char* have_error = strstr(&tmp_buf[0], "ERR");
            if (have_error) {
                printf("%s", ANSI_RED.c_str());
            }
            
            char* have_warning = strstr(&tmp_buf[0], "WARN");
            if (have_warning) {
                printf("%s", ANSI_YELLOW.c_str());
            }

            char* have_verbose = strstr(&tmp_buf[0], "VERB");
            if (have_verbose) {
                printf("%s", ANSI_GREEN.c_str());
            }

            if(fix_modname_length == 0)
                printf("%s", &tmp_buf[0]);
            else {
                char* open = strchr(&tmp_buf[0], '[');
                char* close = NULL;

                if(open)
                    close = strchr(open, ']');

                if(close) {
                    unsigned int len = close - open;
                    if(len == fix_modname_length + 1)
                        close = NULL;
                    else if(len <= fix_modname_length) {
                        // insert padding
                        printf("%-*.*s%-*.*s%s", (int)(close - &tmp_buf[0]), 
                                (int)(close - &tmp_buf[0]), &tmp_buf[0], 
                                fix_modname_length + 1 - len, 
                                fix_modname_length + 1 - len, "", close);
                    } else {
                        // truncate
                        printf("%-*.*s%s",
                                (int)((open + fix_modname_length + 1) - &tmp_buf[0]), 
                                (int)((open + fix_modname_length + 1) - &tmp_buf[0]), 
                                &tmp_buf[0], close);
                    }
                }

                if(!close)
                    // missing closing bracket or length already ok
                    printf("%s", &tmp_buf[0]);

                if (have_error || have_warning || have_verbose) {
                    printf("%s", ANSI_RESET.c_str());
                }
            }

            lock.lock();
            empty_pool.push_back(obj);
        }
    }
}

