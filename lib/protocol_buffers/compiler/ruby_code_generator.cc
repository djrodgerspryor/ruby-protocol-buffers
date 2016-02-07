#include <sstream>

#include "ruby_code_generator.h"
#include "string_utils.h"

namespace pb = google::protobuf;

RubyCodeGenerator::RubyCodeGenerator() {}
RubyCodeGenerator::~RubyCodeGenerator() {}

bool RubyCodeGenerator::Generate(
    const google::protobuf::FileDescriptor *file,
    const std::string& parameter,
    google::protobuf::compiler::OutputDirectory *output_directory,
    std::string *error
) const {

    // Parse parameter string. Eg. "foo=bar,baz,qux=corge" -> ("foo", "bar"), ("baz", ""), ("qux", "corge")
    std::vector< std::pair<std::string, std::string> > parsed_parameters;
    pb::compiler::ParseGeneratorParameter(parameter, &parsed_parameters);

    // Parse pairs into args and kwargs
    std::vector<std::string> args;
    std::map<std::string, std::string> kwargs;

    for(auto parameter_pair : parsed_parameters) {
        std::string key = parameter_pair.first;
        std::string value = parameter_pair.second;

        // Empty values cause the key to go into args (yes, this is imperfect -- kwargs with empty values are a
        // reasonable use-case -- but I didn't feel like rewriting ParseGeneratorParameter).
        if (value == "") {
            args.push_back(key);

        } else {
            kwargs.insert(parameter_pair);
        }
    }

    // Write JavaScript thunks for API using our `sync` module.
    PrintPackage(file, output_directory, args, kwargs, error);

    return true;
}

bool RubyCodeGenerator::PrintPackage(
    const google::protobuf::FileDescriptor *file,
    google::protobuf::compiler::OutputDirectory *output_directory,
    std::vector<std::string>& args,
    std::map<std::string, std::string>& kwargs,
    std::string *error
) const {
    // Build output filename.
    std::string output_file_name = SourceFilename2CompiledFilename(file->name());

    // Get rid of directory
    std::size_t loc = output_file_name.rfind("/");
    output_file_name.erase(0, loc);

    pb::internal::scoped_ptr<pb::io::ZeroCopyOutputStream> output(output_directory->Open(output_file_name));
    pb::io::Printer printer(output.get(), '$');

    // Track success for the whole generator by accumulating success/fail statuses into this bool
    bool result = true;

    printer.Print("# Generated by the protocol buffer compiler. DO NOT EDIT!\n");
    printer.Print("\n");
    printer.Print("require 'protocol_buffers'\n");
    printer.Print("\n");

    // Wrap-up generator context so that it can be neatly passed down the function-chain
    Context context = {printer, *file, args, kwargs, error};

    result = result && PrintImports(context);
    printer.Print("\n");

    std::vector<std::string> package_name_components = StringUtils::Split(file->package(), ".");

    // Print enclosing module definitions corresponding to the proto package name
    for (auto package_component : package_name_components) {
        printer.Print("module $module_name$\n", "module_name", StringUtils::ToCamelCase(package_component, true));
        printer.Indent(); printer.Indent();
    }

    // Print top-level enum definitions
    for (int i = 0; i < file->enum_type_count(); ++i) {
        result = result && PrintEnum(context, *file->enum_type(i));
        context.printer.Print("\n");
    }

    PrintMessages(context);

    for (int i = 0; i < file->service_count(); ++i) {
        result = result && RubyCodeGenerator::PrintService(context, *file->service(i));
    }

    // Exit the enclosing module definitions
    for (auto package_component : package_name_components) {
        printer.Outdent(); printer.Outdent();
        printer.Print("end\n");
    }

    return result;
}

bool RubyCodeGenerator::PrintImports(Context context) const {
    bool result = true;

    for (int i = 0; i < context.file.dependency_count(); ++i) {
        result = result && PrintImport(context, *context.file.dependency(i));
    }

    return result;
}

bool RubyCodeGenerator::PrintImport(
    Context context,
    const pb::FileDescriptor& imported_file
) const {

    std::string compiled_source_name = SourceFilename2CompiledFilename(imported_file.name());

    // Delete '.rb' from the end of the name
    std::size_t loc = compiled_source_name.rfind(".");
    compiled_source_name.erase(loc, compiled_source_name.length() - loc);

    std::map<std::string, std::string>::iterator it = context.kwargs.find("import_prefix");
    if (it != context.kwargs.end()) {
        std::string source_prefix = context.kwargs.find("import_prefix")->second;

        // Add joining forward slash if necessary
        std::string joiner = "";
        if (compiled_source_name[0] != '/' && source_prefix.back() != '/') {
            joiner = "/";
        }

        compiled_source_name = source_prefix + joiner + compiled_source_name;
    }

    context.printer.Print("begin; require '$source$'; rescue LoadError; end\n", "source", compiled_source_name);

    return true;
}

bool RubyCodeGenerator::PrintEnum(
    Context context,
    const pb::EnumDescriptor& descriptor
) const {

    bool result = true;

    context.printer.Print("module $name$\n", "name", StringUtils::ToCamelCase(descriptor.name(), true));
    context.printer.Indent(); context.printer.Indent();
        context.printer.Print("include ::ProtocolBuffers::Enum\n");
        context.printer.Print("\n");
        context.printer.Print("set_fully_qualified_name \"$fully_qualified_name$\"\n", "fully_qualified_name", descriptor.full_name());
        context.printer.Print("\n");

        // Print values
        for (int i = 0; i < descriptor.value_count(); ++i) {
            result = result && PrintEnumValue(context, *descriptor.value(i));
        }

    context.printer.Outdent(); context.printer.Outdent();
    context.printer.Print("end\n");

    return result;
}

bool RubyCodeGenerator::PrintEnumValue(
    Context context,
    const pb::EnumValueDescriptor& descriptor
) const {

    context.printer.Print(
        "$name$ = $number$\n",

        // Ensure that the name is upper-case
        "name", StringUtils::ToUpperCase(descriptor.name()),

        "number", std::to_string(descriptor.number())
    );

    return true;
}

bool RubyCodeGenerator::PrintMessages(Context context) const {
    context.printer.Print("# forward declarations\n");

    bool result = true;

    // Print forward-declarations
    for (int i = 0; i < context.file.message_type_count(); ++i) {
        result = result && PrintMessageForwardDeclaration(context, *context.file.message_type(i));
    }

    context.printer.Print("\n");

    // Print full class definitions
    for (int i = 0; i < context.file.message_type_count(); ++i) {
        result = result && PrintMessage(context, *context.file.message_type(i));
        context.printer.Print("\n");
    }

    return result;
}

bool RubyCodeGenerator::PrintMessageForwardDeclaration(
    Context context,
    const pb::Descriptor& descriptor
) const {

    context.printer.Print("class $message_class_name$ < ::ProtocolBuffers::Message; end\n", "message_class_name", StringUtils::ToCamelCase(descriptor.name(), true));

    return true;
}

bool RubyCodeGenerator::PrintMessage(
    Context context,
    const pb::Descriptor& descriptor
) const {

    bool result = true;

    context.printer.Print("class $message_class_name$ < ::ProtocolBuffers::Message\n", "message_class_name", StringUtils::ToCamelCase(descriptor.name(), true));
    context.printer.Indent(); context.printer.Indent();

        // Print scoped enum definitions
        for (int i = 0; i < descriptor.enum_type_count(); ++i) {
            result = result && PrintEnum(context, *descriptor.enum_type(i));
            context.printer.Print("\n");
        }

        context.printer.Print("set_fully_qualified_name \"$fully_qualified_name$\"\n", "fully_qualified_name", descriptor.full_name());
        context.printer.Print("\n");

        // Print field definitions
        for (int i = 0; i < descriptor.field_count(); ++i) {
            result = result && PrintField(context, *descriptor.field(i));
        }

    context.printer.Outdent(); context.printer.Outdent();
    context.printer.Print("end\n");

    return result;
}

bool RubyCodeGenerator::PrintField(
    Context context,
    const pb::FieldDescriptor& descriptor
) const {

    // Get label
    std::string label;
    if(descriptor.label() == pb::FieldDescriptor::Label::LABEL_OPTIONAL) {
        label = "optional";

    } else if(descriptor.label() == pb::FieldDescriptor::Label::LABEL_REQUIRED) {
        label = "required";

    } else if(descriptor.label() == pb::FieldDescriptor::Label::LABEL_REPEATED) {
        label = "repeated";

    } else {
        *context.error = "Unknown label!";
        return false;
    }

    // Get type
    std::string type = GetRubyType(descriptor);

    if (type.empty()) {
        *context.error = "Unknown type!";
        return false;
    }

    std::map<std::string, std::string> formatter_args = {
        {"label", label},
        {"type", type},
        {"name", descriptor.name()},
        {"number", std::to_string(descriptor.number())}
    };

    context.printer.Print(formatter_args, "$label$ $type$, :$name$, $number$\n");

    return true;
};

const std::string RubyCodeGenerator::GetRubyType(
    const pb::FieldDescriptor& descriptor
) const {

    if (descriptor.type() == google::protobuf::FieldDescriptor::TYPE_DOUBLE){
        return ":double";

    } else if (descriptor.type() == google::protobuf::FieldDescriptor::TYPE_FLOAT) {
        return ":float";

    } else if (descriptor.type() == google::protobuf::FieldDescriptor::TYPE_INT32) {
        return ":int32";

    } else if (descriptor.type() == google::protobuf::FieldDescriptor::TYPE_INT64) {
        return ":int64";

    } else if (descriptor.type() == google::protobuf::FieldDescriptor::TYPE_FIXED32) {
        return ":fixed32";

    } else if (descriptor.type() == google::protobuf::FieldDescriptor::TYPE_FIXED64) {
        return ":fixed64";

    } else if (descriptor.type() == google::protobuf::FieldDescriptor::TYPE_UINT32) {
        return ":uint32";

    } else if (descriptor.type() == google::protobuf::FieldDescriptor::TYPE_UINT64) {
        return ":uint64";

    } else if (descriptor.type() == google::protobuf::FieldDescriptor::TYPE_SINT32) {
        return ":sint32";

    } else if (descriptor.type() == google::protobuf::FieldDescriptor::TYPE_SINT64) {
        return ":sint64";

    } else if (descriptor.type() == google::protobuf::FieldDescriptor::TYPE_SFIXED32) {
        return ":sfixed32";

    } else if (descriptor.type() == google::protobuf::FieldDescriptor::TYPE_SFIXED64) {
        return ":sfixed64";

    } else if (descriptor.type() == google::protobuf::FieldDescriptor::TYPE_BOOL) {
        return ":bool";

    } else if (descriptor.type() == google::protobuf::FieldDescriptor::TYPE_STRING) {
        return ":string";

    } else if (descriptor.type() == google::protobuf::FieldDescriptor::TYPE_BYTES) {
        return ":bytes";

    } else if (descriptor.type() == google::protobuf::FieldDescriptor::TYPE_ENUM) {
        return PackageName2RubyName(descriptor.enum_type()->full_name());

    } else if (descriptor.type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE) {
        return PackageName2RubyName(descriptor.message_type()->full_name());
    }

    // Unknown type
    return "";
}

bool RubyCodeGenerator::PrintService(
    Context context,
    const pb::ServiceDescriptor& descriptor
) const {

    bool result = true;

    context.printer.Print("class $service_class_name$ < ::ProtocolBuffers::Service\n", "service_class_name", StringUtils::ToCamelCase(descriptor.name(), true));
    context.printer.Indent(); context.printer.Indent();

        context.printer.Print("set_fully_qualified_name \"$fully_qualified_name$\"\n", "fully_qualified_name", descriptor.full_name());
        context.printer.Print("\n");

        // Print rpc definitions
        for (int i = 0; i < descriptor.method_count(); ++i) {
            result = result && PrintMethod(context, *descriptor.method(i));
        }

    context.printer.Outdent(); context.printer.Outdent();
    context.printer.Print("end\n");

    return result;
}

bool RubyCodeGenerator::PrintMethod(
    Context context,
    const pb::MethodDescriptor& descriptor
) const {

    std::map<std::string, std::string> formatter_args = {
        {"ruby_method_name", StringUtils::ToSnakeCase(descriptor.name())},
        {"proto_method_name", descriptor.name()},
        {"input_message_type", PackageName2RubyName(descriptor.input_type()->full_name())},
        {"output_message_type", PackageName2RubyName(descriptor.output_type()->full_name())}
    };

    context.printer.Print(formatter_args, "rpc :$ruby_method_name$, \"$proto_method_name$\", $input_message_type$, $output_message_type$\n");

    return true;
}

const std::string RubyCodeGenerator::SourceFilename2CompiledFilename(std::string filename) const {
    std::string temp = filename;
    std::size_t loc = temp.rfind(".");
    temp.erase(loc, temp.length() - loc);
    temp.append(".pb.rb");

    const std::string result = temp;

    return result;
}

const std::string RubyCodeGenerator::PackageName2RubyName(const std::string package_name) const {
    // CamelCase the module name(s)
    std::string temp = StringUtils::ToCamelCase(package_name, true);

    // Replace any package-separators ('.') with Ruby const-accessors ('::'), and add an initital '::'
    const std::string result = "::" + regex_replace(
        temp,
        std::regex("\\."),
        "::"
    );

    return result;
}