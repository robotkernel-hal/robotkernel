//! robotkernel trigger class
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

#ifndef __ROBOTKERNEL__TRIGGER_H__
#define __ROBOTKERNEL__TRIGGER_H__

#include <pthread.h>
#include <string>
#include "robotkernel/module_intf.h"
#include "robotkernel/runnable.h"
#include "robotkernel/device.h"
#include "robotkernel/trigger_base.h"
#include "robotkernel/trigger_worker.h"

namespace robotkernel {
#ifdef EMACS
}
#endif

class trigger :
    public device
{
    private:
        trigger(const trigger&);                // prevent copy-construction
        trigger& operator=(const trigger&);     // prevent assignment

        pthread_mutex_t list_lock;              //!< protection for trigger list
        trigger_list_t triggers;                //!< trigger callback list
        trigger_workers_t workers;              //!< workers

    protected:
        double rate;                            //!< trigger rate in [Hz]

    public:
        //! trigger construction
        /*!
         * \param[in] owner     Name of owning module/bridge/service_provider...
         * \param[in] name      Name of trigger.
         * \param[in] rate      Rate of trigger in seconds [s].
         */
        trigger(
                const std::string& owner, 
                const std::string& name, 
                double rate = 0.);

        //! destruction
        ~trigger();

        //! add a trigger callback function
        /*!
         * \param cb trigger callback
         * \param divisor rate divisor
         * \return trigger object to newly inserted callback
         */
        void add_trigger(sp_trigger_base_t trigger, bool direct_mode=true,
                int worker_prio=0, int worker_affinity=0);

        //! remove a trigger callback function
        /*!
         * \param obj trigger object to trigger callback
         */
        void remove_trigger(sp_trigger_base_t trigger);

        //! get rate of trigger
        /*!
         * return the current rate of the trigger 
         */
        double get_rate() const { return rate; }

        //! set rate of trigger 
        /*!
         * set the rate of the current trigger
         * overload in derived trigger class
         *
         * \param new_rate new trigger rate to set
         */
        virtual void set_rate(double new_rate);

        //! trigger all modules in list
        void trigger_modules();

        //! wait blocking for next trigger
        /*!
         * \param[in] timeout   Wait timeout in seconds.
         */
        void wait(double timeout);
};

typedef std::shared_ptr<trigger> sp_trigger_t;
typedef std::map<std::string, sp_trigger_t> trigger_map_t;

#ifdef EMACS 
{
#endif
} // namespace robotkernel

#endif // __ROBOTKERNEL__TRIGGER_H__

