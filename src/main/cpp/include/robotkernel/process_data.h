//! robotkernel process_data
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

#ifndef __ROBOTKERNEL__PROCESS_DATA_H__
#define __ROBOTKERNEL__PROCESS_DATA_H__

#include <vector>
#include <string>
#include <map>
#include <list>
#include <functional>
#include <atomic>
#include <memory>
#include <cstring>

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
class process_data :
    public device
{
    private:
        /*
         * front (read) buffer index    0x03
         * back (write) buffer index    0x0C
         * flipping     buffer index    0x30
         * written flag                 0x80
         */
        mutable std::atomic_uint_fast8_t indices;
        std::vector<uint8_t> data[3];

        static const uint8_t front_buffer_mask  = 0x03;
        static const uint8_t back_buffer_mask   = 0x0C;
        static const uint8_t flip_buffer_mask   = 0x30;
        static const uint8_t written_mask       = 0x80;

    private:
        process_data();     //!< prevent default construction

    public:
        //! construction
        /*!
         * \param length byte length of process data
         * \param owner name of owning module
         * \param name process data name
         * \param process_data_definition yaml-string representating the structure of the pd
         */
        process_data(size_t length, std::string owner, std::string name, 
                std::string process_data_definition = "") :
            device(owner, name, "pd"), length(length),  
            process_data_definition(process_data_definition)
            {
                // initilize indices with front(0), back(1), flip(2)
                indices.store((0x00) | (0x01 << 2) | (0x02 << 4));

                data[0].resize(length);
                data[1].resize(length);
                data[2].resize(length);
            }

        //! return current read buffer
        const std::vector<uint8_t>& front_buffer() {
            int idx = indices.load(std::memory_order_consume) & front_buffer_mask;
            printf("%d -> ", idx);
            return data[idx];
        }

        //! return current read buffer
        std::vector<uint8_t>& back_buffer() {
            int idx = (indices.load(std::memory_order_consume) & back_buffer_mask) >> 2;
            printf("<- %d ", idx);
            return data[idx];
        }

        //! swaps the buffers 
        void swap_front() {
            uint8_t new_indices, old_indices = indices.load(std::memory_order_consume);

            do {
                if (!(old_indices & written_mask))
                    return; // nothing new, do not swap

                new_indices = 
                    ((old_indices & front_buffer_mask) << 4)        |
                    ((old_indices & back_buffer_mask))              |
                    ((old_indices & flip_buffer_mask) >> 4);
            } while(!indices.compare_exchange_weak(old_indices, new_indices,
                        std::memory_order_release, std::memory_order_relaxed));
        }

        //! swaps the buffers and mark as written
        void swap_back() {
            uint8_t new_indices, old_indices = indices.load(std::memory_order_consume);

            do {
                new_indices = 
                    ((old_indices & written_mask) | written_mask)   | 
                    ((old_indices & front_buffer_mask))             |
                    ((old_indices & back_buffer_mask) << 2)         |
                    ((old_indices & flip_buffer_mask) >> 2);
            } while(!indices.compare_exchange_weak(old_indices, new_indices,
                        std::memory_order_release, std::memory_order_relaxed));
        }

        //! Write data to back buffer.
        /*!
         * \param[in] buf   Buffer with data to write to internal back buffer.
         * \param[in] len   Length of data in buffer.
         */
        void write(uint8_t *buf, size_t len) {
            if (len > length)
                throw std::exception();

            auto& tmp_buf = back_buffer();
            std::memcpy(&tmp_buf[0], buf, len);

            swap_back();
        }

        //! Read data from front buffer.
        /*!
         * \param[in] buf   Buffer with data to write to from internal front buffer.
         * \param[in] len   Length of data in buffer.
         */
        void read(uint8_t *buf, size_t len) {
            if (len > length)
                throw std::exception();

            swap_front();

            auto& tmp_buf = front_buffer();
            std::memcpy(buf, &tmp_buf[0], len);
        }

    public:
        const size_t length;
        const std::string process_data_definition;
};

typedef std::shared_ptr<process_data> sp_process_data_t;
typedef std::map<std::string, sp_process_data_t> process_data_map_t;

#ifdef EMACS
{
#endif
} // namespace robotkernel

#endif // __ROBOTKERNEL__PROCESS_DATA_H__

