//! robotkernel module interface definition
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

#ifndef __MODULE_INTF_H__
#define __MODULE_INTF_H__

#include <unistd.h>
#include <stdint.h>
#include <list>
#include <stdio.h>

#define MODULE_HANDLE void*

const static uint16_t module_state_init     = 0x0001;
const static uint16_t module_state_preop    = 0x0002;
const static uint16_t module_state_safeop   = 0x0004;
const static uint16_t module_state_op       = 0x0008;
const static uint16_t module_state_boot     = 0x0010;
const static uint16_t module_state_error    = 0x8000;

typedef uint16_t module_state_t;

#define GEN_STATE(from, to) \
    ((uint32_t)from << 16 | to)

#define DEFINE_STATE(from, to) \
    const static uint32_t from ## _2_ ## to = GEN_STATE(module_state_ ## from, module_state_ ## to)

DEFINE_STATE(boot, boot);
DEFINE_STATE(boot, init);
DEFINE_STATE(boot, preop);
DEFINE_STATE(boot, safeop);
DEFINE_STATE(boot, op);
DEFINE_STATE(init, boot);
DEFINE_STATE(init, init);
DEFINE_STATE(init, preop);
DEFINE_STATE(init, safeop);
DEFINE_STATE(init, op);
DEFINE_STATE(preop, boot);
DEFINE_STATE(preop, init);
DEFINE_STATE(preop, preop);
DEFINE_STATE(preop, safeop);
DEFINE_STATE(preop, op);
DEFINE_STATE(safeop, boot);
DEFINE_STATE(safeop, init);
DEFINE_STATE(safeop, preop);
DEFINE_STATE(safeop, safeop);
DEFINE_STATE(safeop, op);
DEFINE_STATE(op, boot);
DEFINE_STATE(op, init);
DEFINE_STATE(op, preop);
DEFINE_STATE(op, safeop);
DEFINE_STATE(op, op);

typedef uint32_t transition_t;

#define __MOD_REQUEST(m, r, s)          (((s) << 24) | ((m) << 16) | (r))
#define __MOD_REQUEST_TYPE(s)           (sizeof(s))

#define MOD_REQUEST_MAGIC               0x14
#define MOD_REQUEST(x, s)               \
    __MOD_REQUEST((MOD_REQUEST_MAGIC), (x), __MOD_REQUEST_TYPE(s))

#define MOD_REQUEST_REGISTER_SERVICES   MOD_REQUEST(0x000B, void*)
#define MOD_REQUEST_SLAVE_ID2ADDRESS    MOD_REQUEST(0x000C, int)

// -----------------------------------------------------------------------------
// get process data pointers
//
typedef struct process_data {
    uint32_t slave_id; //! [in]     device slave_id
    void *pd;          //! [in/out] process data pointer
    size_t len;        //! [in/out] process data length
} process_data_t;

#define MOD_REQUEST_GET_PDIN            MOD_REQUEST(0x0001, process_data_t)
#define MOD_REQUEST_GET_PDOUT           MOD_REQUEST(0x0002, process_data_t)
#define MOD_REQUEST_GET_PD_COOKIE       MOD_REQUEST(0x000A, uint64_t *)
#define MOD_REQUEST_GET_CMD_DELAY       MOD_REQUEST(0x0110, int32_t)

// -----------------------------------------------------------------------------
// set pd, used for double/multiple buffering of pd values

//! set pd
typedef struct set_pd {
    uint64_t pd_cookie; //! [in]     process data cookie
    uint32_t cnt;       //! [in]     number of indices
    process_data_t *pd; //! [in/out] field with indices
} set_pd_t;

#define MOD_REQUEST_SET_PDIN            MOD_REQUEST(0x0003, set_pd_t)
#define MOD_REQUEST_SET_PDOUT           MOD_REQUEST(0x0004, set_pd_t)

// ----------------------------------------------------------------------------
// trigger callbacks
#if old
typedef struct set_trigger_cb {
    void (*cb)(MODULE_HANDLE hdl);
    MODULE_HANDLE hdl;
    int divisor;    //! rate divisor
    int cnt;        //! rate counter
} set_trigger_cb_t;
typedef std::list<set_trigger_cb_t> cb_list_t;

#define MOD_REQUEST_SET_TRIGGER_CB      MOD_REQUEST(0x0007, set_trigger_cb_t)
#define MOD_REQUEST_UNSET_TRIGGER_CB    MOD_REQUEST(0x0008, set_trigger_cb_t)
#define MOD_REQUEST_GET_CLOCKSRC_CNT    MOD_REQUEST(0x0009, int)
#define MOD_REQUEST_GET_TRIGGER_INTERVAL  MOD_REQUEST(0x000D, double)
#define MOD_REQUEST_SET_TRIGGER_INTERVAL  MOD_REQUEST(0x000E, double)
#define MOD_REQUEST_TRIGGERED_BY          MOD_REQUEST(0x000F, char*)
#define MOD_REQUEST_SHIFT_NEXT_TRIGGER    MOD_REQUEST(0x0010, double)
#endif
// -----------------------------------------------------------------------------------
// module features
// -----------------------------------------------------------------------------------

static const int MODULE_FEAT_PD         = 0x00000001;
static const int MODULE_FEAT_READ       = 0x00000004;
static const int MODULE_FEAT_WRITE      = 0x00000008;
static const int MODULE_FEAT_TRIGGER    = 0x00000010;
static const int MODULE_FEAT_CANOPEN    = 0x00001000;
static const int MODULE_FEAT_ETHERCAT   = 0x00002000;
static const int MODULE_FEAT_SERCOS     = 0x00004000;

#define MOD_REQUEST_GET_MODULE_FEAT                 MOD_REQUEST(0x2000, int)
#define MOD_REQUEST_GET_CFG_PATH                    MOD_REQUEST(0x2001, char*)

// -----------------------------------------------------------------------------------
// module symbols
// -----------------------------------------------------------------------------------

extern "C" const char *state_to_string(module_state_t state);
extern "C" module_state_t string_to_state(const char *state);

//! cyclic process data read
/*!
  \param hdl module handle
  \param buf process data buffer
  \param bufsize size of process data buffer
  \return size of read bytes
 */
typedef ssize_t (*mod_read_t)(MODULE_HANDLE hdl, void* buf, size_t bufsize);

//! cyclic process data write
/*!
  \param hdl module handle
  \param buf process data buffer
  \param bufsize size of process data buffer
  \return size of written bytes
 */
typedef ssize_t (*mod_write_t)(MODULE_HANDLE hdl, void* buf, size_t bufsize);

//! configures module
/*!
  \param name module name
  \param config configure file
  \return handle on success, NULL otherwise
 */
typedef MODULE_HANDLE (*mod_configure_t)(const char* name, const char* config);

//! unconfigure module
/*!
  \param hdl module handle
  \return success or failure
 */
typedef int (*mod_unconfigure_t)(MODULE_HANDLE hdl);

//! set module state machine to defined state
/*!
  \param hdl module handle
  \param state requested state
  \return success or failure
 */
typedef int (*mod_set_state_t)(MODULE_HANDLE hdl, module_state_t state);

//! get module state machine state
/*!
  \param hdl module handle
  \return current state
 */
typedef module_state_t (*mod_get_state_t)(MODULE_HANDLE hdl);

//! send a request to module
/*!
  \param hdl module handle
  \param reqcode request code
  \param ptr pointer to request structure
  \return success or failure
 */
typedef int (*mod_request_t)(MODULE_HANDLE hdl, int reqcode, void* ptr);

//! module trigger callback
/*!
 * \param hdl module handle
 */
typedef void (*mod_trigger_t)(MODULE_HANDLE hdl);

//! module trigger callback
/*!
 * \param hdl module handle
 * \param slave_id slave id to trigger
 */
typedef void (*mod_trigger_slave_id_t)(MODULE_HANDLE hdl, uint32_t slave_id);

#endif // __MODULE_INTF_H__

