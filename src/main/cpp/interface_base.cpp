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

#include "robotkernel/interface_base.h"
#include <sstream>

using namespace std;
using namespace robotkernel;
        
//! construction
/*!
 * \param intfname interface name
 * \param name instance name
 */
interface_base::interface_base(const std::string& intf_name, const YAML::Node& node) 
    : intf_name(intf_name) {
    kernel &k = *kernel::get_instance();
    mod_name = get_as<std::string>(node, "mod_name");
    dev_name = get_as<std::string>(node, "dev_name");
    slave_id = get_as<int>(node, "slave_id");
    ll = get_as<std::string>(node, "loglevel", k.get_loglevel());
    
    stringstream base;
    base << mod_name << "." << dev_name  << "." << intf_name 
        << ".configure_loglevel";

//    k.add_service(mod_name, base.str(), 
//            interface_base::service_definition_configure_loglevel,
//            boost::bind(&interface_base::service_configure_loglevel, this, _1));
}

//! destruction
interface_base::~interface_base() {
//    unregister_configure_loglevel();
}

//! log to kernel logging facility
void interface_base::log(loglevel lvl, const char *format, ...) {
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
 * message service message
 */
int interface_base::service_configure_loglevel(YAML::Node& message) {
    message["response"]["current_log_level"] = (std::string)ll;
    message["response"]["error_message"] = "";

    string set_ll = get_as<string>(message["request"], "set_log_level");

    if (set_ll != "") {
        try {
            ll = set_ll;
        } catch (const std::exception& e) {
            message["response"]["error_message"] = e.what();
        }
    }

    return 0;
}

const std::string interface_base::service_definition_configure_loglevel = 
    "request:\n"
    "    string: set_log_level\n"
    "response:\n"
    "    string: current_log_level\n"
    "    string: error_message\n";

