//! robotkernel loglevel
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

#ifndef ROBOTKERNEL_LOG_BASE_H
#define ROBOTKERNEL_LOG_BASE_H

#include "robotkernel/loglevel.h"
#include "robotkernel/exceptions.h"
#include "robotkernel/service_definitions.h"
#include "robotkernel/helpers.h"
#include "yaml-cpp/yaml.h"

namespace robotkernel {

class log_base : 
    public services::robotkernel::log_base::svc_base_configure_loglevel
{
    private:
        log_base();                 //!< prevent default construction

    public:
        loglevel ll;                //!< loglevel loglevel
        std::string name;           //!< name of module
        std::string impl;           //!< name of implementation
        std::string service_prefix; //!< name of service

        //! construction
        /*!
         * \param[in]   ll          Loglevel to set.
         */
        log_base(loglevel ll);

        //! construction
        /*!
         * \param node config node
         */
        log_base(const std::string& name, const std::string& impl, 
                const std::string& service_prefix, const YAML::Node& node = YAML::Node());

        //! virtual destruction
        virtual ~log_base();

        //! Return current loglevel
        const loglevel get_loglevel() const;

        //! Set new loglevel
        /*!
         * \param[in]   ll      New loglevel to be set.
         */
        void set_loglevel(loglevel ll);

        //! svc_configure_loglevel
        /*!
         * \param[in]   req     Service request data.
         * \param[out]  resp    Service response data.
         */
        void svc_configure_loglevel(
            const struct services::robotkernel::log_base::svc_req_configure_loglevel& req, 
            struct services::robotkernel::log_base::svc_resp_configure_loglevel& resp) override;

        //! log to kernel logging facility
        void log(robotkernel::loglevel lvl, const char *format, ...);
};

//! Return current loglevel
inline const loglevel log_base::get_loglevel() const { 
    return ll; 
}

//! Set new loglevel
/*!
 * \param[in]   ll      New loglevel to be set.
 */
inline void log_base::set_loglevel(loglevel ll) { 
    this->ll = ll; 
}


}; // namespace robotkernel

#endif // ROBOTKERNEL_LOG_BASE_H

