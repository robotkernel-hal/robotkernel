//! robotkernel helpers definition
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

#include "robotkernel/helpers.h"

#include <regex>

using namespace std;
using namespace robotkernel;

YAML::Node fill_template(const std::string& config, const YAML::Node& instance) {
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

void parse_templates(const YAML::Node& config, std::list<YAML::Node>& instances) {
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
