#include <google/protobuf/compiler/plugin.h>
#include "ruby_code_generator.h"

int main(int argc, char** argv) {
    RubyCodeGenerator generator;
    return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}
