//! ÜBER-control kernel
/*!
  $Id$
  */

#include <stdio.h>
#include "kernel_worker.h"
#include "kernel.h"

using namespace robotkernel;

kernel_worker::kernel_worker(int prio, int affinity_mask, int divisor) 
    : runnable(prio, affinity_mask), _divisor(divisor), _divisor_cnt(0), cnt(0) {
    pthread_cond_init(&_cond, NULL);
    pthread_mutex_init(&_mutex, NULL);

    // start worker thread
    start();

    klog(info, "[kernel_worker] created with prio %d\n", prio);
};
        
//! destruction
kernel_worker::~kernel_worker() {
    // stop worker thread
    stop();

    klog(info, "[kernel_worker] destructed\n");
}

//! add module to trigger
/*!
 * \param mdl module to add
 */
void kernel_worker::add_module(module *mdl) {
    // push to module list
    pthread_mutex_lock(&_mutex);
    _modules.push_back(mdl);
    pthread_mutex_unlock(&_mutex);

    klog(info, "[kernel_worker] added module %s\n", mdl->name.c_str());
}

//! remove module to trigger
/*!
 * \param mdl module to remove
 * \return true if worker module list is empty
 */
bool kernel_worker::remove_module(module *mdl) {
    // remove from module list
    pthread_mutex_lock(&_mutex);
    _modules.remove(mdl);
    pthread_mutex_unlock(&_mutex);
    
    klog(info, "[kernel_worker] removed module %s\n", mdl->name.c_str());

    return (_modules.size() == 0);
}

//! handler function called if thread is running
void kernel_worker::run() {
    klog(info, "[kernel_worker] running worker thread\n");

    // set initial priority
    runnable::set_prio(_prio);

    // lock mutex cause we access _modules
    pthread_mutex_lock(&_mutex);

    while (_running) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_nsec += 1E7;
        if (ts.tv_nsec > 1E9) {
            ts.tv_nsec = ts.tv_nsec % (long int)1E9;
            ts.tv_sec++;
        }

        // wait for trigger, this will unlock mutex
        // other threads can now safely add/remove something from _modules
        int ret = pthread_cond_timedwait(&_cond, &_mutex, &ts);

        if (ret != 0 || (++_divisor_cnt%_divisor != 0))
            continue;

        // reset divisor count
        _divisor_cnt = 0;

        for (module_list_t::iterator it = _modules.begin();
                it != _modules.end(); ++it) {
            (*it)->trigger();
        }
    }
        
    // unlock mutex cause we have accessed _modules
    pthread_mutex_unlock(&_mutex);

    klog(info, "[kernel_worker] finished worker thread\n");
}

