//! robotkernel base class for stream devices
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

// public headers
#include "robotkernel/stream.h"

// private headers
#include "kernel.h"

#include "string_util/string_util.h"

using namespace std;
using namespace robotkernel;
using namespace string_util;

//! Set stream device baudrate
/*!
 * \param[in] baudrate  New baudrate to set.
 */
void serial_stream::set_baudrate(int baudrate) {
    throw string_util::str_exception("setting baudrate not supported!");
}

//! Get stream device baudrate
/*!
 * \return Actual baudrate.
 */
int serial_stream::get_baudrate() const {
    throw string_util::str_exception("getting baudrate not supported!");
}
        
//! Set serial port settings
/*!
 * \param[in] char_size     Character bit size.
 * \param[in] par           Parity setting.
 * \param[in] stopbits      Number of Stopbits.
 */
void serial_stream::set_port_settings(const character_size_t& char_size, 
        const parity_t& par, const stopbits_t& stopbits) {
    throw string_util::str_exception("set port settings not supported!");
}

//! Get serial port settings
/*!
 * \param[out] char_size     Character bit size.
 * \param[out] par           Parity setting.
 * \param[out] stopbits      Number of Stopbits.
 */
void serial_stream::get_port_settings(character_size_t& char_size, 
        parity_t& par, stopbits_t& stopbits) const {
    throw string_util::str_exception("get port settings not supported!");
}

