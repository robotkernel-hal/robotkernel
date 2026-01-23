//! robotkernel Exceptions class
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
 * @file exceptions.h
 * @brief Exception and signal handling utilities for robotkernel.
 *
 * This header defines exception types and support logic for translating
 * POSIX signals (e.g., SIGSEGV, SIGFPE) into C++ exceptions, including
 * automatic backtrace capture.
 *
 * The utilities are intended to ease debugging and error reporting
 * when low-level faults occur during runtime.
 *
 * @note
 * - These classes leverage POSIX signal handling.
 * - The `signal_translator` template installs a signal handler
 *   that throws the associated exception type.
 * - A backtrace string is captured for diagnostics.
 */

#ifndef ROBOTKERNEL__EXCEPTIONS_H
#define ROBOTKERNEL__EXCEPTIONS_H

#include <exception>
#include <iostream>
#include <sstream>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>

namespace robotkernel {

/**
 * @class exception_tracer
 * @brief Helper to capture a backtrace string.
 *
 * On construction, this utility captures the current call stack
 * (backtrace) and stores it as a string for use in diagnostics.
 *
 * Derived exception types include this as a base class to
 * incorporate backtrace information in `what()` messages.
 */
class exception_tracer {
    public:
        /// Captured backtrace text.
        std::string _backtrace;

        /**
         * @brief Construct and capture a snapshot of the backtrace.
         *
         * Uses POSIX `backtrace()` and `backtrace_symbols()` APIs to
         * format the call stack at the point where the exception
         * tracer is created.
         */
        exception_tracer();
};

/**
 * @class signal_translator
 * @brief Installs a signal handler that throws a specific exception type.
 *
 * Template helper that installs a single signal handler for a given
 * signal (e.g., SIGSEGV, SIGFPE). When the signal occurs, the
 * handler throws an instance of the specified exception type.
 *
 * @tparam signal_exception_class
 *         Exception class type that must implement:
 *         - `static int get_signo()` (returns the POSIX signal number)
 *         - A default constructor
 *
 * @note
 * Each instantiation installs the handler only once due to the
 * internal singleton translator object.
 */
template <class signal_exception_class> 
class signal_translator {
    private:
        /// Internal singleton that installs the handlers.
        class singleton_translator {
            public:
                singleton_translator() {
                    signal(signal_exception_class::get_signo(), sig_handler);
                }

                static void sig_handler(int) {
                    throw signal_exception_class();
                }
        };

    public:
        /**
         * @brief Construct and ensure the signal handler is installed.
         *
         * The static singleton_translator object installs the signal
         * handler on the first instantiation of this template.
         */
        signal_translator() {
            static singleton_translator s_objTranslator;
            (void)s_objTranslator;
        }
};

/**
 * @class segmentation_fault_exception
 * @brief Exception thrown on a segmentation fault (SIGSEGV).
 *
 * Inherits backtrace capture and provides a message via `what()`.
 */
class segmentation_fault_exception : public exception_tracer, public std::exception {
    public:
        virtual ~segmentation_fault_exception() throw() {};

        /// The POSIX signal number for segmentation faults.
        static int get_signo() { return SIGSEGV; }
        
        /**
         * @brief Human-readable exception message.
         *
         * Includes the captured backtrace for diagnostics.
         *
         * @return Pointer to an internal C-string.
         */
        virtual const char* what() const throw() { 
            std::string msg = "segmentation fault exception: " + 
                _backtrace; 
            return msg.c_str();
        };
};

/**
 * @class floating_point_exception
 * @brief Exception thrown on a floating-point error (SIGFPE).
 *
 * Inherits backtrace capture and can be thrown automatically
 * via the signal_translator helper.
 */
class floating_point_exception : public exception_tracer, public std::exception {
    public:
        virtual ~floating_point_exception() throw() {};

        /// The POSIX signal number for floating point errors.
        static int get_signo() { return SIGFPE; }
};

}; // namespace robotkernel

#endif // ROBOTKERNEL__EXCEPTIONS_H

