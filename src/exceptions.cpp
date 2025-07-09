//! robotkernel interface definition
/*!
 * (C) Robert Burger <robert.burger@dlr.de>
 */

// vim: set expandtab softtabstop=4 shiftwidth=4
// -*- mode: c++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil; -*- 

/*
 * This file is part of robotkernel.
 *
 * robotkernel is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 * 
 * robotkernel is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with robotkernel; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "robotkernel/exceptions.h"
#include "robotkernel/config.h"

#ifdef HAVE_EXECINFO_H
#include <execinfo.h>
#endif

using namespace std;
using namespace robotkernel;

exception_tracer::exception_tracer() {
#ifdef HAVE_EXECINFO_H 
    std::stringstream buffer;

    void * array[25];
    int nSize = backtrace(array, 25);
    char ** symbols = backtrace_symbols(array, nSize);

    for (int i = 0; i < nSize; i++)
        buffer << symbols[i] << std::endl;

    free(symbols);
    _backtrace = buffer.str();
#else
    _backtrace = "backtrace is not supported";
#endif
}

//signal_translator<segmentation_fault_exception> g_obj_segmentation_fault_exception_translator;
//signal_translator<floating_point_exception>     g_obj_floating_point_exception_translator;

