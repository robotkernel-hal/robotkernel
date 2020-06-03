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

#include "process_data.h"
#include "string_util/string_util.h"
#include "yaml-cpp/yaml.h"

using namespace std;
using namespace robotkernel;
using namespace string_util;

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

//! Find offset and type of given process data member.
/*!
 * \param[in]   field_name      Name of member to find.
 * \param[out]  type_str        Type as string.
 * \param[out]  type            Type as enum.
 * \param[out]  offset          Byte offset of member.
 */
void process_data::find_pd_offset_and_type(const std::string& field_name, 
        std::string& type_str, pd_data_types& type, off_t& offset) {
    // need to find offset and type
    if (process_data_definition == "")
        throw str_exception("process data \"%s\" has no description, "
                "cannot determine pos offset!\n", id().c_str());

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
                throw str_exception("unsupported data type in pd description: %s\n", act_dt.c_str());

            offset += dt_to_len[act_dt];
        }
    }

    throw str_exception("member \"%s\" not found in measurement process data description:\n%s\n",
            field_name.c_str(), process_data_definition.c_str());
}

//! Find offset and type of given process data member.
/*!
 * \param[in,out] entry      Structure to fill.
 */
void process_data::find_pd_offset_and_type(pd_entry_t& e) {
    find_pd_offset_and_type(e.field_name, e.type_str, e.type, e.offset);
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

