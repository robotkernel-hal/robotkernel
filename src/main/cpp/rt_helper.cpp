//
// Created by crem_ja on 2/2/17.
//

#include "robotkernel/rt_helper.h"
#include "robotkernel/kernel.h"

void robotkernel::setPriority(int priority, int policy) {
    if (!priority)
        return;
    struct sched_param param;
    klog(info, "setting thread priority to %d, policy %d\n", priority, policy);

    param.sched_priority = priority;
    if (pthread_setschedparam(pthread_self(), policy, &param) != 0)
        klog(warning, "setPriority: pthread_setschedparam(0x%x, %d, %d): %s\n",
             pthread_self(), policy, priority, strerror(errno));
}

void robotkernel::setAffinityMask(int affinity_mask) {
    if (!affinity_mask)
        return;
#ifdef __VXWORKS__
    taskCpuAffinitySet(taskIdSelf(), (cpuset_t) affinity_mask);
#elif defined __QNX__
    ThreadCtl(_NTO_TCTL_RUNMASK, (void *) affinity_mask);
#elif defined HAVE_CPU_SET_T
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