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
 * @file log_base.h
 * @brief Base class for logging in robotkernel modules.
 *
 * Provides a common logging interface and log level configuration
 * support for modules, services, and other components within the
 * robotkernel framework.
 *
 * This class also implements the `svc_configure_loglevel` service
 * used to change log levels at runtime via service calls.
 *
 * Logging output is routed through the robotkernel logging facility.
 *
 * Derived classes should call `log()` with the appropriate level
 * to emit log messages.
 */

#ifndef ROBOTKERNEL_LOG_BASE_H
#define ROBOTKERNEL_LOG_BASE_H

#include "robotkernel/loglevel.h"
#include "robotkernel/exceptions.h"
#include "robotkernel/service_definitions.h"
#include "robotkernel/helpers.h"
#include "yaml-cpp/yaml.h"

namespace robotkernel {

/**
 * @class log_base
 * @brief Base class implementing logging behavior for components.
 *
 * This class encapsulates:
 *   - A configurable log level.
 *   - A name and implementation identifier used in log output.
 *   - Optional service to configure log levels at runtime.
 *
 * Components deriving from `log_base` gain a consistent logging
 * interface and integration with the robotkernel logging facility.
 */
class log_base : 
    public services::robotkernel::log_base::svc_base_configure_loglevel
{
    private:
    /**
     * @brief Disabled default constructor.
     *
     * This class must be constructed with at least a log level
     * or with all identifying information present.
     */
    log_base();

public:
    /// Current loglevel for this component.
    loglevel ll;

    /// Component name used in log output.
    std::string name;

    /// Implementation identifier (e.g. module/plugin type).
    std::string impl;

    /// Optional prefix for service-related log entries.
    std::string service_prefix;

public:
    /**
     * @brief Construct a log_base with an explicit loglevel.
     *
     * @param[in] ll
     *        Initial log level for this component.
     */
    log_base(loglevel ll);

    /**
     * @brief Construct a log_base with identifying information.
     *
     * @param[in] name
     *        Human identifier for this instance (e.g., module name).
     *
     * @param[in] impl
     *        Implementation name or type.
     *
     * @param[in] service_prefix
     *        Optional prefix for log messages from services.
     *
     * @param[in] node
     *        YAML configuration node (unused in base, available for derived use).
     */
    log_base(const std::string& name,
             const std::string& impl,
             const std::string& service_prefix,
             const YAML::Node& node = YAML::Node());

    /**
     * @brief Virtual destructor.
     */
    virtual ~log_base();

    /**
     * @brief Return the current log level.
     *
     * @return Current loglevel.
     */
    const loglevel get_loglevel() const;

    /**
     * @brief Set a new log level.
     *
     * Used internally and by the `svc_configure_loglevel` service
     * to adjust logging detail at runtime.
     *
     * @param[in] ll New log level value.
     */
    void set_loglevel(loglevel ll);

    /**
     * @brief Service callback for configuring loglevel.
     *
     * Implements the service logic for runtime log level configuration.
     * This override comes from the `svc_base_configure_loglevel` interface.
     *
     * @param[in] req  Service request containing a log level.
     * @param[out] resp Service response.
     */
    void svc_configure_loglevel(
        const struct services::robotkernel::log_base::svc_req_configure_loglevel& req,
        struct services::robotkernel::log_base::svc_resp_configure_loglevel& resp) override;

    /**
     * @brief Emit a log message to the robotkernel logging facility.
     *
     * @param[in] lvl
     *        Log level to use for this message.
     *
     * @param[in] format
     *        printf-style format string.
     *
     * @param[in] …
     *        Arguments passed to the format string.
     */
    void log(robotkernel::loglevel lvl, const char *format, ...);
};

// -----------------------------------------------------------------------------
// Inline member implementations
// -----------------------------------------------------------------------------

/**
 * @brief Return the stored log level.
 *
 * @return The current loglevel.
 */
inline const loglevel log_base::get_loglevel() const
{
    return ll;
}

/**
 * @brief Set the stored loglevel.
 *
 * @param[in] ll The new loglevel.
 */
inline void log_base::set_loglevel(loglevel ll)
{
    this->ll = ll;
}

}; // namespace robotkernel

#endif // ROBOTKERNEL_LOG_BASE_H

