//! robotkernel process_data
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

#ifndef ROBOTKERNEL__PROCESS_DATA_H
#define ROBOTKERNEL__PROCESS_DATA_H

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

#include "string_util/string_util.h"

namespace robotkernel {
#ifdef EMACS
}
#endif

//! process data provider class
/*!
 * derive from this class, if you want to register yourself as
 * process data provider
 */
class pd_provider {
    public:
        std::string provider_name;

        pd_provider(const std::string& provider_name) : 
            provider_name(provider_name) {}
};

//! process data consumer class
/*!
 * derive from this class, if you want to register yourself as
 * process data consumer
 */
class pd_consumer {
    public:
        std::string consumer_name;

        pd_consumer(const std::string& consumer_name) : 
            consumer_name(consumer_name) {}
};

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
        process_data(size_t length, const std::string& owner, const std::string& name, 
                const std::string& process_data_definition = "", const std::string& clk_device = "") :
            device(owner, name, "pd"), pd_cookie(0), length(length), clk_device(clk_device),
            process_data_definition(process_data_definition)
            { }

        //! Get a pointer to the a data buffer which we can write next, has to be
        //  completed with calling \link push \endlink
        /*
         * \param[in] hash      hash value, get it with set_provider!
         */
        virtual uint8_t* next(const std::size_t& hash) {
            if (    (provider_hash != hash))
                throw str_exception_tb("permission denied to write to %s", id().c_str());

            return nullptr;
        }

        //! Get a pointer to the last written data without consuming it, 
        //  which will be available on calling \link pop \endlink
        virtual uint8_t* peek() = 0;

        //! Get a pointer to the actual read data. This call
        //  will consume the data.
        /*
         * \param[in] hash      hash value, get it with set_consumer!
         */
        virtual uint8_t* pop(const std::size_t& hash) {
            if (consumer_hash != hash)
                throw str_exception_tb("permission denied to pop %s", id().c_str());

            return nullptr;
        }

        //! Pushes write data buffer to available on calling \link next \endlink.
        /*
         * \param[in] hash      hash value, get it with set_provider!
         */
        virtual void push(const std::size_t& hash) {
            if (provider_hash != hash)
                throw str_exception_tb("permission denied to push %s", id().c_str());
        }

        //! Write data to buffer.
        /*!
         * \param[in] hash      hash value, get it with set_provider!
         * \param[in] offset    Process data offset from beginning of the buffer.
         * \param[in] buf       Buffer with data to write to internal back buffer.
         * \param[in] len       Length of data in buffer.
         * \param[in] do_push   Push the buffer to set it to the actual one.
         */
        virtual void write(const std::size_t& hash, off_t offset, uint8_t *buf, 
            size_t len, bool do_push = true) 
        {
            if (provider_hash != hash)
                throw str_exception_tb("permission denied to write to %s", id().c_str());
        }

        //! Read data from buffer.
        /*!
         * \param[in] hash      hash value, get it with set_consumer!
         * \param[in] offset    Process data offset from beginning of the buffer.
         * \param[in] buf       Buffer with data to write to from internal front buffer.
         * \param[in] len       Length of data in buffer.
         * \param[in] do_pop    Pop the buffer and consume it.
         */
        virtual void read(const std::size_t& hash, off_t offset, uint8_t *buf, 
                size_t len, bool do_pop = true) 
        {
            if (    do_pop &&
                    (consumer_hash != hash))
                throw str_exception_tb("permission denied to pop %s", id().c_str());
        }

        //! Returns true if new data has been written
        virtual bool new_data() { return true; }

        //! set data provider thread, only thread allowed to write and push
        std::size_t set_provider(std::shared_ptr<robotkernel::pd_provider> mod) {
            if (    (provider != nullptr) && 
                    (provider != mod))
                throw str_exception_tb("cannot set provider, already have one!");

            provider = mod;
            provider_hash = std::hash<std::shared_ptr<robotkernel::pd_provider> >{}(mod);

            return provider_hash;
        }

        //! reset data provider thread
        void reset_provider(const std::size_t& hash) {
            if (provider_hash != hash)
                throw str_exception_tb("cannot reset provider: you are not the provider!");

            provider_hash = 0;
            provider = nullptr;
        }

        //!< set main consumer thread, only thread allowed to pop
        std::size_t set_consumer(std::shared_ptr<robotkernel::pd_consumer> mod) {
            if (    (consumer != nullptr) &&
                    (consumer != mod))
                throw str_exception_tb("cannot set consumer: already have one!");

            consumer = mod;
            consumer_hash = std::hash<std::shared_ptr<robotkernel::pd_consumer> >{}(mod);

            return consumer_hash;
        }
        
        //! reset main consumer thread
        void reset_consumer(const std::size_t& hash) {
            if (consumer_hash != hash)
                throw str_exception_tb("cannot reset consumer: you are not the consumer!");

            consumer_hash = 0;
            consumer = nullptr;
        }

    public:
        uint64_t pd_cookie;
        const size_t length;
        const std::string clk_device;
        const std::string process_data_definition;

        std::shared_ptr<robotkernel::pd_provider> provider;
        std::size_t provider_hash;
        std::shared_ptr<robotkernel::pd_consumer> consumer;
        std::size_t consumer_hash;
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
        single_buffer(size_t length, const std::string& owner, const std::string& name, 
                const std::string& process_data_definition = "", const std::string& clk_device = "") :
            process_data(length, owner, name, process_data_definition, clk_device)
            {
                data.resize(length);
            }
        
        //! Get a pointer to the a data buffer which we can write next, has to be
        //  completed with calling \link push \endlink
        /*
         * \param[in] hash      hash value, get it with set_provider!
         */
        uint8_t* next(const std::size_t& hash) {
            process_data::next(hash);
            
            return (uint8_t *)&data[0];
        }

        //! Get a pointer to the last written data without consuming it, 
        //  which will be available on calling \link pop \endlink
        uint8_t* peek() {
            return (uint8_t *)&data[0];
        }

        //! Get a pointer to the actual read data. This call
        //  will consume the data.
        /*
         * \param[in] hash      hash value, get it with set_consumer!
         */
        uint8_t* pop(const std::size_t& hash) {
            return (uint8_t *)&data[0];
        }

        //! Write data to buffer.
        /*!
         * \param[in] hash      hash value, get it with set_provider!
         * \param[in] offset    Process data offset from beginning of the buffer.
         * \param[in] buf       Buffer with data to write to internal back buffer.
         * \param[in] len       Length of data in buffer.
         * \param[in] do_push   Push the buffer to set it to the actual one.
         */
        void write(const std::size_t& hash, off_t offset, uint8_t *buf, 
                size_t len, bool do_push = true) 
        {
            process_data::write(hash, offset, buf, len, do_push);

            if ((offset + len) > length)
                throw std::exception();

            std::memcpy(&data[offset], buf, len);
        }

        //! Read data from buffer.
        /*!
         * \param[in] hash      hash value, get it with set_consumer!
         * \param[in] offset    Process data offset from beginning of the buffer.
         * \param[in] buf       Buffer with data to write to from internal front buffer.
         * \param[in] len       Length of data in buffer.
         * \param[in] do_pop    Pop the buffer and consume it.
         */
        void read(const std::size_t& hash, off_t offset, uint8_t *buf, 
                size_t len, bool do_pop = true) 
        {
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
        triple_buffer(size_t length, const std::string& owner, const std::string& name, 
                const std::string& process_data_definition = "", const std::string& clk_device = "") :
            process_data(length, owner, name, process_data_definition, clk_device)
            {
                // initilize indices with front(0), back(1), flip(2)
                indices.store((0x00) | (0x01 << 2) | (0x02 << 4));

                data[0].resize(length);
                data[1].resize(length);
                data[2].resize(length);
            }
        
        //! Get a pointer to the a data buffer which we can write next, has to be
        //  completed with calling \link push \endlink
        /*
         * \param[in] hash      hash value, get it with set_provider!
         */
        uint8_t* next(const std::size_t& hash) {
            process_data::next(hash);

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
        /* 
         * \param[in] hash      hash value, get it with set_consumer!
         */
        uint8_t* pop(const std::size_t& hash) {
            process_data::pop(hash);

            swap_front();

            auto& tmp_buf = front_buffer();
            return (uint8_t *)&tmp_buf[0];
        }

        //! Pushes write data buffer to available on calling \link next \endlink.
        /*
         * \param[in] hash      hash value, get it with set_provider!
         */
        void push(const std::size_t& hash) {
            process_data::push(hash);

            swap_back();
        }

        //! Write data to buffer.
        /*!
         * \param[in] hash      hash value, get it with set_provider!
         * \param[in] offset    Process data offset from beginning of the buffer.
         * \param[in] buf       Buffer with data to write to internal back buffer.
         * \param[in] len       Length of data in buffer.
         * \param[in] do_push   Push the buffer to set it to the actual one.
         */
        void write(const std::size_t& hash, off_t offset, uint8_t *buf, 
                size_t len, bool do_push = true) 
        {
            process_data::write(hash, offset, buf, len, do_push);

            if ((offset + len) > length)
                throw std::exception();

            auto& tmp_buf = back_buffer();
            std::memcpy(&tmp_buf[offset], buf, len);

            if (do_push)
                swap_back();
        }

        //! Read data from buffer.
        /*!
         * \param[in] hash      hash value, get it with set_consumer!
         * \param[in] offset    Process data offset from beginning of the buffer.
         * \param[in] buf       Buffer with data to write to from internal front buffer.
         * \param[in] len       Length of data in buffer.
         * \param[in] do_pop    Pop the buffer and consume it.
         */
        void read(const std::size_t& hash, off_t offset, uint8_t *buf, 
                size_t len, bool do_pop = true) {
            process_data::read(hash, offset, buf, len, do_pop);

            if ((offset + len) > length)
                throw std::exception();

            if (do_pop) {
                swap_front();
            }

            auto& tmp_buf = front_buffer();
            std::memcpy(buf, &tmp_buf[offset], len);
        }

        //! Returns true if new data has been written
        bool new_data() {
            uint8_t old_indices = indices.load(std::memory_order_consume);

            if (!(old_indices & written_mask))
                return false; // nothing new
            
            return true; 
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
        pointer_buffer(size_t length, uint8_t *ptr, const std::string& owner, const std::string& name, 
                const std::string& process_data_definition = "", const std::string& clk_device = "") :
            process_data(length, owner, name, process_data_definition, clk_device), ptr(ptr)
            { }
        
        //! Get a pointer to the a data buffer which we can write next, has to be
        //  completed with calling \link push \endlink
        /*
         * \param[in] hash      hash value, get it with set_provider!
         */
        uint8_t* next(const std::size_t& hash) {
            process_data::next(hash);

            return ptr;
        }

        //! Get a pointer to the last written data without consuming it, 
        //  which will be available on calling \link pop \endlink
        uint8_t* peek() {
            return ptr;
        }

        //! Get a pointer to the actual read data. This call
        //  will consume the data.
        /*
         * \param[in] hash      hash value, get it with set_consumer!
         */
        uint8_t* pop(const std::size_t& hash) {
            return ptr;
        }

        //! Write data to buffer.
        /*!
         * \param[in] hash      hash value, get it with set_provider!
         * \param[in] offset    Process data offset from beginning of the buffer.
         * \param[in] buf       Buffer with data to write to internal back buffer.
         * \param[in] len       Length of data in buffer.
         * \param[in] do_push   Push the buffer to set it to the actual one.
         */
        void write(const std::size_t& hash, off_t offset, uint8_t *buf, 
                size_t len, bool do_push = true) 
        {
            process_data::write(hash, offset, buf, len, do_push);

            if ((offset + len) > length)
                throw std::exception();

            std::memcpy(&ptr[offset], buf, len);
        }

        //! Read data from buffer.
        /*!
         * \param[in] hash      hash value, get it with set_consumer!
         * \param[in] offset    Process data offset from beginning of the buffer.
         * \param[in] buf       Buffer with data to write to from internal front buffer.
         * \param[in] len       Length of data in buffer.
         * \param[in] do_pop    Pop the buffer and consume it.
         */
        void read(const std::size_t& hash, off_t offset, uint8_t *buf, 
                size_t len, bool do_pop = true) 
        {
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

#endif // ROBOTKERNEL__PROCESS_DATA_H

