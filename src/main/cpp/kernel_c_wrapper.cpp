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

#include "kernel_c_wrapper.h"

#include "robotkernel/kernel.h"
#include "robotkernel/process_data.h"

using namespace std;
using namespace robotkernel;

class pd_wrapper : 
    public std::enable_shared_from_this<pd_wrapper>,
    public robotkernel::pd_consumer,
    public robotkernel::pd_provider
{   
    public:
        sp_process_data_t pd_dev;
        std::size_t hash;
        int consumer;

    public:
        pd_wrapper(sp_process_data_t pd_dev) : 
            pd_consumer(string("kernel_c_pd_wrapper") + pd_dev->id()),
            pd_provider(string("kernel_c_pd_wrapper") + pd_dev->id()),
            pd_dev(pd_dev), hash(0), consumer(0) {
        }

        ~pd_wrapper() {
            release();
        }

        void aquire() {
            if (consumer == 1) {
                hash = pd_dev->set_consumer(shared_from_this());
            } else {
                hash = pd_dev->set_provider(shared_from_this());
            }
        }

        void release() {
            if (hash != 0) {
                if (consumer) {
                    pd_dev->reset_consumer(hash);
                } else {
                    pd_dev->reset_provider(hash);
                }

                hash = 0;
            }
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
    kernel& k = *(kernel::get_instance());
    auto pd_dev = k.get_process_data(string(pd_name));

    std::shared_ptr<pd_wrapper> *hdl = new std::shared_ptr<pd_wrapper>();
    *hdl = make_shared<pd_wrapper>(pd_dev);
    auto obj = *hdl;
    obj->consumer = consumer;
    obj->aquire();

    return (pdhandle)hdl;
}

//! Checks if process data description does match .
/*!
 * \param[in]   hdl         Handle of robotkernel-5 process data retreaved from <get_pd_handle>.
 * \param[in]   desc        Description to check against.
 * \return 0 on match, -1 otherwise.
 */
int kernel_c_check_pd_desc(pdhandle hdl, const char *desc) {
    auto obj = *(std::shared_ptr<pd_wrapper> *)hdl;
    return string(desc) != obj->pd_dev->process_data_definition;
}

//! Get next buffer to write (outputs data buffer) 
/*!
 * \param[in]   hdl         Handle of robotkernel-5 process data retreaved from <get_pd_handle>.
 * \return buffer for next outputs.
 */
uint8_t *kernel_c_next_write_buffer(pdhandle hdl) {
    auto obj = *(std::shared_ptr<pd_wrapper> *)hdl;
    return obj->pd_dev->next(obj->hash);
}

//! Push next write buffer to be set as outputs.
/*!
 * \param[in]   hdl         Handle of robotkernel-5 process data retreaved from <get_pd_handle>.
 */
void kernel_c_push_write_buffer(pdhandle hdl) {
    auto obj = *(std::shared_ptr<pd_wrapper> *)hdl;
    obj->pd_dev->push(obj->hash);
}

/*! Get actual read buffer with valid inputs.
 * \param[in]   hdl         Handle of robotkernel-5 process data retreaved from <get_pd_handle>.
 * \return buffer to be read with valid inputs.
 */
uint8_t *kernel_c_act_read_buffer(pdhandle hdl) {
    auto obj = *(std::shared_ptr<pd_wrapper> *)hdl;
    return obj->pd_dev->peek();
}

/*! Set actual read buffer as read by application.
 * \param[in]   hdl         Handle of robotkernel-5 process data retreaved from <get_pd_handle>.
 */
void kernel_c_pop_read_buffer(pdhandle hdl) {
    auto obj = *(std::shared_ptr<pd_wrapper> *)hdl;
    obj->pd_dev->pop(obj->hash);
}

/*! Release process data handle (Deregisters as consumer or provider)
 * \param[in]   hdl         Handle of robotkernel-5 process data retreaved from <get_pd_handle>.
 */
void kernel_c_release_pd_handle(pdhandle hdl) {
    auto obj = *(std::shared_ptr<pd_wrapper> *)hdl;
    obj->release();

    delete (std::shared_ptr<pd_wrapper> *)hdl;
}

#ifdef EMACS_IS_STUPID
{
#endif
};


