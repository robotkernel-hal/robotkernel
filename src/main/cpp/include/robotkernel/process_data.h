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

enum pd_data_types {
    PD_DT_NONE = -1,
    PD_DT_FLOAT = 1,
    PD_DT_DOUBLE,
    PD_DT_UINT8,
    PD_DT_UINT16,
    PD_DT_UINT32,
    PD_DT_INT8,
    PD_DT_INT16,
    PD_DT_INT32
};

template <typename T>
void convert_fun(std::vector<uint8_t>& value, T val) {
    value.resize(sizeof(T));
    *(T *)&value[0] = val;
};


inline void convert_str_val(const pd_data_types& type, const std::string& value_str,
        std::vector<uint8_t>& value) {

    switch (type) {
        case PD_DT_FLOAT:  { convert_fun<float>   (value, stof (value_str)); break; }
        case PD_DT_DOUBLE: { convert_fun<double>  (value, stod (value_str)); break; }
        case PD_DT_UINT8:  { convert_fun<uint8_t> (value, stoul(value_str)); break; }
        case PD_DT_UINT16: { convert_fun<uint16_t>(value, stoul(value_str)); break; }
        case PD_DT_UINT32: { convert_fun<uint32_t>(value, stoul(value_str)); break; }
        case PD_DT_INT8:   { convert_fun<int8_t>  (value, stol (value_str)); break; }
        case PD_DT_INT16:  { convert_fun<int16_t> (value, stol (value_str)); break; }
        case PD_DT_INT32:  { convert_fun<int32_t> (value, stol (value_str)); break; }
    }
}

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
        
typedef struct pd_entry {
    pd_entry() : initialized(false) {}

    // input fields
    std::string field_name;
    std::string value_string;
    std::string bitmask_string;

    bool initialized;

    // user fields
    std::vector<uint8_t> value;
    std::vector<uint8_t> bitmask;

    off_t offset;

    std::string type_str;
    pd_data_types type;
} pd_entry_t;

//! process data injection class
class pd_injection_base
{
    public:
        //! construction
        /*!
         * \param length byte length of process data
         * \param owner name of owning module
         * \param name process data name
         * \param process_data_definition yaml-string representating the structure of the pd
         */
        pd_injection_base() {}

        //! inject value to process data
        /*!
         * \param[in]       e       Entry to inject.
         * \param[in,out]   buf     Buffer to inject.
         * \param[in]       len     Length of buffer.
         */
        void inject_val(const pd_entry_t& e, uint8_t* buf, size_t len);

        //! add injection value
        /*!
         * \param[in]   e       Entry to inject.
         */
        void add_injection(pd_entry_t& e) {
            // this may overwrite old injections to that field
            pd_injections[e.field_name] = e;
        }
        
        //! del injection value
        /*!
         * \param[in]   field_name  Entry to inject.
         */
        void del_injection(const std::string& field_name) {
            if (pd_injections.find(field_name) != pd_injections.end())
                return;

            pd_injections.erase(field_name);
        }

    public:
        std::map<std::string, pd_entry_t> pd_injections;
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
                throw str_exception_tb("cannot set provider for %s, already have one!", id().c_str());

            provider = mod;
            provider_hash = std::hash<std::shared_ptr<robotkernel::pd_provider> >{}(mod);

            return provider_hash;
        }

        //! reset data provider thread
        void reset_provider(const std::size_t& hash) {
            if (provider_hash != hash)
                throw str_exception_tb("cannot reset provider for %s: you are not the provider!", id().c_str());

            provider_hash = 0;
            provider = nullptr;
        }

        //!< set main consumer thread, only thread allowed to pop
        std::size_t set_consumer(std::shared_ptr<robotkernel::pd_consumer> mod) {
            if (    (consumer != nullptr) &&
                    (consumer != mod))
                throw str_exception_tb("cannot set consumer for %s: already have one!", id().c_str());

            consumer = mod;
            consumer_hash = std::hash<std::shared_ptr<robotkernel::pd_consumer> >{}(mod);

            return consumer_hash;
        }
        
        //! reset main consumer thread
        void reset_consumer(const std::size_t& hash) {
            if (consumer_hash != hash)
                throw str_exception_tb("cannot reset consumer for %s: you are not the consumer!", id().c_str());

            consumer_hash = 0;
            consumer = nullptr;
        }

        //! Find offset and type of given process data member.
        /*!
         * \param[in]   field_name      Name of member to find.
         * \param[out]  type_str        Type as string.
         * \param[out]  type            Type as enum.
         * \param[out]  offset          Byte offset of member.
         */
        void find_pd_offset_and_type(const std::string& field_name, 
                std::string& type_str, pd_data_types& type, off_t& offset);

        //! Find offset and type of given process data member.
        /*!
         * \param[in,out] entry      Structure to fill.
         */
        void find_pd_offset_and_type(pd_entry_t& e);

    public:
        volatile uint64_t pd_cookie;
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
                throw str_exception_tb("wanted to write to many bytes: %d > length %d\n",
                        (offset + len), length);

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
                throw str_exception_tb("wanted to read to many bytes: %d > length %d\n",
                        (offset + len), length);
            
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
                throw str_exception_tb("wanted to write to many bytes: %d > length %d\n",
                        (offset + len), length);

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
                throw str_exception_tb("wanted to read to many bytes: %d > length %d\n",
                        (offset + len), length);
            

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

class triple_buffer_with_injection :
    public triple_buffer,
    public pd_injection_base 
{
    private:
        triple_buffer_with_injection();     //!< prevent default construction

    public:
        //! construction
        /*!
         * \param length byte length of process data
         * \param owner name of owning module
         * \param name process data name
         * \param process_data_definition yaml-string representating the structure of the pd
         */
        triple_buffer_with_injection(size_t length, const std::string& owner, const std::string& name, 
                const std::string& process_data_definition = "", const std::string& clk_device = "") :
            triple_buffer(length, owner, name, process_data_definition, clk_device) 
            {}

        //! Pushes write data buffer to available on calling \link next \endlink.
        /*
         * \param[in] hash      hash value, get it with set_provider!
         */
        void push(const std::size_t& hash) {
            const auto& buf = next(hash);

            for (auto& kv : pd_injections) {
                auto& pd_entry = kv.second;

                if (!pd_entry.initialized) {
                    find_pd_offset_and_type(pd_entry);
                    convert_str_val(pd_entry.type, pd_entry.value_string, pd_entry.value);
                    printf("offset %d, length %d\n", pd_entry.offset, pd_entry.value.size());

                    pd_entry.initialized = true;
                }

                inject_val(pd_entry, buf, length);
            }
            
            triple_buffer::push(hash);
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
            triple_buffer::write(hash, offset, buf, len, false);

            if (do_push) {
                push(hash);
            }
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
                throw str_exception_tb("wanted to write to many bytes: %d > length %d\n",
                        (offset + len), length);

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
                throw str_exception_tb("wanted to read to many bytes: %d > length %d\n",
                        (offset + len), length);
            

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

