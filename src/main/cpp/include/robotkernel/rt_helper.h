//! robotkernel rt helpers
/*!
 * author: Jan Cremer <jan.cremer@dlr.de>
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


#ifndef ROBOTKERNEL_RT_HELPER_H
#define ROBOTKERNEL_RT_HELPER_H

#if defined (HAVE_PTHREAD_SETNAME_NP_3) || defined (HAVE_PTHREAD_SETNAME_NP_2) || defined (HAVE_PTHREAD_SETNAME_NP_1)
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#endif

#include <pthread.h>

#include <string>

namespace robotkernel {
#ifdef EMACS
}
#endif

void set_priority(int priority, int policy = SCHED_FIFO);
void set_affinity_mask(int affinity_mask);

void set_thread_name(pthread_t tid, const std::string& thread_name);
void set_thread_name(const std::string& thread_name);

#ifdef EMACS
{
#endif
}

#endif //PROJECT_RT_HELPER_H

