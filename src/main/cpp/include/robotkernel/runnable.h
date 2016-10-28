//! robotkernel runnable
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

#ifndef __RUNNABLE_H__
#define __RUNNABLE_H__

#include <string>
#include <pthread.h>
#include <stdio.h>
#include <list>
#include "robotkernel/module.h"

namespace robotkernel {

//! runnable base class
/*!
 * derive from this class, if you need a runnable instance
 */
class runnable {
    private:
        //! run wrapper to create posix thread
        /*!
         * \param arg thread argument
         * \return NULL
         */
        static void *run_wrapper(void *arg);

        runnable(const runnable&);             // prevent copy-construction
        runnable& operator=(const runnable&);  // prevent assignment


        int policy;             //! thread scheduling policy
        int prio;               //! thread priority
        int affinity_mask;      //! thread cpu affinity mask
protected:
        pthread_t tid;          //! posix thread handle
        bool run_flag;          //! running flag

    public:
        //! construction with yaml node
        /*!
         * \param node yaml node which must contain prio and affinity
         */
        runnable(const YAML::Node& node);

        //! construction with prio and affinity_mask
        /*!
         * \param prio thread priority
         * \param affinity_mask thread cpu affinity mask
         */
        runnable(const int prio=0, const int affinity_mask=0);

        //! destruction
        virtual ~runnable() {
            stop();
        };

        void start();                       //!< run thread
        void stop();                        //!< stop thread
        virtual void run() = 0;             //!< handler function called if 
                                            //   thread is running

        const int& get_policy() const;        //!< return policy
        const int& get_affinity_mask() const; //!< return affinity mask
        const int& get_prio() const;          //!< return priority 

        void set_policy(int policy);        //!< set policy
        void set_prio(int prio);            //!< set priority
        void set_affinity_mask(int mask);   //!< set affinity mask
        void set_name(std::string name);    //!< set thread name

        bool running();                     //!< returns true if thread is running
};

inline bool runnable::running() {
    return this->run_flag;
}
        
inline const int& runnable::get_policy() const {
    return policy;
}
        
inline const int& runnable::get_affinity_mask() const { 
    return affinity_mask; 
};

inline const int& runnable::get_prio() const {
    return prio; 
};
        
inline void runnable::set_policy(int policy) {
    this->policy = policy;
};

} // namespace robotkernel

#endif // __RUNNABLE_H__

