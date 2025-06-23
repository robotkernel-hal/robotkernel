//! kernel_c_c_wrapper
/*!
 * (C) Robert Burger <robert.burger@dlr.de>
 */

// vim: set expandtab softtabstop=4 shiftwidth=4
// -*- mode: c++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil; -*- 

/*
 * This file is part of module_kernel_c.
 *
 * module_kernel_c is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * module_kernel_c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with module_kernel_c.  If not, see <http://www.gnu.org/licenses/>.
 */

// pubic headers
#include "robotkernel/kernel_c_wrapper.h"
#include "robotkernel/process_data.h"

// private headers
#include "kernel.h"

using namespace std;
using namespace robotkernel;

class pd_wrapper {
    public:
        sp_process_data_t pd_dev;

    public:
        pd_wrapper(sp_process_data_t pd_dev) : pd_dev(pd_dev) {}
        virtual ~pd_wrapper() {}
};

class pd_wrapper_consumer : public pd_wrapper {
    public:
        robotkernel::sp_pd_consumer_t cons;

    public:
        pd_wrapper_consumer(sp_process_data_t pd_dev) : pd_wrapper(pd_dev) {
            cons = make_shared<robotkernel::pd_consumer>(string("kernel_c_pd_wrapper_consumer") + pd_dev->id());
            pd_dev->set_consumer(cons);
        }

        virtual ~pd_wrapper_consumer() {
            pd_dev->reset_consumer(cons);
            cons = nullptr;
        }
};

class pd_wrapper_provider : public pd_wrapper {
    public:
        robotkernel::sp_pd_provider_t prov;

    public:
        pd_wrapper_provider(sp_process_data_t pd_dev) : pd_wrapper(pd_dev) {
            prov = make_shared<robotkernel::pd_provider>(string("kernel_c_pd_wrapper_provider") + pd_dev->id());
            pd_dev->set_provider(prov);
        }

        virtual ~pd_wrapper_provider() {
            pd_dev->reset_provider(prov);
            prov = nullptr;
        }
};

extern "C" {
#ifdef EMACS_IS_STUPID
}
#endif

//! Retreave a handle to a robotkernel-5 process data (registers as consumer or provider)
/*!
 * \param[in]   pd_name     Name of robotkernel-5 process data.
 * \param[in]   consumer    If 1 register as consumer, otherwise as provider.
 * \returns handle to process data device
 */
pdhandle kernel_c_get_pd_handle(const char *pd_name, int consumer) {
    auto pd_dev = kernel::instance.get_device<process_data>(string(pd_name));
    pd_wrapper *hdl = nullptr;

    if (consumer) {
        hdl = new pd_wrapper_provider(pd_dev);
    } else {
        hdl = new pd_wrapper_consumer(pd_dev);
    }

    return (pdhandle)hdl;
}

//! Checks if process data description does match .
/*!
 * \param[in]   hdl         Handle of robotkernel-5 process data retreaved from <get_pd_handle>.
 * \param[in]   desc        Description to check against.
 * \return 0 on match, -1 otherwise.
 */
int kernel_c_check_pd_desc(pdhandle hdl, const char *desc) {
    auto obj = (pd_wrapper *)hdl;
    return string(desc) != obj->pd_dev->process_data_definition;
}

//! Get next buffer to write (outputs data buffer) 
/*!
 * \param[in]   hdl         Handle of robotkernel-5 process data retreaved from <get_pd_handle>.
 * \return buffer for next outputs.
 */
uint8_t *kernel_c_next_write_buffer(pdhandle hdl) {
    auto obj = reinterpret_cast<pd_wrapper_provider *>((pd_wrapper *)hdl);
    return obj->pd_dev->next(obj->prov);
}

//! Push next write buffer to be set as outputs.
/*!
 * \param[in]   hdl         Handle of robotkernel-5 process data retreaved from <get_pd_handle>.
 */
void kernel_c_push_write_buffer(pdhandle hdl) {
    auto obj = reinterpret_cast<pd_wrapper_provider *>((pd_wrapper *)hdl);
    obj->pd_dev->push(obj->prov);
}

/*! Get actual read buffer with valid inputs.
 * \param[in]   hdl         Handle of robotkernel-5 process data retreaved from <get_pd_handle>.
 * \return buffer to be read with valid inputs.
 */
uint8_t *kernel_c_act_read_buffer(pdhandle hdl) {
    auto obj = reinterpret_cast<pd_wrapper_consumer *>((pd_wrapper *)hdl);
    return obj->pd_dev->peek();
}

/*! Set actual read buffer as read by application.
 * \param[in]   hdl         Handle of robotkernel-5 process data retreaved from <get_pd_handle>.
 */
void kernel_c_pop_read_buffer(pdhandle hdl) {
    auto obj = reinterpret_cast<pd_wrapper_consumer *>((pd_wrapper *)hdl);
    obj->pd_dev->pop(obj->cons);
}

/*! Release process data handle (Deregisters as consumer or provider)
 * \param[in]   hdl         Handle of robotkernel-5 process data retreaved from <get_pd_handle>.
 */
void kernel_c_release_pd_handle(pdhandle hdl) {
    delete (pd_wrapper *)hdl;
}

#ifdef EMACS_IS_STUPID
{
#endif
};


