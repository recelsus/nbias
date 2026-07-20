#include "file_ops.hpp"

#include "cli_defaults.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <system_error>

namespace
{
    std::filesystem::path reproduce_subdir(
        std::filesystem::path const& item_dir,
        std::filesystem::path const& base_dir,
        std::filesystem::path const& mapped_root)
    {
        std::error_code error{};
        auto relative = std::filesystem::relative(item_dir, base_dir, error);
        if(error) {
            return mapped_root;
        }
        return mapped_root / relative;
    }

    std::vector<unsigned char> read_file_bytes(std::filesystem::path const& path)
    {
        std::ifstream stream(path, std::ios::binary);
        return std::vector<unsigned char>(
            (std::istreambuf_iterator<char>(stream)),
            std::istreambuf_iterator<char>());
    }

    std::filesystem::path make_numbered_path(std::filesystem::path const& desired_path)
    {
        auto stem = desired_path.stem().string();
        auto extension = desired_path.extension().string();
        auto dir = desired_path.parent_path();

        for(int sequence = 1; sequence <= numbered_suffix_max_sequence; ++sequence) {
            std::ostringstream suffix{};
            suffix << '.' << std::setw(numbered_suffix_digit_width) << std::setfill('0') << sequence;
            auto candidate = dir / (stem + suffix.str() + extension);
            if(!std::filesystem::exists(candidate)) {
                return candidate;
            }
        }
        throw std::runtime_error("too many colliding output files for " + desired_path.string());
    }

    bool prompt_overwrite_confirmation(std::filesystem::path const& path)
    {
        std::cout << path.string() << " already exists with different content. Overwrite? [y/N] ";
        std::cout.flush();
        return read_yes_no_answer();
    }
}

bool read_yes_no_answer()
{
    std::string answer{};
    std::getline(std::cin, answer);
    return answer == "y" || answer == "Y";
}

std::filesystem::path make_vault_output_path(
    std::filesystem::path const& input_path,
    std::optional<std::filesystem::path> const& output_dir_override,
    std::optional<std::string> const& env_output_dir,
    std::filesystem::path const& base_dir)
{
    auto filename = input_path.filename().string() + std::string{vault_extension};

    if(output_dir_override) {
        return *output_dir_override / filename;
    }
    if(env_output_dir) {
        auto dir = reproduce_subdir(input_path.parent_path(), base_dir, base_dir / *env_output_dir);
        return dir / filename;
    }
    return input_path.parent_path() / filename;
}

std::filesystem::path make_plain_output_dir(
    std::filesystem::path const& vault_path,
    std::optional<std::filesystem::path> const& output_dir_override,
    std::optional<std::string> const& env_output_dir,
    std::optional<std::string> const& env_input_dir,
    std::filesystem::path const& base_dir)
{
    if(output_dir_override) {
        return *output_dir_override;
    }
    if(env_input_dir) {
        auto root_to_strip = env_output_dir ? (base_dir / *env_output_dir) : vault_path.parent_path();
        return reproduce_subdir(vault_path.parent_path(), root_to_strip, base_dir / *env_input_dir);
    }
    return vault_path.parent_path();
}

std::filesystem::path choose_decrypt_write_path(
    std::filesystem::path const& desired_path,
    std::vector<unsigned char> const& new_content)
{
    if(!std::filesystem::exists(desired_path)) {
        return desired_path;
    }

    if(read_file_bytes(desired_path) == new_content) {
        return desired_path;
    }

    if(prompt_overwrite_confirmation(desired_path)) {
        return desired_path;
    }

    return make_numbered_path(desired_path);
}
