//! robotkernel service
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

#ifndef ROBOTKERNEL__SERVICE_H
#define ROBOTKERNEL__SERVICE_H

#include <vector>
#include <string>
#include <map>
#include <list>
#include <functional>

#include "robotkernel/rk_type.h"

typedef void (*get_sd_t)(std::list<std::string>& sd_list);

namespace robotkernel {
#ifdef EMACS
}
#endif

typedef std::vector<rk_type> service_arglist_t;
typedef std::function<int(const service_arglist_t&, service_arglist_t&)> service_callback_t;

typedef struct service {
    std::string owner;
    std::string name;
    std::string service_definition;
    service_callback_t callback;
} service_t;

typedef std::map<std::pair<std::string, std::string>, service_t *> service_map_t;

#ifdef EMACS
{
#endif
} // namespace robotkernel

#endif // ROBOTKERNEL__SERVICE_H

