//! robotkernel interface base
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

#ifndef ROBOTKERNEL_INTERFACE_BASE_H
#define ROBOTKERNEL_INTERFACE_BASE_H

#include "robotkernel/interface_intf.h"
#include "robotkernel/kernel.h"
#include "robotkernel/exceptions.h"
#include "robotkernel/helpers.h"
#include "robotkernel/service_provider_intf.h"
#include "robotkernel/service_provider.h"
#include "robotkernel/log_base.h"

#include "yaml-cpp/yaml.h"

#include <map>
#include <functional>
#include <string_util/string_util.h>
#include <typeindex>
#include <typeinfo>

#ifdef __cplusplus
#define EXPORT_C extern "C"
#else
#define EXPORT_C
#endif

#define HDL_2_SERVICE_PROVIDERCLASS(hdl, spname, spclass)              \
    spclass *dev = reinterpret_cast<spclass *>(hdl);                   \
    if (!dev)                                                          \
        throw string_util::str_exception("["#spname"] invalid sp "     \
            "handle to <"#spclass" *>\n"); 

#define SERVICE_PROVIDER_DEF(spname, spclass)                          \
EXPORT_C SERVICE_PROVIDER_HANDLE sp_register() {                       \
    spclass *dev = new spclass();                                      \
    if (!dev)                                                          \
        throw string_util::str_exception("error allocating memory\n"); \
                                                                       \
    return (SERVICE_PROVIDER_HANDLE)dev;                               \
}                                                                      \
                                                                       \
EXPORT_C int sp_unregister(SERVICE_PROVIDER_HANDLE hdl) {              \
    HDL_2_SERVICE_PROVIDERCLASS(hdl, spname, spclass)                  \
    delete dev;                                                        \
    return 0;                                                          \
}                                                                      \
                                                                       \
EXPORT_C void sp_add_interface(SERVICE_PROVIDER_HANDLE hdl,            \
        robotkernel::sp_service_interface_t req) {                     \
    HDL_2_SERVICE_PROVIDERCLASS(hdl, spname, spclass)                  \
    dev->add_interface(req);                                           \
}                                                                      \
                                                                       \
EXPORT_C void sp_remove_interface(SERVICE_PROVIDER_HANDLE hdl,         \
        robotkernel::sp_service_interface_t req) {                     \
    HDL_2_SERVICE_PROVIDERCLASS(hdl, spname, spclass)                  \
    dev->remove_interface(req);                                        \
}                                                                      \
                                                                       \
EXPORT_C void sp_remove_module(SERVICE_PROVIDER_HANDLE hdl,            \
        const char *mod_name) {                                        \
    HDL_2_SERVICE_PROVIDERCLASS(hdl, spname, spclass)                  \
    dev->remove_module(mod_name);                                      \
}                                                                      \
                                                                       \
EXPORT_C bool sp_test_interface(SERVICE_PROVIDER_HANDLE hdl,           \
        robotkernel::sp_service_interface_t req) {                     \
    HDL_2_SERVICE_PROVIDERCLASS(hdl, spname, spclass)                  \
    return dev->test_interface(req);                                   \
}                                                                      \

namespace robotkernel {
#ifdef EMACS
}
#endif

template <class T, class S>
class service_provider_base : 
    public log_base 
{
    private:
        service_provider_base();                  //!< prevent default construction

    public:
        typedef std::map<std::pair<std::string, std::string>, T *> handler_map_t;

        //! construction
        /*!
         * \param instance_name service_provider name
         * \param name instance name
         */
        service_provider_base(const std::string& instance_name)
            : log_base("service_provider", instance_name) {};

        virtual ~service_provider_base() {};

        //! Add a interface to our provided services
        /*!
         * \param[in] req   Shared pointer to a interface which should be added.
         */
        void add_interface(sp_service_interface_t req);

        //! Remove a previously registered interface.
        /*!
         * \param[in] req   Shared pointer to a interface which should be removed.
         */
        void remove_interface(sp_service_interface_t req);

        //! Remove all interfaces from one module.
        /*!
         * \param owner module owning slaves
         */
        void remove_module(const char *owner);

        //! Test interface if it belongs to us
        /*!
         * \param[in] req   Shared pointer to a interface which should be testet.
         * \returns True if \p req belongs to us
         */
        bool test_interface(sp_service_interface_t req);

        //! hold all created handlers, so we can remove them by name
        handler_map_t handler_map;
};
        
// add slave
template <class T, class S>
inline void service_provider_base<T, S>::add_interface(sp_service_interface_t req) {
    T *handler;
    if (handler_map.find(std::make_pair(req->owner, req->id())) != handler_map.end())
        return; // already in our handler map ....

    try {
        handler = new T(req);
    } catch (std::exception& e) {
        // if exception is thrown, req does not belong to us
        handler = NULL;
    }

    if (handler) {
        log(info, "got new service_interface: owner %s, id %s\n", 
                req->owner.c_str(), req->id().c_str());
        handler_map[std::make_pair(req->owner, req->id())] = handler;
    }
};

// remove registered slave
template <class T, class S>
inline void service_provider_base<T, S>::remove_interface(sp_service_interface_t req) {
    for (class handler_map_t::iterator it =
            handler_map.begin(); it != handler_map.end(); ++it) {
        if (it->first.first != req->owner)
            continue; // skip this, not owr module

        if (it->first.second != req->id()) 
            continue; // skip this, not owr slave

        delete it->second;
        handler_map.erase(it);
        return;
    }
};

// remove all slaves from module
template <class T, class S>
inline void service_provider_base<T, S>::remove_module(const char *owner) {
    for (class handler_map_t::iterator it =
            handler_map.begin(); it != handler_map.end(); ) {
        if (it->first.first != owner) {
            it++;
            continue; // skip this, not owr module
        }

        delete it->second;
        it = handler_map.erase(it);
    }
};

// test interface if it belongs to us
template <class T, class S>
inline bool service_provider_base<T, S>::test_interface(sp_service_interface_t req) {
    return (std::dynamic_pointer_cast<S>(req) != NULL);
}

#ifdef EMACS
{
#endif
}; // namespace robotkernel

#endif // ROBOTKERNEL_INTERFACE_BASE_H

