//! robotkernel module base
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

#include "robotkernel/module_base.h"
#include <sstream>

using namespace std;
using namespace std::placeholders;
using namespace robotkernel;

//! construction
/*!
 * \param instance_name module name
 * \param name instance name
 */
module_base::module_base(const std::string& instance_name, const std::string& name, 
        const YAML::Node& node) : log_base(instance_name, name, node) {
    state = module_state_init;
}

module_base::~module_base() {
}

//! get robotkernel module
/*!
 * \returns shared pointer to our robotkernel module class
 */
robotkernel::kernel::sp_module_t module_base::get_module() {
    kernel& k = *kernel::get_instance();
    return k.get_module(name);
}

