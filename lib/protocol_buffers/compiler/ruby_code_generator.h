#ifndef RUBY_CODE_GENERATOR_H_
#define RUBY_CODE_GENERATOR_H_

#include <string>
#include <regex>
#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/message.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>

struct Context {
    google::protobuf::io::Printer& printer;
    const google::protobuf::FileDescriptor& file;
    std::vector<std::string>& args;
    std::map<std::string, std::string>& kwargs;
    std::string* error;
};

class RubyCodeGenerator : public ::google::protobuf::compiler::CodeGenerator {
 public:
    explicit RubyCodeGenerator();
    virtual ~RubyCodeGenerator();

    virtual bool Generate(
        const google::protobuf::FileDescriptor *file,
        const std::string &parameter,
        google::protobuf::compiler::OutputDirectory *output_directory,
        std::string *error
    ) const;

    private:
        bool PrintPackage(
            const google::protobuf::FileDescriptor *file,
            google::protobuf::compiler::OutputDirectory *output_directory,
            std::vector<std::string>& args,
            std::map<std::string, std::string>& kwargs,
            std::string *error
        ) const;

        bool PrintImports(Context context) const;

        bool PrintImport(
            Context context,
            const google::protobuf::FileDescriptor& imported_file
        ) const;

        bool PrintEnum(
            Context context,
            const google::protobuf::EnumDescriptor& descriptor
        ) const;

        bool PrintEnumValue(
            Context context,
            const google::protobuf::EnumValueDescriptor& descriptor
        ) const;

        bool PrintMessages(Context context) const;

        bool PrintMessageForwardDeclaration(
            Context context,
            const google::protobuf::Descriptor& descriptor
        ) const;

        bool PrintMessage(
            Context context,
            const google::protobuf::Descriptor& descriptor
        ) const;

        bool PrintField(
            Context context,
            const google::protobuf::FieldDescriptor& descriptor
        ) const;

        const std::string GetRubyType(const google::protobuf::FieldDescriptor& descriptor) const;

        bool PrintService(
            Context context,
            const google::protobuf::ServiceDescriptor& descriptor
        ) const;

        bool PrintMethod(
            Context context,
            const google::protobuf::MethodDescriptor& descriptor
        ) const;

        const std::string SourceFilename2CompiledFilename(std::string filename) const;

        const std::string PackageName2RubyName(const std::string package_name) const;
};

#endif // RUBY_CODE_GENERATOR_H_
