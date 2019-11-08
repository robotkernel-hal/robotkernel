//! robotkernel stream class
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

#ifndef ROBOTKERNEL__STREAM_H
#define ROBOTKERNEL__STREAM_H

#include <string>
#include "robotkernel/device.h"

namespace robotkernel {
#ifdef EMACS
}
#endif

class stream :
    public device
{
    private:
        stream(const stream&);             // prevent copy-construction
        stream& operator=(const stream&);  // prevent assignment

    public:
        //! stream construction
        /*!
         * \param[in] owner     Name of owning module/bridge/service_provider...
         * \param[in] name      Name of stream.
         */
        stream(
                const std::string& owner, 
                const std::string& name)
            : device(owner, name, "stream") {}

        //! destruction
        virtual ~stream() {};

        //! Character stream based data read
        /*!
         * \param[out] buf       Pointer to a buffer which will return data.
         * \param[in] bufsize    Size of data buffer.
         *
         * \return Size of read bytes.
         */
        virtual size_t read(void* buf, size_t bufsize) = 0;
        
        //! Character stream based data write
        /*!
         * \param[in] buf       Pointer to a buffer which holds data.
         * \param[in] bufsize   Size of data buffer.
         *
         * \return Size of written bytes.
         */
        virtual size_t write(void* buf, size_t bufsize) = 0;
};

class serial_stream : 
    public stream
{
    public:
        typedef enum character_size {
            char_size_5 = 5,
            char_size_6 = 6,
            char_size_7 = 7,
            char_size_8 = 8
        } character_size_t;

        typedef enum parity {
            parity_off = 0,
            parity_even,
            parity_odd
        } parity_t;

        typedef enum stopbits {
            stopbits_1  = 1,
            stopbits_2
        } stopbits_t;

    public:
        //! serial_stream construction
        /*!
         * \param[in] owner     Name of owning module/bridge/service_provider...
         * \param[in] name      Name of stream.
         */
        serial_stream(
                const std::string& owner, 
                const std::string& name)
            : stream(owner, name) {}

        //! destruction
        virtual ~serial_stream() {};

        //! Set stream device baudrate
        /*!
         * \param[in] baudrate  New baudrate to set.
         */
        virtual void set_baudrate(int baudrate);
        
        //! Get stream device baudrate
        /*!
         * \return Actual baudrate.
         */
        virtual int get_baudrate() const;

        //! Set serial port settings
        /*!
         * \param[in] char_size     Character bit size.
         * \param[in] par           Parity setting.
         * \param[in] stopbits      Number of Stopbits.
         */
        virtual void set_port_settings(const character_size_t& char_size, 
                const parity_t& par, const stopbits_t& stopbits);

        //! Get serial port settings
        /*!
         * \param[out] char_size     Character bit size.
         * \param[out] par           Parity setting.
         * \param[out] stopbits      Number of Stopbits.
         */
        virtual void get_port_settings(character_size_t& char_size, 
                parity_t& par, stopbits_t& stopbits) const;
};

typedef std::shared_ptr<stream> sp_stream_t;
typedef std::map<std::string, sp_stream_t> stream_map_t;

#ifdef EMACS 
{
#endif
} // namespace robotkernel

#endif // ROBOTKERNEL__STREAM_H

