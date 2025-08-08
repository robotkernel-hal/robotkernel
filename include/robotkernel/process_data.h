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
#include <stdexcept>

#include "robotkernel/device.h"
#include "robotkernel/trigger.h"
#include "robotkernel/rk_type.h"
#include "robotkernel/helpers.h"

namespace robotkernel {

// forward declarations
class process_data;

enum pd_data_types {
    PD_DT_UNKNOWN = -2,
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

//! convert string values to uint8 array depending on datatype
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
        default: break;
    }
}

//! process data provider class
/*!
 * derive from this class, if you want to register yourself as
 * process data provider
 */
class pd_provcon_base {
    public:
        std::string name;
        size_t hash;

        pd_provcon_base(const std::string& name) : 
            name(name), hash(0) {}
};

class pd_provider : public pd_provcon_base {
    public:
        pd_provider(const std::string& name) : pd_provcon_base(name) {}
};

class pd_consumer : public pd_provcon_base {
    public:
        pd_consumer(const std::string& name) : pd_provcon_base(name) {}
};
    
typedef std::shared_ptr<pd_provider> sp_pd_provider_t;
typedef std::shared_ptr<pd_consumer> sp_pd_consumer_t;

        
typedef struct pd_entry {
    pd_entry() : initialized(false) {}

    //! construct and initialize pd_entry
    /*!
     * \param[in]  pd               Corresponding process data.
     * \param[in]  field_name       Field name in process data.
     * \param[in]  value_string     Value to inject.
     * \param[in]  bitmask_string   Bitmask for value.
     */
    pd_entry(std::shared_ptr<process_data> pd, 
            const std::string& field_name, const std::string& value_string,
            const std::string& bitmask_string);

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
    public device,
    public pd_injection_base 
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
                const std::string& process_data_definition = "", const std::string& clk_device = "");
        virtual ~process_data();

        //! Get a pointer to the a data buffer which we can write next, has to be
        //  completed with calling \link push \endlink
        /*
         * \param[in] hash      hash value, get it with set_provider!
         */
        virtual uint8_t* next(sp_pd_provider_t& prov) {
            if ((provider == nullptr) || (provider->hash != prov->hash)) {
                throw std::runtime_error(robotkernel::string_printf("permission denied to write to %s", id().c_str()));
            }

            return nullptr;
        }

        virtual void trigger(void) { trigger_dev->do_trigger(); }

        //! Get a pointer to the last written data without consuming it, 
        //  which will be available on calling \link pop \endlink
        virtual uint8_t* peek() = 0;

        //! Get a pointer to the actual read data. This call
        //  will consume the data.
        /*
         * \param[in] hash      hash value, get it with set_consumer!
         */
        virtual uint8_t* pop(sp_pd_consumer_t& cons, bool do_trigger = true) {
            if ((consumer == nullptr) || (consumer->hash != cons->hash)) {
                throw std::runtime_error(robotkernel::string_printf("permission denied to pop %s: consumer_hash %d, your hash %d", 
                        id().c_str(), consumer->hash, cons->hash));
            }

            return nullptr;
        }

        //! Pushes write data buffer to available on calling \link next \endlink.
        /*
         * \param[in] proc      provider, set it with set_provider!
         */
        virtual void push(sp_pd_provider_t& prov, bool do_trigger = true);

        //! Write data to buffer.
        /*!
         * \param[in] hash      hash value, get it with set_provider!
         * \param[in] offset    Process data offset from beginning of the buffer.
         * \param[in] buf       Buffer with data to write to internal back buffer.
         * \param[in] len       Length of data in buffer.
         * \param[in] do_push   Push the buffer to set it to the actual one.
         */
        virtual void write(sp_pd_provider_t& prov, off_t offset, uint8_t *buf, 
            size_t len, bool do_push = true, bool do_trigger = true) 
        {
            if ((provider == nullptr) || (provider->hash != prov->hash)) {
                throw std::runtime_error(robotkernel::string_printf("permission denied to write to %s", id().c_str()));
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
        virtual void read(sp_pd_consumer_t& cons, off_t offset, uint8_t *buf, 
                size_t len, bool do_pop = true, bool do_trigger = true) 
        {
            if (do_pop && ((consumer == nullptr) || (consumer->hash != cons->hash))) {
                throw std::runtime_error(robotkernel::string_printf("permission denied to pop %s: consumer_hash %d, your hash %d", 
                        id().c_str(), consumer->hash, cons->hash));
            }

            pd_cookie++;
        }

        //! Returns true if new data has been written
        virtual bool new_data() { return true; }

        //! set data provider thread, only thread allowed to write and push
        void set_provider(robotkernel::sp_pd_provider_t& prov);

        //! reset data provider thread
        void reset_provider(sp_pd_provider_t& prov);

        //!< set main consumer thread, only thread allowed to pop
        void set_consumer(robotkernel::sp_pd_consumer_t& cons);

        
        //! reset main consumer thread
        void reset_consumer(sp_pd_consumer_t& cons);

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

        //! Return data type length from string
        /*!
         * \param[in]   dt_name         String representation of datatype.
         * \return data type length
         */
        static ssize_t get_data_type_length(const std::string& dt_name);

        //! Return data type from string
        /*!
         * \param[in]   dt_name         String representation of datatype.
         * \return data type
         */
        static pd_data_types get_data_type(const std::string& dt_name);

        //! Return if trigger was generated by pd
        bool is_trigger_dev_generated() const { return trigger_dev_generated; }

    public:
        volatile uint64_t pd_cookie;
        const size_t length;
        const std::string process_data_definition;

        std::shared_ptr<robotkernel::trigger> trigger_dev;
        std::shared_ptr<robotkernel::pd_provider> provider;
        std::shared_ptr<robotkernel::pd_consumer> consumer;

    private: 
        bool trigger_dev_generated = false;
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
                const std::string& process_data_definition = "", const std::string& clk_device = "");
        
        //! Get a pointer to the a data buffer which we can write next, has to be
        //  completed with calling \link push \endlink
        /*
         * \param[in] hash      hash value, get it with set_provider!
         */
        uint8_t* next(sp_pd_provider_t& prov) override;

        //! Get a pointer to the last written data without consuming it, 
        //  which will be available on calling \link pop \endlink
        uint8_t* peek() override;

        //! Get a pointer to the actual read data. This call
        //  will consume the data.
        /*
         * \param[in] hash      hash value, get it with set_consumer!
         */
        uint8_t* pop(sp_pd_consumer_t& cons, bool do_trigger = true) override;

        //! Write data to buffer.
        /*!
         * \param[in] hash      hash value, get it with set_provider!
         * \param[in] offset    Process data offset from beginning of the buffer.
         * \param[in] buf       Buffer with data to write to internal back buffer.
         * \param[in] len       Length of data in buffer.
         * \param[in] do_push   Push the buffer to set it to the actual one.
         */
        void write(sp_pd_provider_t& prov, off_t offset, uint8_t *buf, 
                size_t len, bool do_push = true, bool do_trigger = true) override;

        //! Read data from buffer.
        /*!
         * \param[in] hash      hash value, get it with set_consumer!
         * \param[in] offset    Process data offset from beginning of the buffer.
         * \param[in] buf       Buffer with data to write to from internal front buffer.
         * \param[in] len       Length of data in buffer.
         * \param[in] do_pop    Pop the buffer and consume it.
         */
        void read(sp_pd_consumer_t& cons, off_t offset, uint8_t *buf, 
                size_t len, bool do_pop = true, bool do_trigger = true) override;
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
         * \param[in]   length                  Byte length of process data.
         * \param[in]   owner                   Name of owning module.
         * \param[in]   name                    Process data name.
         * \param[in]   process_data_definition YAML-string representating the structure of the pd.
         */
        triple_buffer(size_t length, const std::string& owner, const std::string& name, 
                const std::string& process_data_definition = "", const std::string& clk_device = "");
        
        //! Get a pointer to the a data buffer which we can write next, has to be
        //  completed with calling \link push \endlink
        /*
         * \param[in] hash      hash value, get it with set_provider!
         */
        uint8_t* next(sp_pd_provider_t& prov) override;

        //! Get a pointer to the last written data without consuming it, 
        //  which will be available on calling \link pop \endlink
        uint8_t* peek() override;

        //! Get a pointer to the actual read data. This call
        //  will consume the data.
        /* 
         * \param[in] hash      hash value, get it with set_consumer!
         */
        uint8_t* pop(sp_pd_consumer_t& cons, bool do_trigger = true) override;

        //! Pushes write data buffer to available on calling \link next \endlink.
        /*
         * \param[in] hash      hash value, get it with set_provider!
         */
        void push(sp_pd_provider_t& prov, bool do_trigger = true) override;

        //! Write data to buffer.
        /*!
         * \param[in] hash      hash value, get it with set_provider!
         * \param[in] offset    Process data offset from beginning of the buffer.
         * \param[in] buf       Buffer with data to write to internal back buffer.
         * \param[in] len       Length of data in buffer.
         * \param[in] do_push   Push the buffer to set it to the actual one.
         */
        void write(sp_pd_provider_t& prov, off_t offset, uint8_t *buf, 
                size_t len, bool do_push = true, bool do_trigger = true) override;

        //! Read data from buffer.
        /*!
         * \param[in] hash      hash value, get it with set_consumer!
         * \param[in] offset    Process data offset from beginning of the buffer.
         * \param[in] buf       Buffer with data to write to from internal front buffer.
         * \param[in] len       Length of data in buffer.
         * \param[in] do_pop    Pop the buffer and consume it.
         */
        void read(sp_pd_consumer_t& cons, off_t offset, uint8_t *buf, 
                size_t len, bool do_pop = true, bool do_trigger = true) override;

        //! Returns true if new data has been written
        bool new_data() override;

    private:
        //! return current read buffer
        const std::vector<uint8_t>& front_buffer();

        //! return current flip buffer
        std::vector<uint8_t>& flip_buffer();
        
        //! return current write buffer
        std::vector<uint8_t>& back_buffer();

        //! swaps the buffers 
        void swap_front();

        //! swaps the buffers and mark as written
        void swap_back();
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
                const std::string& process_data_definition = "", const std::string& clk_device = "");
        
        //! Set pointer to a new address.
        /*!
         * \param[in]   prov    PD Provider, must match set provider.
         * \param[in]   ptr     New address to process data.
         *
         * \exception   runtime_error  Permission denied
         */
        void set_ptr(sp_pd_provider_t& prov, uint8_t *ptr) {
            if ((provider == nullptr) || (provider->hash != prov->hash)) {
                throw std::runtime_error(robotkernel::string_printf("permission denied set pointer %s: provider_hash %d, your hash %d", 
                        id().c_str(), provider->hash, prov->hash));
            }

            this->ptr = ptr;
        }

        //! Get a pointer to the a data buffer which we can write next, has to be
        //  completed with calling \link push \endlink
        /*
         * \param[in] hash      hash value, get it with set_provider!
         */
        uint8_t* next(sp_pd_provider_t& prov) override;

        //! Get a pointer to the last written data without consuming it, 
        //  which will be available on calling \link pop \endlink
        uint8_t* peek() override;

        //! Get a pointer to the actual read data. This call
        //  will consume the data.
        /*
         * \param[in] hash      hash value, get it with set_consumer!
         */
        uint8_t* pop(sp_pd_consumer_t& cons, bool do_trigger = true) override;

        //! Write data to buffer.
        /*!
         * \param[in] hash      hash value, get it with set_provider!
         * \param[in] offset    Process data offset from beginning of the buffer.
         * \param[in] buf       Buffer with data to write to internal back buffer.
         * \param[in] len       Length of data in buffer.
         * \param[in] do_push   Push the buffer to set it to the actual one.
         */
        void write(sp_pd_provider_t& prov, off_t offset, uint8_t *buf, 
                size_t len, bool do_push = true, bool do_trigger = true) override;

        //! Read data from buffer.
        /*!
         * \param[in] hash      hash value, get it with set_consumer!
         * \param[in] offset    Process data offset from beginning of the buffer.
         * \param[in] buf       Buffer with data to write to from internal front buffer.
         * \param[in] len       Length of data in buffer.
         * \param[in] do_pop    Pop the buffer and consume it.
         */
        void read(sp_pd_consumer_t& cons, off_t offset, uint8_t *buf, 
                size_t len, bool do_pop = true, bool do_trigger = true) override;
};


typedef std::shared_ptr<process_data> sp_process_data_t;
typedef std::shared_ptr<single_buffer> sp_single_buffer_t;
typedef std::shared_ptr<triple_buffer> sp_triple_buffer_t;
typedef std::shared_ptr<pointer_buffer> sp_pointer_buffer_t;
typedef std::map<std::string, sp_process_data_t> process_data_map_t;

} // namespace robotkernel

#endif // ROBOTKERNEL__PROCESS_DATA_H

