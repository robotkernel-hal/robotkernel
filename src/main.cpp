#include "robotkernel/config.h"
#include "robotkernel/kernel.h"
#include "robotkernel/module_intf.h"
#include "robotkernel/module.h"
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
    klog(info, "usage: robotkernel [--config, -c <filename>] "
            "[--quiet, -q] [--verbose, -v] [--help, -h]\n");
    klog(info, "\n");
    klog(info, "  --config, -c <filename>     specify config file\n");
    klog(info, "  --quiet, -q                 run in quiet mode\n");
    klog(info, "  --verbose, -v               be more verbose\n");
    klog(info, "  --help, -h                  this help page\n");
    return 0;
}

int main(int argc, char** argv) {

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
    int ret = getrlimit(RLIMIT_RTPRIO, &rlim);

    if (ret == 0) {
        rlim.rlim_cur = rlim.rlim_max;
        setrlimit(RLIMIT_RTPRIO, &rlim);
    }
#endif

    kernel &k = *kernel::get_instance();

    klog(info, "build by: " BUILD_USER "@" BUILD_HOST "\n");
    klog(info, "build date: " BUILD_DATE "\n");
    klog(info, "links_and_nodes: " LN_VERSION "\n");

    string config_file = "";
    struct sigaction action;

    for (int i = 1; i < argc; ++i) {
        if ((strcmp(argv[i], "--config") == 0) || (strcmp(argv[i], "-c") == 0)) {
            if (++i >= argc) {
                klog(info, "--config filename missing\n");
                return usage(argc, argv);
            }

            config_file = string(argv[i]);

        } else if ((strcmp(argv[i], "--quiet") == 0) || (strcmp(argv[i], "-q") == 0))
            k.set_loglevel(error);
        else if ((strcmp(argv[i], "--verbose") == 0) || (strcmp(argv[i], "-v") == 0))
            k.set_loglevel(verbose);
        else if ((strcmp(argv[i], "--help") == 0) || (strcmp(argv[i], "-h") == 0))
            return usage(argc, argv);
    }

    if (config_file == "")
        klog(info, "no config file supplied, starting up without config.\n");

    bool power_up_state = false;

    try {
        k.config(config_file, argc, argv);        
        power_up_state = k.power_up();
    } catch (exception& e) {
        klog(warning, "exception: %s\n", e.what());
        goto Exit;
    }

    if (power_up_state)
        klog(info, "up and running!\n");
    else
        klog(info, "not powered up!\n");

    /* attach signal handler */
    action.sa_handler = signal_handler;
    sigemptyset (&action.sa_mask);
    action.sa_flags = 0;
    sigaction (SIGINT, &action, NULL);
    sigaction (SIGTERM, &action, NULL);
    sigaction (SIGABRT, &action, NULL);

    try {
        while (!sig_shutdown) {
            k.handle_ln_request(argc, argv);
            k.state_check();
        }
    } catch (exception& e) {
        printf("exception: %s\n", e.what());
    }

Exit:
    klog(info, "exiting\n");

#ifdef __VXWORKS__
    // on vxworks we have to call select once to do magic cleanup
    if (k.clnt)
        k.clnt->wait_for_service_requests(0);
#endif // __VXWORKS__

    try {
        kernel::destroy_instance();
    } catch (exception& e) {
        printf("exception: %s\n", e.what());
    }

    printf("done ... returning now from main\n");
    return 0;
}

