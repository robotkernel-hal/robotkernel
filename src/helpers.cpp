//! robotkernel helpers definition
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
#include "robotkernel/helpers.h"
#include "robotkernel/config.h"

// private headers
#include "kernel.h"

#ifdef __VXWORKS__
#include <vxWorks.h>
#include <taskLib.h>
#endif

#include <regex>
#include <string>
#include <cstdarg>
#include <vector>
#include <cstdio>
#include <sstream>
#include <iostream>

using namespace std;
using namespace robotkernel;

YAML::Node robotkernel::fill_template(const std::string& config, const YAML::Node& instance) {
    std::string inst_config = config;
    
    for (const auto& kv : instance) {
        string var = string("(\\$") + kv.first.as<string>() + string(")");

        std::string result;
        std::regex e (var);
        std::regex_replace(std::back_inserter(result), inst_config.begin(), inst_config.end(), 
                e, kv.second.as<string>());
        inst_config = result;
    }

    return YAML::Load(inst_config);
}

void robotkernel::parse_templates(const YAML::Node& config, std::list<YAML::Node>& instances) {
     std::map<std::string, std::string> class_map;
 
     for (const auto& cls : config["classes"]) {
         auto class_name = cls.first.as<string>();
 
         YAML::Emitter out;
         out << cls.second;
 
         auto class_config = out.c_str();
         class_map[class_name] = class_config;
     }
 
     for (const auto& inst : config["instances"]) {
         if (!inst["use_class"]) {
             instances.push_back(inst);
             continue;
         }
 
         auto class_name = get_as<string>(inst, "use_class");
         std::string inst_config = class_map[class_name];
         instances.push_back(fill_template(inst_config, inst));
     }
}

std::string robotkernel::string_printf(const char* format, ...) {
    va_list args1;
    va_start(args1, format);

    // Copy args to get the size first
    va_list args2;
    va_copy(args2, args1);
    int size = std::vsnprintf(nullptr, 0, format, args2);
    va_end(args2);

    if (size < 0) {
        va_end(args1);
        throw std::runtime_error("string_printf: vsnprintf encoding error");
    }

    std::vector<char> buffer(size + 1);
    std::vsnprintf(buffer.data(), buffer.size(), format, args1);
    va_end(args1);

    return std::string(buffer.data(), size);
}

std::vector<std::string> robotkernel::string_split(const std::string& str, const char delimiter) {
    std::vector<std::string> result;
    std::istringstream ss(str);
    std::string token;

    while (std::getline(ss, token, delimiter)) {
        result.push_back(token);
    }

    return result;
}

void robotkernel::set_priority(int priority, int policy) {
    if (!priority)
        return;
    struct sched_param param;
    robotkernel::kernel::instance.log(info, "setting thread priority to %d, policy %d\n", priority, policy);

    param.sched_priority = priority;
    if (pthread_setschedparam(pthread_self(), policy, &param) != 0)
        robotkernel::kernel::instance.log(warning, "setPriority: pthread_setschedparam(0x%x, %d, %d): %s\n",
                pthread_self(), policy, priority, strerror(errno));
}

void robotkernel::set_affinity_mask(int affinity_mask) {
    if (!affinity_mask)
        return;
#ifdef __VXWORKS__
    taskCpuAffinitySet(taskIdSelf(), (cpuset_t) affinity_mask);
#elif defined __QNX__
    ThreadCtl(_NTO_TCTL_RUNMASK, (void *) affinity_mask);
#else
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    for (unsigned i = 0; i < (sizeof(affinity_mask) * 8); ++i)
        if (affinity_mask & (1 << i))
            CPU_SET(i, &cpuset);

    robotkernel::kernel::instance.log(info, "setting cpu affinity mask %#x\n", affinity_mask);

    int ret = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    if (ret != 0)
        robotkernel::kernel::instance.log(warning, "setAffinityMask: pthread_setaffinity(%p, %#x): %d %s\n", 
                (void *) pthread_self(), affinity_mask, ret, strerror(ret));
#endif
}

void robotkernel::set_thread_name(std::thread& tid, const std::string& thread_name) {
    set_thread_name(tid.native_handle(), thread_name);
}

void robotkernel::set_thread_name(pthread_t tid, const std::string& thread_name) {
    char buffer[17];
    snprintf(buffer, 16, "%s", thread_name.c_str());

#if defined(HAVE_PTHREAD_SETNAME_NP_3)
    pthread_setname_np(tid, buffer, (void *)0);
#elif defined(HAVE_PTHREAD_SETNAME_NP_2)
    pthread_setname_np(tid, buffer);
#endif
}

void robotkernel::set_thread_name(const std::string& thread_name) {
#if defined(HAVE_PTHREAD_SETNAME_NP_3) || defined(HAVE_PTHREAD_SETNAME_NP_2)
    set_thread_name(pthread_self(), thread_name);
#elif defined(HAVE_PTHREAD_SETNAME_NP_1)
    char buffer[17];
    snprintf(buffer, 16, "%s", thread_name.c_str());

    pthread_setname_np(buffer);
#endif
}

