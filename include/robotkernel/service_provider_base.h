//! robotkernel interface base
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

#ifndef ROBOTKERNEL_INTERFACE_BASE_H
#define ROBOTKERNEL_INTERFACE_BASE_H

#include <unistd.h>
#include <stdint.h>
#include <list>
#include <stdio.h>

// public headers
#include "robotkernel/device_listener.h"
#include "robotkernel/exceptions.h"
#include "robotkernel/helpers.h"
#include "robotkernel/log_base.h"
#include "robotkernel/service_interface.h"

#include "yaml-cpp/yaml.h"

#include <map>
#include <functional>
#include <typeindex>
#include <typeinfo>
#include <stdexcept>

#ifdef __cplusplus
#define EXPORT_C extern "C"
#else
#define EXPORT_C
#endif

#define SERVICE_PROVIDER_HANDLE void*

//! service_provider register
/*!
 * \param mod_name module name to register
 * \return service_provider handle
 */
typedef SERVICE_PROVIDER_HANDLE(*sp_register_t)(const char* name);

//! service_provider unregister
/*!
 * \param hdl service_provider handle
 * \return success
 */
typedef int (*sp_unregister_t)(SERVICE_PROVIDER_HANDLE hdl);

//! test service requester
/*!
 * \return true if we can handle service requester
 */
typedef bool (*sp_test_interface_t)(SERVICE_PROVIDER_HANDLE hdl,
        robotkernel::sp_service_interface_t req);

//! add slave
/*!
 * \param hdl service provider handle
 * \param req slave inteface specialization         
 */
typedef void (*sp_add_interface_t)(SERVICE_PROVIDER_HANDLE hdl, 
        robotkernel::sp_service_interface_t req);

//! remove registered slave
/*!
 * \param hdl service provider handle
 * \param req slave inteface specialization         
 * \param slave_id id in module
 */
typedef void (*sp_remove_interface_t)(SERVICE_PROVIDER_HANDLE hdl, 
        robotkernel::sp_service_interface_t req);

//! remove module
/*!
 * \param hdl service provider handle
 * \param mod_name module owning slaves
 */
typedef void (*sp_remove_module_t)(SERVICE_PROVIDER_HANDLE hdl, 
        const char *mod_name);

//! service provider magic 
/*!
 * \return return service provider magic string
 */
typedef const char* (*sp_get_sp_magic_t)(SERVICE_PROVIDER_HANDLE hdl);

#ifdef __cplusplus

#define HDL_2_SERVICE_PROVIDERCLASS(hdl, spname, spclass)              \
    struct spname ## _wrapper *wr =                                    \
        reinterpret_cast<struct spname ## _wrapper *>(hdl);            \
    std::shared_ptr<spclass> dev = wr->sp;                             \
    if (!dev)                                                          \
        throw std::runtime_error("["#spname"] invalid sp "             \
                "handle to <"#spclass" *>\n"); 

#define SERVICE_PROVIDER_DEF(spname, spclass)                          \
struct spname ## _wrapper {                                            \
    std::shared_ptr<spclass> sp;                                       \
};                                                                     \
                                                                       \
EXPORT_C SERVICE_PROVIDER_HANDLE sp_register(const char* name) {       \
    struct spname ## _wrapper *wr;                                     \
    wr = new struct spname ## _wrapper();                              \
    if (!wr)                                                           \
        throw std::runtime_error(                                      \
                "["#spname"] error allocating memory\n");              \
    wr->sp = std::make_shared<spclass>(name);                          \
    wr->sp->init();                                                    \
                                                                       \
    return (SERVICE_PROVIDER_HANDLE)wr;                                \
}                                                                      \
                                                                       \
EXPORT_C int sp_unregister(SERVICE_PROVIDER_HANDLE hdl) {              \
    HDL_2_SERVICE_PROVIDERCLASS(hdl, spname, spclass)                  \
    wr->sp->deinit();                                                  \
    wr->sp = nullptr;                                                  \
    delete wr;                                                         \
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

template <class T, class S>
class service_provider_base : 
    public log_base, 
    public device_listener,
    public std::enable_shared_from_this<service_provider_base<T, S> >
{
    private:
        service_provider_base();                  //!< prevent default construction

    public:
        using std::enable_shared_from_this<service_provider_base<T, S> >::shared_from_this;

        typedef std::map<std::pair<std::string, std::string>, T *> handler_map_t;

        //! construction
        /*!
         * \param instance_name service_provider name
         * \param name instance name
         */
        service_provider_base(const std::string& name, const std::string& impl) : 
            log_base(name, impl, ""),
            device_listener(name, "listener")
        {};
        
        void init() {
            ::robotkernel::add_device_listener(shared_from_this());
        };
        
        virtual ~service_provider_base() {};
        
        void deinit() {
            ::robotkernel::remove_device_listener(shared_from_this());
        };

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

        // add a named device
        void notify_add_device(sp_device_t req) {
            const auto& interface = std::dynamic_pointer_cast<service_interface>(req);
            if (interface) {
                add_interface(interface);
            }
        };

        // remove a named device
        void notify_remove_device(sp_device_t req) {
            const auto& interface = std::dynamic_pointer_cast<service_interface>(req);
            if (interface) {
                remove_interface(interface);
            }
        };

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
    auto local_req = std::dynamic_pointer_cast<S>(req);

    if (!test_interface(local_req))
        return;

    if (handler_map.find(std::make_pair(local_req->owner, local_req->id())) != handler_map.end())
        return; // already in our handler map ....

    try {
        handler = new T(local_req);
    } catch (std::exception& e) {
        // if exception is thrown, req does not belong to us
        handler = NULL;
    }

    if (handler) {
        log(verbose, "got new service_interface: owner %s, id %s\n", 
                local_req->owner.c_str(), local_req->id().c_str());
        handler_map[std::make_pair(local_req->owner, local_req->id())] = handler;
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

}; // namespace robotkernel

#endif // __cplusplus

#endif // ROBOTKERNEL_INTERFACE_BASE_H

