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
#include <dirent.h>
#include <sys/stat.h>

using namespace std;
using namespace robotkernel;
using namespace robotkernel::helpers;

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/syscall.h>

/* Maximum length for thread names (Linux kernel limit is 16 bytes including null terminator) */
#define THREAD_NAME_LEN 16

/**
 * Retrieves the kernel thread ID (TID) for a given pthread_t.
 * This is required to construct the path /proc/self/task/<tid>/comm.
 *
 * Note: On Linux, pthread_t is not necessarily the TID.
 * This helper works reliably only for the calling thread (pthread_self)
 * without additional mapping structures.
 */
static pid_t get_tid(pthread_t thread) {
#if defined(__linux__)
    if (thread == pthread_self()) {
        return (pid_t)syscall(SYS_gettid);
    }
    /*
     * Fallback for other threads:
     * Without glibc helpers, we cannot safely determine the TID of a foreign pthread_t.
     * Return -1 to indicate that the /proc method is not viable for foreign threads
     * without an external TID mapping.
     */
    return -1;
#else
    return -1; /* Not Linux */
#endif
}

/**
 * Reads the thread name directly from the procfs.
 * Robust only for the calling thread unless an external TID mapping is available.
 */
static int get_name_procfs(char *name, size_t len) {
    char path[64];
    pid_t tid = syscall(SYS_gettid);
    int fd;
    ssize_t n;

    if (len == 0) return EINVAL;
    name[0] = '\0';

    snprintf(path, sizeof(path), "/proc/self/task/%d/comm", tid);

    fd = open(path, O_RDONLY);
    if (fd == -1) {
        return errno;
    }

    n = read(fd, name, len - 1);
    close(fd);

    if (n > 0) {
        /* Remove trailing newline if present */
        if (name[n-1] == '\n') {
            name[n-1] = '\0';
        } else {
            name[n] = '\0';
        }
        return 0;
    }

    return (n == 0) ? 0 : errno;
}

/**
 * Portable wrapper for pthread_getname_np.
 *
 * Returns: 0 on success, otherwise an error code.
 */
static int portable_pthread_getname(pthread_t thread, char *name, size_t len) {
    int rc = -1;

    if (len == 0) return EINVAL;
    name[0] = '\0';

    /* Attempt 1: Native glibc/macOS implementation (3 arguments: thread, buf, len) */
#if defined(HAVE_PTHREAD_GETNAME_NP_3) /* Standard Linux/glibc signature */
    rc = pthread_getname_np(thread, name, len);

    /* Attempt 2: macOS / BSD variants (usually also 3 arguments) */
#elif defined(HAVE_PTHREAD_GETNAME_NP) /* General check via Autoconf */
    rc = pthread_getname_np(thread, name, len);

    /* Attempt 3: Fallback to procfs (Linux only, safe only for current thread) */
#else
    /*
     * If the function was not found at compile time,
     * try the direct filesystem approach.
     * Note: This works robustly only for the current thread,
     * as resolving the TID of a foreign pthread_t is difficult without glibc.
     */
    if (thread == pthread_self()) {
        rc = get_name_procfs(name, len);
    } else {
        /* No way to handle foreign threads in pure userspace without a mapping table */
        rc = ENOTSUP;
    }
#endif

    /*
     * Additional safety check: Even if HAVE was defined, the function might be missing
     * at runtime on very old or non-standard systems (e.g., some musl configurations).
     * If rc remains -1, attempt the procfs fallback.
     */
    if (rc == -1) {
         if (thread == pthread_self()) {
            rc = get_name_procfs(name, len);
         } else {
            rc = ENOTSUP;
         }
    }

    return rc;
}

/**
 * Converts a scheduling policy integer to a human-readable string.
 * 
 * @param policy The scheduling policy integer (e.g., from pthread_getschedparam).
 * @return A constant string representing the policy name, or "UNKNOWN" if unrecognized.
 */
const char* decode_sched_policy(int policy) {
    switch (policy) {
        case SCHED_OTHER:
            return "SCHED_OTHER";
        case SCHED_FIFO:
            return "SCHED_FIFO";
        case SCHED_RR:
            return "SCHED_RR";
#ifdef SCHED_BATCH
        case SCHED_BATCH:
            return "SCHED_BATCH";
#endif
#ifdef SCHED_IDLE
        case SCHED_IDLE:
            return "SCHED_IDLE";
#endif
#ifdef SCHED_DEADLINE
        case SCHED_DEADLINE:
            return "SCHED_DEADLINE";
#endif
#ifdef SCHED_SPORADIC
        case SCHED_SPORADIC:
            return "SCHED_SPORADIC";
#endif
        default:
            return "UNKNOWN";
    }
}

//! convert buffer to hex string
std::string robotkernel::helpers::hex_string(const void *data, size_t len) {
    char hex_buf[4];
    std::stringstream ss;
    
    for (size_t i = 0; i < len; ++i) {
        uint8_t c = ((uint8_t*) data)[i];
        snprintf(hex_buf, sizeof(hex_buf), "%02X ", c);
        ss << hex_buf;
    }

    ss << "\n";

    return ss.str();
}

YAML::Node robotkernel::helpers::fill_template(const std::string& config, const YAML::Node& instance) {
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

void robotkernel::helpers::parse_templates(const YAML::Node& config, std::list<YAML::Node>& instances) {
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

std::string robotkernel::helpers::string_printf(const char* format, ...) {
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

std::vector<std::string> robotkernel::helpers::string_split(const std::string& str, const char delimiter) {
    std::vector<std::string> result;
    std::istringstream ss(str);
    std::string token;

    while (std::getline(ss, token, delimiter)) {
        result.push_back(token);
    }

    return result;
}

void robotkernel::helpers::set_priority(int priority, int policy) {
    if (!priority)
        return;

    char thread_name[17];
    int local_ret = portable_pthread_getname(pthread_self(), thread_name, 17);
    (void)local_ret;

    struct sched_param param;
    robotkernel::kernel::instance.log(info, "event=set_priority thread_name=\"%s\" priority=%d policy=%s\n", thread_name, priority, decode_sched_policy(policy));

    param.sched_priority = priority;
    if (pthread_setschedparam(pthread_self(), policy, &param) != 0) {
        throw runtime_error(robotkernel::helpers::string_printf(
                    "setPriority: pthread_setschedparam(%p, %d, %d): %s\n",
                    (void *) pthread_self(), policy, priority, strerror(errno)));
    }
}

void robotkernel::helpers::set_affinity_mask(int affinity_mask) {
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

    char thread_name[17];
    int local_ret = portable_pthread_getname(pthread_self(), thread_name, 17);
    (void)local_ret;

    robotkernel::kernel::instance.log(info, "event=set_affinity_mask thread_name=\"%s\" cpu_affinity_mask=%#x\n", thread_name, affinity_mask);

    int ret = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    if (ret != 0) {
        throw runtime_error(robotkernel::helpers::string_printf(
                    "setAffinityMask: pthread_setaffinity(%p, %#x): %d %s\n", 
                    (void *) pthread_self(), affinity_mask, ret, strerror(ret)));
    }
#endif
}

void robotkernel::helpers::set_thread_name(std::thread& tid, const std::string& thread_name) {
    set_thread_name(tid.native_handle(), thread_name);
}

void robotkernel::helpers::set_thread_name(pthread_t tid, const std::string& thread_name) {
    char buffer[17];
    snprintf(buffer, 16, "%s", thread_name.c_str());

#if defined(HAVE_PTHREAD_SETNAME_NP_3)
    pthread_setname_np(tid, buffer, (void *)0);
#elif defined(HAVE_PTHREAD_SETNAME_NP_2)
    pthread_setname_np(tid, buffer);
#endif
}

void robotkernel::helpers::set_thread_name(const std::string& thread_name) {
#if defined(HAVE_PTHREAD_SETNAME_NP_3) || defined(HAVE_PTHREAD_SETNAME_NP_2)
    set_thread_name(pthread_self(), thread_name);
#elif defined(HAVE_PTHREAD_SETNAME_NP_1)
    char buffer[17];
    snprintf(buffer, 16, "%s", thread_name.c_str());

    pthread_setname_np(buffer);
#endif
}

#ifdef _WIN32
// Use Windows version
#include <windows.h>

bool robotkernel::helpers::filesystem::exists(const std::string& path) {
    struct _stat buffer;
    return (_stat(path.c_str(), &buffer) == 0);
}

bool robotkernel::helpers::filesystem::create_directories(const std::string& path) {
    std::string dir = path;
    // Replace backslashes with forward slashes for consistency
    for (char& c : dir) {
        if (c == '\\') c = '/';
    }

    size_t pos = 0;
    std::string current_dir;

    while ((pos = dir.find('/', pos)) != std::string::npos) {
        current_dir += dir.substr(0, pos + 1);
        if (current_dir.length() > 0 && current_dir.back() == '/') {
            current_dir.pop_back();
        }

        if (_stat(current_dir.c_str(), &buffer) != 0) {
            if (_mkdir(current_dir.c_str()) != 0) {
                std::cerr << "Failed to create directory: " << current_dir << "\n";
                return false;
            }
        }
        pos++;
    }

    // Final directory
    if (_stat(dir.c_str(), &buffer) != 0) {
        if (_mkdir(dir.c_str()) != 0) {
            std::cerr << "Failed to create final directory: " << dir << "\n";
            return false;
        }
    }

    return true;
}
#else
// Use Unix version
#include <iostream>
#include <sys/stat.h>
#include <errno.h>

// Check if path exists
bool robotkernel::helpers::filesystem::exists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

// Create directories recursively
bool robotkernel::helpers::filesystem::create_directories(const std::string& path) {
    const char* p = path.c_str();
    struct stat buffer;
    std::string dir;

    // Skip leading slash
    if (*p == '/') {
        dir += '/';
        p++;
    }

    while (*p) {
        if (*p == '/') {
            dir += '/';
            if (stat(dir.c_str(), &buffer) != 0) {
                if (mkdir(dir.c_str(), 0755) != 0) {
                    std::cerr << "Failed to create directory: " << dir << "\n";
                    return false;
                }
            }
        }
        dir += *p++;
    }

    // Handle the final directory
    if (stat(dir.c_str(), &buffer) != 0) {
        if (mkdir(dir.c_str(), 0755) != 0) {
            std::cerr << "Failed to create final directory: " << dir << "\n";
            return false;
        }
    }

    return true;
}

/**
 * @brief Remove directories recursively
 *
 * @param[in] path
 *            Path to remove directory recursively.
 *
 * @return 
 *            True on success, false otherwise.
 */
bool robotkernel::helpers::filesystem::remove_directories(const std::string& path) {
    DIR* dir = opendir(path.c_str());
    if (!dir) {
        std::cerr << "Kann Verzeichnis nicht öffnen: " << path << "\n";
        return false;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (std::string(entry->d_name) == "." || std::string(entry->d_name) == "..")
            continue;

        std::string full_path = path + "/" + entry->d_name;

        struct stat st;
        if (stat(full_path.c_str(), &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                // Rekursiv Unterordner löschen
                if (!remove_directories(full_path)) {
                    closedir(dir);
                    return false;
                }
            } else {
                // Datei löschen
                if (unlink(full_path.c_str()) != 0) {
                    std::cerr << "Kann Datei nicht löschen: " << full_path << "\n";
                    closedir(dir);
                    return false;
                }
            }
        }
    }

    closedir(dir);

    // Jetzt das Verzeichnis selbst löschen
    if (rmdir(path.c_str()) != 0) {
        std::cerr << "Kann Verzeichnis nicht löschen: " << path << "\n";
        return false;
    }

    return true;
}

#endif
