//! robotkernel example module
/*!
  $Id$
 */

#include "robotkernel/kernel.h"
#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <iostream>

using namespace std;
using namespace robotkernel;

extern "C" {

#define MODNAME "[module_example] "

typedef struct example {
    char* config;
    module_state_t state;
} example_t;

//! cyclic process data read
/*!
  \param hdl module handle
  \param buf process data buffer 
  \param bufsize size of process data buffer
  \return size of read bytes
 */
size_t mod_read(MODULE_HANDLE hdl, void* buf, size_t bufsize) {
//    example_t* ex = (example_t*)hdl;

    return bufsize;
}

//! cyclic process data write
/*!
  \param hdl module handle
  \param buf process data buffer
  \param bufsize size of process data buffer 
  \return size of written bytes
 */
size_t mod_write(MODULE_HANDLE hdl, void* buf, size_t bufsize) {
//    example_t* ex = (example_t*)hdl;

    return bufsize;
}

//! configures module
/*!
  \param name module name
  \param config configure string
  \return handle on success, NULL otherwise
*/
MODULE_HANDLE mod_configure(const char* name, const char* config) {
    klog(info, MODNAME "build by: %s@%s\n", BUILD_USER, BUILD_HOST);
    klog(info, MODNAME "build date: %s\n", BUILD_DATE);

    example_t* ex = new example_t();
    if (!ex) {
        klog(error, MODNAME "cannot allocate memory");
        goto ErrorExit;
    }

    // copy config string 1
    ex->config = strdup(config);

    // set state to init
    ex->state = module_state_init;

    return (MODULE_HANDLE)ex;

ErrorExit:
    if (ex) {
        if (ex->config)
            free(ex->config);

        delete ex;
    }

    return (MODULE_HANDLE)NULL;
}

//! unconfigure module
/*!
  \param hdl module handle
  \return success or failure
 */
int mod_unconfigure(MODULE_HANDLE hdl) {
    example_t* ex = (example_t*)hdl;

    if (ex) {
        if (ex->config)
            free(ex->config);

        delete ex;
    }

    return 0;
}

//! set module state machine to defined state
/*!
  \param hdl module handle
  \param state requested state
  \return success or failure
 */
int mod_set_state(MODULE_HANDLE hdl, module_state_t state) {
    example_t* ex = (example_t*)hdl;

    switch (state) {
        case module_state_init: 
        case module_state_preop:
        case module_state_safeop:
        case module_state_op:
            break;
        default:
            // invalid state 
            return -1;
    }

    // set actual state 
    ex->state = state;

    return 0;
}

//! get module state machine state
/*!
  \param hdl module handle
  \return current state
 */
module_state_t mod_get_state(MODULE_HANDLE hdl) {
    example_t* ex = (example_t*)hdl;

    // return state
    return ex->state;
}

} // extern "C" {

