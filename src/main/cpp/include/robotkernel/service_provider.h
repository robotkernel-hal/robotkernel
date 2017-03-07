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

namespace robotkernel {
	typedef struct service_requester {
		std::string magic;
		std::string owner;
		std::string name;
		int slave_id;
	} service_requester_t;

	typedef std::list<service_requester_t *> service_requester_list_t;

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
			 * \param mod_name slave owning module
			 * \param dev_name name of device
			 * \param slave_id id in module
			 * \return slv_hdl slave handle
			 */
			void add_slave(std::string mod_name, std::string dev_name, int slave_id);

			//! remove registered slave
			/*!
			 * \param mod_name slave owning module
			 * \param slave_id id in module
			 */
			void remove_slave(std::string mod_name, int slave_id);

			//! remove all slaves from module
			/*!
			 * \param mod_name module owning slaves
			 */
			void remove_module(std::string mod_name);

			//! service provider magic 
			/*!
			 * \return return service provider magic string
			 */
			std::string get_sp_magic(); 

			std::string name;					   //!< service_provider name

		private:
			SERVICE_PROVIDER_HANDLE sp_handle;   //! service_provider handle

			//! service_provider symbols
			sp_register_t   	sp_register;
			sp_unregister_t 	sp_unregister;
			sp_add_slave_t  	sp_add_slave;
			sp_remove_slave_t 	sp_remove_slave;
			sp_remove_module_t  sp_remove_module;
			sp_get_sp_magic_t   sp_get_sp_magic;
	};

	class ServiceProvider {
		public:
			ServiceProvider() {}

			virtual ~ServiceProvider() {}

			virtual void init(const YAML::Node &node, void *interface) = 0;

			virtual void log(robotkernel::loglevel lvl, const char *format, ...) = 0;

			virtual int service_configure_loglevel(const robotkernel::service_arglist_t &request,
					robotkernel::service_arglist_t &response) = 0;
	};
}

#endif //PROJECT_SERVICE_PROVIDER_H
