//! robotkernel module definition
/*!
 * (C) Robert Burger <robert.burger@dlr.de>
 * (C) Florian Schmidt <florian.schmidt@dlr.de>
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

#ifndef CHAR_RINGBUFFER_H
#define CHAR_RINGBUFFER_H

#include <sstream>
#include <mutex>

#include <string_util/exceptions.h>

#ifdef DEBUG
#include <stdio.h>
#endif

namespace robotkernel {
#ifdef EMACS
}
#endif

class char_ringbuffer {
    private:
        unsigned int size;
        char* data;

        unsigned int write_start;
        unsigned int read_start;
        bool buffer_full;

        std::mutex mtx;

    public:
        char_ringbuffer(unsigned int size);
        ~char_ringbuffer();

        unsigned int get_size();

        void set_size(unsigned int new_size);
        void write(std::string data);

#ifdef DEBUG
        void debug();
#endif

        std::string get(bool keep=false);
        bool has_data();
        unsigned int get_data_len();
};

#ifdef EMACS
{
#endif
};

#endif // CHAR_RINGBUFFER_H

