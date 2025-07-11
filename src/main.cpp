//! robotkernel main program
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

// public headers
#include "robotkernel/config.h"
#include "robotkernel/helpers.h"

// private headers
#include "kernel.h"
#include "module.h"

#include <stdio.h>
#include <string>
#include <string.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <stdlib.h>
#include <signal.h>
#include <stdint.h>
#include <math.h>
#include <map>
#include <pthread.h>
#include <sys/time.h>
#include <sys/resource.h>

using namespace std;
using namespace robotkernel;

bool sig_shutdown = false;
void signal_handler(int s) {
    switch (s) {
        default: 
            break;
        case SIGINT:
        case SIGTERM:
            printf("\n\n");
            sig_shutdown = true;
            break;
    }
}

int usage(int argc, char** argv) {
    robotkernel::kernel::instance.log(info, "usage: robotkernel --config | -c <filename> "
            "[--quiet, -q] [--verbose, -v] [--power_up, -p module=[boot,init,preop,safeop,op] "
            "[--help, -h]\n");
    robotkernel::kernel::instance.log(info, "\n");
    robotkernel::kernel::instance.log(info, "  --config, -c <filename>     specify config file\n");
    robotkernel::kernel::instance.log(info, "  --quiet, -q                 run in quiet mode\n");
    robotkernel::kernel::instance.log(info, "  --verbose, -v               be more verbose\n");
    robotkernel::kernel::instance.log(info, "  --help, -h                  this help page\n");
    robotkernel::kernel::instance.log(info, "  --test-run, -t              doing test run, load modules and quit\n");
    robotkernel::kernel::instance.log(info, "  --power_up, -p              powering up modules\n");
    return 0;
}

string service_datatype_to_ln(string datatype) {
    if (datatype == "string")
        return string("char*");

    return datatype;
}

std::pair<std::string, int> ln_datatype_size_data[] = {
    std::make_pair("int64_t", 8),
    std::make_pair("uint64_t", 8),
    std::make_pair("int32_t", 4),
    std::make_pair("uint32_t", 4),
    std::make_pair("int16_t", 2),
    std::make_pair("uint16_t", 2),
    std::make_pair("int8_t", 1),
    std::make_pair("uint8_t", 1),
    std::make_pair("char*", 1)
};

std::map<std::string, int> ln_datatype_size_map(
        ln_datatype_size_data,
        ln_datatype_size_data + sizeof(ln_datatype_size_data) / 
        sizeof(ln_datatype_size_data[0]));

int ln_datatype_size(const std::string& ln_datatype) {
    if (ln_datatype_size_map.find(ln_datatype) != 
                ln_datatype_size_map.end())
        return ln_datatype_size_map[ln_datatype];

    return 0;
}

int main(int argc, char* argv[]) {
    int ret = 0;
    bool power_up_state = false;
    bool test_run = false;

    sigset_t set;
    if (sigemptyset (&set) == -1)
        perror ("sigemptyset");

    int n;
    for (n = SIGRTMIN; n <= SIGRTMAX; ++n) {
        if (sigaddset (&set, n) == -1)
            perror ("sigaddset");
    }

    if (sigprocmask (SIG_BLOCK, &set, NULL) != 0)
        perror ("sigprocmask");
    if (pthread_sigmask (SIG_BLOCK, &set, NULL) != 0)
        perror ("sigprocmask");

#ifdef __linux__
    struct rlimit rlim;
    int ret2 = getrlimit(RLIMIT_RTPRIO, &rlim);

    if (ret2 == 0) {
        rlim.rlim_cur = rlim.rlim_max;
        setrlimit(RLIMIT_RTPRIO, &rlim);
    }
#endif

    string config_file = "";
    struct sigaction action;

    std::map<std::string, module_state_t>  power_up_map;

    for (int i = 1; i < argc; ++i) {
        if ((strcmp(argv[i], "--config") == 0) || (strcmp(argv[i], "-c") == 0)) {
            if (++i >= argc) {
                robotkernel::kernel::instance.log(info, "--config filename missing\n");
                usage(argc, argv);
                goto Exit;
            }

            config_file = string(argv[i]);

        } else if ((strcmp(argv[i], "--quiet") == 0) || (strcmp(argv[i], "-q") == 0))
            kernel::instance.set_loglevel(error);
        else if ((strcmp(argv[i], "--verbose") == 0) || (strcmp(argv[i], "-v") == 0))
            kernel::instance.set_loglevel(verbose);
        else if ((strcmp(argv[i], "--help") == 0) || (strcmp(argv[i], "-h") == 0))
            return usage(argc, argv);
        else if ((strcmp(argv[i], "--test-run") == 0) || (strcmp(argv[i], "-t") == 0))
            test_run = true;
        else if ((strcmp(argv[i], "--power_up") == 0) || (strcmp(argv[i], "-p") == 0)) {
            if (++i >= argc) {
                robotkernel::kernel::instance.log(info, "--power_up argument missing\n");
                usage(argc, argv);
                goto Exit;
            }

            string power_up_arg = string(argv[i]);
            vector<string> power_up_split = robotkernel::string_split(power_up_arg, '=');

            power_up_map[power_up_split[0]] = decode_power_up_state(power_up_split[1]);
        }
    }

    if (config_file == "") {
        usage(argc, argv);
        goto Exit;
    }

    try {
        kernel::instance.config(config_file, argc, argv);        

        for (const auto& kv : power_up_map) {
            auto mdl = kernel::instance.get_module(kv.first);
            mdl->set_power_up(kv.second);
        }

        power_up_state = kernel::instance.power_up();
    } catch (exception& e) {
        robotkernel::kernel::instance.log(error, "config exception: %s\n", e.what());
        ret = -1;
        goto Exit;
    }

    if (power_up_state)
        robotkernel::kernel::instance.log(info, "up and running!\n");
    else
        robotkernel::kernel::instance.log(info, "not powered up!\n");

    /* attach signal handler */
    action.sa_handler = signal_handler;
    sigemptyset (&action.sa_mask);
    action.sa_flags = 0;
    sigaction (SIGINT, &action, NULL);
    sigaction (SIGTERM, &action, NULL);
    sigaction (SIGABRT, &action, NULL);

    if (test_run)
        goto Exit;

    try {
        while (!sig_shutdown) {
            kernel::instance.state_check();

            struct timespec ts = {0, 500000000 };
            nanosleep(&ts, NULL);
        }
    } catch (exception& e) {
        printf("exception: %s\n", e.what());
        ret = -1;
    }

Exit:
    robotkernel::kernel::instance.log(info, "exiting\n");

#ifdef __VXWORKS__
    // on vxworks we have to call select once to do magic cleanup
//    if (kernel::instance.clnt)
//        kernel::instance.clnt->wait_for_service_requests(0);
#endif // __VXWORKS__

    return ret;
}

