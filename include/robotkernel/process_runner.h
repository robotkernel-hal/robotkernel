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
        std::string command = "";           //!< @brief Command to be executed.
        pid_t child_pid = 0;                //!< @brief Child PID.
        int stdout_pipe[2], stderr_pipe[2]; //!< @brief Pipes for stdout, stderr.

    public:
        /**
         * @brief Construction
         */
        process_runner(const std::string& name) :
            robotkernel::log_base("process_runner", name, "")
        {}

        /**
         * @brief Destruction 
         */
        virtual ~process_runner() { stop(); }

        /**
         * @brief Set command to be execeuted with execlp.
         *
         * @param command
         *        Command to be executed.
         */
        void set_command(const std::string& command);
        
        /**
         * @brief Run thread, calls execlp to execute command process.
         */
        virtual void run() override;

        /**
         * @brief Doing cleanup
         */
        virtual void stop() override;
};

/**
 * @brief Set command to be execeuted with execlp.
 *
 * @param command
 *        Command to be executed.
 */
inline void process_runner::set_command(const std::string& command) { 
    this->command = command; 
}
        

} // namespace robotkernel

#endif // ROBOTKERNEL__PROCESS_RUNNER_H

