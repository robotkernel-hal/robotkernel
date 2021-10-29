//! robotkernel bridge base
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

#ifndef ROBOTKERNEL_BRIDGE_BASE_H
#define ROBOTKERNEL_BRIDGE_BASE_H

#include "robotkernel/bridge_intf.h"
#include "robotkernel/log_base.h"
#include "robotkernel/kernel.h"
#include "robotkernel/exceptions.h"
#include "robotkernel/helpers.h"

#include "yaml-cpp/yaml.h"

#ifdef __cplusplus
#define EXPORT_C extern "C" 
#else
#define EXPORT_C
#endif

#define HDL_2_BRIDGECLASS(hdl, bridgename, bridgeclass)                                     \
    struct instance_name ## _wrapper *wr =                                                  \
        reinterpret_cast<struct instance_name ## _wrapper *>(hdl);                          \
    if (!wr->sp)                                                                            \
        throw string_util::str_exception("["#bridgename"] invalid bridge "                  \
                "handle to <"#bridgeclass" *>\n"); 

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
        throw string_util::str_exception(                                                   \
                "["#bridgename"] error allocating memory\n");                               \
    wr->sp = std::make_shared<bridgeclass>(name, doc);                                      \
    wr->sp->init();                                                                         \
                                                                                            \
    return (BRIDGE_HANDLE)wr;                                                               \
}

namespace robotkernel {
#ifdef EMACS
}
#endif

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

#ifdef EMACS
{
#endif
};

#endif // ROBOTKERNEL_BRIDGE_BASE_H

