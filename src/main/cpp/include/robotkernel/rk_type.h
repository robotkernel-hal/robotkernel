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

namespace robotkernel {

class rk_type {
    protected:
        template <typename T>
        friend T rk_type_cast(rk_type&);

        std::type_index __type;     //!< data type for type conversions
        uint8_t *__value;           //!< data storage pointer

    public:
        //! default constructor
        rk_type() : __type(typeid(int)), __value(NULL) {}

        //! destructor
        ~rk_type() { if (__value) delete __value; }

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
        template <typename T>
        rk_type(const T& value) : __type(typeid(T)) {
            __value = (uint8_t *)new T;
            *((T *)__value) = value;
        }

        //! assign operator
        /*!
         * \param rhs rk_type to assign
         */
        rk_type& operator=(rk_type rhs) {
            __type = rhs.__type;
            std::swap(__value, rhs.__value);
            return *this;
        }

        //! cast operator
        /*!
         * \return casted data
         */
        template <typename T>
        operator T () const {
            if (__type != typeid(T))
                throw std::exception();

            return *static_cast<T*>((void*)__value);
        }

};

template <typename T>
T rk_type_cast(rk_type& rhs) {
    if (rhs.type() != typeid(T))
        throw std::exception();

    return *static_cast<T*>((void*)rhs.__value);
}


} // namespace robotkernel

#endif // __RK_TYPE_H__

