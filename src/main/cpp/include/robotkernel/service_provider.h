//! robotkernel service_provider class
/*!
 * author: Jan Cremer, Robert Burger
 *
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

#ifndef PROJECT_SERVICE_PROVIDER_H
#define PROJECT_SERVICE_PROVIDER_H

#include <string>
#include "yaml-cpp/yaml.h"
#include "robotkernel/kernel.h"
#include "robotkernel/service_provider_intf.h"
#include "robotkernel/so_file.h"

namespace robotkernel {
#ifdef EMACS
}
#endif

//! service_provider class
/*!
  This class opens a shared service_provider and loads all needed symbols
  */
class service_provider : public so_file {
    public:
        typedef struct slave_id {
            std::string mod_name;           //! module name
            std::string dev_name;           //! device name
            int offset;                     //! module offset
        } slave_id_t;

        typedef std::function<void(const slave_id_t& svc)> cb_t;

        typedef struct cbs {
            cb_t add_slave_id;
            cb_t remove_slave_id;
        } cbs_t;

        typedef std::list<cbs_t *> cbs_list_t;

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
        void add_slave(sp_service_requester_t req);

        //! remove registered slave
        /*!
         * \param req slave inteface specialization         
         */
        void remove_slave(sp_service_requester_t req);

        //! remove all slaves from module
        /*!
         * \param mod_name module owning slaves
         */
        void remove_module(std::string mod_name);

        //! test slave service requester
        /*!
         * \param return true if we can handle it
         */
        bool test_slave(sp_service_requester_t req);

        std::string name;                      //!< service_provider name

    private:
        SERVICE_PROVIDER_HANDLE sp_handle;   //! service_provider handle

        //! service_provider symbols
        sp_register_t                   sp_register;
        sp_unregister_t                 sp_unregister;
        sp_add_slave_t                  sp_add_slave;
        sp_remove_slave_t               sp_remove_slave;
        sp_remove_module_t              sp_remove_module;
        sp_test_slave_t                 sp_test_slave;
};

typedef std::shared_ptr<service_provider> sp_service_provider_t;
typedef std::map<std::string, sp_service_provider_t> service_provider_map_t;

#ifdef EMACS
{
#endif
}

#endif //PROJECT_SERVICE_PROVIDER_H
