//! robotkernel interface definition
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

#include "robotkernel/dump_log.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sstream>
#include <pthread.h>

#include <list>

#include "robotkernel/config.h"

#if (HAVE_LTTNG_UST == 1)
#include "robotkernel/dump_log_tp.h"
#endif

#include "robotkernel/char_ringbuffer.h"

using namespace std;

//static pthread_mutex_t _dump_log_mutex = PTHREAD_MUTEX_INITIALIZER;
static char_ringbuffer* _dump_log_buffer = NULL;
static unsigned int _dump_log_len = 0;
static unsigned int _do_ust = 0;

static double ts2double(struct timespec* ts) {
	return (double)ts->tv_sec + (ts->tv_nsec / 1e9);
}
static double get_time() {
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return ts2double(&ts);
}

void format_time(char* ts_buffer, int ts_buffer_len) {
	double timestamp = get_time();
	time_t seconds = (time_t)timestamp;
	double mseconds = (timestamp - (double)seconds) * 1e6;
	struct tm btime;
	localtime_r(&seconds, &btime);
	strftime(ts_buffer, ts_buffer_len, "%Y-%m-%d %H:%M:%S", &btime);
	int sl = strlen(ts_buffer);
	snprintf(ts_buffer + sl, ts_buffer_len - sl, ".%06.0f", mseconds);
}

void dump_log_set_len(unsigned int len, unsigned int do_ust) {
	_dump_log_len = len;
	_do_ust = do_ust;

        if(_dump_log_len > 0) {
            if(!_dump_log_buffer)
                _dump_log_buffer = new char_ringbuffer(_dump_log_len);
            else
                _dump_log_buffer->set_size(_dump_log_len);
        }
}

void dump_log(const char* format, ...) {
	if(!_dump_log_len)
		return;
	va_list ap;
	va_start(ap, format);
	vdump_log(format, ap);
	va_end(ap);
}
void vdump_log(const char* format, va_list nap) {
	if(!_dump_log_len && !_do_ust)
		return;

	char msg[1024];
	va_list ap;
	va_copy(ap, nap);
	vsnprintf(msg, 1024, format, ap);
	va_end(ap);
	
#if (HAVE_LTTNG_UST == 1)
	if(_do_ust) {
		tracepoint(rk_trace, dump_log, msg);
	}
#endif
	
	if(_dump_log_len && _dump_log_buffer) {
		char ts_buffer[64];
		// only need to have menaingful timestamp: pthread_mutex_lock(&_dump_log_mutex);
		format_time(ts_buffer, 64);
                _dump_log_buffer->write(string(ts_buffer) + " " + string(msg));
		// pthread_mutex_unlock(&_dump_log_mutex);
	}
}

std::string dump_log_dump(bool keep) {
        if(!_dump_log_buffer)
            return string();
        return _dump_log_buffer->get(keep);
}

