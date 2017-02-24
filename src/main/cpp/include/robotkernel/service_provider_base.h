//! robotkernel interface base
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

#ifndef __ROBOTKERNEL_INTERFACE_BASE_H__
#define __ROBOTKERNEL_INTERFACE_BASE_H__

#include "robotkernel/interface_intf.h"
#include "robotkernel/kernel.h"
#include "robotkernel/exceptions.h"
#include "robotkernel/helpers.h"
#include "robotkernel/service_provider.h"

#include "yaml-cpp/yaml.h"

#include <map>
#include <functional>

#ifdef __cplusplus
#define EXPORT_C extern "C"
#else
#define EXPORT_C
#endif

#define HDL_2_INTFCLASS(hdl, intfname, intfclass)               \
    intfclass *dev = reinterpret_cast<intfclass *>(hdl);        \
    if (!dev)                                                   \
        throw robotkernel::str_exception(                       \
                "["#intfname"] invalid interface "              \
                "handle to <"#intfclass" *>\n");

#define INTERFACE_DEF(intfname, intfclass)                      \
                                                                \
EXPORT_C int intf_unregister(INTERFACE_HANDLE hdl) {            \
    HDL_2_INTFCLASS(hdl, intfname, intfclass)                   \
    delete dev;                                                 \
    return 0;                                                   \
}                                                               \
                                                                \
EXPORT_C INTERFACE_HANDLE intf_register() {                     \
    intfclass *dev;                                             \
                                                                \
    dev = new intfclass(doc);                                   \
    if (!dev)                                                   \
        throw robotkernel::str_exception(                       \
                "["#intfname"] error allocating memory\n");     \
                                                                \
    return (INTERFACE_HANDLE)dev;                               \
}

namespace robotkernel {

    static const std::string service_definition_configure_loglevel =
    "request:\n"
    "    string: set_loglevel\n"
    "response:\n"
    "    string: current_loglevel\n"
    "    string: error_message\n";


    template<typename T>
    class ServiceProviderBase : ServiceProvider{
        typedef map<std::string, T *> ServiceInterfaceMap;
    private:
        ServiceProviderBase();       //!< prevent default construction

    protected:
        std::string intf_name;  //!< interface name
        std::string mod_name;   //!< module name
        std::string dev_name;   //!< service device name
        int slave_id;           //!< device slave id
        loglevel ll;            //!< interface loglevel
        T *interface;

        virtual void initServices() = 0;
        
        std::string getFullQualifiedServiceName(std::string simpleName){
            stringstream base;
            base << mod_name << "." << dev_name << "." << intf_name << "." << simpleName;
            return base.str();
        }
        
        void addService(const std::string& simpleName, 
                        const std::string& service_definition, 
                        service_callback_t callback){
            kernel &k = *kernel::get_instance();
            k.add_service(mod_name, getFullQualifiedServiceName(simpleName), service_definition, callback);
        }
        
    public:
        //! construction
        /*!
         * \param intfname interface name
         * \param name instance name
         */
        ServiceProviderBase(const std::string &intf_name)
                : intf_name(intf_name) {}

        void init(const YAML::Node &node, void* interface) {
            this->interface = static_cast<T*>(interface);
            mod_name = get_as<std::string>(node, "mod_name");
            dev_name = get_as<std::string>(node, "dev_name");
            slave_id = get_as<int>(node, "slave_id");
            ll = get_as<std::string>(node, "loglevel", kernel::get_instance()->get_loglevel());
            
            addService("configure_loglevel", service_definition_configure_loglevel, 
                       std::bind(&ServiceProviderBase::service_configure_loglevel, this, 
                                 std::placeholders::_1, std::placeholders::_2));
            initServices();
        }

        //! destruction
        ~ServiceProviderBase(){
//    unregister_configure_loglevel();
        }


        //! log to kernel logging facility
        void log(robotkernel::loglevel lvl, const char *format, ...){
            kernel& k = *kernel::get_instance();
            struct log_thread::log_pool_object *obj;

            char buf[1024];
            int bufpos = 0;
            bufpos += snprintf(buf+bufpos, sizeof(buf)+bufpos, "[%s|%s] ",
                               mod_name.c_str(), intf_name.c_str());

            // format argument list    
            va_list args;
            va_start(args, format);
            vsnprintf(buf+bufpos, sizeof(buf)-bufpos, format, args);
            va_end(args);

            if (lvl > ll)
                goto log_exit;

            if ((obj = k.rk_log.get_pool_object()) != NULL) {
                // only ifempty log pool avaliable!
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
                snprintf(obj->buf + len, sizeof(obj->buf) - len, "%s %s",
                         k.ll_to_string(lvl).c_str(), buf);
                k.rk_log.log(obj);
            }

            log_exit:
            dump_log(buf);
        }

        //! service to configure interface loglevel
        /*!
         * \param request service request data
         * \parma response service response data
         */
        int service_configure_loglevel(const service_arglist_t &request, service_arglist_t &response){
            // request data
#define CONFIGURE_LOGLEVEL_REQ_SET_LOGLEVEL         0
            string set_loglevel = request[CONFIGURE_LOGLEVEL_REQ_SET_LOGLEVEL];

            // response data
            string current_loglevel = (std::string)ll;
            string error_message = "";

            if (set_loglevel != "") {
                try {
                    ll = set_loglevel;
                } catch (const std::exception& e) {
                    error_message = e.what();
                }
            }

#define CONFIGURE_LOGLEVEL_RESP_CURRENT_LOGLEVEL    0
#define CONFIGURE_LOGLEVEL_RESP_ERROR_MESSAGE       1
            response.resize(2);
            response[CONFIGURE_LOGLEVEL_RESP_CURRENT_LOGLEVEL] = current_loglevel;
            response[CONFIGURE_LOGLEVEL_RESP_ERROR_MESSAGE]    = error_message;

            return 0;
        }

        

    };

};

#endif // __ROBOTKERNEL_INTERFACE_BASE_H__

