//! robotkernel module definition
/*!
 * author: Robert Burger
 *
 * $Id$
 */

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

// -*- mode: c++ -*-
#ifndef CHAR_RINGBUFFER_H
#define CHAR_RINGBUFFER_H

#include <sstream>
#include <pthread.h>

#include <string_util/exceptions.h>

#ifdef DEBUG
#include <stdio.h>
#endif

using namespace std;


class flolock {
	pthread_mutex_t rwlock;
	bool locked;

public:
	flolock(bool locked=true);
	~flolock();

	void operator()();
	bool trylock();
	void unlock();
  
};
void cleanup_unlock_mutex(void* mutex);

class char_ringbuffer {
	unsigned int size;
	char* data;

	unsigned int write_start;
	unsigned int read_start;
	bool buffer_full;
	
	flolock l;

public:
	char_ringbuffer(unsigned int size) {
		data = (char*) malloc(size);
		if(!data)
			throw string_util::errno_exception_tb("could not get char_ringbuffer of size %d bytes\n", size);
		memset(data, 0, size);
		this->size = size;
		write_start = 0;
		read_start = 0;
		buffer_full = false;
		l.unlock();
	}
	
	~char_ringbuffer() {
		if(data)
			free(data);
		data = NULL;
	}

	unsigned int get_size() {
		return size;
	}
	
	void set_size(unsigned int new_size) {
		l();
		pthread_cleanup_push(cleanup_unlock_mutex, &l);
		void* new_mem = realloc(data, new_size);
		if(new_mem) {
			data = (char*)new_mem;
			size = new_size;
			memset(data, 0, size);
			write_start = 0;
			read_start = 0;
			buffer_full = false;
		}
		l.unlock();
		pthread_cleanup_pop(0);
	}
	
	void write(string data) {
		const char* new_data = data.c_str();
		int n = data.size();
		l();
		pthread_cleanup_push(cleanup_unlock_mutex, &l);
		
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
		l.unlock();
		pthread_cleanup_pop(0);
	}

#ifdef DEBUG
	void debug() {
		printf("write_start: %d\nread_start: %d\nbuffer_full: %d\n", write_start, read_start, buffer_full);
		printf("contents:\n");
		for(unsigned int i = 0; i < size; i++) {
			printf("%c", isprint(data[i]) ? data[i] : '.');
		}
		printf("\n");
		printf("get: %s\n\n", repr(get(true)).c_str());
	}
#endif
	string get(bool keep=false) {
		stringstream ss;
		unsigned int this_read_start;
		l();
		pthread_cleanup_push(cleanup_unlock_mutex, &l);
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
		l.unlock();
		pthread_cleanup_pop(0);
		return ss.str();
	}

	bool has_data() {
		return read_start != write_start || buffer_full;
	}
	unsigned int get_data_len() {
		unsigned int s = 0;
		unsigned int rs = read_start;
		if(buffer_full) {
			s = size - read_start;
			rs = 0;
		}
		s += write_start - rs;
		return s;
	}
};

#endif // CHAR_RINGBUFFER_H
