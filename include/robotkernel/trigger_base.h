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
#include <functional>

#include <yaml-cpp/yaml.h>

#include "robotkernel/helpers.h"

namespace robotkernel {

class trigger;

//! trigger_base
/*
 * derive from this class if you want to get 
 * triggered by a trigger_device
 */
class trigger_base :
    public virtual shared_base
{
    public:
        int divisor = 1;                    //!< trigger every ""divisor"" step
        int cnt = 0;                        //!< internal step counter
        bool direct_mode = true;
        int worker_prio = 0;
        int worker_affinity = 0xFFFFFFFF;
        std::string dev_name = "";
        std::shared_ptr<robotkernel::trigger> dev = nullptr;

        trigger_base(int divisor=1) : 
            divisor(divisor)
        {};

        trigger_base(const YAML::Node& node) {
            divisor = get_as<int>(node, "divisor", 1);
            direct_mode = get_as<bool>(node, "direct_mode", true);
            worker_prio = get_as<int>(node, "worker_prio", 0);
            worker_affinity = get_as<int>(node, "worker_affinity", 0xFFFFFFFF);
            dev_name = get_as<std::string>(node, "dev_name");
        }

        ~trigger_base() { release(); }
    
        //! trigger function
        virtual void tick() = 0;

        /*! @brief Get trigger device from robotkernel and register us
         * as trigger.
         */
        void aquire(void);

        /*! @brief Remove us as trigger devce.
         */
        void release(void);
};

typedef std::shared_ptr<trigger_base> sp_trigger_base_t;
typedef std::list<sp_trigger_base_t> trigger_list_t;

class triggerable : 
    public trigger_base
{
    private:
        std::function<void(void)> cb;

    public:
        triggerable(const YAML::Node& node, std::function<void(void)> cb) :
            trigger_base(node), cb(cb) {}
         
        //! trigger function
        virtual void tick() override { cb(); }
};

}; // namespace robotkernel;

#endif // ROBOTKERNEL__TRIGGER_BASE_H

