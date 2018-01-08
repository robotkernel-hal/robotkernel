//! robotkernel so_file class
/*!
 * (C) Robert Burger <robert.burger@dlr.de>
 */

// vim: set expandtab softtabstop=4 shiftwidth=4
// -*- mode: c++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil; -*- 

/*
 * This file is part of robotkernel.
 *
 * robotkernel is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * robotkernel is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with robotkernel.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "robotkernel/kernel.h"
#include "robotkernel/so_file.h"
#include "robotkernel/helpers.h"
#include "robotkernel/config.h"

#include "string_util/string_util.h"

#include <sys/stat.h>
#include <sys/types.h>
#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif
#include <dlfcn.h>
#include <dirent.h>
#include <fstream>

#ifdef __VXWORKS__
#include <vxWorks.h>
#include <taskLib.h>
#endif

using namespace std;
using namespace robotkernel;
using namespace string_util;

//! so_file construction
/*!
  \param node configuration node
  */
so_file::so_file(const YAML::Node& node) {
    struct stat buf;
    file_name = get_as<string>(node, "so_file");
    kernel& k = *kernel::get_instance();
    
    if (node["config_file"]) {
        string config_file_name = get_as<string>(node, "config_file");
        // check for absolute/relative path
        if (config_file_name[0] != '/') {
            // relative path to config file
            config_file_name = k.config_file_path + "/" + config_file_name;
        }

        klog(info, "so_file %s config file \"%s\"\n", 
                file_name.c_str(), config_file_name.c_str());

        ifstream t(config_file_name.c_str());
        if(t.fail()) // check failbit
            throw str_exception("could not open config file of so_file %s: %s",
                                file_name.c_str(), config_file_name.c_str());
        stringstream buffer;
        buffer << t.rdbuf();
        config = buffer.str();
    } else if (node["config"]) {
        YAML::Emitter t;
        t << node["config"];
        config = string(t.c_str());
    }

    if ((file_name.c_str()[0] != '/') || (stat(file_name.c_str(), &buf) != 0)) {
        // try path relative to config file first
        string tmp_fn = k.config_file_path + "/" + file_name;
        if (stat(tmp_fn.c_str(), &buf) != 0) {
            bool found = false;

            // no in relative path, try enviroment search path
            char *libpathes = getenv("ROBOTKERNEL_LIBRARY_PATH");
            if (libpathes) {
                char *pch, *str = strdup(libpathes);
                pch = strtok(str, ":");
                while (pch != NULL) {
                    tmp_fn = string(pch) + "/" + file_name;
                    if (stat(tmp_fn.c_str(), &buf) == 0) {
                        file_name = tmp_fn;
                        found = true;
                        break;
                    }
                    pch = strtok(NULL, ":");
                }

                free(str);
            }

            if (!found) {
                // try path from robotkernel release
                string tmp_fn = kernel::get_instance()->_internal_modpath + "/" + file_name;
                if (stat(tmp_fn.c_str(), &buf) == 0) {
                    file_name = tmp_fn;
                    found = true;
                }
            }
        } else
            file_name = tmp_fn;
    }

#ifdef __VXWORKS__
    klog(info, "loading so_file %s\n", file_name.c_str());
    so_handle = dlopen(file_name.c_str(), RTLD_GLOBAL |
            RTLD_NOW);
#else
    if ((so_handle = dlopen(file_name.c_str(), RTLD_GLOBAL | RTLD_NOW |
                    RTLD_NOLOAD)) == NULL) {
        klog(info, "loading so_file %s\n", file_name.c_str());

        if (access(file_name.c_str(), R_OK) != 0) {
            throw str_exception("%s so_file file name not given as absolute filename, either set\n"
                    "         ROBOTKERNEL_LIBRARY_PATH environment variable or specify absolut path!\n",
                    file_name.c_str());
        }

        char dirname_buffer[1024];
        strcpy(dirname_buffer, file_name.c_str());
        char* dir = dirname(dirname_buffer);
        DIR* dirp = opendir(dir);
        if(dirp) {
            struct dirent* de;
            while((de = readdir(dirp))) {
                // klog(error, "[%s] dir entry: %s\n", name.c_str(), de->d_name);
            }
            closedir(dirp);
        }

        so_handle = dlopen(file_name.c_str(), RTLD_GLOBAL |
                RTLD_NOW);
    }
#endif

    if (!so_handle) {
        throw str_exception("%s dlopen signaled error opening so_file: %s\n", 
                file_name.c_str(), dlerror());
    }
}

//! so_file destruction
/*!
  destroys so_file
  */
so_file::~so_file() {
    if (so_handle && !kernel::get_instance()->_do_not_unload_modules) {
        klog(verbose, "unloading so_file %s\n", file_name.c_str());

        if (dlclose(so_handle) != 0)
            klog(error, "error on unloading so_file %s\n", file_name.c_str());
        else
            so_handle = NULL;
    }
}

