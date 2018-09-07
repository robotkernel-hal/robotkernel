//! robotkernel rt helpers
/*!
 * (C) Robert Burger <robert.burger@dlr.de>
 * (C) Jan Cremer <jan.cremer@dlr.de>
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

#include "robotkernel/rt_helper.h"
#include "robotkernel/kernel.h"

#ifdef __VXWORKS__
#include <vxWorks.h>
#include <taskLib.h>
#endif

void robotkernel::set_priority(int priority, int policy) {
    if (!priority)
        return;
    struct sched_param param;
    klog(info, "setting thread priority to %d, policy %d\n", priority, policy);

    param.sched_priority = priority;
    if (pthread_setschedparam(pthread_self(), policy, &param) != 0)
        klog(warning, "setPriority: pthread_setschedparam(0x%x, %d, %d): %s\n",
                pthread_self(), policy, priority, strerror(errno));
}

void robotkernel::set_affinity_mask(int affinity_mask) {
    if (!affinity_mask)
        return;
#ifdef __VXWORKS__
    taskCpuAffinitySet(taskIdSelf(), (cpuset_t) affinity_mask);
#elif defined __QNX__
    ThreadCtl(_NTO_TCTL_RUNMASK, (void *) affinity_mask);
#else
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    for (unsigned i = 0; i < (sizeof(affinity_mask) * 8); ++i)
        if (affinity_mask & (1 << i))
            CPU_SET(i, &cpuset);

    klog(info, "setting cpu affinity mask %#x\n", affinity_mask);

    int ret = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    if (ret != 0)
        klog(warning, "setAffinityMask: pthread_setaffinity(%p, %#x): %d %s\n", 
                (void *) pthread_self(), affinity_mask, ret, strerror(ret));
#endif
}

void robotkernel::set_thread_name(std::thread& tid, const std::string& thread_name) {
    set_thread_name(tid.native_handle(), thread_name);
}

void robotkernel::set_thread_name(pthread_t tid, const std::string& thread_name) {
    char buffer[17];
    snprintf(buffer, 16, "%s", thread_name.c_str());

#if defined(HAVE_PTHREAD_SETNAME_NP_3)
    pthread_setname_np(tid, buffer, (void *)0);
#elif defined(HAVE_PTHREAD_SETNAME_NP_2)
    pthread_setname_np(tid, buffer);
#endif
}

void robotkernel::set_thread_name(const std::string& thread_name) {
#if defined(HAVE_PTHREAD_SETNAME_NP_3) || defined(HAVE_PTHREAD_SETNAME_NP_2)
    set_thread_name(pthread_self(), thread_name);
#elif defined(HAVE_PTHREAD_SETNAME_NP_1)
    char buffer[17];
    snprintf(buffer, 16, "%s", thread_name.c_str());

    pthread_setname_np(buffer);
#endif
}

