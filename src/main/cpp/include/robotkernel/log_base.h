//! robotkernel loglevel
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

#ifndef __ROBOTKERNEL_log_base_H__
#define __ROBOTKERNEL_log_base_H__

#include "robotkernel/kernel.h"
#include "robotkernel/exceptions.h"
#include "robotkernel/helpers.h"
#include "yaml-cpp/yaml.h"

namespace robotkernel {
	class log_base {
		private:
			log_base();          	//!< prevent default construction

		public:
			loglevel ll;                //!< loglevel loglevel
			std::string instance_name;  //!< name of instance
			std::string name;           //!< name of module/interface/bridge...

			//! construction
			/*!
			 * \param node config node
			 */
			log_base(const std::string& instancename, const std::string& name, 
					const YAML::Node& node = YAML::Node());

			//! virtual destruction
			virtual ~log_base() {};

			//! service to configure modules loglevel
			/*!
			 * \param request service request data
			 * \parma response service response data
			 */
			int service_configure_loglevel(const service_arglist_t& request, 
					service_arglist_t& response);
			static const std::string service_definition_configure_loglevel;

			//! log to kernel logging facility
			void log(robotkernel::loglevel lvl, const char *format, ...);
	};
};

#endif // __ROBOTKERNEL_log_base_H__

