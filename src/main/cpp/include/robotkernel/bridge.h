//! robotkernel bridge class
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
 * along with robotkernel.	If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __ROBOTKERNEL_BRIDGE_H__
#define __ROBOTKERNEL_BRIDGE_H__

#include <string>
#include <stdio.h>
#include "robotkernel/bridge_intf.h"
#include "robotkernel/so_file.h"
#include "robotkernel/service.h"
#include "yaml-cpp/yaml.h"

namespace robotkernel {
#ifdef EMACS
}
#endif

//! bridge class
/*!
  This class opens a shared bridge and loads all needed symbols
  */
class bridge : public so_file {
    public:
        typedef std::function<void(const robotkernel::service_t& svc)> cb_t;

        typedef struct cbs {
            cb_t add_service;
            cb_t remove_service;
        } cbs_t;

        typedef std::list<cbs_t *> cbs_list_t;

    private:
        bridge();
        bridge(const bridge&);			   // prevent copy-construction
        bridge& operator=(const bridge&);  // prevent assignment

    public:
        //! bridge construction
        /*!
          \param node configuration node
          */
        bridge(const YAML::Node& node);

        //! bridge destruction
        /*!
          destroys bridge
          */
        ~bridge();

        std::string name;							//!< bridge name

    private:
        BRIDGE_HANDLE bridge_handle;				//!< bridge handle

        //! bridge symbols
        bridge_configure_t bridge_configure;		//!< configure bridge
        bridge_unconfigure_t bridge_unconfigure;	//!< unconfigure bridge
};

typedef std::shared_ptr<bridge> sp_bridge_t;
typedef std::map<std::string, sp_bridge_t> bridge_map_t;

#ifdef EMACS
{
#endif
} // namespace robotkernel

#endif // __ROBOTKERNEL_BRIDGE_H__

