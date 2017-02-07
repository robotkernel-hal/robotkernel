//
// Created by crem_ja on 2/2/17.
//

#ifndef ROBOTKERNEL_RT_HELPER_H
#define ROBOTKERNEL_RT_HELPER_H

#include <pthread.h>


namespace robotkernel {
    
    void setPriority(int priority, int policy = SCHED_FIFO);
    void setAffinityMask(int affinity_mask);

}

#endif //PROJECT_RT_HELPER_H
