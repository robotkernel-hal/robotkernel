//! robotkernel bridge base
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

#ifndef __ROBOTKERNEL_BRIDGE_BASE_H__
#define __ROBOTKERNEL_BRIDGE_BASE_H__

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

#define HDL_2_BRIDGECLASS(hdl, bridgename, bridgeclass)                             \
	bridgeclass *dev = reinterpret_cast<bridgeclass *>(hdl);                        \
	if (!dev)                                                                       \
		throw str_exception("["#bridgename"] invalid bridge "                       \
			"handle to <"#bridgeclass" *>\n"); 

#define BRIDGE_DEF(bridgename, bridgeclass)                                         \
EXPORT_C int bridge_unconfigure(BRIDGE_HANDLE hdl) {                                \
	HDL_2_BRIDGECLASS(hdl, bridgename, bridgeclass)                                 \
	delete dev;                                                                     \
	return 0;                                                                       \
}                                                                                   \
																					\
EXPORT_C BRIDGE_HANDLE bridge_configure(const char* name, const char* config) {     \
	bridgeclass *dev;                                                               \
	YAML::Node doc = YAML::Load(config);                                            \
																					\
	dev = new bridgeclass(name, doc);                                               \
	if (!dev)                                                                       \
		throw str_exception("["#bridgename"] error allocating memory\n");           \
																					\
	return (BRIDGE_HANDLE)dev;                                                      \
}

namespace robotkernel {

	class bridge_base : public log_base {
		private:
			bridge_base();                  //!< prevent default construction

		public:
			//! construction
			/*!
			 * \param instance_name bridge name
			 * \param name instance name
			 */
			bridge_base(const std::string& instance_name, const std::string& name, 
					const YAML::Node& node = YAML::Node());

			virtual ~bridge_base() {};
	};

};

#endif // __ROBOTKERNEL_BRIDGE_BASE_H__

