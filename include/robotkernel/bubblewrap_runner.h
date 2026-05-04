//! robotkernel bubblewrap_runner
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

#ifndef ROBOTKERNEL__BUBBLEWRAP_RUNNER_H
#define ROBOTKERNEL__BUBBLEWRAP_RUNNER_H

#include <string>
#include <thread>

#include <yaml-cpp/yaml.h>

#include <robotkernel/process_runner.h>

namespace robotkernel {

class bubblewrap_runner :
    public robotkernel::process_runner
{
    private:
        std::string rootfs_dir;
        std::string program_path;
        std::vector<std::string> args;
        std::string bind_mounts;
        std::string ro_bind_mounts;

    public:
        // construction
        bubblewrap_runner(const std::string& name, const std::string& rootfs = "/tmp/bubblewrap_root");

        // destruction
        virtual ~bubblewrap_runner() { stop(); }

        // Add bind mounts (e.g., to share data with host)
        void add_bind_mount(
                const std::string& host_path, 
                const std::string& container_path);

        // Add bind mounts (e.g., to share data with host)
        void add_ro_bind_mount(
                const std::string& host_path, 
                const std::string& container_path);

        // Set the program to run
        void set_program(
                const std::string& path, 
                const std::vector<std::string>& program_args = {});

        virtual void start() override;

        // Clean up
        virtual void stop() override;
};

// Add bind mounts (e.g., to share data with host)
inline void bubblewrap_runner::add_bind_mount(
        const std::string& host_path, 
        const std::string& container_path) 
{
    bind_mounts += " --bind " + host_path + " " + container_path + " ";
}

// Add bind mounts (e.g., to share data with host)
inline void bubblewrap_runner::add_ro_bind_mount(
        const std::string& host_path, 
        const std::string& container_path) 
{
    bind_mounts += " --ro-bind " + host_path + " " + container_path + " ";
}

// Set the program to run
inline void bubblewrap_runner::set_program(
        const std::string& path, 
        const std::vector<std::string>& program_args) 
{
    program_path = path;
    args = program_args;
}

} // namespace robotkernel

#endif // ROBOTKERNEL__BUBBLEWRAP_RUNNER_H


