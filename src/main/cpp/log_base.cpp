//! robotkernel loglevel base
/*!
 * (C) Robert Burger <robert.burger@dlr.de>
 */

// vim: set expandtab softtabstop=4 shiftwidth=4
// -*- mode: c++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil; -*- 

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

namespace robotkernel {
const std::string log_base::service_definition_configure_loglevel = services::robotkernel_log_base_configure_loglevel_service_definition;
};

//! construction
/*!
 * \param node config node
 */
log_base::log_base(const std::string& name, const std::string& impl, 
        const std::string& service_prefix, const YAML::Node& node) :
    name(name), impl(impl), service_prefix(service_prefix)
{
    ll = pkernel->get_loglevel();

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

    pkernel->add_service(name, service_name, log_base::service_definition_configure_loglevel,
            std::bind(&log_base::service_configure_loglevel, this, _1, _2));
}

//! destruction
log_base::~log_base() {
    std::string service_name = "configure_loglevel";
    if (service_prefix != "") 
        service_name = service_prefix + "." + service_name;

    pkernel->remove_service(name, service_name);
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

//! log to kernel logging facility
void log_base::log(loglevel lvl, const char *format, ...) {
    struct log_thread::log_pool_object *obj;

    if ((obj = pkernel->rk_log.get_pool_object()) != NULL) {
        // only ifempty log pool avaliable!
        obj->lvl = lvl;
        int bufpos = snprintf(obj->buf+bufpos, sizeof(obj->buf)-bufpos, "[%s|%s] ", 
            name.c_str(), impl.c_str());

        // format argument list    
        va_list args;
        va_start(args, format);
        vsnprintf(obj->buf+bufpos, sizeof(obj->buf)-bufpos, format, args);
        va_end(args);
    
        if (pkernel->do_log_to_trace_fd()) {
            pkernel->trace_write(obj->buf);
        }

#if (HAVE_LTTNG_UST == 1)
        if (pkernel->log_to_lttng_ust) {
            tracepoint(robotkernel, lttng_log, obj->buf);
        }
#endif

        dump_log(obj->buf);

        if (lvl > ll) {
            pkernel->rk_log.return_pool_object(obj);
        } else {
            pkernel->rk_log.log(obj);
        }
    }
}

