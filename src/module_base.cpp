//! robotkernel module base
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

#include "robotkernel/module_base.h"
#include <sstream>

using namespace std;
using namespace robotkernel;
        
//! construction
/*!
 * \param modname module name
 * \param name instance name
 */
module_base::module_base(const std::string& modname, const std::string& name) :
    modname(modname), name(name) {
    state = module_state_unknown;
    kernel& k = *kernel::get_instance();
    ll = k.get_loglevel();
	
    if (k.clnt) {
        stringstream base;
        base << k.clnt->name << "." << name << ".";

        register_configure_loglevel(k.clnt, base.str() + "configure_loglevel");
    }
}

//! construction
/*!
 * \param modname module name
 * \param name instance name
 */
module_base::module_base(const std::string& modname, const std::string& name, 
        const YAML::Node& node) : modname(modname), name(name) {
    state = module_state_init;
    kernel& k = *kernel::get_instance();
    ll = k.get_loglevel();
	
    if (k.clnt) {
        stringstream base;
        base << k.clnt->name << "." << name << ".";

        register_configure_loglevel(k.clnt, base.str() + "configure_loglevel");
    }

    // search for loglevel
    if (node["loglevel"]) {
        ll = info;
        std::string ll_string = get_as<std::string>(node, "loglevel");

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

module_base::~module_base() {
    unregister_configure_loglevel();
}

//! log to kernel logging facility
void module_base::log(loglevel lvl, const char *format, ...) {
    kernel& k = *kernel::get_instance();
    struct log_thread::log_pool_object *obj;

    char buf[1024];
    int bufpos = 0;
    bufpos += snprintf(buf+bufpos, sizeof(buf)-bufpos, "[%s|%s] ", 
            name.c_str(), modname.c_str());

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

//! configure loglevel service
int module_base::on_configure_loglevel(ln::service_request& req,
        ln_service_robotkernel_module_configure_loglevel& svc) {
    
    svc.resp.current_log_level = strdup(((std::string)ll).c_str());
    svc.resp.current_log_level_len = strlen(svc.resp.current_log_level);

    if (svc.req.set_log_level_len > 0) {
        std::string set_log_level(svc.req.set_log_level, svc.req.set_log_level_len);

        try {
            ll = set_log_level;
        } catch (const std::exception& e) {
            svc.resp.error_message = strdup(e.what());
            svc.resp.error_message_len = strlen(svc.resp.error_message);
        }
    }

    req.respond();

    if (svc.resp.error_message)
        free(svc.resp.error_message);

    free(svc.resp.current_log_level);
    return 0;
}

