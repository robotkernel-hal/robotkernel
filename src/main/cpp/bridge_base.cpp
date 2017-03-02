//! robotkernel bridge base
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

#include "robotkernel/bridge_base.h"
#include <sstream>

using namespace std;
using namespace std::placeholders;
using namespace robotkernel;

//! construction
/*!
 * \param instance_name bridge name
 * \param name instance name
 */
bridge_base::bridge_base(const std::string& instance_name, 
		const std::string& name, const YAML::Node& node) 
	: log_base(instance_name, name, node) {
}

