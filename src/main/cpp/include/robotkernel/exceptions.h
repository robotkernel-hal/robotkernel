//! robotkernel Exceptions class
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

#ifndef ROBOTKERNEL__EXCEPTIONS_H
#define ROBOTKERNEL__EXCEPTIONS_H

#include "robotkernel/config.h"
#include "robotkernel/module_intf.h"
#include <exception>
#include <iostream>
#include <sstream>
#ifdef HAVE_EXECINFO_H
#include <execinfo.h>
#endif
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include "string_util/string_util.h" // str_exception

namespace robotkernel {

//! exception_tracer class
/*!
  This class prints backtrace on stdout
 */
class exception_tracer {
    public:
        exception_tracer() {
#ifdef HAVE_EXECINFO_H 
            std::stringstream buffer;

            void * array[25];
            int nSize = backtrace(array, 25);
            char ** symbols = backtrace_symbols(array, nSize);

            for (int i = 0; i < nSize; i++)
                buffer << symbols[i] << std::endl;

            free(symbols);
            _backtrace = buffer.str();
#else
            _backtrace = "backtrace is not supported";
#endif
        }

        std::string _backtrace;
};

//! signal_translator base class
/*!
  This Class attaches a signal handler to specified signal 
 */
template <class signal_exception_class> 
class signal_translator {
    private:
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
        signal_translator() {
            static singleton_translator s_objTranslator;
        }
};

//! segmentation_fault_exception
/*!
  This class catches SIGSEGV and throws and exception
 */
class segmentation_fault_exception : public exception_tracer, public std::exception {
    public:
        virtual ~segmentation_fault_exception() throw() {};

        //! returns signal number
        static int get_signo() { return SIGSEGV; }
        
        virtual const char* what() const throw() { 
            std::string msg = "segmentation fault exception: " + 
                _backtrace; 
            return msg.c_str();
        };
};

//! floating_point_exception
/*!
  This class catches SIGFPE and throws and exception
 */
class floating_point_exception : public exception_tracer, public std::exception {
    public:
        virtual ~floating_point_exception() throw() {};

        //! returns signal number
        static int get_signo() { return SIGFPE; }
};

}; // namespace robotkernel

#endif // ROBOTKERNEL__EXCEPTIONS_H

