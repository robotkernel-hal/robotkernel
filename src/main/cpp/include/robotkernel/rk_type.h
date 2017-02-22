//! robotkernel rk_type
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

#ifndef __RK_TYPE_H__
#define __RK_TYPE_H__


#include <functional>
#include <typeindex>
#include <map>
#include <pthread.h>
#include "string_util/string_util.h"

namespace robotkernel {

    typedef std::map<uint8_t *, size_t> ReferenceMap;

    class rk_type {
    protected:
        template<typename T>
        friend T rk_type_cast(rk_type &);

        static ReferenceMap __refs;
        static pthread_mutex_t __refsLock;

        std::type_index __type;      //!< data type for type conversions
        uint8_t *__value;     //!< data storage pointer


        /*
        void printRefs(){
            printf("Refs:\n");
            for(ReferenceMap::iterator it = __refs.begin(); it != __refs.end(); ++it) {
                printf("      %p -> %d\n", it->first, it->second);
            }
        }
         */

        void decreaseReference(uint8_t *ref) {
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

        void increaseReference(uint8_t *ref) {
            if (!ref) {
                return;
            }
            pthread_mutex_lock(&__refsLock);
            ReferenceMap::iterator r = __refs.find(ref);
            if (r == __refs.end()) {
                __refs[ref] = 1;
            } else {
                r->second++;
            }
            pthread_mutex_unlock(&__refsLock);
        }

        void setValue(uint8_t *value) {
            if (value == __value) {
                return;
            }
            decreaseReference(__value);
            increaseReference(value);
            __value = value;
        }

    public:
        //! default constructor
        rk_type() : __type(typeid(int)), __value(NULL) {
        }

        //! destructor
        ~rk_type() {
            if (__value) {
                decreaseReference(__value);
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
            setValue((uint8_t *) new T);
            *((T *) __value) = value;
        }

        //! copy constructor
        /*!
         * \param value initial value
         */
        rk_type(const rk_type &obj) : __type(obj.__type), __value(NULL) {
            setValue(obj.__value);
        }

        //! assign operator
        /*!
         * \param rhs rk_type to assign
         */
        rk_type &operator=(rk_type rhs) {
            setValue(rhs.__value);
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
                throw str_exception("Unsupported cast");
            } else if (__value == NULL) {
                throw str_exception("Uninitialized value!");
            }
            return *static_cast<T *>((void *) __value);
        }


        std::string toString();

    };

    template<typename T>
    T rk_type_cast(rk_type &rhs) {
        if (rhs.type() != typeid(T))
            throw std::exception();

        return *static_cast<T *>((void *) rhs.__value);
    }


} // namespace robotkernel

#endif // __RK_TYPE_H__

