//! robotkernel base class for triggers
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

#ifndef ROBOTKERNEL__TRIGGER_BASE_H
#define ROBOTKERNEL__TRIGGER_BASE_H

#include <string>
#include <memory>
#include <list>

namespace robotkernel {

//! trigger_base
/*
 * derive from this class if you want to get 
 * triggered by a trigger_device
 */
class trigger_base {
    public:
        int divisor;        //!< trigger every ""divisor"" step
        int cnt;            //!< internal step counter

        trigger_base(int divisor=1) : divisor(divisor), cnt(0) {};
    
        //! trigger function
        virtual void tick() = 0;
};

typedef std::shared_ptr<trigger_base> sp_trigger_base_t;
typedef std::list<sp_trigger_base_t> trigger_list_t;

}; // namespace robotkernel;

#endif // ROBOTKERNEL__TRIGGER_BASE_H

