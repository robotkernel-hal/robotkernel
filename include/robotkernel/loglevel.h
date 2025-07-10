//! robotkernel loglevel
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

#ifndef ROBOTKERNEL__LOGLEVEL_H
#define ROBOTKERNEL__LOGLEVEL_H

#include <string>

namespace robotkernel {

enum level { 
    error   = 1,
    warning = 2,
    info    = 3,
    verbose = 4, 
};
    
class loglevel {
    public:
        level value;
    
        loglevel() { value = info; }
        loglevel(const level& l) { value = l; };
        loglevel(const loglevel& ll)
        { this->value = ll.value; }

        loglevel& operator=(const loglevel& ll)
        { this->value = ll.value; return *this; }
        loglevel& operator=(const std::string& ll_string);
        bool operator==(const loglevel& ll)
        { return (this->value == ll.value); }
        bool operator==(const level& l)
        { return (this->value == l); }
        bool operator>(const loglevel& ll)
        { return (this->value > ll.value); }
        bool operator>(const level& l)
        { return (this->value > l); }
        bool operator<(const loglevel& ll)
        { return (this->value < ll.value); }
        bool operator<(const level& l)
        { return (this->value < l); }
        operator std::string() const
        { 
            switch (value) {
                case error:
                    return std::string("error");
                case warning:
                    return std::string("warning");
                case info:
                    return std::string("info");
                case verbose:
                    return std::string("verbose");
            }

            return std::string("unknown");
        }
};

}; // namespace robotkernel

#endif // ROBOTKERNEL__LOGLEVEL_H

