//! robotkernel device base class
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

#ifndef ROBOTKERNEL__DEVICE_H
#define ROBOTKERNEL__DEVICE_H

namespace robotkernel {
#ifdef EMACS
}
#endif

class device {
    public:
        const std::string owner;        //!< device owner
        const std::string device_name;  //!< device instance name
        const std::string suffix;       //!< device suffix name

    public:
        device(
                const std::string& owner, 
                const std::string& device_name,
                const std::string& suffix) :
            owner(owner), device_name(device_name), suffix(suffix) {};
        virtual ~device() {};

        std::string id() const {
            return owner + std::string(".") + device_name + std::string(".") + suffix;
        };
};

typedef std::shared_ptr<device> sp_device_t;
typedef std::map<std::string, sp_device_t> device_map_t;

#ifdef EMACS
{
#endif
} // namespace robotkernel

#endif // ROBOTKERNEL__DEVICE_H

