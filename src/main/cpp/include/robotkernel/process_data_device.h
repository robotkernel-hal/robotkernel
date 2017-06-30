//! robotkernel process_data_device
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

#ifndef __PROCESS_DATA_H__
#define __PROCESS_DATA_H__

#include <vector>
#include <string>
#include <map>
#include <list>
#include <functional>

#include "robotkernel/device.h"
#include "robotkernel/rk_type.h"

namespace robotkernel {
#ifdef EMACS
}
#endif

//! process data management class 
/*!
 * This class describe managed process data by the robotkernel. It also uses
 * a double buffer support to ensure data consistency.
 */
class process_data_device :
    public device
{
    private:
        process_data_device();     //!< prevent default construction

    public:
        //! construction
        /*!
         * \param length byte length of process data
         * \param owner name of owning module
         * \param name process data name
         * \param process_data_definition yaml-string representating the structure of the pd
         */
        process_data_device(size_t length, std::string owner, std::string name, 
                std::string process_data_definition = "") :
            device(owner, name, "pd"), length(length),  
            process_data_definition(process_data_definition)
            {
                data_index = 0;

                data[0].resize(length);
                data[1].resize(length);
            }

        //! return current read buffer
        const std::vector<uint8_t>& get_read_buffer() 
        {
            written = false;
            return data[data_index];
        }

        //! return current read buffer
        std::vector<uint8_t>& get_write_buffer()
        {
            int write_index = (data_index + 1) % 2;
            return data[write_index];
        }

        //! swaps the buffers and mark as written
        void swap_buffers() 
        {
            data_index = (data_index + 1) % 2;
            written = true;
        }

        //! returns if there was a write since last read
        bool written_since_last_read() {
            return written;
        }

    public:
        const size_t length;
        const std::string process_data_definition;

    private:
        std::vector<uint8_t> data[2];
        int data_index;
        bool written;
};

typedef std::shared_ptr<process_data_device> sp_process_data_device_t;
typedef std::map<std::string, sp_process_data_device_t> process_data_device_map_t;

#ifdef EMACS
{
#endif
} // namespace robotkernel

#endif // __PROCESS_DATA_H__

