//
// Created by crem_ja on 2/7/17.
//

#include "robotkernel/rk_type.h"
#include "string_util/string_util.h"

namespace robotkernel {
    ReferenceMap rk_type::__refs;
    pthread_mutex_t rk_type::__refsLock = PTHREAD_MUTEX_INITIALIZER;


    std::string rk_type::toString(){
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
            return v;
        } else {
            return string("unknown_type");
        }
    }
    
}

