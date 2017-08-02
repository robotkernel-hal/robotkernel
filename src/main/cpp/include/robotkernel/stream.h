//! robotkernel stream class
/*!
 * author: Robert Burger <robert.burger@dlr.de>
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

#ifndef _ROBOTKERNEL__STREAM_H_
#define _ROBOTKERNEL__STREAM_H_

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
        ~stream() {};

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

typedef std::shared_ptr<stream> sp_stream_t;
typedef std::map<std::string, sp_stream_t> stream_map_t;

#ifdef EMACS 
{
#endif
} // namespace robotkernel

#endif // _ROBOTKERNEL__STREAM_H_

