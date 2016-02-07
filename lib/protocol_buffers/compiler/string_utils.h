#ifndef STRING_UTILS_H_
#define STRING_UTILS_H_

#include <string>
#include <iostream>
#include <regex>

class StringUtils {
    public:
        static const std::string ToCamelCase(const std::string &str, bool capitalize_first_character=true);
        static const std::string ToSnakeCase(const std::string &str);
        static const std::string ToUpperCase(const std::string& str);
        static const std::string ToScreamingSnakeCase(const std::string& str);
        static std::vector<std::string> Split(const std::string& str, const std::string& delimeter);
        static const bool HasSuffix(const std::string& str, const std::string& suffix);
};

#endif  // STRING_UTILS_H_
