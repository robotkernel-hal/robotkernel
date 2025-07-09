//! robotkernel loglevel base
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

#include <string_util/string_util.h>

// public headers
#include "robotkernel/log_base.h"
#include "robotkernel/config.h"
#include "robotkernel/service_definitions.h"

// private headers
#include "kernel.h"

#if (HAVE_LTTNG_UST == 1)
#define TRACEPOINT_CREATE_PROBES
#define TRACEPOINT_DEFINE

#include "lttng_tp.h"
#endif

using namespace std;
using namespace std::placeholders;
using namespace robotkernel;
using namespace string_util;

//! construction
/*!
 * \param[in]   ll          Loglevel to set.
 */
log_base::log_base(loglevel ll) :
    ll(ll), name("robotkernel"), impl("robotkernel"), service_prefix("")
{
    //add_svc_configure_loglevel(name, "configure_loglevel");
}

//! construction
/*!
 * \param node config node
 */
log_base::log_base(const std::string& name, const std::string& impl, 
        const std::string& service_prefix, const YAML::Node& node) :
    name(name), impl(impl), service_prefix(service_prefix)
{
    ll = robotkernel::kernel::instance.get_loglevel();

    // search for loglevel
    if (node.IsDefined() && !node.IsNull()) {
        if (!node.IsMap())
            throw str_exception("module `config` needs to be a yaml-mapping! -- you provided:\n```yaml\n%s\n```",
                                YAML::Dump(node).c_str());
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

    std::string service_name = "configure_loglevel";
    if (service_prefix != "") 
        service_name = service_prefix + "." + service_name;

    add_svc_configure_loglevel(name, service_name);
}

//! destruction
log_base::~log_base() {
    remove_svc_configure_loglevel();
}

//! svc_configure_loglevel
/*!
 * \param[in]   req     Service request data.
 * \param[out]  resp    Service response data.
 */
void log_base::svc_configure_loglevel(
        const struct services::robotkernel::log_base::svc_req_configure_loglevel& req, 
        struct services::robotkernel::log_base::svc_resp_configure_loglevel& resp) 
{
    resp.current_loglevel = ll;
    ll = req.set_loglevel;
    resp.error_message = "";
}

//! log to kernel logging facility
void log_base::log(loglevel lvl, const char *format, ...) {
    struct log_thread::log_pool_object *obj;

    if ((obj = robotkernel::kernel::instance.rk_log.get_pool_object()) != NULL) {
        // only ifempty log pool avaliable!
        obj->lvl = lvl;
        int bufpos = snprintf(obj->buf+bufpos, sizeof(obj->buf)-bufpos, "[%s|%s] ", 
            name.c_str(), impl.c_str());

        // format argument list    
        va_list args;
        va_start(args, format);
        bufpos += vsnprintf(obj->buf+bufpos, sizeof(obj->buf)-bufpos, format, args);
        va_end(args);
    
        obj->len = bufpos;

        if (robotkernel::kernel::instance.do_log_to_trace_fd()) {
            robotkernel::kernel::instance.trace_write(obj);
        }

#if (HAVE_LTTNG_UST == 1)
        if (robotkernel::kernel::instance.log_to_lttng_ust) {
            tracepoint(robotkernel, lttng_log, obj->buf);
        }
#endif

        dump_log(obj->buf);

        if (lvl > ll) {
            robotkernel::kernel::instance.rk_log.return_pool_object(obj);
        } else {
            robotkernel::kernel::instance.rk_log.log(obj);
        }
    }
}

