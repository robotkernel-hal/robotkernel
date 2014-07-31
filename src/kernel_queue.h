//! an O/S-independent queueing mechanism which is typically used for
//  inter-thread communication in a multi-threaded environment.
/*!
 * author: Robert Burger, DLR
 *
 * $Id$
 */

/*
 * This file is part of snarf_ll library
 *
 * snarf_ll is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * snarf_ll is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with snarf_ll.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __KERNEL_QUEUE_H__
#define __KERNEL_QUEUE_H__

template <typename T>
class kernel_queue {
    public:
        kernel_queue(uint32_t len);

        void put(T elem); 

        T get();

    private:
        T *_elems;
        sem_t _full_cnt;
        sem_t _empty_cnt;
        pthread_mutex_t _lock;
        int _in;
        int _out;
        int _len;
};

template <typename T>
kernel_queue<T>::kernel_queue(uint32_t len) {
    _len = len;
    _elems = new T[len];
    _in = _out = 0;
    sem_init(&_full_cnt, 1, 0);
    sem_init(&_empty_cnt, 1, len);
    pthread_mutex_init(&_lock, NULL);
}

template <typename T>
void kernel_queue<T>::put(T elem) {
    sem_wait(&_empty_cnt);
    pthread_mutex_lock(&_lock);

    _elems[_in] = elem;

    if (++_in == _len)
        _in = 0;

    sem_post(&_full_cnt);
    pthread_mutex_unlock(&_lock);
}

template <typename T>
T kernel_queue<T>::get() {
    sem_wait(&_full_cnt);
    pthread_mutex_lock(&_lock);
  
    T act = _elems[_out];
    
    if (++_out == _len)
        _out = 0;

    sem_post(&_empty_cnt);
    pthread_mutex_unlock(&_lock);
    
    return act;
}

#endif /* __KERNEL_QUEUE_H__ */

