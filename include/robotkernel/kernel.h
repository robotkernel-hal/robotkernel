//! robotkernel kernel
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

#ifndef __KERNEL_H__
#define __KERNEL_H__

#include <string>
#include "robotkernel/module.h"
#include "robotkernel/log_thread.h"
#include "robotkernel/dump_log.h"
#include "robotkernel/ln_kernel_messages.h"

#include "ln.h"
#include "ln_cppwrapper.h"

#define ROBOTKERNEL "[robotkernel] "

#define klog(...) robotkernel::kernel::get_instance()->logging(__VA_ARGS__)

namespace robotkernel {

enum loglevel {
    error = 1,
    warning = 2,
    info = 3,
    verbose = 4
};

class kernel :
    public ln_service_set_state_base,
    public ln_service_get_state_base,
    public ln_service_get_states_base,
    public ln_service_add_module_base,
    public ln_service_remove_module_base,
    public ln_service_module_list_base,
    public ln_service_reconfigure_module_base,
    public ln_service_get_dump_log_base,
    public ln_service_config_dump_log_base {
    private:
        //! kernel singleton instance
        static kernel *instance;

    protected:
        //! construction
        /*!
         */
        kernel();

        //! destruction
        ~kernel();

    public:
        //! get kernel singleton instance
        /*!
         * \return kernel singleton instance
         */
        static kernel * get_instance();

        //! destroy singleton instance
        static void destroy_instance();

        //! initialize links and nodes client
        /*!
         * \param argc command line argument counter
         * \param argv command line arguments
         */
        void init_ln(int argc, char **argv);

        //! handle links and nodes service requests
        void handle_ln_request(int argc, char **argv);

        //! construction
        /*!
         * \param configfile config file name
         */
        void config(std::string config_file, int argc, char **argv);

        //! powering up modules
        bool power_up();

        //! powering down modules
        void power_down();

        //! set state of module
        /*!
         * \param mod_name name of module
         * \param state new module state
         * \return state
         */
        int set_state(std::string mod_name, module_state_t state);

        //! return module state
        /*!
         * \param mod_name name of module which state to return
         * \return module state
         */
        module_state_t get_state(std::string mod_name);

        //! checks if all modules are at requested state
        /*!
         * \param mod_name module name to check
         * \param state requested state
         *
         * \return true if we are in right  state
         */
        bool state_check(std::string mod_name, module_state_t state);
        bool state_check();

        // modules map
        typedef std::map<std::string, module *> module_map_t;
        module_map_t module_map;

        // config file name
        std::string config_file;

        //! kernel request callback
        /*!
         * \param mod_name module name to send request to
         * \param reqcode request code
         * \param ptr request specifix pointer
         * \return request status code
         */
        static int request_cb(const char *mod_name, int reqcode, void *ptr);

        typedef void* interface_id_t;

        //! kernel register interface callback
        /*!
         * \param mod_name module name to send request to
         * \param if_name interface name to register
         * \param offset module offset
         * \return interface id or -1
         */
        static interface_id_t register_interface_cb(const char *mod_name, 
                const char *if_name, const char* dev_name, int offset); 

        //! kernel unregister interface callback
        /*!
         * \param interface_id interface id
         * \return N/A
         */
        static void unregister_interface_cb(interface_id_t interface_id);

        //! module state change
        /*!
         * \param mod_name module name which changed state
         * \param new_state new state of module
         * \retun success
         */
        int state_change(const char *mod_name, module_state_t new_state);

        //! Send a request to kernel
        /*!
          \param reqcode request code
          \param ptr pointer to request structure
          \return success or failure
          */
        int request(int reqcode, void* ptr);

        //! get module
        /*!
         * \param mod_name name of module
         * \return pointer to module
         */
        module *get_module(const char *mod_name);

        //! links and node client
        ln::client *clnt;

        std::string _name;
        std::string _internal_modpath;
        std::string _internal_intfpath;

        loglevel _ll;

        log_thread _log;

        // write to logfile
        void logging(loglevel ll, const char *format, ...) {
            if (ll > _ll) {
                va_list args;
                va_start(args, format);
                vdump_log(format, args);
                va_end(args);
                return;
            }

//            char buf[512];
            struct log_thread::log_pool_object *obj = _log.get_pool_object();
            struct tm timeinfo;
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            double timestamp = (double)ts.tv_sec + (ts.tv_nsec / 1e9);
            time_t seconds = (time_t)timestamp;
            double mseconds = (timestamp - (double)seconds) * 1000.;
            localtime_r(&seconds, &timeinfo);
            strftime(obj->buf, sizeof(obj->buf), "%F %T", &timeinfo);


            int len = strlen(obj->buf);
            snprintf(obj->buf + len, sizeof(obj->buf) - len, ".%03.0f ", mseconds);
            len = strlen(obj->buf);


            // format argument list
            va_list args;
            va_start(args, format);
            vsnprintf(obj->buf + len, obj->len - len, format, args);
            vdump_log(format, args);
            _log.log(obj);
//            printf("%s", obj->buf);
            va_end(args);
        }

        std::string dump_log() {
            return dump_log_dump();
        }
        void config_dump_log(unsigned int max_len, unsigned int do_ust=0) {
            dump_log_set_len(max_len, do_ust);
        }
        
        //! get dump log
        /*!
         * \param req service request
         * \param svn service container
         * \return success
         */
        virtual int on_get_dump_log(ln::service_request& req, 
                ln_service_robotkernel_get_dump_log& svc);
        
        //! config dump log
        /*!
         * \param req service request
         * \param svn service container
         * \return success
         */
        virtual int on_config_dump_log(ln::service_request& req,
                ln_service_robotkernel_config_dump_log& svc);

        //! set state
        /*!
         * \param req service request
         * \param svn service container
         * \return success
         */
        virtual int on_set_state(ln::service_request& req, 
                ln_service_robotkernel_set_state& svc);

        //! get state
        /*!
         * \param req service request
         * \param svn service container
         * \return success
         */
        virtual int on_get_state(ln::service_request& req,
                ln_service_robotkernel_get_state& svc);
        
        //! get states
        /*!
         * \param req service request
         * \param svn service container
         * \return success
         */
        virtual int on_get_states(ln::service_request& req, 
                ln_service_robotkernel_get_states& svc);
        
        //! add module
        /*!
         * \param req service request
         * \param svn service container
         * \return success
         */
        virtual int on_add_module(ln::service_request& req,
                ln_service_robotkernel_add_module& svc);
        
        //! remove module
        /*!
         * \param req service request
         * \param svn service container
         * \return success
         */
        virtual int on_remove_module(ln::service_request& req,
                ln_service_robotkernel_remove_module& svc);
        
        //! module list
        /*!
         * \param req service request
         * \param svn service container
         * \return success
         */
        virtual int on_module_list(ln::service_request& req,
                ln_service_robotkernel_module_list& svc);
        
        //! reconfigure module
        /*!
         * \param req service request
         * \param svn service container
         * \return success
         */
        virtual int on_reconfigure_module(ln::service_request& req,
                ln_service_robotkernel_reconfigure_module& svc);
};

} // namespace robotkernel

#endif // __KERNEL_H__

