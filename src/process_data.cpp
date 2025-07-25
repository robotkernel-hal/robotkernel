//! robotkernel process_data
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

#include "kernel.h"
#include "process_data.h"
#include "yaml-cpp/yaml.h"

using namespace std;
using namespace robotkernel;

std::map<std::string, size_t> dt_to_len = {
    { "float",    4 },
    { "double",   8 },
    { "uint8_t",  1 },
    { "uint16_t", 2 },
    { "uint32_t", 4 },
    { "int8_t",   1 },
    { "int16_t",  2 },
    { "int32_t",  4 },
};

std::map<std::string, pd_data_types> pd_dt_map = {
    { "float",    PD_DT_FLOAT  },
    { "double",   PD_DT_DOUBLE },
    { "uint8_t",  PD_DT_UINT8  },
    { "uint16_t", PD_DT_UINT16 },
    { "uint32_t", PD_DT_UINT32 },
    { "int8_t",   PD_DT_INT8   },
    { "int16_t",  PD_DT_INT16  },
    { "int32_t",  PD_DT_INT32  },
};
    
//! construct and initialize pd_entry
/*!
 * \param[in]  pd               Corresponding process data.
 * \param[in]  field_name       Field name in process data.
 * \param[in]  value_string     Value to inject.
 * \param[in]  bitmask_string   Bitmask for value.
 */
pd_entry::pd_entry(std::shared_ptr<process_data> pd, 
        const std::string& field_name, const std::string& value_string,
        const std::string& bitmask_string) :
    field_name(field_name), value_string(value_string), bitmask_string(bitmask_string)
{
    pd->find_pd_offset_and_type(*this);
    convert_str_val(type, value_string, value);
                    
    initialized = true;
}

//! construction
/*!
 * \param length byte length of process data
 * \param owner name of owning module
 * \param name process data name
 * \param process_data_definition yaml-string representating the structure of the pd
 */
process_data::process_data(size_t length, const std::string& owner, const std::string& name, 
        const std::string& process_data_definition, const std::string& clk_device) :
device(owner, name, "pd"), pd_cookie(0), length(length), process_data_definition(process_data_definition)
{
    if (clk_device != "") {
        trigger_dev = kernel::instance.get_device<robotkernel::trigger>(clk_device);
    } else {
        trigger_dev = make_shared<robotkernel::trigger>(owner, name);
        trigger_dev_generated = true;
    }
}
        
process_data::~process_data() {
    if (trigger_dev_generated) {
        trigger_dev = nullptr;
    }
}
       
//! Pushes write data buffer to available on calling \link next \endlink.
/*
 * \param[in] hash      hash value, get it with set_provider!
 */
void process_data::push(sp_pd_provider_t& prov, bool do_trigger) {
    if ((provider == nullptr) || (provider->hash != prov->hash)) {
        throw runtime_error(string_printf("permission denied to push %s", id().c_str()));
    }

    const auto& buf = next(prov);

    for (auto& kv : pd_injections) {
        auto& pd_entry = kv.second;

        if (!pd_entry.initialized) {
            find_pd_offset_and_type(pd_entry);
            convert_str_val(pd_entry.type, pd_entry.value_string, pd_entry.value);

            pd_entry.initialized = true;
        }

        inject_val(pd_entry, buf, length);
    }

    pd_cookie++;

    if (trigger_dev && do_trigger) {
        trigger_dev->do_trigger();
    }
}

//! Return data type length from string
/*!
 * \param[in]   dt_name         String representation of datatype.
 * \return data type length
 */
ssize_t process_data::get_data_type_length(const std::string& dt_name) {
    if (dt_to_len.find(dt_name) != dt_to_len.end()) {
        return dt_to_len[dt_name];
    }

    return -1;
}

//! Return data type from string
/*!
 * \param[in]   dt_name         String representation of datatype.
 * \return data type
 */
pd_data_types process_data::get_data_type(const std::string& dt_name) {
    if (pd_dt_map.find(dt_name) != pd_dt_map.end()) {
        return pd_dt_map[dt_name];
    }

    return PD_DT_UNKNOWN;
}

//! Find offset and type of given process data member.
/*!
 * \param[in]   field_name      Name of member to find.
 * \param[out]  type_str        Type as string.
 * \param[out]  type            Type as enum.
 * \param[out]  offset          Byte offset of memberi
 */
void process_data::find_pd_offset_and_type(const std::string& field_name, 
        std::string& type_str, pd_data_types& type, off_t& offset) {
    // need to find offset and type
    if (process_data_definition == "")
        throw runtime_error(string_printf("process data \"%s\" has no description, "
                "cannot determine pos offset!\n", id().c_str()));

    YAML::Node pdd_node = YAML::Load(process_data_definition);

    offset = 0;

    for (const auto& list_el : pdd_node) {
        for (const auto& kv : list_el) {
            string act_dt = kv.first.as<string>();
            string act_name = kv.second.as<string>();

            if (act_name == field_name) {
                type_str = act_dt;
                type = pd_dt_map[act_dt];

                return;
            }

            if (dt_to_len.find(act_dt) == dt_to_len.end())
                throw runtime_error(string_printf("unsupported data type in pd description: %s\n", act_dt.c_str()));

            offset += dt_to_len[act_dt];
        }
    }

    throw runtime_error(string_printf("member \"%s\" not found in measurement process data description:\n%s\n",
            field_name.c_str(), process_data_definition.c_str()));
}

//! Find offset and type of given process data member.
/*!
 * \param[in,out] entry      Structure to fill.
 */
void process_data::find_pd_offset_and_type(pd_entry_t& e) {
    find_pd_offset_and_type(e.field_name, e.type_str, e.type, e.offset);
}

//! set data provider thread, only thread allowed to write and push
void process_data::set_provider(sp_pd_provider_t& prov) { 
    if (    (provider != nullptr) && 
            (provider != prov)) {
        throw runtime_error(string_printf("cannot set provider for %s, already have one!", id().c_str()));
    }

    provider = prov;
    prov->hash = std::hash<std::shared_ptr<robotkernel::pd_provider> >{}(prov);
}

//! reset data provider thread
void process_data::reset_provider(sp_pd_provider_t& prov) {
    if (prov->hash != provider->hash) {
        throw runtime_error(string_printf("cannot reset provider for %s: you are not the provider!", id().c_str()));
    }

    prov->hash = 0;
    provider = nullptr;
}

//!< set main consumer thread, only thread allowed to pop
void process_data::set_consumer(sp_pd_consumer_t& cons) {
    if (    (consumer != nullptr) &&
            (consumer != cons))
        throw runtime_error(string_printf("cannot set consumer for %s: already have one!", id().c_str()));

    consumer = cons;
    cons->hash = std::hash<std::shared_ptr<robotkernel::pd_consumer> >{}(cons);
}

//! reset main consumer thread
void process_data::reset_consumer(sp_pd_consumer_t& cons) {
    if (cons->hash != consumer->hash)
        throw runtime_error(string_printf("cannot reset consumer for %s: you are not the consumer!", id().c_str()));

    cons->hash = 0;
    consumer = nullptr;
}
        
//! inject value to process data
/*!
 * \param[in]       e       Entry to inject.
 * \param[in,out]   buf     Buffer to inject.
 * \param[in]       len     Length of buffer.
 */
void pd_injection_base::inject_val(const pd_entry_t& e, uint8_t* buf, size_t len) {
    switch (e.type) {
#define CASE_PD_DT(dt_enum, dtype)                                   \
        case dt_enum: {                                              \
            *(dtype *)(&buf[e.offset]) = *(dtype *)(&e.value[0]); \
            break;                                                   \
        }

        CASE_PD_DT(PD_DT_FLOAT, float)
        CASE_PD_DT(PD_DT_DOUBLE, double)
        CASE_PD_DT(PD_DT_UINT8, uint8_t)
        CASE_PD_DT(PD_DT_UINT16, uint16_t)
        CASE_PD_DT(PD_DT_UINT32, uint32_t)
        CASE_PD_DT(PD_DT_INT8, int8_t)
        CASE_PD_DT(PD_DT_INT16, int16_t)
        CASE_PD_DT(PD_DT_INT32, int32_t)

#undef CASE_PD_DT
    }
}

//! construction
/*!
 * \param length byte length of process data
 * \param owner name of owning module
 * \param name process data name
 * \param process_data_definition yaml-string representating the structure of the pd
 */
single_buffer::single_buffer(size_t length, const std::string& owner, const std::string& name, 
        const std::string& process_data_definition, const std::string& clk_device) :
process_data(length, owner, name, process_data_definition, clk_device)
{
    data.resize(length);
}

//! Get a pointer to the a data buffer which we can write next, has to be
//  completed with calling \link push \endlink
/*
 * \param[in] hash      hash value, get it with set_provider!
 */
uint8_t* single_buffer::next(sp_pd_provider_t& prov) {
    process_data::next(prov);

    return (uint8_t *)&data[0];
}

//! Get a pointer to the last written data without consuming it, 
//  which will be available on calling \link pop \endlink
uint8_t* single_buffer::peek() {
    return (uint8_t *)&data[0];
}

//! Get a pointer to the actual read data. This call
//  will consume the data.
/*
 * \param[in] hash      hash value, get it with set_consumer!
 */
uint8_t* single_buffer::pop(sp_pd_consumer_t& cons, bool do_trigger) {
    (void)process_data::pop(cons); 

    if (do_trigger) {
        trigger_dev->do_trigger();
    }

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
void single_buffer::write(sp_pd_provider_t& prov, off_t offset, uint8_t *buf, 
        size_t len, bool do_push, bool do_trigger) 
{
    process_data::write(prov, offset, buf, len, do_push, do_trigger);

    if ((offset + len) > length)
        throw runtime_error(string_printf("wanted to write to many bytes: %d > length %d\n",
                (offset + len), length));

    std::memcpy(&data[offset], buf, len);

    if (do_trigger) {
        trigger_dev->do_trigger();
    }
}

//! Read data from buffer.
/*!
 * \param[in] hash      hash value, get it with set_consumer!
 * \param[in] offset    Process data offset from beginning of the buffer.
 * \param[in] buf       Buffer with data to write to from internal front buffer.
 * \param[in] len       Length of data in buffer.
 * \param[in] do_pop    Pop the buffer and consume it.
 */
void single_buffer::read(sp_pd_consumer_t& cons, off_t offset, uint8_t *buf, 
        size_t len, bool do_pop, bool do_trigger) 
{
    process_data::read(cons, offset, buf, len, do_pop, do_trigger);

    if ((offset + len) > length)
        throw runtime_error(string_printf("wanted to read to many bytes: %d > length %d\n",
                (offset + len), length));

    std::memcpy(buf, &data[offset], len);

    if (do_trigger) {
        trigger_dev->do_trigger();
    }
}

//! construction
/*!
 * \param length byte length of process data
 * \param owner name of owning module
 * \param name process data name
 * \param process_data_definition yaml-string representating the structure of the pd
 */
triple_buffer::triple_buffer(size_t length, const std::string& owner, const std::string& name, 
        const std::string& process_data_definition, const std::string& clk_device) :
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
uint8_t* triple_buffer::next(sp_pd_provider_t& prov) {
    (void)process_data::next(prov);

    auto& tmp_buf = back_buffer();
    return (uint8_t *)&tmp_buf[0];
}

//! Get a pointer to the last written data without consuming it, 
//  which will be available on calling \link pop \endlink
uint8_t* triple_buffer::peek() {
    auto& tmp_buf = flip_buffer();
    return (uint8_t *)&tmp_buf[0];
}

//! Get a pointer to the actual read data. This call
//  will consume the data.
/* 
 * \param[in] hash      hash value, get it with set_consumer!
 */
uint8_t* triple_buffer::pop(sp_pd_consumer_t& cons, bool do_trigger) {
    (void)process_data::pop(cons);

    swap_front();

    auto& tmp_buf = front_buffer();

    if (do_trigger) {
        trigger_dev->do_trigger();
    }

    return (uint8_t *)&tmp_buf[0];
}

//! Pushes write data buffer to available on calling \link next \endlink.
/*
 * \param[in] hash      hash value, get it with set_provider!
 */
void triple_buffer::push(sp_pd_provider_t& prov, bool do_trigger) {
    process_data::push(prov, false);

    swap_back();

    if (trigger_dev && do_trigger) {
        trigger_dev->do_trigger();
    }
}

//! Write data to buffer.
/*!
 * \param[in] hash      hash value, get it with set_provider!
 * \param[in] offset    Process data offset from beginning of the buffer.
 * \param[in] buf       Buffer with data to write to internal back buffer.
 * \param[in] len       Length of data in buffer.
 * \param[in] do_push   Push the buffer to set it to the actual one.
 */
void triple_buffer::write(sp_pd_provider_t& prov, off_t offset, uint8_t *buf, 
        size_t len, bool do_push, bool do_trigger) 
{
    process_data::write(prov, offset, buf, len, do_push, do_trigger);

    if ((offset + len) > length)
        throw runtime_error(string_printf("wanted to write to many bytes: %d > length %d\n",
                (offset + len), length));

    auto& tmp_buf = back_buffer();
    std::memcpy(&tmp_buf[offset], buf, len);

    if (do_push)
        push(prov, do_trigger);
    else if (do_trigger) {
        trigger_dev->do_trigger();
    }
}

//! Read data from buffer.
/*!
 * \param[in] hash      hash value, get it with set_consumer!
 * \param[in] offset    Process data offset from beginning of the buffer.
 * \param[in] buf       Buffer with data to write to from internal front buffer.
 * \param[in] len       Length of data in buffer.
 * \param[in] do_pop    Pop the buffer and consume it.
 */
void triple_buffer::read(sp_pd_consumer_t& cons, off_t offset, uint8_t *buf, 
        size_t len, bool do_pop, bool do_trigger) 
{
    process_data::read(cons, offset, buf, len, do_pop);

    if ((offset + len) > length)
        throw runtime_error(string_printf("wanted to read to many bytes: %d > length %d\n",
                (offset + len), length));


    if (do_pop) {
        swap_front();
    }

    auto& tmp_buf = front_buffer();
    std::memcpy(buf, &tmp_buf[offset], len);

    if (do_trigger) {
        trigger_dev->do_trigger();
    }
}

//! Returns true if new data has been written
bool triple_buffer::new_data() {
    uint8_t old_indices = indices.load(std::memory_order_consume);

    if (!(old_indices & written_mask))
        return false; // nothing new

    return true; 
}

//! return current read buffer
const std::vector<uint8_t>& triple_buffer::front_buffer() {
    int idx = indices.load(std::memory_order_consume) & front_buffer_mask;
    return data[idx];
}

//! return current flip buffer
std::vector<uint8_t>& triple_buffer::flip_buffer() {
    int idx = (indices.load(std::memory_order_consume) & flip_buffer_mask) >> 4;
    return data[idx];
}
//! return current write buffer
std::vector<uint8_t>& triple_buffer::back_buffer() {
    int idx = (indices.load(std::memory_order_consume) & back_buffer_mask) >> 2;
    return data[idx];
}

//! swaps the buffers 
void triple_buffer::swap_front() {
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
void triple_buffer::swap_back() {
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

//! construction
/*!
 * \param length byte length of process data
 * \param owner name of owning module
 * \param name process data name
 * \param process_data_definition yaml-string representating the structure of the pd
 */
pointer_buffer::pointer_buffer(size_t length, uint8_t *ptr, const std::string& owner, const std::string& name, 
        const std::string& process_data_definition, const std::string& clk_device) :
    process_data(length, owner, name, process_data_definition, clk_device), ptr(ptr)
{ }

//! Get a pointer to the a data buffer which we can write next, has to be
//  completed with calling \link push \endlink
/*
 * \param[in] hash      hash value, get it with set_provider!
 */
uint8_t* pointer_buffer::next(sp_pd_provider_t& prov) {
    (void)process_data::next(prov);

    return ptr;
}

//! Get a pointer to the last written data without consuming it, 
//  which will be available on calling \link pop \endlink
uint8_t* pointer_buffer::peek() {
    return ptr;
}

//! Get a pointer to the actual read data. This call
//  will consume the data.
/*
 * \param[in] hash      hash value, get it with set_consumer!
 */
uint8_t* pointer_buffer::pop(sp_pd_consumer_t& cons, bool do_trigger) {
    (void)process_data::pop(cons, do_trigger);

    if (do_trigger) {
        trigger_dev->do_trigger();
    }

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
void pointer_buffer::write(sp_pd_provider_t& prov, off_t offset, uint8_t *buf, 
        size_t len, bool do_push, bool do_trigger) 
{
    process_data::write(prov, offset, buf, len, do_push, do_trigger);

    if ((offset + len) > length)
        throw runtime_error(string_printf("wanted to write to many bytes: %d > length %d\n",
                (offset + len), length));

    std::memcpy(&ptr[offset], buf, len);

    if (do_trigger) {
        trigger_dev->do_trigger();
    }
}

//! Read data from buffer.
/*!
 * \param[in] hash      hash value, get it with set_consumer!
 * \param[in] offset    Process data offset from beginning of the buffer.
 * \param[in] buf       Buffer with data to write to from internal front buffer.
 * \param[in] len       Length of data in buffer.
 * \param[in] do_pop    Pop the buffer and consume it.
 */
void pointer_buffer::read(sp_pd_consumer_t& cons, off_t offset, uint8_t *buf, 
        size_t len, bool do_pop, bool do_trigger) 
{
    process_data::read(cons, offset, buf, len, do_pop, do_trigger);

    if ((offset + len) > length)
        throw runtime_error(string_printf("wanted to read to many bytes: %d > length %d\n",
                (offset + len), length));

    std::memcpy(buf, &ptr[offset], len);

    if (do_trigger) {
        trigger_dev->do_trigger();
    }
}

