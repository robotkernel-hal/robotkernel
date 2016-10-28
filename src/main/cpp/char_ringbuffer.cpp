//! robotkernel interface definition
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


#include "robotkernel/char_ringbuffer.h"

flolock::flolock(bool locked) {
	this->locked = false;
	int ret = pthread_mutex_init(&rwlock, NULL);
	if(ret)
		throw retval_exception_tb(ret, "pthread_mutex_init");
	if(locked)
		(*this)();
}

flolock::~flolock() {
	if(this->locked)
		unlock();
	pthread_mutex_destroy(&rwlock);
}

void flolock::operator()() {
	int ret = pthread_mutex_lock(&rwlock);
	if(ret)
		throw retval_exception_tb(ret, "pthread_mutex_lock");
	this->locked = true;
}

bool flolock::trylock() {
	if(pthread_mutex_trylock(&rwlock) == 0) {
		this->locked = true;
		return true;
	}
	return false;
		
}

void flolock::unlock() {
 	this->locked = false;
	int ret = pthread_mutex_unlock(&rwlock);
	if(ret)
		throw retval_exception_tb(ret, "pthread_mutex_unlock");
}

void cleanup_unlock_mutex(void* mutex) {
	flolock* l = (flolock*)mutex;
	l->unlock();
}
