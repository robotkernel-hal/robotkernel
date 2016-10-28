//! robotkernel base class for triggers
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

#ifndef __TRIGGER_BASE_H__
#define __TRIGGER_BASE_H__

#include <pthread.h>
#include "robotkernel/kernel.h"
#include "robotkernel/module_intf.h"

namespace robotkernel {

class trigger_base {
    private:
        trigger_base(const trigger_base&);             // prevent copy-construction
        trigger_base& operator=(const trigger_base&);  // prevent assignment

        //! protection for trigger list
        pthread_mutex_t list_lock;
        
        typedef std::list<set_trigger_cb_t> cb_list_t;
        
        //! trigger callback list
        cb_list_t trigger_cbs;
    public:
        //! construction
        trigger_base();

        //! destruction
        ~trigger_base();

        //! add a trigger callback function
        /*!
         * \param cb trigger callback
         * \return true on success, false if no ring is present or
         *         callback function pointer is NULL
         */
        bool add_trigger_module(set_trigger_cb_t& cb);

        //! remove a trigger callback function
        /*!
         * \param cb trigger callback
         * \return true on success, false if no ring is present or
         *         callback function pointer is NULL
         */
        bool remove_trigger_module(set_trigger_cb_t& cb);

        //! trigger all modules in list
        /*!
         * \param clk_id clock id to trigger, -1 all clocks
         */
        void trigger_modules(int clk_id = -1);

        //! handle request for trigger base
        /*! 
          \param reqcode request code
          \param ptr pointer to request structure
          \return success or failure
          */
        int request(int reqcode, void* ptr);
        
        int get_trigger_size(); //!< return size of trigger list
};

inline int trigger_base::get_trigger_size() {
    return trigger_cbs.size();
}

} // namespace robotkernel

#endif // __TRIGGER_BASE_H__

