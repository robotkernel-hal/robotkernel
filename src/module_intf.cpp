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

#include "robotkernel/module_intf.h"

sercos_service_transfer::sercos_service_transfer() {
    slave_id = 0; 
    idn = 0;
    element = SSE_NONE;
    direction = SSD_DRIVE_TO_MASTER;
    buflen = 0;
    buf = NULL;
    scerror = 0;
    receipt = 0;
    change = 0;
}

sercos_service_transfer::sercos_service_transfer(uint32_t slave_id, uint16_t idn) {
    this->slave_id = slave_id; 
    this->idn = idn;
    element = SSE_NONE;
    direction = SSD_DRIVE_TO_MASTER;
    buflen = 0;
    buf = NULL;
    scerror = 0;
    receipt = 0;
    change = 0;
}

