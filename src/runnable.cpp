//! robotkernel runnable
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

#include <stdio.h>
#include <signal.h>

#include "robotkernel/runnable.h"
#include "robotkernel/kernel.h"
#include "robotkernel/config.h"
#include "robotkernel/helpers.h"

#ifdef __QNX__
#include <sys/neutrino.h>
#endif

#ifdef __VXWORKS__
#include <vxWorks.h>
#include <taskLib.h>
#endif
        
using namespace std;
using namespace robotkernel;

//! construction with yaml node
/*!
 * \param node yaml node which should contain prio and affinity
 */
runnable::runnable(const YAML::Node& node) {
    run_flag       = false;
    prio           = get_as<int>(node, "prio", 0);
    policy         = SCHED_FIFO;
    affinity_mask  = 0;

    if(node["affinity"]) {
        const YAML::Node& affinity = node["affinit"];
        if (affinity.Type() == YAML::NodeType::Scalar)
            affinity_mask = (1 << affinity.as<int>());
        else if (affinity.Type() == YAML::NodeType::Sequence)
            for (YAML::const_iterator it = affinity.begin(); it != affinity.end(); ++it)
                affinity_mask |= (1 << it->as<int>());
    }
}

//! construction with prio and affinity_mask
/*!
 * \param prio thread priority
 * \param affinity_mask thread cpu affinity mask
 */
runnable::runnable(const int prio, const int affinity_mask)  
    : run_flag(false), policy(SCHED_FIFO), prio(prio), affinity_mask(affinity_mask) {
}
        
//! run wrapper to create posix thread
/*!
 * \param arg thread argument
 * \return NULL
 */
void *runnable::run_wrapper(void *arg) { 
    runnable *r = static_cast<runnable *>(arg);
    
    if (r->prio) {
        struct sched_param param;
        int policy;
        if (pthread_getschedparam(pthread_self(), &policy, &param) != 0)
            klog(warning, "[runnable] run_wrapper: pthread_getschedparam(0x%x):"
                    " %s\n", pthread_self(), strerror(errno));
    
        klog(info, "[runnable] setting thread priority to %d, policy %d\n", 
                r->prio, r->policy);

        param.sched_priority = r->prio;
        if (pthread_setschedparam(pthread_self(), r->policy, &param) != 0)
            klog(warning, "[runnable] run_wrapper: pthread_setschedparam(0x%x,"
                    " %d, %d): %s\n", pthread_self(), r->policy, r->prio, 
                    strerror(errno));
    }

    if (r->affinity_mask) {
#ifdef __VXWORKS__
        taskCpuAffinitySet(taskIdSelf(),  (cpuset_t)r->affinity_mask);
#elif defined __QNX__
        ThreadCtl(_NTO_TCTL_RUNMASK, (void *)r->affinity_mask);
#else
#ifdef HAVE_CPU_SET_T
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        for (unsigned i = 0; i < (sizeof(r->affinity_mask)*8); ++i) 
            if (r->affinity_mask & (1 << i))
                CPU_SET(i, &cpuset);

        klog(info, "[runnable] setting cpu affinity mask %#x\n", r->affinity_mask);

        int ret = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
        if (ret != 0)
            klog(warning, "[runnable] run_wrapper: pthread_setaffinity(%p, %#x):"
                    " %d %s\n", (void*)pthread_self(), r->affinity_mask, 
                    ret, strerror(ret));
#endif
#endif
    }

    r->run(); 
    return NULL; 
};
        
//! handler function called if thread is running
void runnable::run() {
    klog(error, "[runnable] run() not implemented!\n");
}

//! run thread
void runnable::start() {
    if (run_flag)
        return;
    
    run_flag = true;
    pthread_create(&tid, NULL, run_wrapper, this);
}

//! stop thread
void runnable::stop() {
    if (!run_flag)
        return;

    run_flag = false;
    pthread_join(tid, NULL);
}

//! set priority
/*!
 * \param prio new max prio
 */
void runnable::set_prio(int prio) { 
    struct sched_param param;
    int policy;
    if (pthread_getschedparam(tid, &policy, &param) != 0) {
        klog(warning, "[runnable] pthread_getschedparam(0x%x): %s\n", 
                tid, strerror(errno));
        return;
    }

    policy = SCHED_FIFO;
    this->prio = prio;
    param.sched_priority = prio;
    if (pthread_setschedparam(tid, policy, &param) != 0) {
        klog(warning, "[runnable] pthread_setschedparam(0x%x, %d, %d): %s\n", 
                tid, policy, prio, strerror(errno));
        return;
    }
}
        
//! set affinity mask
/*!
 * \param mask new cup affinity mask 
 */
void runnable::set_affinity_mask(int mask) {
#ifdef __VXWORKS__
    taskCpuAffinitySet(taskIdSelf(),  (cpuset_t)mask);
#elif defined __QNX__
    ThreadCtl(_NTO_TCTL_RUNMASK, (void *)mask);
#else
#ifdef HAVE_CPU_SET_T
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    for (unsigned i = 0; i < (sizeof(mask)*8); ++i) 
        if (mask & (1 << i))
            CPU_SET(i, &cpuset);
    
    if (pthread_setaffinity_np(tid, sizeof(cpu_set_t), &cpuset) != 0)
        klog(warning, "[runnable] pthread_setaffinity(0x%x, 0x%02f): %s\n", 
                tid, mask, strerror(errno));
#endif
#endif
}
        
//! set thread name
void runnable::set_name(std::string name) {
#ifdef HAVE_PTHREAD_SETNAME_NP
    klog(verbose, "[runnable] setting thread name to %s\n", name.c_str());
	pthread_setname_np(tid, name.c_str());
#endif
}

