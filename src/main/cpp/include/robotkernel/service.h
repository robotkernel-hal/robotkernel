//! robotkernel service
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

#ifndef __SERVICE_H__
#define __SERVICE_H__

#include <vector>
#include <string>
#include <map>
#include <list>
#include <functional>

#include "robotkernel/rk_type.h"

namespace robotkernel {

	typedef std::vector<rk_type> service_arglist_t;
	typedef std::function<int(const service_arglist_t&, service_arglist_t&)> service_callback_t;

	typedef struct service {
		std::string owner;
		std::string name;
		std::string service_definition;
		service_callback_t callback;
	} service_t;

	typedef std::map<std::string, service_t *> service_list_t;

} // namespace robotkernel

#endif // __SERVICE_H__

