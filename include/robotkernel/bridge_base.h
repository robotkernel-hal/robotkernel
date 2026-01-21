//! robotkernel bridge base
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
 * @file bridge_base.h
 * @brief Base definitions for robotkernel bridge components.
 *
 * This header defines the fundamental types and base class used
 * by *bridge* modules in robotkernel. A *bridge* is a plugin-style
 * component that can be dynamically configured from C code,
 * exposes services, and interacts with the robotkernel runtime.
 *
 * The header provides:
 *   - A C-ABI for configuring/unconfiguring bridges
 *   - Function pointer typedefs for common bridge operations
 *   - A C++ abstract base class `robotkernel::bridge_base`
 *     for implementing concrete bridge logic
 *
 * The C ABI is intended for foreign language bindings or dynamic
 * loading of bridge modules via shared objects.
 *
 * @note
 * - The C interface uses opaque bridge handles (`void*`).
 * - Concrete bridge classes must derive from `bridge_base`
 *   and implement service registration methods.
 */

#ifndef ROBOTKERNEL_BRIDGE_BASE_H
#define ROBOTKERNEL_BRIDGE_BASE_H

#include <unistd.h>
#include <stdint.h>
#include <list>
#include <stdio.h>
#include <stdexcept>

// public headers
#include "robotkernel/log_base.h"
#include "robotkernel/exceptions.h"
#include "robotkernel/helpers.h"
#include <robotkernel/service.h>

#include "yaml-cpp/yaml.h"

#ifdef __cplusplus
#define EXPORT_C extern "C" 
#else
#define EXPORT_C
#endif

/**
 * @typedef BRIDGE_HANDLE
 * @brief Opaque pointer representing a configured bridge instance.
 *
 * This type is used in the C-ABI to refer to a bridge object
 * without exposing C++ types.
 */
#define BRIDGE_HANDLE void*

/**
 * @brief Signature for the bridge configuration function.
 *
 * This function is called to construct and initialize a bridge
 * instance. It receives a name and a YAML configuration string.
 *
 * @param name
 *        Null-terminated string identifying the bridge instance.
 *
 * @param config
 *        Null-terminated YAML configuration.
 *
 * @return
 *        A handle to the newly created bridge instance,
 *        or nullptr on failure.
 */
typedef BRIDGE_HANDLE (*bridge_configure_t)(const char* name, const char* config);

/**
 * @brief Signature for the bridge unconfigure function.
 *
 * This function is called to destroy and clean up a bridge.
 *
 * @param hdl
 *        The bridge handle returned from bridge_configure().
 *
 * @return
 *        Zero on success or a negative error code.
 */
typedef int (*bridge_unconfigure_t)(BRIDGE_HANDLE hdl);

/**
 * @brief Signature for adding a robotkernel service.
 *
 * Allows the bridge to register a service with robotkernel.
 *
 * @param hdl
 *        The bridge handle.
 *
 * @param svc
 *        A `service_t` struct defining the service to add.
 */
typedef void (*bridge_add_service_t)(BRIDGE_HANDLE hdl, const robotkernel::service_t &svc);

/**
 * @brief Signature for removing a robotkernel service.
 *
 * Allows the bridge to unregister a service.
 *
 * @param hdl
 *        The bridge handle.
 *
 * @param svc
 *        A `service_t` struct identifying the service to remove.
 */
typedef void (*bridge_remove_service_t)(BRIDGE_HANDLE hdl, const robotkernel::service_t &svc);

#ifdef __cplusplus

#define HDL_2_BRIDGECLASS(hdl, bridgename, bridgeclass)                                     \
    struct instance_name ## _wrapper *wr =                                                  \
        reinterpret_cast<struct instance_name ## _wrapper *>(hdl);                          \
    if (!wr->sp)                                                                            \
        throw std::runtime_error(string("["#bridgename"] invalid bridge "                   \
                "handle to <"#bridgeclass" *>\n")); 

/**
 * @def BRIDGE_DEF
 * @brief Define the C interface entry points for a robotkernel bridge.
 *
 * This macro generates the required C symbols that allow the
 * robotkernel runtime to load and manage a bridge implemented
 * in C++.
 *
 * It defines:
 *  - bridge_configure(): creates a bridge instance
 *  - bridge_unconfigure(): destroys the bridge instance
 *  - bridge_add_service(): register a robotkernel service
 *  - bridge_remove_service(): unregister a robotkernel service
 *
 * The macro binds a C++ class to the C ABI expected by robotkernel.
 *
 * @param class_name
 *        Name of the C++ class implementing the bridge.
 *
 * @param impl_name
 *        String identifying the bridge implementation type.
 *
 * @note
 * - The class must derive from robotkernel::bridge_base
 * - The constructor must accept:
 *     (const std::string&, const std::string&, const YAML::Node&)
 * - The returned object is owned by the kernel and deleted
 *   via bridge_unconfigure()
 */
#define BRIDGE_DEF(bridgename, bridgeclass)                                                 \
struct instance_name ## _wrapper {                                                          \
    std::shared_ptr<bridgeclass> sp;                                                        \
};                                                                                          \
                                                                                            \
EXPORT_C void bridge_add_service(BRIDGE_HANDLE hdl, const robotkernel::service_t &svc) {    \
    HDL_2_BRIDGECLASS(hdl, bridgename, bridgeclass)                                         \
    wr->sp->add_service(svc);                                                               \
}                                                                                           \
                                                                                            \
EXPORT_C void bridge_remove_service(BRIDGE_HANDLE hdl, const robotkernel::service_t &svc) { \
    HDL_2_BRIDGECLASS(hdl, bridgename, bridgeclass)                                         \
    wr->sp->remove_service(svc);                                                            \
}                                                                                           \
                                                                                            \
EXPORT_C int bridge_unconfigure(BRIDGE_HANDLE hdl) {                                        \
    HDL_2_BRIDGECLASS(hdl, bridgename, bridgeclass)                                         \
    wr->sp->deinit();                                                                       \
    wr->sp = nullptr;                                                                       \
    delete wr;                                                                              \
    return 0;                                                                               \
}                                                                                           \
                                                                                            \
EXPORT_C BRIDGE_HANDLE bridge_configure(const char* name, const char* config) {             \
    struct instance_name ## _wrapper *wr;                                                   \
    YAML::Node doc = YAML::Load(config);                                                    \
                                                                                            \
    wr = new struct instance_name ## _wrapper();                                            \
    if (!wr)                                                                                \
        throw std::runtime_error(string(                                                    \
                "["#bridgename"] error allocating memory\n"));                              \
    wr->sp = std::make_shared<bridgeclass>(name, doc);                                      \
    wr->sp->init();                                                                         \
                                                                                            \
    return (BRIDGE_HANDLE)wr;                                                               \
}

namespace robotkernel {

/**
 * @class bridge_base
 * @brief Abstract base class for a robotkernel bridge implementation.
 *
 * Concrete bridge modules should derive from this class and
 * implement service registration methods. The bridge participates
 * in robotkernel logging via `log_base` (inherited).
 *
 * The bridge lifecycle involves:
 *   - Construction with a name, implementation type, and config
 *   - Optional `init()`/`deinit()` calls
 *   - Service registration via `add_service()`
 *   - Cleanup via `remove_service()`
 *
 * Instances are typically wrapped in a shared pointer.
 */
class bridge_base : 
    public log_base 
{
    private:
        // Prevent default construction
        bridge_base();

    public:
        /**
         * @brief Construct a bridge with the given names.
         *
         * @param name
         *        The configured instance name.
         * @param impl
         *        The implementation type string.
         * @param node
         *        Parsed YAML node for configuration.
         */
        bridge_base(const std::string& name,
                    const std::string& impl,
                    const YAML::Node& node = YAML::Node())
            : log_base(name, impl, "", node)
        {}
    
        /**
         * @brief Virtual destructor.
         */
        virtual ~bridge_base() {};
    
        /**
         * @brief Optional initialization method.
         *
         * Called after construction to perform any additional setup.
         * Useful when the implementation needs to use `shared_from_this()`
         * during initialization.
         */
        void init() {};
    
        /**
         * @brief Optional deinitialization method.
         *
         * Called before destruction to perform cleanup that
         * depends on shared ownership.
         */
        void deinit() {};
    
        /**
         * @brief Register a service with the robotkernel runtime.
         *
         * Derived classes must implement this to expose functionality.
         *
         * @param svc
         *        The service descriptor to register.
         */
        virtual void add_service(const robotkernel::service_t &svc) = 0;
    
        /**
         * @brief Unregister a previously registered service.
         *
         * Derived classes implement this to clean up.
         *
         * @param svc
         *        The service descriptor to unregister.
         */
        virtual void remove_service(const robotkernel::service_t &svc) = 0;
};

}; // namespace robotkernel
   
#endif // __cplusplus

#endif // ROBOTKERNEL_BRIDGE_BASE_H

