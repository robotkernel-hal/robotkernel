//! robotkernel module base
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

#include "robotkernel/module_base.h"

using namespace robotkernel;

//! log to kernel logging facility
void module_base::log(loglevel lvl, const char *format, ...) {
    char buf[1024];

    // format argument list
    va_list args;
    va_start(args, format);
    vsnprintf(buf, 1024, format, args);

    if (logmode == kernel) {
        klog(lvl, "[%s|%s] %s", modname.c_str(), name.c_str(), buf);
    } else {
        if(((1 << (lvl-1)) & ll_bits))
            mlog(lvl, "[%s|%s] %s", modname.c_str(), name.c_str(), buf);        
    }
}

