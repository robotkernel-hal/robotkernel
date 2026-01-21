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

/**
 * @file loglevel.h
 * @brief Definition of log level types and helpers for robotkernel.
 *
 * This header defines the log levels usable throughout the robotkernel
 * framework as both an enum and a lightweight wrapper class.
 *
 * The wrapper `loglevel` class holds one of the severity levels,
 * supports comparison operators, assignment, and conversion from
 * strings, and can be implicitly converted to `std::string`.
 *
 * Usage of log levels is consistent across bridges, devices, services,
 * and the robotkernel logging facility. :contentReference[oaicite:0]{index=0}
 */

#ifndef ROBOTKERNEL__LOGLEVEL_H
#define ROBOTKERNEL__LOGLEVEL_H

#include <string>

namespace robotkernel {

/**
 * @enum level
 * @brief Log severity levels for robotkernel components.
 *
 * These represent the increasing verbosity of logged output:
 *
 * - `error`   — only error messages
 * - `warning` — error + warnings
 * - `info`    — informational messages
 * - `verbose` — highly detailed output
 *
 * The default is `info`. :contentReference[oaicite:1]{index=1}
 */
enum level
{
    error   = 1, ///< Critical error conditions.
    warning = 2, ///< Warning conditions.
    info    = 3, ///< Informational (default).
    verbose = 4  ///< Most detailed, verbose logging.
};

/**
 * @class loglevel
 * @brief Lightweight wrapper around the `level` enum.
 *
 * This class wraps a `level` value and provides:
 *   - Default, copy, and string assignment constructors.
 *   - Comparison operators (==, <, >).
 *   - Implicit conversion to `std::string` for human output.
 *
 * It is used by components (bridges, devices, listeners, services)
 * to hold and compare log severity in a consistent way. :contentReference[oaicite:2]{index=2}
 */
class loglevel
{
    public:
        /// Current log level value.
        level value;
    
    public:
        /**
         * @brief Default constructor initializes to `info`.
         */
        loglevel()
            : value(info)
        {
        }
    
        /**
         * @brief Construct from a `level` enum value.
         *
         * @param l
         *        The enum value to assign.
         */
        loglevel(const level &l)
            : value(l)
        {
        }
    
        /**
         * @brief Copy constructor.
         *
         * @param ll
         *        Another loglevel to copy from.
         */
        loglevel(const loglevel &ll)
            : value(ll.value)
        {
        }
    
        /**
         * @brief Assign from another loglevel.
         *
         * @param ll
         *        The source loglevel.
         * @return Reference to this.
         */
        loglevel &operator=(const loglevel &ll)
        {
            this->value = ll.value;
            return *this;
        }
    
        /**
         * @brief Assign from a string representation.
         *
         * Accepted case-insensitive strings:
         *   - "error"
         *   - "warning"
         *   - "info"
         *   - "verbose"
         *
         * @param ll_string
         *        String containing a log level name.
         * @return Reference to this.
         *
         * @note
         * Implementations typically map known strings to enum values
         * and leave the value unchanged for unknown names.
         */
        loglevel &operator=(const std::string &ll_string);
    
        /** Comparison with another `loglevel`. */
        bool operator==(const loglevel &ll) { return (this->value == ll.value); }
    
        /** Comparison with a `level` enum. */
        bool operator==(const level &l) { return (this->value == l); }
    
        /** Greater-than comparison with another `loglevel`. */
        bool operator>(const loglevel &ll) { return (this->value > ll.value); }
    
        /** Greater-than comparison with a `level` enum. */
        bool operator>(const level &l) { return (this->value > l); }
    
        /** Less-than comparison with another `loglevel`. */
        bool operator<(const loglevel &ll) { return (this->value < ll.value); }
    
        /** Less-than comparison with a `level` enum. */
        bool operator<(const level &l) { return (this->value < l); }
    
        /**
         * @brief Convert to a human readable string.
         *
         * Returns one of:
         *   - "error"
         *   - "warning"
         *   - "info"
         *   - "verbose"
         *
         * Unknown values yield "unknown". :contentReference[oaicite:3]{index=3}
         */
        operator std::string() const
        {
            switch (value) {
            case error:   return std::string("error");
            case warning: return std::string("warning");
            case info:    return std::string("info");
            case verbose: return std::string("verbose");
            }
            return std::string("unknown");
        }
};

}; // namespace robotkernel

#endif // ROBOTKERNEL__LOGLEVEL_H

