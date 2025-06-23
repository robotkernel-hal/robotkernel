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

// public headers
#include "robotkernel/helpers.h"
#include "robotkernel/config.h"

// private headers
#include "kernel.h"
#include "so_file.h"

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

string searchFile(string file_name, vector<const char*> &locations){
    for(auto libpathes : locations){
        if(!libpathes)
            continue;
        vector<string> split = split_string(libpathes, ":");
        for (auto & el : split) {
            el.append("/").append(file_name);
            if (access(el.c_str(), R_OK) != -1) {
                return el;
            }
        }
    }
    return file_name;
}


//! so_file construction
/*!
  \param node configuration node
  */
so_file::so_file(const YAML::Node& node) : config("") {
    file_name = get_as<string>(node, "so_file");
    
    if (node["config_file"]) {
        kernel::instance.log(warning, "entry 'config_file' of so_file %s is deprecated !!! It will be removed in "
                "future versions. Use 'config: !include config_file.rkc' instead.\n", 
                file_name.c_str());

        string config_file_name = get_as<string>(node, "config_file");
        // check for absolute/relative path
        if (config_file_name[0] != '/') {
            // relative path to config file
            config_file_name = kernel::instance.config_file_path + "/" + config_file_name;
        }

        kernel::instance.log(verbose, "so_file %s config file \"%s\"\n", 
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

    if (file_name.c_str()[0] != '/') {
        vector<const char*> locations{
                ".",                                // local dir
                kernel::instance.config_file_path.c_str(),         // relative to config file
                getenv("ROBOTKERNEL_LIBRARY_PATH"), // search environment
                getenv("LD_LIBRARY_PATH"),          // dlopen respects this, but we want to ensure a load order
                kernel::instance._internal_modpath.c_str(),
        };
        file_name = searchFile(file_name, locations);
    }

#ifndef __VXWORKS__
    if((so_handle = dlopen(file_name.c_str(), RTLD_LOCAL | RTLD_NOW | RTLD_DEEPBIND | RTLD_NOLOAD)))
        return; //already loaded
#endif
    if (!(so_handle = dlopen(file_name.c_str(), RTLD_LOCAL | RTLD_NOW | RTLD_DEEPBIND | RTLD_NODELETE)))
        throw str_exception("%s dlopen signaled error opening so_file: %s\nCheck if file path is in ROBOTKERNEL_MODULE_PATH or LD_LIBRARY_PATH\n", file_name.c_str(), dlerror());
}

//! so_file destruction
/*!
  destroys so_file
  */
so_file::~so_file() {
    if (so_handle && !kernel::instance._do_not_unload_modules) {
        kernel::instance.log(verbose, "unloading so_file %s\n", file_name.c_str());

        if (dlclose(so_handle) != 0)
            kernel::instance.log(error, "error on unloading so_file %s\n", file_name.c_str());
        else
            so_handle = NULL;
    }
}

