//! robotkernel process_data
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
            device(owner, name, "pd"), pd_cookie(0), length(length),  
            process_data_definition(process_data_definition)
            { }

        //! Get a pointer to the a data buffer which we can write next, has to be
        //  completed with calling \link push \endlink
        virtual uint8_t* next() = 0;

        //! Get a pointer to the last written data without consuming it, 
        //  which will be available on calling \link pop \endlink
        virtual uint8_t* peek() = 0;

        //! Get a pointer to the actual read data. This call
        //  will consume the data.
        virtual uint8_t* pop() = 0;

        //! Pushes write data buffer to available on calling \link next \endlink.
        virtual void push() {};

        //! Write data to buffer.
        /*!
         * \param[in] offset    Process data offset from beginning of the buffer.
         * \param[in] buf       Buffer with data to write to internal back buffer.
         * \param[in] len       Length of data in buffer.
         * \param[in] do_push   Push the buffer to set it to the actual one.
         */
        virtual void write(off_t offset, uint8_t *buf, size_t len, bool do_push = true) = 0;

        //! Read data from buffer.
        /*!
         * \param[in] offset    Process data offset from beginning of the buffer.
         * \param[in] buf       Buffer with data to write to from internal front buffer.
         * \param[in] len       Length of data in buffer.
         * \param[in] do_pop    Pop the buffer and consume it.
         */
        virtual void read(off_t offset, uint8_t *buf, size_t len, bool do_pop = true) = 0;

    public:
        uint64_t pd_cookie;
        const size_t length;
        const std::string process_data_definition;
};

//! process data management class with single buffering
/*!
 * This class describe managed process data by the robotkernel. It also uses
 * a double buffer support to ensure data consistency.
 */
class single_buffer :
    public process_data
{
    private:
        std::vector<uint8_t> data;

    private:
        single_buffer();     //!< prevent default construction

    public:
        //! construction
        /*!
         * \param length byte length of process data
         * \param owner name of owning module
         * \param name process data name
         * \param process_data_definition yaml-string representating the structure of the pd
         */
        single_buffer(size_t length, std::string owner, std::string name, 
                std::string process_data_definition = "") :
            process_data(length, owner, name, process_data_definition)
            {
                data.resize(length);
            }
        
        //! Get a pointer to the a data buffer which we can write next, has to be
        //  completed with calling \link push \endlink
        uint8_t* next() {
            return (uint8_t *)&data[0];
        }

        //! Get a pointer to the last written data without consuming it, 
        //  which will be available on calling \link pop \endlink
        uint8_t* peek() {
            return (uint8_t *)&data[0];
        }

        //! Get a pointer to the actual read data. This call
        //  will consume the data.
        uint8_t* pop() {
            return (uint8_t *)&data[0];
        }

        //! Write data to buffer.
        /*!
         * \param[in] offset    Process data offset from beginning of the buffer.
         * \param[in] buf       Buffer with data to write to internal back buffer.
         * \param[in] len       Length of data in buffer.
         * \param[in] do_push   Push the buffer to set it to the actual one.
         */
        void write(off_t offset, uint8_t *buf, size_t len, bool do_push = true) {
            if ((offset + len) > length)
                throw std::exception();

            std::memcpy(&data[offset], buf, len);
        }

        //! Read data from buffer.
        /*!
         * \param[in] offset    Process data offset from beginning of the buffer.
         * \param[in] buf       Buffer with data to write to from internal front buffer.
         * \param[in] len       Length of data in buffer.
         * \param[in] do_pop    Pop the buffer and consume it.
         */
        void read(off_t offset, uint8_t *buf, size_t len, bool do_pop = true) {
            if ((offset + len) > length)
                throw std::exception();
            
            std::memcpy(buf, &data[offset], len);
        }
};

//! process data management class with triple buffering
/*!
 * This class describe managed process data by the robotkernel. It also uses
 * a double buffer support to ensure data consistency.
 */
class triple_buffer :
    public process_data
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
        triple_buffer();     //!< prevent default construction

    public:
        //! construction
        /*!
         * \param length byte length of process data
         * \param owner name of owning module
         * \param name process data name
         * \param process_data_definition yaml-string representating the structure of the pd
         */
        triple_buffer(size_t length, std::string owner, std::string name, 
                std::string process_data_definition = "") :
            process_data(length, owner, name, process_data_definition)
            {
                // initilize indices with front(0), back(1), flip(2)
                indices.store((0x00) | (0x01 << 2) | (0x02 << 4));

                data[0].resize(length);
                data[1].resize(length);
                data[2].resize(length);
            }
        
        //! Get a pointer to the a data buffer which we can write next, has to be
        //  completed with calling \link push \endlink
        uint8_t* next() {
            auto& tmp_buf = back_buffer();
            return (uint8_t *)&tmp_buf[0];
        }

        //! Get a pointer to the last written data without consuming it, 
        //  which will be available on calling \link pop \endlink
        uint8_t* peek() {
            auto& tmp_buf = flip_buffer();
            return (uint8_t *)&tmp_buf[0];
        }

        //! Get a pointer to the actual read data. This call
        //  will consume the data.
        uint8_t* pop() {
            swap_front();

            auto& tmp_buf = front_buffer();
            return (uint8_t *)&tmp_buf[0];
        }

        //! Pushes write data buffer to available on calling \link next \endlink.
        void push() {
            swap_back();
        }

        //! Write data to buffer.
        /*!
         * \param[in] offset    Process data offset from beginning of the buffer.
         * \param[in] buf       Buffer with data to write to internal back buffer.
         * \param[in] len       Length of data in buffer.
         * \param[in] do_push   Push the buffer to set it to the actual one.
         */
        void write(off_t offset, uint8_t *buf, size_t len, bool do_push = true) {
            if ((offset + len) > length)
                throw std::exception();

            auto& tmp_buf = back_buffer();
            std::memcpy(&tmp_buf[offset], buf, len);

            if (do_push)
                swap_back();
        }

        //! Read data from buffer.
        /*!
         * \param[in] offset    Process data offset from beginning of the buffer.
         * \param[in] buf       Buffer with data to write to from internal front buffer.
         * \param[in] len       Length of data in buffer.
         * \param[in] do_pop    Pop the buffer and consume it.
         */
        void read(off_t offset, uint8_t *buf, size_t len, bool do_pop = true) {
            if ((offset + len) > length)
                throw std::exception();

            if (do_pop) {
                swap_front();

                auto& tmp_buf = front_buffer();
                std::memcpy(buf, &tmp_buf[offset], len);
            } else {
                auto& tmp_buf = flip_buffer();
                std::memcpy(buf, &tmp_buf[offset], len);
            }
        }

    private:
        //! return current read buffer
        const std::vector<uint8_t>& front_buffer() {
            int idx = indices.load(std::memory_order_consume) & front_buffer_mask;
            return data[idx];
        }

        //! return current flip buffer
        std::vector<uint8_t>& flip_buffer() {
            int idx = (indices.load(std::memory_order_consume) & flip_buffer_mask) >> 4;
            return data[idx];
        }
        //! return current write buffer
        std::vector<uint8_t>& back_buffer() {
            int idx = (indices.load(std::memory_order_consume) & back_buffer_mask) >> 2;
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
};

//! process data management class with pointer buffer
/*!
 * This class describe managed process data by the robotkernel. It also uses
 * a double buffer support to ensure data consistency.
 */
class pointer_buffer :
    public process_data
{
    private:
        uint8_t *ptr;

    private:
        pointer_buffer();     //!< prevent default construction

    public:
        //! construction
        /*!
         * \param length byte length of process data
         * \param owner name of owning module
         * \param name process data name
         * \param process_data_definition yaml-string representating the structure of the pd
         */
        pointer_buffer(size_t length, uint8_t *ptr, std::string owner, std::string name, 
                std::string process_data_definition = "") :
            process_data(length, owner, name, process_data_definition), ptr(ptr)
            { }
        
        //! Get a pointer to the a data buffer which we can write next, has to be
        //  completed with calling \link push \endlink
        uint8_t* next() {
            return ptr;
        }

        //! Get a pointer to the last written data without consuming it, 
        //  which will be available on calling \link pop \endlink
        uint8_t* peek() {
            return ptr;
        }

        //! Get a pointer to the actual read data. This call
        //  will consume the data.
        uint8_t* pop() {
            return ptr;
        }

        //! Write data to buffer.
        /*!
         * \param[in] offset    Process data offset from beginning of the buffer.
         * \param[in] buf       Buffer with data to write to internal back buffer.
         * \param[in] len       Length of data in buffer.
         * \param[in] do_push   Push the buffer to set it to the actual one.
         */
        void write(off_t offset, uint8_t *buf, size_t len, bool do_push = true) {
            if ((offset + len) > length)
                throw std::exception();

            std::memcpy(&ptr[offset], buf, len);
        }

        //! Read data from buffer.
        /*!
         * \param[in] offset    Process data offset from beginning of the buffer.
         * \param[in] buf       Buffer with data to write to from internal front buffer.
         * \param[in] len       Length of data in buffer.
         * \param[in] do_pop    Pop the buffer and consume it.
         */
        void read(off_t offset, uint8_t *buf, size_t len, bool do_pop = true) {
            if ((offset + len) > length)
                throw std::exception();

            std::memcpy(buf, &ptr[offset], len);
        }
};


typedef std::shared_ptr<process_data> sp_process_data_t;
typedef std::shared_ptr<single_buffer> sp_single_buffer_t;
typedef std::shared_ptr<triple_buffer> sp_triple_buffer_t;
typedef std::map<std::string, sp_process_data_t> process_data_map_t;

#ifdef EMACS
{
#endif
} // namespace robotkernel

#endif // __ROBOTKERNEL__PROCESS_DATA_H__

