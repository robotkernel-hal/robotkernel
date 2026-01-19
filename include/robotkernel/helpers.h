//! robotkernel helper functions
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

#ifndef ROBOTKERNEL__HELPERS_H
#define ROBOTKERNEL__HELPERS_H

#include <algorithm>
#include <time.h>
#include <yaml-cpp/yaml.h>
#include <stdarg.h>  // For va_start, etc.
#include <memory>    // For std::unique_ptr
#include <pthread.h>
#include <vector>

#include <string>
#include <thread>
#include <stdexcept>
#include <dlfcn.h>

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

         

namespace robotkernel {

/*! @brief Getting symbols from loaded shared object file handle. */
template <typename T>
void get_symbol(T& out, void* so_handle, const std::string& symbol_name) {
    out = reinterpret_cast<T>(dlsym(so_handle, symbol_name.c_str()));
    if (!out) {
        throw std::runtime_error("Missing symbol: " + symbol_name);
    }
}

//! convert buffer to hex string
std::string hex_string(const void *data, size_t len);

YAML::Node fill_template(const std::string& config, const YAML::Node& instance);
void parse_templates(const YAML::Node& config, std::list<YAML::Node>& instances);

std::string string_printf(const char* format, ...);
std::vector<std::string> string_split(const std::string& str, const char delimiter); 

void set_priority(int priority, int policy = SCHED_FIFO);
void set_affinity_mask(int affinity_mask);

void set_thread_name(std::thread& tid, const std::string& thread_name);
void set_thread_name(pthread_t tid, const std::string& thread_name);
void set_thread_name(const std::string& thread_name);

class shared_base : public std::enable_shared_from_this<shared_base> {
    public:
        virtual ~shared_base() = default;

        template<typename T>
            std::shared_ptr<T> shared_from_this_as() {
                return std::dynamic_pointer_cast<T>(shared_from_this());
            }

        template<typename T>
            std::shared_ptr<const T> shared_from_this_as() const {
                return std::dynamic_pointer_cast<const T>(shared_from_this());
            }
};

}; // namespace robotkernel

template <typename type>
type get_as(const YAML::Node& node, const std::string key) {
    if (!node[key]) {
        YAML::Emitter out;
        out << node;

        throw std::runtime_error(robotkernel::string_printf("[config-error] key \"%s\" not found!\n\n%s\n", 
                key.c_str(), out.c_str()));
    }

    try {
        // gcc3.3 need this syntax for calling this template 
        return node[key].template as<type>();
    } catch(const std::exception& e) {
        throw std::runtime_error(robotkernel::string_printf("[config-error] key \"%s\" is probably of " 
                "wrong data-type:\n%s", key.c_str(), e.what()));
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
        throw std::runtime_error(robotkernel::string_printf("[config-error] key \"%s\" is probably of "
                "wrong data-type:\n%s", key.c_str(), e.what()));
    }
}

template<typename type>
type match_map(const YAML::Node& node, std::string entry, std::map<std::string, type>& configmapping) {
    std::transform(entry.begin(), entry.end(), entry.begin(), ::tolower);
    const std::string default_str("default");
    typename std::map<std::string, type>::iterator el;

    if (node[entry]) {
        std::string key = node[entry].as<std::string>();
        el = configmapping.find(key);

        if (el != configmapping.end()) {
            return el->second;
        }

        goto match_map_exit;
    }

    el = configmapping.find(default_str);

    if (el != configmapping.end()) {
        return el->second;
    }

match_map_exit:
    std::stringstream ss;
    ss << "Unable to find value for MANDATORY key %s in config. Possible values for " << entry << " are: ";
    for (const auto& kv : configmapping) {
        ss << "\'" << kv.first << "\', ";
    }

    throw std::runtime_error(ss.str());
}

#endif // ROBOTKERNEL__MODULE_H

