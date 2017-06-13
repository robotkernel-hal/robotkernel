//! robotkernel service_requester class
/*!
 * author: Robert Burger <robert.burger@dlr.de>
 *
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

#ifndef __ROBOTKERNEL_SERVICE_REQUESTER_H__
#define __ROBOTKERNEL_SERVICE_REQUESTER_H__

#include <string>

#include "yaml-cpp/yaml.h"
#include "robotkernel/kernel.h"

namespace robotkernel {
#ifdef EMACS
}
#endif

class service_requester_base {
    public:
        std::string owner;              //!< owner
        std::string service_prefix;     //!< service prefix

        //! construction
        /*!
         * \param owner service requester owner string
         * \param service_prefix prefix all service with this prefix
         */
        service_requester_base(std::string owner, 
                std::string service_prefix) : 
            owner(owner), service_prefix(service_prefix) {};

        //! destruction
        virtual ~service_requester_base() = 0;
};

//! destruction
inline service_requester_base::~service_requester_base() {}

typedef std::shared_ptr<service_requester_base> sp_service_requester_t;
typedef std::list<sp_service_requester_t> service_requester_list_t;

#ifdef EMACS
{
#endif
}

#endif // __ROBOTKERNEL_SERVICE_PROVIDER_H__

