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

#define BRIDGE_HANDLE void*

//! bridge configure
/*!
 * \param name bridge name
 * \param config bridge config
 * \return bridge handle
 */
typedef BRIDGE_HANDLE (*bridge_configure_t)(const char* name, const char* config);

//! bridge unconfigure
/*!
 * \param hdl bridge handle
 */
typedef int (*bridge_unconfigure_t)(BRIDGE_HANDLE hdl);

//! create and register ln service
/*!
 * \param svc robotkernel service struct
 */
typedef void (*bridge_add_service_t)(BRIDGE_HANDLE hdl, const robotkernel::service_t &svc);

//! unregister and remove ln service 
/*!
 * \param svc robotkernel service struct
 */
typedef void (*bridge_remove_service_t)(BRIDGE_HANDLE hdl, const robotkernel::service_t &svc);

#ifdef __cplusplus

#define HDL_2_BRIDGECLASS(hdl, bridgename, bridgeclass)                                     \
    struct instance_name ## _wrapper *wr =                                                  \
        reinterpret_cast<struct instance_name ## _wrapper *>(hdl);                          \
    if (!wr->sp)                                                                            \
        throw std::runtime_error(string("["#bridgename"] invalid bridge "                   \
                "handle to <"#bridgeclass" *>\n")); 

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

class bridge_base : 
    public log_base 
{
    private:
        bridge_base();                  //!< prevent default construction

    public:
        //! construction
        /*!
         * \param instance_name bridge name
         * \param name instance name
         */
        bridge_base(const std::string& name, const std::string& impl, 
                const YAML::Node& node = YAML::Node()) :
            log_base(name, impl, "", node)
        {
        }

        virtual ~bridge_base() {};

        //! optional initiazation methon
        /* 
         * usefull to call shared_from_this() at construction time
         */
        void init() {};

        //! optional deinitiazation methon
        /* 
         * usefull to call shared_from_this() at construction time
         */
        void deinit() {};

        //! create and register service
        /*!
         * \param svc robotkernel service struct
         */
        virtual void add_service(const robotkernel::service_t &svc) = 0;

        //! unregister and remove service 
        /*!
         * \param svc robotkernel service struct
         */
        virtual void remove_service(const robotkernel::service_t &svc) = 0;
};

}; // namespace robotkernel
   
#endif // __cplusplus

#endif // ROBOTKERNEL_BRIDGE_BASE_H

