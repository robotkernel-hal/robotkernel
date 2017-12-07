//! robotkernel rk_type
/*!
 * (C) Robert Burger <robert.burger@dlr.de>
 * (C) Jan Cremer <jan.cremer@dlr.de>
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

#include "robotkernel/rk_type.h"
#include "string_util/string_util.h"
#include <sstream>

using namespace std;
using namespace string_util;

namespace robotkernel {
    reference_map_t rk_type::__refs;
    std::mutex      rk_type::__refsLock;

    std::string rk_type::to_string(){
        if(__type == typeid(int8_t)){
            return format_string("%d", (int8_t) *this);
        } else if(__type == typeid(int16_t)){
            return format_string("%d", (int16_t) *this);
        } else if(__type == typeid(int32_t)){
            return format_string("%d", (int32_t) *this);
        } else if(__type == typeid(int64_t)){
            return format_string("%d", (int64_t) *this);
        } else if(__type == typeid(uint8_t)){
            return format_string("%d", (uint8_t) *this);
        } else if(__type == typeid(uint16_t)){
            return format_string("%d", (uint16_t) *this);
        } else if(__type == typeid(uint32_t)){
            return format_string("%d", (uint32_t) *this);
        } else if(__type == typeid(uint64_t)){
            return format_string("%d", (uint64_t) *this);
        } else if(__type == typeid(float)){
            return format_string("%f", (float) *this);
        } else if(__type == typeid(double)){
            return format_string("%f", (double) *this);
        } else if(__type == typeid(std::string)){
            std::string v = *static_cast<std::string*>((void*)__value);
            return std::string("\"") + v + "\"";
        } else if(__type == typeid(std::vector<rk_type>)){
            std::vector<rk_type> v = *static_cast<std::vector<rk_type>*>((void*)__value);
            std::stringstream response;
            response << '{';
            for (unsigned int i=0; i<v.size(); ++i){
                if(i>0){
                    response << ", ";
                }
                response << v[i].to_string();
            }
            response << '}';
            return response.str();
        } else {
            return string("unknown_type");
        }
    }
    
}

