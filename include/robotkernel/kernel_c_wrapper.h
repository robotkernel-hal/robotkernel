//! kernel_c_wrapper
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

#ifndef PD_SFUN_HELPER_H
#define PD_SFUN_HELPER_H

#include <stdint.h>

extern "C" {

typedef void * pdhandle;      //!< handle to robotkernel-5 process data

//! Retreave a handle to a robotkernel-5 process data (registers as consumer or provider)
/*!
 * \param[in]   pd_name     Name of robotkernel-5 process data.
 * \param[in]   consumer    If 1 register as consumer, otherwise as provider.
 * \returns handle to process data device
 */
pdhandle kernel_c_get_pd_handle(const char *pd_name, int consumer);

//! Checks if process data description does match .
/*!
 * \param[in]   hdl         Handle of robotkernel-5 process data retreaved from <get_pd_handle>.
 * \param[in]   desc        Description to check against.
 * \return 0 on match, -1 otherwise.
 */
int kernel_c_check_pd_desc(pdhandle hdl, const char *desc);

//! Get next buffer to write (outputs data buffer) 
/*!
 * \param[in]   hdl         Handle of robotkernel-5 process data retreaved from <get_pd_handle>.
 * \return buffer for next outputs.
 */
uint8_t *kernel_c_next_write_buffer(pdhandle hdl);

//! Push next write buffer to be set as outputs.
/*!
 * \param[in]   hdl         Handle of robotkernel-5 process data retreaved from <get_pd_handle>.
 */
void kernel_c_push_write_buffer(pdhandle hdl);

/*! Get actual read buffer with valid inputs.
 * \param[in]   hdl         Handle of robotkernel-5 process data retreaved from <get_pd_handle>.
 * \return buffer to be read with valid inputs.
 */
uint8_t *kernel_c_act_read_buffer(pdhandle hdl);

/*! Set actual read buffer as read by application.
 * \param[in]   hdl         Handle of robotkernel-5 process data retreaved from <get_pd_handle>.
 */
void kernel_c_pop_read_buffer(pdhandle hdl);

/*! Release process data handle (Deregisters as consumer or provider)
 * \param[in]   hdl         Handle of robotkernel-5 process data retreaved from <get_pd_handle>.
 */
void kernel_c_release_pd_handle(pdhandle hdl);

};

#endif // PD_SFUN_HELPER_H

