//! robotkernel module base
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

#ifndef ROBOTKERNEL_KERNEL_BASE_H
#define ROBOTKERNEL_KERNEL_BASE_H

#include "robotkernel/device.h"
#include "robotkernel/device_listener.h"
#include "robotkernel/service.h"

namespace robotkernel {

class kernel_base {
    public:
        //! add service to kernel
        /*!
         * \param owner service owner
         * \param name service name
         * \param service_definition service definition
         * \param callback service callback
         */
        static void add_service(
                const std::string &owner,
                const std::string &name,
                const std::string &service_definition,
                service_callback_t callback);

        //! remove on service given by name
        /*!
         * \param[in] owner     Owner of service.
         * \param[in] name      Name of service.
         */
        static void remove_service(
                const std::string& owner, 
                const std::string& name);

        //! adds a device listener
        /*
         * \param[in] dl    device listener to add. this device listener
         *                  will be notified whenever a new device is added.
         */
        static void add_device_listener(sp_device_listener_t dl);
        
        //! remove a device listener
        /*
         * \param[in] dl    device listener to reomve. this device listener
         *                  will no longer be notified when a new device is added.
         */
        static void remove_device_listener(sp_device_listener_t dl);

        //! add a named device
        /*
         * \param req device to add
         */
        static void add_device(sp_device_t req);

        //! remove a named device
        /*!
         * \param req device to remove
         */
        static void remove_device(sp_device_t req);
};

}; // namespace robotkernel;

#endif // ROBOTKERNEL_KERNEL_BASE_H


