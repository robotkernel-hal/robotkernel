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
#include "robotkernel/service_provider_intf.h"
#include "robotkernel/service_provider.h"
#include "robotkernel/log_base.h"

#include "yaml-cpp/yaml.h"

#include <map>
#include <functional>
#include <string_util/string_util.h>

#ifdef __cplusplus
#define EXPORT_C extern "C"
#else
#define EXPORT_C
#endif

#define HDL_2_SERVICE_PROVIDERCLASS(hdl, spname, spclass)                                 \
	spclass *dev = reinterpret_cast<spclass *>(hdl);                                      \
	if (!dev)                                                                             \
		throw string_util::str_exception("["#spname"] invalid sp "                        \
			"handle to <"#spclass" *>\n"); 

#define SERVICE_PROVIDER_DEF(spname, spclass)                                             \
EXPORT_C SERVICE_PROVIDER_HANDLE sp_register() {                                          \
	spclass *dev = new spclass();                                                         \
	if (!dev)                                                                             \
		throw string_util::str_exception("error allocating memory\n");                    \
																					      \
	return (SERVICE_PROVIDER_HANDLE)dev;                                                  \
}                                                                                         \
                                                                                          \
EXPORT_C int sp_unregister(SERVICE_PROVIDER_HANDLE hdl) {                                 \
	HDL_2_SERVICE_PROVIDERCLASS(hdl, spname, spclass)                                     \
	delete dev;                                                                           \
	return 0;                                                                             \
}                                                                                         \
																					      \
EXPORT_C void sp_add_slave(SERVICE_PROVIDER_HANDLE hdl,                                   \
		robotkernel::sp_service_requester_t req) {                                        \
	HDL_2_SERVICE_PROVIDERCLASS(hdl, spname, spclass)                                     \
	dev->add_slave(req);                                                                  \
}                                                                                         \
                                                                                          \
EXPORT_C void sp_remove_slave(SERVICE_PROVIDER_HANDLE hdl,                                \
		robotkernel::sp_service_requester_t req) {                                        \
	HDL_2_SERVICE_PROVIDERCLASS(hdl, spname, spclass)                                     \
	dev->remove_slave(req);                                                               \
}                                                                                         \
                                                                                          \
EXPORT_C void sp_remove_module(SERVICE_PROVIDER_HANDLE hdl,                               \
		const char *mod_name) {                                                           \
	HDL_2_SERVICE_PROVIDERCLASS(hdl, spname, spclass)                                     \
	dev->remove_module(mod_name);                                                         \
}                                                                                         \
                                                                                    
namespace robotkernel {

	template <typename T>
	class service_provider_base : public log_base {
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

			//! add slave
			/*!
			 * \param mod_name slave owning module
			 * \param dev_name name of device
			 * \param slave_id id in module
			 */
			void add_slave(sp_service_requester_t req) {
                T *handler;

                try {
				    handler = new T(req);
                } catch (std::exception& e) {
                    // if exception is thrown, req does not belong to us
                    handler = NULL;
                }

                if (handler)
                    handler_map[make_pair(req->owner, req->service_prefix)] = handler;
			};

			//! remove registered slave
			/*!
			 * \param mod_name slave owning module
			 * \param slave_id id in module
			 */
			void remove_slave(sp_service_requester_t req) {
				for (typename handler_map_t::iterator it =
						handler_map.begin(); it != handler_map.end(); ++it) {
					if (it->first.first != req->owner)
						continue; // skip this, not owr module

					if (it->first.second != req->service_prefix) 
						continue; // skip this, not owr slave

					delete it->second;
					handler_map.erase(it);
					return;
				}
			};

			//! remove all slaves from module
			/*!
			 * \param mod_name module owning slaves
			 */
			void remove_module(const char *owner) {
				for (typename handler_map_t::iterator it =
						handler_map.begin(); it != handler_map.end(); ) {
					if (it->first.first != owner) {
						it++;
						continue; // skip this, not owr module
					}

					delete it->second;
					it = handler_map.erase(it);
				}
			};

			//! hold all created handlers, so we can remove them by name
			handler_map_t handler_map;
	};

}; // namespace robotkernel

#endif // __ROBOTKERNEL_INTERFACE_BASE_H__

