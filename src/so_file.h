//! robotkernel so_file class
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

#ifndef ROBOTKERNEL_SO_FILE_H
#define ROBOTKERNEL_SO_FILE_H

#include <string>
#include <stdio.h>
#include "yaml-cpp/yaml.h"

namespace robotkernel {
#ifdef EMACS
}
#endif

//! so_file class
/*!
 * This class opens a shared so_file and loads all needed symbols
 */
class so_file {
    private:
        so_file();
        so_file(const so_file&);                // prevent copy-construction
        so_file& operator=(const so_file&);     // prevent assignment

    public:
        //! so_file construction
        /*!
         * \param node configuration node
         */
        so_file(const YAML::Node& node);

        //! so_file destruction
        /*!
         * destroys so_file
         */
        ~so_file();

        std::string file_name;          //!< so_file shared object file name
        std::string config;             //!< config passed to so_file init

    protected:
        void* so_handle;                //!< dlopen handle
};

#ifdef EMACS
{
#endif
} // namespace robotkernel

#endif // ROBOTKERNEL_SO_FILE_H

