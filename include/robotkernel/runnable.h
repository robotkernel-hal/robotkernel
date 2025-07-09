//! robotkernel runnable
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

#ifndef ROBOTKERNEL__RUNNABLE_H
#define ROBOTKERNEL__RUNNABLE_H

#include <string>
#include <thread>

#include <yaml-cpp/yaml.h>

namespace robotkernel {

//! runnable base class
/*!
 * derive from this class, if you need a runnable instance
 */
class runnable {
    private:
        //! run wrapper to create thread
        /*!
         */
        void run_wrapper();

        runnable(const runnable&);             // prevent copy-construction
        runnable& operator=(const runnable&);  // prevent assignment

        int policy;                 //!< thread scheduling policy
        int prio;                   //!< thread priority
        int affinity_mask;          //!< thread cpu affinity mask

    protected:
        std::string thread_name;    //!< thread name used to set threads name with pthread_setname_np

        std::thread tid;            //!< thread handle
        bool run_flag;              //!< running flag

    public:
        //! construction with yaml node
        /*!
         * \param node yaml node which must contain prio and affinity
         */
        runnable(const YAML::Node& node);

        //! construction with prio and affinity_mask
        /*!
         * \param[in] prio          Runnable thread priority.
         * \param[in] affinity_mask Runnable thread cpu affinity mask.
         * \param[in] thread_name   Name of runnable thread if started. Only
         *                          used if pthread_setname_np is present.
         */
        runnable(const int prio = 0, const int affinity_mask = 0, 
                std::string thread_name = "runnable");

        //! destruction
        virtual ~runnable() {
            stop();
        };

        void start();                       //!< run thread
        void stop();                        //!< stop thread
        void join();                        //!< join thread
        virtual void run() = 0;             //!< handler function called if 
                                            //   thread is running

        const int& get_policy() const;        //!< return policy
        const int& get_prio() const;          //!< return priority 
        const int& get_affinity_mask() const; //!< return affinity mask
        const std::string& get_name() const;  //!< return thread name.

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
}

inline const int& runnable::get_prio() const {
    return prio; 
}

inline const std::string& runnable::get_name() const {
    return thread_name;
}

inline void runnable::set_policy(int policy) {
    this->policy = policy;
}

} // namespace robotkernel

#endif // ROBOTKERNEL__RUNNABLE_H

