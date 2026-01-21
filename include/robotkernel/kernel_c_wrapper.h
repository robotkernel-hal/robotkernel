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

/**
 * @file kernel_c_wrapper.h
 * @brief C interface for accessing robotkernel process data.
 *
 * This header exposes a minimal C API for retrieving and
 * managing process data handles, buffers, and synchronization.
 * It is intended for use when linking against robotkernel
 * from plain C code or foreign language bindings.
 *
 * @note
 * All functions are extern "C" to ensure stable symbols.
 * The API works with opaque pdhandle pointers.
 */

#ifndef ROBOTKERNEL__KERNEL_C_WRAPPER__H
#define ROBOTKERNEL__KERNEL_C_WRAPPER__H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @typedef pdhandle
 * @brief Opaque handle to a robotkernel process data instance.
 *
 * This handle represents either a *consumer* or *provider*
 * registration for robotkernel process data.
 * It must be released with kernel_c_release_pd_handle().
 */
typedef void * pdhandle;      //!< handle to robotkernel-5 process data

/**
 * @brief Retrieve a handle to robotkernel process data.
 *
 * This call registers the caller as either a *consumer* or
 * *provider* for the named process data.
 *
 * Once acquired:
 * - Consumers can read inputs and negotiate buffers.
 * - Providers can write outputs and commit them.
 *
 * @param[in] pd_name
 *        Null-terminated name of the process data entity
 *        (typically defined in the robotkernel configuration).
 *
 * @param[in] consumer
 *        If non-zero, register as a *consumer*.
 *        Otherwise, register as a *provider*.
 *
 * @return
 *        A handle usable for subsequent API calls,
 *        or `nullptr` on failure.
 */
pdhandle kernel_c_get_pd_handle(const char *pd_name, int consumer);

/**
 * @brief Check if a process data description matches an expected string.
 *
 * This function compares the internal process data description
 * with a provided textual description.
 *
 * @param[in] hdl
 *        Previously acquired handle (from kernel_c_get_pd_handle()).
 *
 * @param[in] desc
 *        Null-terminated description to check against.
 *
 * @return
 *        `0` if the description matches,
 *        `-1` otherwise.
 */
int kernel_c_check_pd_desc(pdhandle hdl, const char *desc);

/**
 * @brief Get pointer to the next write buffer.
 *
 * When acting as a provider, this returns a pointer to the
 * next output buffer that can be written to.
 *
 * After filling this buffer, the caller must commit it
 * with kernel_c_push_write_buffer().
 *
 * @param[in] hdl
 *        A valid process data handle.
 *
 * @return
 *        Pointer to a buffer of type `uint8_t*`
 *        containing writable output data.
 */
uint8_t *kernel_c_next_write_buffer(pdhandle hdl);

/**
 * @brief Push the previously acquired write buffer.
 *
 * After writing to the buffer returned by
 * kernel_c_next_write_buffer(), this call commits
 * it as the current output image for the provider.
 *
 * @param[in] hdl
 *        A valid process data handle.
 */
void kernel_c_push_write_buffer(pdhandle hdl);

/**
 * @brief Retrieve pointer to the current read buffer.
 *
 * For a consumer, this returns a pointer to the
 * latest buffer containing valid input data.
 *
 * @param[in] hdl
 *        A valid process data handle.
 *
 * @return
 *        Pointer to the current input buffer.
 */
uint8_t *kernel_c_act_read_buffer(pdhandle hdl);

/**
 * @brief Mark the current read buffer as read.
 *
 * After inspecting the buffer returned by
 * kernel_c_act_read_buffer(), the caller should
 * inform the system that the buffer was consumed.
 *
 * @param[in] hdl
 *        A valid process data handle.
 */
void kernel_c_pop_read_buffer(pdhandle hdl);

/**
 * @brief Release a previously acquired process data handle.
 *
 * Deregisters the consumer/provider and frees any
 * internal resources associated with the handle.
 *
 * @param[in] hdl
 *        Handle obtained via kernel_c_get_pd_handle().
 */
void kernel_c_release_pd_handle(pdhandle hdl);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // ROBOTKERNEL__KERNEL_C_WRAPPER__H

