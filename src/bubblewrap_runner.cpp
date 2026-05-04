//! robotkernel bubblewrap runner
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
#include <robotkernel/bubblewrap_runner.h>
#include "robotkernel/config.h"
#include "robotkernel/helpers.h"

using namespace std;
using namespace robotkernel;
using namespace robotkernel::helpers;

// Run the program with bubblewrap
bubblewrap_runner::bubblewrap_runner(const std::string& name,
        const std::string& rootfs) :
process_runner(name),
    rootfs_dir(rootfs), 
    program_path(""), 
    bind_mounts("")
{
    // Create root directory
    if (!helpers::filesystem::exists(rootfs_dir)) {
        helpers::filesystem::create_directories(rootfs_dir);
    }

    // Create essential directories
    std::vector<std::string> dirs = {
        rootfs_dir + "/bin",
        rootfs_dir + "/lib",
        rootfs_dir + "/lib64",
        rootfs_dir + "/tmp",
        rootfs_dir + "/dev",
        rootfs_dir + "/proc",
        rootfs_dir + "/sys"
    };

    for (const auto& dir : dirs) {
        if (!helpers::filesystem::exists(dir)) {
            helpers::filesystem::create_directories(dir);
        }
    }

    log(info, "Created minimal root filesystem at: %s\n", rootfs_dir.c_str());
}

void bubblewrap_runner::start() {
    if (program_path.empty()) {
        std::cerr << "No program set to run\n";
    }

    // Build the bubblewrap command
    std::string command = "bwrap --ro-bind / " + rootfs_dir + " ";

    // Add bind mounts
    command += bind_mounts;

    // Add essential mounts
    command += "--ro-bind /sys /sys --proc /proc --dev /dev --tmpfs /tmp --tmpfs /run --tmpfs /var/tmp ";

    // Add the program
    command += " -- " + program_path;

    // Add arguments
    for (const auto& arg : args) {
        command += " " + arg;
    }

    process_runner::set_command(command);
    process_runner::start();
}

// Clean up
void bubblewrap_runner::stop() {
    process_runner::stop();

    log(info, "Cleaning up root filesystem at: %s\n", rootfs_dir.c_str());
    robotkernel::helpers::filesystem::remove_directories(rootfs_dir);
}
