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

#ifndef ROBOTKERNEL__RK_TYPE_H
#define ROBOTKERNEL__RK_TYPE_H

#include <functional>
#include <typeindex>
#include <map>
#include <pthread.h>
#include "string_util/string_util.h"

namespace robotkernel {
#ifdef EMACS
}
#endif

typedef std::map<uint8_t *, size_t> reference_map_t;

class rk_type {
    protected:
        template<typename T>
            friend T rk_type_cast(rk_type &);

        static reference_map_t __refs;
        static pthread_mutex_t __refsLock;

        std::type_index __type;     //!< data type for type conversions
        uint8_t *__value;           //!< data storage pointer

        void decrease_reference(uint8_t *ref) {
            if (!ref) {
                return;
            }
            pthread_mutex_lock(&__refsLock);
            if (__refs[ref] <= 1) {
                __refs.erase(ref);
                delete ref;
            } else {
                __refs[ref] = __refs[ref] - 1;
            }
            pthread_mutex_unlock(&__refsLock);
        }

        void increase_reference(uint8_t *ref) {
            if (!ref) {
                return;
            }
            pthread_mutex_lock(&__refsLock);
            reference_map_t::iterator r = __refs.find(ref);
            if (r == __refs.end()) {
                __refs[ref] = 1;
            } else {
                r->second++;
            }
            pthread_mutex_unlock(&__refsLock);
        }

        void set_value(uint8_t *value) {
            if (value == __value) {
                return;
            }
            decrease_reference(__value);
            increase_reference(value);
            __value = value;
        }

    public:
        //! default constructor
        rk_type() : __type(typeid(int)), __value(NULL) {
        }

        //! destructor
        ~rk_type() {
            if (__value) {
                decrease_reference(__value);
                __value = NULL;
            }
        }

        //! get type of rk_type
        /*!
        */
        std::type_index type() {
            return __type;
        }

        //! construction with initial value
        /*!
         * \param value initial value
         */
        template<typename T>
            rk_type(const T &value) : __type(typeid(T)), __value(NULL) {
                set_value((uint8_t *) new T);
                *((T *) __value) = value;
            }

        //! copy constructor
        /*!
         * \param value initial value
         */
        rk_type(const rk_type &obj) : __type(obj.__type), __value(NULL) {
            set_value(obj.__value);
        }

        //! assign operator
        /*!
         * \param rhs rk_type to assign
         */
        rk_type &operator=(rk_type rhs) {
            set_value(rhs.__value);
            __type = rhs.__type;
            return *this;
        }

        //! cast operator
        /*!
         * \return casted data
         */
        template<typename T>
            operator T() const {
                if (__type != typeid(T)) {
                    throw string_util::str_exception("Unsupported cast");
                } else if (__value == NULL) {
                    throw string_util::str_exception("Uninitialized value!");
                }
                return *static_cast<T *>((void *) __value);
            }


        std::string to_string();

};

template<typename T>
T rk_type_cast(rk_type &rhs) {
    if (rhs.type() != typeid(T))
        throw std::exception();

    return *static_cast<T *>((void *) rhs.__value);
}

template<typename T>
std::vector<rk_type> convertVector(std::vector<T> &in) {
    std::vector<rk_type> out(in.size());
    for (int i = 0; i < in.size(); ++i) {
        out[i] = in[i];
    }
    return out;
}

template<typename T>
std::vector<T> convertVector2(std::vector<rk_type> &in) {
    std::vector<T> out(in.size());
    for (int i = 0; i < in.size(); ++i) {
        out[i] = (T) in[i];
    }
    return out;
}

#ifdef EMACS
{
#endif
} // namespace robotkernel

#endif // ROBOTKERNEL__RK_TYPE_H

