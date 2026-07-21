#include "env_file.hpp"

#include <fstream>

namespace
{
    std::string trim(std::string const& text)
    {
        auto begin = text.find_first_not_of(" \t\r\n");
        if(begin == std::string::npos) {
            return {};
        }
        auto end = text.find_last_not_of(" \t\r\n");
        return text.substr(begin, end - begin + 1);
    }

    std::string strip_quotes(std::string const& text)
    {
        if(text.size() >= 2) {
            auto front = text.front();
            auto back = text.back();
            if((front == '"' && back == '"') || (front == '\'' && back == '\'')) {
                return text.substr(1, text.size() - 2);
            }
        }
        return text;
    }

    bool has_content(std::string const& line)
    {
        return !line.empty() && line.front() != '#';
    }
}  // namespace

env_file load_env_file(std::filesystem::path const& directory)
{
    env_file result{};

    std::ifstream stream(directory / ".env");
    if(!stream) {
        return result;
    }

    std::string line{};
    while(std::getline(stream, line)) {
        auto trimmed = trim(line);
        if(!has_content(trimmed)) {
            continue;
        }

        auto separator = trimmed.find('=');
        if(separator == std::string::npos) {
            continue;
        }

        auto key = trim(trimmed.substr(0, separator));
        auto value = strip_quotes(trim(trimmed.substr(separator + 1)));

        if(key == "OUTPUT") {
            result.output_dir = value;
        }
        else if(key == "INPUT") {
            result.input_dir = value;
        }
        else if(key == "NBIAS_KEY") {
            result.nbias_key = value;
        }
    }

    return result;
}
