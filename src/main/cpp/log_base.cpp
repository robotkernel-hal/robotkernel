//! robotkernel loglevel base
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

#include "robotkernel/log_base.h"

using namespace std;
using namespace std::placeholders;
using namespace robotkernel;
using namespace string_util;

//! construction
/*!
 * \param node config node
 */
log_base::log_base(const std::string& instance_name, 
        const std::string& name, const YAML::Node& node) :
    instance_name(instance_name), name(name) 
{
    kernel& k = *kernel::get_instance();
    ll = k.get_loglevel();

    // search for loglevel
    if (node.IsDefined()) {
        if (node["loglevel"]) {
            ll = info;
            std::string ll_string = get_as<std::string>(node, "loglevel", "");

            if (ll_string == "error")
                ll = error;
            else if (ll_string == "warning")
                ll = warning;
            else if (ll_string == "info")
                ll = info;
            else if (ll_string == "verbose")
                ll = verbose;
        } 
    }

    k.add_service(instance_name, "configure_loglevel", log_base::service_definition_configure_loglevel,
            std::bind(&log_base::service_configure_loglevel, this, _1, _2));
}

//! destruction
log_base::~log_base() {
    kernel& k = *kernel::get_instance();
    k.remove_service(name, "configure_loglevel");
}

//! service to configure loglevels loglevel
/*!
 * \param request service request data
 * \parma response service response data
 */
int log_base::service_configure_loglevel(const service_arglist_t& request, 
        service_arglist_t& response) {
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

const std::string log_base::service_definition_configure_loglevel = 
"request:\n"
"    string: set_loglevel\n"
"response:\n"
"    string: current_loglevel\n"
"    string: error_message\n";

//! log to kernel logging facility
void log_base::log(loglevel lvl, const char *format, ...) {
    kernel& k = *kernel::get_instance();
    struct log_thread::log_pool_object *obj;

    char buf[1024];
    int bufpos = 0;
    bufpos += snprintf(buf+bufpos, sizeof(buf)-bufpos, "[%s|%s] ", 
            name.c_str(), instance_name.c_str());

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

