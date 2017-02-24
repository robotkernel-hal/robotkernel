//
// Created by crem_ja on 2/24/17.
//

#ifndef PROJECT_SERVICE_PROVIDER_H
#define PROJECT_SERVICE_PROVIDER_H

#include <string>
#include "yaml-cpp/yaml.h"
#include "robotkernel/kernel.h"

namespace robotkernel {
    class ServiceProvider {
    public:
        ServiceProvider() {}

        virtual ~ServiceProvider() {}

        virtual void init(const YAML::Node &node, void *interface) = 0;

        virtual void log(robotkernel::loglevel lvl, const char *format, ...) = 0;

        virtual int service_configure_loglevel(const robotkernel::service_arglist_t &request,
                                               robotkernel::service_arglist_t &response) = 0;
    };
}

#endif //PROJECT_SERVICE_PROVIDER_H
