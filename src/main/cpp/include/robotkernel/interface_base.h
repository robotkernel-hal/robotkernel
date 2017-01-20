//! robotkernel interface base
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

#ifndef __ROBOTKERNEL_INTERFACE_BASE_H__
#define __ROBOTKERNEL_INTERFACE_BASE_H__

#include "robotkernel/interface_intf.h"
#include "robotkernel/kernel.h"
#include "robotkernel/exceptions.h"
#include "robotkernel/helpers.h"

#include "yaml-cpp/yaml.h"

#ifdef __cplusplus
#define EXPORT_C extern "C" 
#else
#define EXPORT_C
#endif

#define HDL_2_INTFCLASS(hdl, intfname, intfclass)               \
    intfclass *dev = reinterpret_cast<intfclass *>(hdl);        \
    if (!dev)                                                   \
        throw robotkernel::str_exception(                       \
                "["#intfname"] invalid interface "              \
                "handle to <"#intfclass" *>\n"); 

#define INTERFACE_DEF(intfname, intfclass)                      \
                                                                \
EXPORT_C int intf_unregister(INTERFACE_HANDLE hdl) {            \
    HDL_2_INTFCLASS(hdl, intfname, intfclass)                   \
    delete dev;                                                 \
    return 0;                                                   \
}                                                               \
                                                                \
EXPORT_C INTERFACE_HANDLE intf_register(const char* config) {   \
    intfclass *dev;                                             \
    YAML::Node doc = YAML::Load(config);                        \
                                                                \
    dev = new intfclass(doc);                                   \
    if (!dev)                                                   \
        throw robotkernel::str_exception(                       \
                "["#intfname"] error allocating memory\n");     \
                                                                \
    return (INTERFACE_HANDLE)dev;                               \
}

namespace robotkernel {

class interface_base {
    private:
        interface_base();       //!< prevent default construction

    protected:
        std::string intf_name;  //!< interface name
        std::string mod_name;   //!< module name
        std::string dev_name;   //!< service device name
        int slave_id;           //!< device slave id
        loglevel ll;            //!< interface loglevel

    public:
        //! construction
        /*!
         * \param intfname interface name
         * \param name instance name
         */
        interface_base(const std::string& intf_name, const YAML::Node& node);

        //! destruction
        ~interface_base();

        //! log to kernel logging facility
        void log(robotkernel::loglevel lvl, const char *format, ...);

        //! service to configure interface loglevel
        /*!
         * message service message
         */
        int service_configure_loglevel(YAML::Node& message);
        static const std::string service_definition_configure_loglevel;
};

};

#endif // __ROBOTKERNEL_INTERFACE_BASE_H__

