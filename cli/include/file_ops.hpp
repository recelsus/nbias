#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

inline constexpr std::string_view vault_extension{".knty"};

bool read_yes_no_answer();

std::filesystem::path make_vault_output_path(
    std::filesystem::path const& input_path,
    std::optional<std::filesystem::path> const& output_dir_override,
    std::optional<std::string> const& env_output_dir,
    std::filesystem::path const& base_dir);

std::filesystem::path make_plain_output_dir(
    std::filesystem::path const& vault_path,
    std::optional<std::filesystem::path> const& output_dir_override,
    std::optional<std::string> const& env_output_dir,
    std::optional<std::string> const& env_input_dir,
    std::filesystem::path const& base_dir);

std::filesystem::path choose_decrypt_write_path(
    std::filesystem::path const& desired_path,
    std::vector<unsigned char> const& new_content);

std::filesystem::path sanitize_orig_name(std::string const& orig_name);
