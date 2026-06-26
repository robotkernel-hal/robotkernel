//! robotkernel runnable
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
#include <robotkernel/process_runner.h>
#include "robotkernel/config.h"
#include "robotkernel/helpers.h"

#include <sys/wait.h>

using namespace std;
using namespace robotkernel;
using namespace robotkernel::helpers;

// Run the program with bubblewrap
void process_runner::run() {
    if (    (pipe(stdout_pipe) == -1) ||
            (pipe(stderr_pipe) == -1) )
    { 
        throw std::runtime_error(
                robotkernel::helpers::string_printf("Error on pipe: %s\n", strerror(errno))); 
    }

    // Start the process
    child_pid = fork();

    if (child_pid == 0) {
        close(stdout_pipe[0]); // close in
        dup2(stdout_pipe[1], STDOUT_FILENO);
        close(stdout_pipe[1]); // close in
                               //
        close(stderr_pipe[0]); // close in
        dup2(stderr_pipe[1], STDERR_FILENO);
        close(stderr_pipe[1]); // close in

        // Execute
        printf("Executing %s\n", command.c_str()); // printf is fine here, runs through pipe
        execlp("sh", "sh", "-c", command.c_str(), nullptr);
        printf("Process exited.\n");
    } else {
        // Parent process
        log(info, "event=process_runner_run pid=%ld\n", child_pid);

        close(stdout_pipe[1]); // close in
        close(stderr_pipe[1]); // close in

        char buffer[1024];
        ssize_t n;

        int out_fd = stdout_pipe[0];
        int err_fd = stderr_pipe[0];

        // Pipe-Deskriptor in einen FILE-Stream umwandeln
        FILE *stream_o = fdopen(stdout_pipe[0], "r");
        FILE *stream_e = fdopen(stderr_pipe[0], "r");

        // Zeilenweise lesen bis EOF
        int counter = 1;

        fd_set read_fds;
        int max_fd = (stdout_pipe[0] > stderr_pipe[0]) ? stdout_pipe[0] :stderr_pipe[0];
        int open_pipes = 2;

        while (open_pipes > 0) {
            FD_ZERO(&read_fds);
            if (out_fd != -1) FD_SET(out_fd, &read_fds);
            if (err_fd != -1) FD_SET(err_fd, &read_fds);

            // Warten, bis eine der Pipes Daten hat
            if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) == -1) {
                throw std::runtime_error(
                        robotkernel::helpers::string_printf("Error on select: %s\n", strerror(errno)));
            }

            // reading process STDOUT
            if (out_fd != -1 && FD_ISSET(out_fd, &read_fds)) {
                if (fgets(buffer, sizeof(buffer), stream_o) == NULL) {
                    log(info, "event=process_runner_run message=\"Closing stdout pipe\"\n");
                    open_pipes--;
                } else {
                    log(info, "event=process_runner_run message=\"%s\"", buffer);
                }
            }

            // reading process STDERR
            if (err_fd != -1 && FD_ISSET(err_fd, &read_fds)) {
                if (fgets(buffer, sizeof(buffer), stream_e) == NULL) {
                    log(info, "event=process_runner_run message=\"Closing stderr pipe\"\n");
                    open_pipes--;
                } else {
                    log(error, "event=process_runner_run messge=\"%s\"", buffer);
                }
            }
        }
    }
}

void process_runner::stop() {
    std::chrono::seconds timeout(5);
    auto start = std::chrono::steady_clock::now();
    auto end = start + timeout;

    if (child_pid) {
        log(info, "event=process_runner_stop pid=%lu\n", child_pid);

        // Send SIGTERM first
        kill(child_pid, SIGTERM);

        int status;
        pid_t result = 0;

        while (
                ((result = waitpid(child_pid, &status, WNOHANG)) == 0) &&
                (std::chrono::steady_clock::now() < end) )
        {
            // Wait a bit for graceful shutdown polling
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        if (result == 0) {
            // Der Prozess existiert noch (ist noch nicht beendet)
            log(warning, "event=process_runner_stop message=\"Process didn't stop gracefully, sending SIGKILL\"\n");
            kill(child_pid, SIGKILL);
        }

        child_pid = 0;
    }

    runnable::stop();
}

