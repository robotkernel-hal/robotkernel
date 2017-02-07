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
#include "robotkernel/rt_helper.h"

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
        const YAML::Node& affinity = node["affinity"];
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
    : policy(SCHED_FIFO), prio(prio), affinity_mask(affinity_mask), run_flag(false) {
}
        
//! run wrapper to create posix thread
/*!
 * \param arg thread argument
 * \return NULL
 */
void *runnable::run_wrapper(void *arg) { 
    runnable *r = static_cast<runnable *>(arg);
    setPriority(r->prio);
    setAffinityMask(r->affinity_mask);
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
    this->prio = prio;
    if (running())
        setPriority(prio);
}
        
//! set affinity mask
/*!
 * \param mask new cup affinity mask 
 */
void runnable::set_affinity_mask(int mask) {
    this->affinity_mask = mask;
    if (running())
        setAffinityMask(mask);
}
        
//! set thread name
void runnable::set_name(std::string name) {
#ifdef HAVE_PTHREAD_SETNAME_NP
    klog(verbose, "[runnable] setting thread name to %s\n", name.c_str());
	pthread_setname_np(tid, name.c_str());
#endif
}

