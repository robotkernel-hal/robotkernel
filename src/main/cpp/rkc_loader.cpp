#include <sys/stat.h>
#include "rkc_loader.h"
#include "robotkernel/kernel.h"
#include "string_util/string_util.h"

using namespace string_util;
using namespace robotkernel;
using namespace std;

static std::map<std::string, std::string> arg_map;

namespace robotkernel {
void split_file_name(const string& str, string& path, string& file);
}

void parse_node(YAML::Node e, const std::string& config_file_path) {
    kernel& k = *kernel::get_instance();

    switch (e.Type()) {
        case YAML::NodeType::Undefined:
            k.log(verbose, "got NodeType: Undefined\n");
            break;
        case YAML::NodeType::Null:
            k.log(verbose, "got NodeType: Null\n");
            break;
        case YAML::NodeType::Scalar:
            k.log(verbose, "got NodeType: Scalar\n");

            if (e.Tag() == "!include") {
                string fn = e.as<string>();
                k.log(verbose, "got !include tag: %s\n", fn.c_str());
                
                // check for absolute/relative path
                if (fn[0] != '/') {
                    // relative path to config file
                    fn = config_file_path + "/" + fn;
                }

    
                char *real_config_file = realpath(fn.c_str(), NULL);
                if (!real_config_file)
                    throw str_exception("supplied config file \"%s\" not found!", fn.c_str());
                string file, new_config_file_path;
                split_file_name(string(real_config_file), new_config_file_path, file);
                
                klog(verbose, "config file \"%s\"\n", fn.c_str());

                struct stat buffer;   
                int ret = stat(fn.c_str(), &buffer);
                if(ret != 0) { // check failbit
                    throw str_exception("could not open config file: %s", fn.c_str());
                }
                
                e = YAML::LoadFile(fn);
                parse_node(e, new_config_file_path);
            } else if (e.Tag() == "!env") {
                string env_var_name = e.as<string>();
                char* env_var = getenv(env_var_name.c_str());

                if (nullptr == env_var) {
                    throw str_exception("environment variable \"%s\" does not exist or is empty!", 
                            env_var_name.c_str());
                }

                e = YAML::Load(string(env_var));
                parse_node(e, config_file_path);
            } else if (e.Tag() == "!arg") {
                string arg_name = e.as<string>();
                if (arg_map.find(arg_name) == arg_map.end()) {
                    throw str_exception("no argument name \"%s\" provided to robotkernel!", arg_name.c_str());
                }

                e = YAML::Load(arg_map[arg_name]);
                parse_node(e, config_file_path);
            }
            break;
        case YAML::NodeType::Sequence:
            k.log(verbose, "got NodeType: Sequence\n");
            for (auto s : e) {
                k.log(verbose, "seq -> ");
                parse_node(s, config_file_path);
            }
            break;
        case YAML::NodeType::Map:
            k.log(verbose, "got NodeType: Map\n");

            for (auto kv : e) {
                k.log(verbose, "first -> ");
                parse_node(kv.first, config_file_path);
                k.log(verbose, "second -> ");
                parse_node(kv.second, config_file_path);
            }
            break;
    }
}

YAML::Node robotkernel::rkc_load_file(const std::string& filename) {
    kernel& k = *kernel::get_instance();
    k.log(verbose, "rkc_load_file %s\n", filename.c_str());
    
    for (int i = 1; i < k.main_argc; ++i) {
        if ((strncmp(k.main_argv[i], "--", 2) == 0) && ((i+1) < k.main_argc) && !(strncmp(k.main_argv[i+1], "-", 1) == 0)) {
            string key = k.main_argv[i++];
            string value = k.main_argv[i];
            key.erase(0, 2);
            arg_map[key] = value;
        }
    }

    YAML::Node node = YAML::LoadFile(filename);

    parse_node(node, k.config_file_path);

    YAML::Emitter emit;
    emit << node;
    k.log(verbose, "preproc config: %s\n", emit.c_str());

    return node;
}

