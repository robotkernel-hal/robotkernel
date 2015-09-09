//! robotkernel module base
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

#include "robotkernel/module_base.h"

using namespace robotkernel;

//! log to kernel logging facility
void module_base::log(loglevel lvl, const char *format, ...) {
    kernel& k = *kernel::get_instance();
    struct log_thread::log_pool_object *obj;

    if (ll > this->ll)
        goto log_exit;

    if ((obj = k.rk_log.get_pool_object()) != NULL) {
        // only ifempty log pool avaliable!
        struct tm timeinfo;
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        double timestamp = (double)ts.tv_sec + (ts.tv_nsec / 1e9);
        time_t seconds = (time_t)timestamp;
        double mseconds = (timestamp - (double)seconds) * 1000.;
        localtime_r(&seconds, &timeinfo);
        strftime(obj->buf, sizeof(obj->buf), "%F %T", &timeinfo);


        int len = strlen(obj->buf);
        snprintf(obj->buf + len, sizeof(obj->buf) - len, ".%03.0f ", mseconds);
        len = strlen(obj->buf);

        snprintf(obj->buf + len, sizeof(obj->buf) - len, "%s ", k.ll_to_string(ll).c_str());
        len = strlen(obj->buf);

        // format argument list
        va_list args;
        va_start(args, format);
        vsnprintf(obj->buf + len, obj->len - len, format, args);
        va_end(args);
        k.rk_log.log(obj);
    }
    
log_exit:
    va_list args;
    va_start(args, format);
    vdump_log(format, args);
    va_end(args);
}

