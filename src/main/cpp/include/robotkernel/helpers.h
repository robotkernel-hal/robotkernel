//! robotkernel helper functions
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

#ifndef ROBOTKERNEL__HELPERS_H
#define ROBOTKERNEL__HELPERS_H

#include <time.h>
#include <yaml-cpp/yaml.h>

#include "robotkernel/exceptions.h"

#define timespec_add(tvp, sec, nsec) { \
    (tvp)->tv_nsec += nsec; \
    (tvp)->tv_sec += sec; \
    if ((tvp)->tv_nsec > (long int)1E9) { \
        (tvp)->tv_nsec -= (long int)1E9; \
        (tvp)->tv_sec++; } }

#define timespec_cmp(tvp, uvp, cmp) \
    (((tvp)->tv_sec == (uvp)->tv_sec) ? \
     ((tvp)->tv_nsec cmp (uvp)->tv_nsec) : \
     ((tvp)->tv_sec cmp (uvp)->tv_sec))

template <typename type>
type get_as(const YAML::Node& node, const std::string key) {
    if (!node[key]) {
        YAML::Emitter out;
        out << node;

        throw string_util::str_exception("[config-error] key \"%s\" not found!\n\n%s\n", 
                key.c_str(), out.c_str());
    }

    try {
        // gcc3.3 need this syntax for calling this template 
        return node[key].template as<type>();
    } catch(const std::exception& e) {
        throw string_util::str_exception("[config-error] key \"%s\" is probably of " 
                "wrong data-type:\n%s", key.c_str(), e.what());
    }
}

template <typename type>
type get_as(const YAML::Node& node, const std::string key, type dflt) {
    if (!node[key])
        return dflt;

    try {
        // gcc3.3 need this syntax for calling this template 
        return node[key].template as<type>();
    } catch(const std::exception& e) {
        throw string_util::str_exception("[config-error] key \"%s\" is probably of "
                "wrong data-type:\n%s", key.c_str(), e.what());
    }
}

#endif // ROBOTKERNEL__MODULE_H

