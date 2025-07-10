//! robotkernel interface definition
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
#include "robotkernel/helpers.h"

// private headers
#include "char_ringbuffer.h"

#include <string.h>

using namespace std;
using namespace robotkernel;

char_ringbuffer::char_ringbuffer(unsigned int size) {
    data = (char*) malloc(size);

    if(!data)
        throw runtime_error(string_printf("could not get char_ringbuffer of size %d bytes\n", size));

    memset(data, 0, size);
    this->size = size;
    write_start = 0;
    read_start = 0;
    buffer_full = false;
}

char_ringbuffer::~char_ringbuffer() {
    if(data)
        free(data);
    data = NULL;
}

unsigned int char_ringbuffer::get_size() {
    return size;
}

void char_ringbuffer::set_size(unsigned int new_size) {
    std::unique_lock<std::mutex> lock(mtx);

    void* new_mem = realloc(data, new_size);
    if(new_mem) {
        data = (char*)new_mem;
        size = new_size;
        memset(data, 0, size);
        write_start = 0;
        read_start = 0;
        buffer_full = false;
    }
}

void char_ringbuffer::write(string data) {
    const char* new_data = data.c_str();
    int n = data.size();

    std::unique_lock<std::mutex> lock(mtx);

    while((write_start + n) >= size) {
        buffer_full = true;

        unsigned int first_write = size - write_start;
        // printf("write at first only %d bytes: '%*.*s'\n", first_write, first_write, first_write, new_data);
        memcpy(this->data + write_start, new_data, first_write);
        new_data += first_write;
        n -= first_write;
        if(read_start > write_start) {
            // printf("set read_start to 0\n");
            read_start = 0;
        }
        write_start = 0;
    }
    if(n) {
        //printf("write at last %d bytes: '%*.*s'\n", n, n, n, new_data);
        memcpy(this->data + write_start, new_data, n);
        if(buffer_full && write_start == read_start) {
            //printf("increment read_start by %d\n", n);
            read_start += n;
        } else if(read_start > write_start && write_start + n > read_start) {
            //printf("set read start to new write_start!\n");
            read_start = write_start + n;
        }
        write_start += n;
    }
}

#ifdef DEBUG
void char_ringbuffer::debug() {
    printf("write_start: %d\nread_start: %d\nbuffer_full: %d\n", write_start, read_start, buffer_full);
    printf("contents:\n");
    for(unsigned int i = 0; i < size; i++) {
        printf("%c", isprint(data[i]) ? data[i] : '.');
    }
    printf("\n");
    printf("get: %s\n\n", repr(get(true)).c_str());
}
#endif

string char_ringbuffer::get(bool keep) {
    stringstream ss;
    unsigned int this_read_start;

    std::unique_lock<std::mutex> lock(mtx);

    this_read_start = read_start;

    if(buffer_full) {
        ss << string(data + read_start, size - this_read_start);
        this_read_start = 0;
    }
    ss << string(data + this_read_start, write_start - this_read_start);
    if(!keep) {
        read_start = write_start;
        buffer_full = false;
    }

    return ss.str();
}

bool char_ringbuffer::has_data() {
    return read_start != write_start || buffer_full;
}

unsigned int char_ringbuffer::get_data_len() {
    unsigned int s = 0;
    unsigned int rs = read_start;
    if(buffer_full) {
        s = size - read_start;
        rs = 0;
    }
    s += write_start - rs;
    return s;
}

