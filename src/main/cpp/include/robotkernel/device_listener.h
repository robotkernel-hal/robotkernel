//! robotkernel device listener class
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

#ifndef ROBOTKERNEL_DEVICE_LISTENER_H
#define ROBOTKERNEL_DEVICE_LISTENER_H

#include <string>

#include "yaml-cpp/yaml.h"
#include "robotkernel/device.h"

namespace robotkernel {
#ifdef EMACS
}
#endif

class device_listener 
{
    public: 
        const std::string owner;        //!< device owner
        const std::string name;         //!< listener name

    public:
        //! construction
        device_listener(const std::string& owner, const std::string& name) :
            owner(owner), name(name) {};

        //! destruction
        virtual ~device_listener() = 0;

        // add a named device
        virtual void notify_add_device(sp_device_t req) = 0;

        // remove a named device
        virtual void notify_remove_device(sp_device_t req) = 0;
};

//! destruction
inline device_listener::~device_listener() {}

typedef std::shared_ptr<device_listener> sp_device_listener_t;
typedef std::map<std::pair<std::string, std::string>, sp_device_listener_t> device_listener_map_t;

#ifdef EMACS
{
#endif
}

#endif // ROBOTKERNEL_device_listener_H
