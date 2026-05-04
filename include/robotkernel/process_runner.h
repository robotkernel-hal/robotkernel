//! robotkernel process_runner
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

#ifndef ROBOTKERNEL__PROCESS_RUNNER_H
#define ROBOTKERNEL__PROCESS_RUNNER_H

#include <string>
#include <thread>

#include <yaml-cpp/yaml.h>

#include <robotkernel/runnable.h>
#include <robotkernel/log_base.h>

namespace robotkernel {

//! process runner class
/*!
 */
class process_runner :
    public robotkernel::runnable,
    public robotkernel::log_base
{
    public:
        std::string command;
        pid_t child_pid;
        int stdout_pipe[2], stderr_pipe[2];

    public:
        // construction
        process_runner(const std::string& name) :
            robotkernel::log_base("process_runner", name, "")
        {}

        virtual ~process_runner() { stop(); }

        void set_command(const std::string& command) { 
            this->command = command; 
        }
        
        // run thread, calls execlp
        virtual void run() override;

        // doing cleanup
        virtual void stop() override;
};


} // namespace robotkernel

#endif // ROBOTKERNEL__PROCESS_RUNNER_H

