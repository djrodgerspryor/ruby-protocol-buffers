#include "string_utils.h"

// CamelCase or camelCase
const std::string StringUtils::ToCamelCase(const std::string& str, bool capitalize_first_character) {
    std::string temp = str;

    char previous_character;
    bool is_first_character_of_word, is_first_character_of_phrase;
    for (int i = 0; i < temp.length(); i++) {
        // We only care about changing the case of alphabetic characters
        if (isalpha(temp[i])) {
            // Only alphabetic characters are allowed in 'words' (for the purposes of casing)
            is_first_character_of_word = (i == 0) || !isalpha(previous_character);

            // Digits, hyphens and underscores are included in phrases
            is_first_character_of_phrase = (
                (i == 0) ||
                (
                    is_first_character_of_word &&
                    (previous_character != '_') &&
                    (previous_character != '-') &&
                    !isdigit(previous_character)
                )
            );

            if (is_first_character_of_word && (!is_first_character_of_phrase || capitalize_first_character)) {
                temp[i] = toupper(temp[i]);
            }

            if(is_first_character_of_word && is_first_character_of_phrase && !capitalize_first_character) {
                temp[i] = tolower(temp[i]);
            }
        }

        // Store the current character for the next loop
        previous_character = temp[i];
    }

    // Get rid of any underscores or hyphens between words (used by snake_case and kebab-case respectivley).
    // Note: C++ std regexps don't support look-behinds, so we need to capture the preceeding character and
    // then put it back unaltered.
    temp = regex_replace(temp, std::regex("([a-zA-Z0-9])(_|-)(?=[a-zA-Z0-9])"), "$1");

    const std::string result = temp;
    return result;
}

// snake_case
const std::string StringUtils::ToSnakeCase(const std::string& str) {
    std::string temp = "";

    char previous_character;
    bool is_first_character_of_word, is_first_character_of_phrase;
    for (int i = 0; i < str.length(); i++) {
        // We only care about changing the case of alphabetic characters
        if (isalpha(str[i])) {
            is_first_character_of_word = (
                (i == 0) ||
                !isalpha(previous_character) ||
                isupper(str[i])
            );

            // Digits, hyphens and underscores are included in phrases
            is_first_character_of_phrase = (
                (i == 0) ||
                (
                    is_first_character_of_word &&
                    !isalpha(previous_character) &&
                    !isdigit(previous_character) &&
                    (previous_character != '_') &&
                    (previous_character != '-')
                )
            );

            // If this is a word-boundary
            if (is_first_character_of_word && !is_first_character_of_phrase) {
                // Separate words with an underscore
                temp += "_";
            }

            // All alphabetic characters should be lower case
            temp += tolower(str[i]);

        } else {
            // Put non-alphabetic characters into the result unchanged
            temp += str[i];
        }

        // Store the current character for the next loop
        previous_character = str[i];
    }

    const std::string result = temp;
    return result;
}

// UPPER CASE
const std::string StringUtils::ToUpperCase(const std::string& str) {
    std::string temp = str;

    for(int i = 0; i < str.length(); i++) {
        if (isalpha(str[i])) {
            temp[i] = toupper(str[i]);
        }
    }

    const std::string result = temp;
    return result;
}

// SCREAMING_SNAKE_CASE
const std::string StringUtils::ToScreamingSnakeCase(const std::string& str) {
    return ToUpperCase(ToSnakeCase(str));
}

std::vector<std::string> StringUtils::Split(const std::string& str, const std::string& delimeter) {
    std::vector<std::string> results;

    // Running string of chars which haven't yet been matched to a delimeter or a delimited value
    std::string unmatched_chars = "";

    for(int i = 0; i < str.length(); i++) {
        unmatched_chars += str[i];

        if (HasSuffix(unmatched_chars, delimeter)) {
            // Strip-off the delimeter from the end and push the rest of the string into the results
            results.push_back(
                unmatched_chars.substr(0, unmatched_chars.size() - delimeter.size())
            );

            // Reset the search for matching substrings
            unmatched_chars = "";
        }
    }

    // If there are any characters left
    if (!unmatched_chars.empty()) {
        // Use them as a result too
        results.push_back(unmatched_chars);
    }

    return results;
}

const bool StringUtils::HasSuffix(const std::string& str, const std::string& suffix) {
    return (
        str.size() >= suffix.size() &&
        str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0
    );
}