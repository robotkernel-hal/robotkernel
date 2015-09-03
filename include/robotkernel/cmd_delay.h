//! ÜBER-control base class for modules which support cmd_delay
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

#ifndef __CMD_DELAY_H__
#define __CMD_DELAY_H__

#include <string>
#include "yaml-cpp/yaml.h"
#include "robotkernel/kernel.h"
#include "robotkernel/module_intf.h"
#include "robotkernel/exceptions.h"

namespace robotkernel {

class cmd_delay {
    private:
        cmd_delay();
        cmd_delay(const cmd_delay&);             // prevent copy-construction
        cmd_delay& operator=(const cmd_delay&);  // prevent assignment

    public:
        cmd_delay(const YAML::Node& node) {
            std::string mode = get_as<std::string>(node, "cmd_mode");
            if (mode == "automatic")
                _cmd_mode = automatic;
            else
                _cmd_mode = user_defined;

            const YAML::Node *delay;
            if (!(delay = node.FindValue("cmd_delay")) && (_cmd_mode == user_defined)) {
                klog(error, "[cmd_delay] you have to define 'cmd_delay' when using mode user_defined!\n");
                throw std::exception();
            } else if (delay)
                _cmd_delay = (*delay).to<uint64_t>();
            else
                _cmd_delay = 0;
        };
        ~cmd_delay() {}

        enum {
            user_defined = 0,
            automatic = 1
        } _cmd_mode;
        uint64_t _cmd_delay;

        int32_t mod_request_get_cmd_delay() {
            if(_cmd_mode == automatic)
                return -1;
            return _cmd_delay;
        }
};

}; // namespace robotkernel

#endif // __CMD_DELAY_H__

