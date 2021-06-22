#include "rkc_loader.h"
#include "robotkernel/kernel.h"

using namespace robotkernel;
using namespace std;

void parse_node(YAML::Node e) {
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
                e = YAML::LoadFile(fn);
            }
            break;
        case YAML::NodeType::Sequence:
            k.log(verbose, "got NodeType: Sequence\n");
            for (auto s : e) {
                k.log(verbose, "seq -> ");
                parse_node(s);
            }
            break;
        case YAML::NodeType::Map:
            k.log(verbose, "got NodeType: Map\n");

            for (auto kv : e) {
                k.log(verbose, "first -> ");
                parse_node(kv.first);
                k.log(verbose, "second -> ");
                parse_node(kv.second);
            }
            break;
    }
}

YAML::Node robotkernel::rkc_load_file(const std::string& filename) {
    kernel& k = *kernel::get_instance();
    k.log(verbose, "rkc_load_file %s\n", filename.c_str());

    YAML::Node node = YAML::LoadFile(filename);

    parse_node(node);

    YAML::Emitter emit;
    emit << node;
    k.log(verbose, "preproc config: %s\n", emit.c_str());

    return node;
}

