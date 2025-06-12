//! robotkernel module definition
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

// public headers
#include "robotkernel/module_base.h"

// private headers
#include "kernel.h"

using namespace robotkernel;

//! Module construction
/*!
 * \param[in] impl     Name of the module.
 * \param[in] name     Instance name of the module.
 */
module_base::module_base(const std::string& impl, const std::string& name, const YAML::Node& node) : 
    log_base(name, impl, "", node), name(name), state(module_state_init)
{ }

//// Get robotkernel module
//robotkernel::sp_module_t module_base::get_module() {
//    robotkernel::kernel& k = *robotkernel::kernel::get_instance();
//    return k.get_module(name);
//}

//! Set module state machine to defined state.
/*!
 * \param[in] state     Requested state which will be tried to switch to.
 *
 * \return success or failure
 */
int module_base::set_state(module_state_t state) {
    std::unique_lock<std::mutex> lock(state_mtx);

    // get transition
    uint32_t transition = GEN_STATE(this->state, state);

    switch (transition) {
        case op_2_safeop:
        case op_2_preop:
        case op_2_init:
        case op_2_boot:
            // ====> stop sending commands
            set_state_op_2_safeop();

            if (state == module_state_safeop)
                break;
        case safeop_2_preop:
        case safeop_2_init:
        case safeop_2_boot:
            // ====> stop receiving measurements
            set_state_safeop_2_preop();

            if (state == module_state_preop)
                break;
        case preop_2_init:
        case preop_2_boot:
            // ====> deinit devices
            set_state_preop_2_init();
        case init_2_init:
            if (state == module_state_init)
                break;
        case init_2_boot:
            break;
        case boot_2_init:
        case boot_2_preop:
        case boot_2_safeop:
        case boot_2_op:
            if (state == module_state_init)
                break;
        case init_2_op:
        case init_2_safeop:
        case init_2_preop:
            // ====> init devices
            set_state_init_2_preop();

            if (state == module_state_preop)
                break;
        case preop_2_op:
        case preop_2_safeop:
            // ====> start receiving measurements
            set_state_preop_2_safeop();

            if (state == module_state_safeop)
                break;
        case safeop_2_op:
            // ====> start sending commands
            set_state_safeop_2_op();
            break;
        case op_2_op:
        case safeop_2_safeop:
        case preop_2_preop:
            // ====> do nothing
            break;

        default:
            break;
    }

    return (this->state = state);
}

//! svc_configure_loglevel
/*!
 * \param[in]   req     Service request data.
 * \param[out]  resp    Service response data.
 */
void module_base::svc_configure_loglevel(
        const struct services::robotkernel::log_base::svc_req_configure_loglevel& req,
        struct services::robotkernel::log_base::svc_resp_configure_loglevel& resp)
{
}
