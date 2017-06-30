//! robotkernel service_collector_device class
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

#ifndef __ROBOTKERNEL_SERVICE_COLLECTOR_DEVICE_H__
#define __ROBOTKERNEL_SERVICE_COLLECTOR_DEVICE_H__

#include <string>

#include "yaml-cpp/yaml.h"
#include "robotkernel/device.h"

namespace robotkernel {
#ifdef EMACS
}
#endif

class service_collector_device :
    public device
{
    public:
        //! construction
        /*!
         * \param owner service requester owner string
         * \param service_prefix prefix all service with this prefix
         */
        service_collector_device(std::string owner, 
                std::string service_prefix) : 
            device(owner, service_prefix, "service_collector") {};

        //! destruction
        virtual ~service_collector_device() = 0;
};

//! destruction
inline service_collector_device::~service_collector_device() {}

typedef std::shared_ptr<service_collector_device> sp_service_collector_device_t;
typedef std::list<sp_service_collector_device_t> service_collector_device_list_t;

#ifdef EMACS
{
#endif
}

#endif // __ROBOTKERNEL_SERVICE_COLLECTOR_DEVICE_H__

