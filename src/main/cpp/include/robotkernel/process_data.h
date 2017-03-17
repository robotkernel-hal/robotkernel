//! robotkernel process_data
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

#ifndef __PROCESS_DATA_H__
#define __PROCESS_DATA_H__

#include <vector>
#include <string>
#include <map>
#include <list>
#include <functional>

#include "robotkernel/rk_type.h"

namespace robotkernel {

	typedef std::vector<rk_type> process_data_arglist_t;
	typedef std::function<int(const process_data_arglist_t&, process_data_arglist_t&)> process_data_callback_t;

	typedef struct process_data {
		std::string owner;
		std::string name;
		std::string process_data_definition;
		process_data_callback_t callback;
	} process_data_t;

	typedef std::map<std::string, process_data_t *> process_data_list_t;

    // typedef std::function<int(char *buf, ssize_t buflen)> pd_cb_t;
    //
    // module calls 
    //    k.register_pd_provider(owner, name, pd_definition, pd_cb) 
    //
    //    
} // namespace robotkernel

#endif // __PROCESS_DATA_H__

