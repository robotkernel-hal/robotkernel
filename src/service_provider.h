//! robotkernel service_provider class
/*!
 * (C) Robert Burger <robert.burger@dlr.de>
 * (C) Jan Cremer <jan.cremer@dlr.de>
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

#ifndef ROBOTKERNEL__SERVICE_PROVIDER_H
#define ROBOTKERNEL__SERVICE_PROVIDER_H

#include <string>
#include "yaml-cpp/yaml.h"

// public headers
#include "robotkernel/service_provider_base.h"

// private headers
#include "kernel.h"
#include "so_file.h"

namespace robotkernel {

//! service_provider class
/*!
  This class opens a shared service_provider and loads all needed symbols
  */
class service_provider : public so_file {
    private:
        service_provider();
        service_provider(const service_provider&);             // prevent copy-construction
        service_provider& operator=(const service_provider&);  // prevent assignment

    public:
        //! service_provider construction
        /*!
          \param service_provider_file filename of service_provider
          \param node configuration node
          */
        service_provider(const YAML::Node& node);

        //! service_provider destruction
        /*!
          destroys service_provider
          */
        ~service_provider();

        //! add slave
        /*!
         * \param req slave inteface specialization
         */
        void add_interface(sp_service_interface_t req);

        //! remove registered slave
        /*!
         * \param req slave inteface specialization
         */
        void remove_interface(sp_service_interface_t req);

        //! remove all slaves from module
        /*!
         * \param mod_name module owning slaves
         */
        void remove_module(std::string mod_name);

        //! test slave service requester
        /*!
         * \param return true if we can handle it
         */
        bool test_interface(sp_service_interface_t req);

        std::string name;                      //!< service_provider name

    private:
        SERVICE_PROVIDER_HANDLE sp_handle;   //! service_provider handle

        //! service_provider symbols
        sp_register_t                   sp_register;
        sp_unregister_t                 sp_unregister;
        sp_add_interface_t              sp_add_interface;
        sp_remove_interface_t           sp_remove_interface;
        sp_remove_module_t              sp_remove_module;
        sp_test_interface_t             sp_test_interface;
};

typedef std::shared_ptr<service_provider> sp_service_provider_t;
typedef std::map<std::string, sp_service_provider_t> service_provider_map_t;

}; // namespace robotkernel

#endif // ROBOTKERNEL__SERVICE_PROVIDER_H

