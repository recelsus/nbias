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

// Looks for a ".env" file directly inside `directory` (no upward search).
// Recognizes OUTPUT / INPUT / NBIAS_KEY, per reference/requirements.md section 4.1 and 6.
env_file load_env_file(std::filesystem::path const& directory);
