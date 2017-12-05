//! robotkernel module interface definition
/*!
 * (C) Robert Burger <robert.burger@dlr.de>
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

#ifndef ROBOTKERNEL__MODULE_INTF_H
#define ROBOTKERNEL__MODULE_INTF_H

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

// -----------------------------------------------------------------------------------
// module symbols
// -----------------------------------------------------------------------------------

extern "C" const char *state_to_string(module_state_t state);
extern "C" module_state_t string_to_state(const char *state);

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

//! module tick callback
/*!
 * \param hdl module handle
 */
typedef void (*mod_tick_t)(MODULE_HANDLE hdl);

#endif // ROBOTKERNEL__MODULE_INTF_H

