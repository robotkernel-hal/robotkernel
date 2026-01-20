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
#include <ctime>

#include "robotkernel/exceptions.h"

namespace robotkernel {

/**
 * @brief Base class providing typed shared_from_this() helpers.
 *
 * Convenience base class that extends `std::enable_shared_from_this`
 * with type-safe casting helpers.
 *
 * This allows derived classes to obtain `std::shared_ptr<T>` or
 * `std::shared_ptr<const T>` without repeating
 * `std::dynamic_pointer_cast<>` logic.
 *
 * @note
 * - Objects must be created inside a `std::shared_ptr`.
 * - Calling `shared_from_this()` on objects not owned by a
 *   `std::shared_ptr` results in undefined behavior.
 *
 * @warning
 * - The cast is performed using `dynamic_pointer_cast`.
 * - If the type is incorrect, the returned pointer will be `nullptr`.
 *
 * @example
 * @code
 * struct my_type : public shared_base {
 *     void foo();
 * };
 *
 * std::shared_ptr<shared_base> base = std::make_shared<my_type>();
 *
 * auto derived = base->shared_from_this_as<my_type>();
 * if (derived) {
 *     derived->foo();
 * }
 * @endcode
 */
class shared_base : public std::enable_shared_from_this<shared_base> {
    public:
        /**
         * @brief Virtual destructor.
         *
         * Ensures proper cleanup of derived objects through base-class
         * pointers.
         */
        virtual ~shared_base() = default;

        /**
         * @brief Obtain a shared_ptr of a derived type.
         *
         * Performs a `dynamic_pointer_cast` on the internally managed
         * shared pointer.
         *
         * @tparam T
         *         Desired target type.
         *
         * @return
         *         Shared pointer to the requested type, or `nullptr`
         *         if the cast fails.
         */
        template<typename T>
            std::shared_ptr<T> shared_from_this_as() {
                return std::dynamic_pointer_cast<T>(shared_from_this());
            }

        /**
         * @brief Obtain a const shared_ptr of a derived type.
         *
         * Const-qualified variant of `shared_from_this_as<T>()`.
         *
         * @tparam T
         *         Desired target type.
         *
         * @return
         *         Shared pointer to the requested type, or `nullptr`
         *         if the cast fails.
         */
        template<typename T>
            std::shared_ptr<const T> shared_from_this_as() const {
                return std::dynamic_pointer_cast<const T>(shared_from_this());
            }
};

namespace helpers {

/**
 * @brief Add seconds and nanoseconds to a timespec.
 *
 * Handles nanosecond overflow properly.
 *
 * @param ts
 *        Pointer or reference to timespec to modify.
 *
 * @param sec
 *        Seconds to add.
 *
 * @param nsec
 *        Nanoseconds to add.
 */
inline void timespec_add(timespec& ts, time_t sec, long nsec)
{
    ts.tv_nsec += nsec;
    ts.tv_sec  += sec;

    if (ts.tv_nsec >= 1000000000L) {
        ts.tv_nsec -= 1000000000L;
        ts.tv_sec++;
    }
}

/**
 * @brief Compare two timespec structs using a comparison operator.
 *
 * @tparam Op
 *        Comparison operator function or lambda (e.g., std::less<>).
 *
 * @param lhs
 *        Left-hand side timespec.
 *
 * @param rhs
 *        Right-hand side timespec.
 *
 * @return
 *        Result of the comparison.
 *
 * @note
 * This mirrors the old macro: first compares seconds, then nanoseconds.
 */
template <typename Compare>
inline bool timespec_cmp(const timespec& lhs, const timespec& rhs, Compare cmp)
{
    if (lhs.tv_sec == rhs.tv_sec) {
        return cmp(lhs.tv_nsec, rhs.tv_nsec);
    } else {
        return cmp(lhs.tv_sec, rhs.tv_sec);
    }
}

/**
 * @brief Convenience functions for common comparisons.
 */
inline bool timespec_eq(const timespec& a, const timespec& b)
{ return timespec_cmp(a, b, [](auto lhs, auto rhs){ return lhs == rhs; }); }
inline bool timespec_lt(const timespec& a, const timespec& b)
{ return timespec_cmp(a, b, [](auto lhs, auto rhs){ return lhs < rhs; }); }
inline bool timespec_le(const timespec& a, const timespec& b)
{ return timespec_cmp(a, b, [](auto lhs, auto rhs){ return lhs <= rhs; }); }
inline bool timespec_gt(const timespec& a, const timespec& b)
{ return timespec_cmp(a, b, [](auto lhs, auto rhs){ return lhs > rhs; }); }
inline bool timespec_ge(const timespec& a, const timespec& b)
{ return timespec_cmp(a, b, [](auto lhs, auto rhs){ return lhs >= rhs; }); }

/**
 * @brief Resolve a symbol from a shared object and assign it to a variable.
 *
 * Retrieves a symbol from a dynamically loaded shared object and assigns
 * it to the provided output reference.
 *
 * This function is typically used to resolve function pointers or
 * global variables from shared libraries loaded via `dlopen()`.
 *
 * @tparam T
 *         Type of the symbol. Usually a function pointer or object pointer.
 *
 * @param out
 *        Reference that will receive the resolved symbol.
 *
 * @param so_handle
 *        Handle to the shared object obtained from `dlopen()`.
 *
 * @param symbol_name
 *        Name of the symbol to resolve.
 *
 * @throws std::runtime_error
 *         If the symbol cannot be resolved or the handle is invalid.
 *
 * @note
 * - Internally uses `dlsym()`.
 * - The caller is responsible for ensuring correct symbol type.
 * - No type checking is performed at runtime.
 *
 * @warning
 * Casting an incorrect symbol type results in undefined behavior.
 *
 * @example
 * @code
 * using init_fn_t = int(*)(int);
 *
 * init_fn_t init_fn;
 * get_symbol(init_fn, handle, "plugin_init");
 *
 * init_fn(42);
 * @endcode
 */
template <typename T>
void get_symbol(T& out, void* so_handle, const std::string& symbol_name) {
    out = reinterpret_cast<T>(dlsym(so_handle, symbol_name.c_str()));
    if (!out) {
        throw std::runtime_error("Missing symbol: " + symbol_name);
    }
}

/**
 * @brief Convert binary data to a hexadecimal string.
 *
 * Converts a raw memory buffer into a lowercase hexadecimal
 * string representation.
 *
 * @param data
 *        Pointer to input data buffer.
 *
 * @param len
 *        Number of bytes to convert.
 *
 * @return
 *        Hexadecimal string representation of the input data.
 *
 * @note
 * - Output contains no separators (e.g. "0a1bff").
 * - Commonly used for debugging, logging, and identifiers.
 */
std::string hex_string(const void *data, size_t len);

/**
 * @brief Fill a YAML template with instance-specific values.
 *
 * Takes a YAML configuration template and substitutes
 * placeholders using values from a provided instance node.
 *
 * Typical use case:
 * - Template defines reusable structure
 * - Instance provides concrete values
 * - Result is a fully resolved configuration
 *
 * @param config
 *        YAML node containing the template definition.
 *
 * @param instance
 *        YAML node containing instance-specific overrides.
 *
 * @return
 *        A new YAML node with substituted values.
 *
 * @throws YAML::Exception
 *         If parsing or substitution fails.
 *
 * @note
 * The exact placeholder syntax depends on the implementation,
 * e.g. `${var}` or similar.
 */
YAML::Node fill_template(const std::string& config, const YAML::Node& instance);

/**
 * @brief Parse a YAML configuration containing template instances.
 *
 * Parses a YAML configuration and expands all template-based
 * entries into a list of concrete instances.
 *
 * This function is typically used to:
 * - Expand configuration templates
 * - Generate multiple runtime instances from a single definition
 *
 * @param config
 *        Root YAML configuration node.
 *
 * @param instances
 *        Output list of resolved instance nodes.
 *
 * @throws YAML::Exception
 *         If parsing or expansion fails.
 */
void parse_templates(const YAML::Node& config, std::list<YAML::Node>& instances);

/**
 * @brief Format a string using printf-style formatting.
 *
 * Creates a std::string using printf-style formatting rules.
 *
 * @param format
 *        printf-compatible format string.
 *
 * @param ...
 *        Variable argument list.
 *
 * @return
 *        Formatted string.
 *
 * @throws std::runtime_error
 *         If formatting fails or memory allocation fails.
 *
 * @warning
 * This function is not type-safe.
 */
std::string string_printf(const char* format, ...) __attribute__((format (printf, 1, 2)));

/**
 * @brief Split a string by a delimiter character.
 *
 * Splits the input string into substrings separated
 * by the given delimiter.
 *
 * @param str
 *        Input string to split.
 *
 * @param delimiter
 *        Delimiting character.
 *
 * @return
 *        Vector of substrings.
 *
 * @note
 * - Empty tokens are preserved.
 * - No trimming is performed.
 *
 * @example
 * @code
 * auto parts = string_split("a,b,c", ',');
 * // → {"a", "b", "c"}
 * @endcode
 */
std::vector<std::string> string_split(const std::string& str, const char delimiter); 

/**
 * @brief Set scheduling policy and priority of the calling thread.
 *
 * Configures the scheduling policy and priority of the calling thread
 * using POSIX real-time scheduling APIs.
 *
 * Typical usage is to enable deterministic execution for real-time
 * threads using policies like `SCHED_FIFO` or `SCHED_RR`.
 *
 * @param priority
 *        Thread priority.
 *        Valid range depends on the selected policy:
 *        - `SCHED_FIFO`, `SCHED_RR`: usually 1–99
 *        - `SCHED_OTHER`: priority is ignored
 *
 * @param policy
 *        Scheduling policy (default: `SCHED_FIFO`).
 *        Supported values:
 *        - `SCHED_FIFO`
 *        - `SCHED_RR`
 *        - `SCHED_OTHER`
 *
 * @throws std::runtime_error
 *         If the scheduling parameters cannot be applied.
 *
 * @note
 * - Requires `CAP_SYS_NICE` or root privileges.
 * - On failure, `errno` may be set to:
 *   - `EPERM`  – insufficient privileges
 *   - `EINVAL` – invalid policy or priority
 *   - `ESRCH`  – thread not found
 *
 * @warning
 * Misuse of real-time priorities can starve the system.
 * Always leave at least one core for non-RT tasks.
 */
void set_priority(int priority, int policy = SCHED_FIFO);

/**
 * @brief Set CPU affinity mask for the calling thread.
 *
 * Restricts execution of the calling thread to the CPUs specified
 * by the given bitmask.
 *
 * @param affinity_mask
 *        Bitmask representing allowed CPUs.
 *        Example:
 *        - `0x01` → CPU 0
 *        - `0x03` → CPU 0 and 1
 *
 * @throws std::runtime_error
 *         If the affinity cannot be applied.
 *
 * @note
 * - Uses `pthread_setaffinity_np()`
 * - CPU indices correspond to logical CPUs as seen by the OS
 *
 * @warning
 * Binding threads incorrectly may reduce performance or cause
 * priority inversion.
 */
void set_affinity_mask(int affinity_mask);

/**
 * @brief Set name of a thread using a std::thread object.
 *
 * Assigns a human-readable name to a thread for debugging
 * and diagnostic tools (e.g. `top`, `htop`, `perf`).
 *
 * @param tid
 *        Reference to a valid `std::thread`.
 *
 * @param thread_name
 *        Name to assign.
 *        Linux limits names to 16 bytes including null terminator
 *        (15 visible characters).
 *
 * @throws std::runtime_error
 *         If the thread name cannot be set.
 *
 * @note
 * Internally uses `pthread_setname_np()`.
 */
void set_thread_name(std::thread& tid, const std::string& thread_name);

/**
 * @brief Set name of a thread using a pthread handle.
 *
 * Assigns a human-readable name to a POSIX thread.
 *
 * @param tid
 *        pthread handle of the target thread.
 *
 * @param thread_name
 *        Name to assign (max 15 characters).
 *
 * @throws std::runtime_error
 *         If the thread name cannot be set.
 */
void set_thread_name(pthread_t tid, const std::string& thread_name);


/**
 * @brief Set name of the calling thread.
 *
 * Convenience overload to rename the currently executing thread.
 *
 * @param thread_name
 *        Name to assign (max 15 characters).
 *
 * @throws std::runtime_error
 *         If the thread name cannot be set.
 */
void set_thread_name(const std::string& thread_name);

/**
 * @brief Retrieve a typed value from a YAML node.
 *
 * Extracts a value associated with the given key and converts it
 * to the requested type.
 *
 * @tparam type
 *         Target type for conversion.
 *
 * @param node
 *        YAML node containing the key.
 *
 * @param key
 *        Key to look up in the node.
 *
 * @return
 *        Value converted to the requested type.
 *
 * @throws YAML::Exception
 *         If the key does not exist or the value cannot be converted.
 *
 * @note
 * This function is strict:
 * - Missing keys result in an exception.
 * - Type mismatches result in an exception.
 */
template <typename type>
type get_as(const YAML::Node& node, const std::string key) {
    if (!node[key]) {
        YAML::Emitter out;
        out << node;

        throw std::runtime_error(robotkernel::helpers::string_printf("[config-error] key \"%s\" not found!\n\n%s\n", 
                key.c_str(), out.c_str()));
    }

    try {
        // gcc3.3 need this syntax for calling this template 
        return node[key].template as<type>();
    } catch(const std::exception& e) {
        throw std::runtime_error(robotkernel::helpers::string_printf("[config-error] key \"%s\" is probably of " 
                "wrong data-type:\n%s", key.c_str(), e.what()));
    }
}

/**
 * @brief Retrieve a typed value from a YAML node with a default fallback.
 *
 * Attempts to extract a value associated with the given key and convert
 * it to the requested type. If the key does not exist or conversion fails,
 * the provided default value is returned.
 *
 * @tparam type
 *         Target type for conversion.
 *
 * @param node
 *        YAML node containing the key.
 *
 * @param key
 *        Key to look up in the node.
 *
 * @param dflt
 *        Default value to return if the key is missing or invalid.
 *
 * @return
 *        Extracted value or the default.
 *
 * @note
 * This function is non-throwing for missing keys.
 *
 * @example
 * @code
 * int timeout = get_as<int>(cfg, "timeout", 1000);
 * @endcode
 */
template <typename type>
type get_as(const YAML::Node& node, const std::string key, type dflt) {
    if (!node[key])
        return dflt;

    try {
        // gcc3.3 need this syntax for calling this template 
        return node[key].template as<type>();
    } catch(const std::exception& e) {
        throw std::runtime_error(robotkernel::helpers::string_printf("[config-error] key \"%s\" is probably of "
                "wrong data-type:\n%s", key.c_str(), e.what()));
    }
}

/**
 * @brief Map a YAML string entry to a typed configuration value.
 *
 * Looks up a string value in a YAML node and maps it to a corresponding
 * value using a lookup table.
 *
 * Commonly used to translate human-readable configuration values
 * into enums or constants.
 *
 * @tparam type
 *         Target value type.
 *
 * @param node
 *        YAML node containing the entry.
 *
 * @param entry
 *        Key to look up in the YAML node.
 *
 * @param configmapping
 *        Map of string keys to corresponding values.
 *
 * @return
 *        Mapped value.
 *
 * @throws std::runtime_error
 *         If the key does not exist or mapping fails.
 *
 * @example
 * @code
 * std::map<std::string, int> modes = {
 *     {"auto",  0},
 *     {"manual", 1}
 * };
 *
 * int mode = match_map(cfg, "mode", modes);
 * @endcode
 */
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

}; // namespace helpers

}; // namespace robotkernel

#endif /* ROBOTKERNEL__HELPERS_H */

