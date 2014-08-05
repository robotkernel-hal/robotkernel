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

#define ROBOTKERNEL "[robotkernel] "

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
    klog(info, ROBOTKERNEL "usage: robotkernel [--config, -c <filename>] "
            "[--quiet, -q] [--verbose, -v] [--help, -h]\n");
    klog(info, ROBOTKERNEL "\n");
    klog(info, ROBOTKERNEL "  --config, -c <filename>     specify config file\n");
    klog(info, ROBOTKERNEL "  --quiet, -q                 run in quiet mode\n");
    klog(info, ROBOTKERNEL "  --verbose, -v               be more verbose\n");
    klog(info, ROBOTKERNEL "  --help, -h                  this help page\n");
    return 0;
}

int main(int argc, char** argv) {
    klog(info, ROBOTKERNEL "build by: " BUILD_USER "@" BUILD_HOST "\n");
    klog(info, ROBOTKERNEL "build date: " BUILD_DATE "\n");
    klog(info, ROBOTKERNEL "links_and_nodes: " LN_LIBS "\n");

    string config_file = "";
    loglevel level = info;
    struct sigaction action;
    kernel &k = *kernel::get_instance();

    for (int i = 1; i < argc; ++i) {
        if ((strcmp(argv[i], "--config") == 0) || (strcmp(argv[i], "-c") == 0)) {
            if (++i >= argc) {
                klog(info, ROBOTKERNEL "--config filename missing\n");
                return usage(argc, argv);
            }

            config_file = string(argv[i]);
        } else if ((strcmp(argv[i], "--quiet") == 0) || (strcmp(argv[i], "-q") == 0))
            level = error;
        else if ((strcmp(argv[i], "--verbose") == 0) || (strcmp(argv[i], "-v") == 0))
            level = verbose;
        else if ((strcmp(argv[i], "--help") == 0) || (strcmp(argv[i], "-h") == 0))
            return usage(argc, argv);
    }

    if (config_file == "")
        klog(info, ROBOTKERNEL "no config file supplied, starting up without config.\n");

    k._ll = level;
    bool power_up_state = false;

    try {
        k.config(config_file, argc, argv);        
        power_up_state = k.power_up();
    } catch (exception& e) {
        klog(warning, ROBOTKERNEL "exception: %s\n", e.what());
        goto Exit;
    }

    if (power_up_state)
        klog(info, ROBOTKERNEL "up and running!\n");
    else
        klog(info, ROBOTKERNEL "not powered up!\n");
    
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
    klog(info, ROBOTKERNEL "exiting\n");
    
#ifdef __VXWORKS__
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

