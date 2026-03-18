//! robotkernel module definition
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

//! Set module state machine to defined state.
/*!
 * \param[in] state     Requested state which will be tried to switch to.
 *
 * \return success or failure
 */
int module_base::set_state(module_state_t target_state) {

    // get transition
    uint32_t transition = GEN_STATE(this->state, target_state);

    auto try_set_state = [target_state, this](auto transition_func) {
        try {
            std::unique_lock<std::mutex> lock(state_mtx);
            transition_func();
        } catch (std::exception& e) {
            log(error, "caught exception during %s_2_%s: %s\n", 
                    state_to_string(this->state), 
                    state_to_string(target_state), 
                    e.what());

            set_error();
            return this->state;
        }

        return target_state;
    };

    switch (transition) {
        case op_2_safeop:
        case op_2_preop:
        case op_2_init:
        case op_2_boot: {
            // ====> stop sending commands
            this->state = try_set_state([this]() { set_state_op_2_safeop(); });

            if (is_error() || (state == module_state_safeop))
                break;
        }
        case safeop_2_preop:
        case safeop_2_init:
        case safeop_2_boot: {
            // ====> stop receiving measurements
            this->state = try_set_state([this]() { set_state_safeop_2_preop(); });

            if (is_error() || (state == module_state_preop))
                break;
        }
        case preop_2_init:
        case preop_2_boot: {
            // ====> deinit devices
            this->state = try_set_state([this]() { set_state_preop_2_init(); });
        }
        case init_2_init:
            if (is_error() || (state == module_state_init))
                break;
        case init_2_boot:
            break;
        case boot_2_init:
        case boot_2_preop:
        case boot_2_safeop:
        case boot_2_op:
            if (is_error() || (state == module_state_init))
                break;
        case init_2_op:
        case init_2_safeop:
        case init_2_preop: {
            // ====> init devices
            this->state = try_set_state([this]() { set_state_init_2_preop(); });

            if (is_error() || (state == module_state_preop))
                break;
        }
        case preop_2_op:
        case preop_2_safeop: {
            // ====> start receiving measurements
            this->state = try_set_state([this]() { set_state_preop_2_safeop(); });

            if (is_error() || (state == module_state_safeop))
                break;
        }
        case safeop_2_op: {
            // ====> start sending commands
            this->state = try_set_state([this]() { set_state_safeop_2_op(); });
            break;
        }
        case op_2_op:
        case safeop_2_safeop:
        case preop_2_preop:
            // ====> do nothing
            break;

        default:
            break;
    }

    return this->state;
}

