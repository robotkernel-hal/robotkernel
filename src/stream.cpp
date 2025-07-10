//! robotkernel base class for stream devices
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
#include "robotkernel/stream.h"

// private headers
#include "kernel.h"

using namespace std;
using namespace robotkernel;

//! Set stream device baudrate
/*!
 * \param[in] baudrate  New baudrate to set.
 */
void serial_stream::set_baudrate(int baudrate) {
    throw runtime_error(string_printf("setting baudrate not supported!"));
}

//! Get stream device baudrate
/*!
 * \return Actual baudrate.
 */
int serial_stream::get_baudrate() const {
    throw runtime_error(string_printf("getting baudrate not supported!"));
}
        
//! Set serial port settings
/*!
 * \param[in] char_size     Character bit size.
 * \param[in] par           Parity setting.
 * \param[in] stopbits      Number of Stopbits.
 */
void serial_stream::set_port_settings(const character_size_t& char_size, 
        const parity_t& par, const stopbits_t& stopbits) {
    throw runtime_error(string_printf("set port settings not supported!"));
}

//! Get serial port settings
/*!
 * \param[out] char_size     Character bit size.
 * \param[out] par           Parity setting.
 * \param[out] stopbits      Number of Stopbits.
 */
void serial_stream::get_port_settings(character_size_t& char_size, 
        parity_t& par, stopbits_t& stopbits) const {
    throw runtime_error(string_printf("get port settings not supported!"));
}

