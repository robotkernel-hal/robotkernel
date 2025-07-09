//! robotkernel service_interface class
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

#ifndef ROBOTKERNEL_SERVICE_INTERFACE_H
#define ROBOTKERNEL_SERVICE_INTERFACE_H

#include <string>

#include "yaml-cpp/yaml.h"
#include "robotkernel/device.h"

namespace robotkernel {
#ifdef EMACS
}
#endif

class service_interface :
    public device
{
    public:
        //! construction
        /*!
         * \param[in] owner             Service interface owner string.
         * \param[in] service_suffix    Name of service, will be prepended by \p owner.
         */
        service_interface(std::string owner, std::string service_suffix) : 
            device(owner, service_suffix, "service_interface") {};

        //! destruction
        virtual ~service_interface() = 0;
};

//! destruction
inline service_interface::~service_interface() {}

typedef std::shared_ptr<service_interface> sp_service_interface_t;
typedef std::list<sp_service_interface_t> service_interface_list_t;

#ifdef EMACS
{
#endif
}

#endif // ROBOTKERNEL_SERVICE_INTERFACE_H

