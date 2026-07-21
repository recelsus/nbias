#pragma once

#include <filesystem>
#include <optional>
#include <string>

struct env_file
{
    std::optional<std::string> output_dir{};
    std::optional<std::string> input_dir{};
    std::optional<std::string> nbias_key{};
};

env_file load_env_file(std::filesystem::path const& directory);
