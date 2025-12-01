//! robotkernel trigger_base class
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

// public headers
#include "robotkernel/robotkernel.h"
#include "robotkernel/trigger.h"
#include "robotkernel/trigger_base.h"

using namespace robotkernel;

/* Get trigger device from robotkernel and register us as trigger. */
void trigger_base::aquire(void) {
    dev = robotkernel::get_device<robotkernel::trigger>(dev_name);
    dev->add_trigger(shared_from_this());
}

/* Remove us as trigger devce. */
void trigger_base::release(void) {
    if (dev) {
        dev->remove_trigger(shared_from_this());
        dev = nullptr;    
    }
}
