//! robotkernel base class for triggers
/*!
 * author: Robert Burger <robert.burger@dlr.de>
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

#ifndef __trigger_device_H__
#define __trigger_device_H__

#include <pthread.h>
#include <string>
#include "robotkernel/module_intf.h"
#include "robotkernel/runnable.h"

namespace robotkernel {
#ifdef EMACS
}
#endif

//! trigger_base
/*
 * derive from this class if you want to get 
 * triggered by a trigger_device
 */
class trigger_base {
    public:
        int divisor;        //!< trigger every ""divisor"" step
        int cnt;            //!< internal step counter

        trigger_base(int divisor=1) : divisor(divisor), cnt(0) {};
    
        //! trigger function
        virtual void trigger() = 0;
};

typedef std::shared_ptr<trigger_base> sp_trigger_base_t;
typedef std::list<sp_trigger_base_t> trigger_list_t;

//! trigger worker thread
class trigger_worker : 
    public robotkernel::runnable, 
    public robotkernel::trigger_base 
{
    private:
        trigger_worker();
        trigger_worker(const trigger_worker&);             // prevent copy-construction
        trigger_worker& operator=(const trigger_worker&);  // prevent assignment

    public:
        struct worker_key {
            int prio;
            int affinity;
            int divisor;

            bool operator<(const worker_key& a) const;
        };

        //! default construction
        trigger_worker(int prio = 60, int affinity_mask = 0xFF, int divisor=1);

        //! destruction
        ~trigger_worker();

        //! add trigger_callback to worker
        /*!
         * \param trigger trigger callback to add
         */
        void add_trigger_callback(sp_trigger_base_t trigger);

        //! remove trigger_callback from worker
        /*!
         * \param trigger trigger callback to remove
         */
        void remove_trigger_callback(sp_trigger_base_t trigger);

        //! trigger worker
        void trigger();

        //! handler function called if thread is running
        void run();

        //! return size of triggers
        size_t size() { return triggers.size(); }

    private:
        trigger_list_t triggers;

        pthread_mutex_t mutex;  //!< ipc lock
        pthread_cond_t cond;    //!< ipc condition
};
        
typedef std::shared_ptr<trigger_worker> sp_trigger_worker_t;
typedef std::map<trigger_worker::worker_key, sp_trigger_worker_t> trigger_workers_t;

class trigger_device 
{
    private:
        trigger_device(const trigger_device&);             // prevent copy-construction
        trigger_device& operator=(const trigger_device&);  // prevent assignment

        pthread_mutex_t list_lock;              //!< protection for trigger list
        trigger_list_t triggers;                //!< trigger callback list
        trigger_workers_t workers;              //!< workers

    protected:
        double rate;                            //!< trigger rate in [Hz]

    public:
        const std::string trigger_dev_name;     //!< trigger device name

    public:
        //! construction
        trigger_device(const std::string& trigger_dev_name, double rate=0.);

        //! destruction
        ~trigger_device();

        //! add a trigger callback function
        /*!
         * \param cb trigger callback
         * \param divisor rate divisor
         * \return trigger object to newly inserted callback
         */
        void add_trigger_callback(sp_trigger_base_t trigger, bool direct_mode=true,
                int worker_prio=0, int worker_affinity=0);

        //! remove a trigger callback function
        /*!
         * \param obj trigger object to trigger callback
         */
        void remove_trigger_callback(sp_trigger_base_t trigger);

        //! get rate of trigger
        /*!
         * return the current rate of the trigger device
         */
        double get_rate() const { return rate; }

        //! set rate of trigger device
        /*!
         * set the rate of the current trigger
         * overload in derived trigger class
         *
         * \param new_rate new trigger rate to set
         */
        virtual void set_rate(double new_rate);

        //! trigger all modules in list
        void trigger_modules();
};

typedef std::shared_ptr<trigger_device> sp_trigger_device_t;
typedef std::map<std::string, sp_trigger_device_t> trigger_device_map_t;

#ifdef EMACS 
{
#endif
} // namespace robotkernel

#endif // __trigger_device_H__

