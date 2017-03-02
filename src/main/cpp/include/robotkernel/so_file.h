//! robotkernel so_file class
/*!
 * author: Robert Burger
 *
 * $Id$
 */

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
 * along with robotkernel.	If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __ROBOTKERNEL_SO_FILE_H__
#define __ROBOTKERNEL_SO_FILE_H__

#include <string>
#include <stdio.h>
#include "yaml-cpp/yaml.h"

namespace robotkernel {

	//! so_file class
	/*!
	  This class opens a shared so_file and loads all needed symbols
	  */
	class so_file {
		private:
			so_file();
			so_file(const so_file&);				// prevent copy-construction
			so_file& operator=(const so_file&);		// prevent assignment

		public:
			//! so_file construction
			/*!
			  \param node configuration node
			  */
			so_file(const YAML::Node& node);

			//! so_file destruction
			/*!
			  destroys so_file
			  */
			~so_file();

			std::string file_name;			//!< so_file shared object file name
			std::string config;				//!< config passed to so_file init

		protected:
			void* so_handle;				//!< dlopen handle
	};

} // namespace robotkernel

#endif // __ROBOTKERNEL_SO_FILE_H__

