//! robotkernel lttng tracepoints
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

#undef TRACEPOINT_PROVIDER
#define TRACEPOINT_PROVIDER robotkernel

#undef TRACEPOINT_INCLUDE
#define TRACEPOINT_INCLUDE "lttng_tp.h"

#if !defined(ROBOTKERNEL_LTTNG_TP_H) || defined(TRACEPOINT_HEADER_MULTI_READ)
#define ROBOTKERNEL_LTTNG_TP_H

#include <lttng/tracepoint.h>

TRACEPOINT_EVENT(
    robotkernel,
    lttng_log,
    TP_ARGS(
        char*, log_msg_arg
    ),
    TP_FIELDS(
        ctf_string(log_msg, log_msg_arg)
    )
)

TRACEPOINT_EVENT(
    robotkernel,
    dump_log,
    TP_ARGS(
        char*, log_msg_arg
    ),
    TP_FIELDS(
        ctf_string(log_msg, log_msg_arg)
    )
)

#endif // ROBOTKERNEL_LTTNG_TP_H

#include <lttng/tracepoint-event.h>

