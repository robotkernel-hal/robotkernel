#include "char_ringbuffer.h"

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
