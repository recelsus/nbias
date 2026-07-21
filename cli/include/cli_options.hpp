#pragma once

#include <nbias/core/vault.hpp>

#include <filesystem>
#include <optional>
#include <string>
#include <variant>
#include <vector>

enum class command_kind
{
    enc,
    dec,
    edit,
    info,
};

struct common_key_options
{
    bool password_protected{false};
    bool explicit_no_password{false};
    std::optional<std::string> key{};
};

struct enc_options : common_key_options
{
    std::vector<std::filesystem::path> paths{};
    std::optional<std::filesystem::path> output_dir{};
    nbias::core::kdf_profile profile{nbias::core::kdf_profile::fast};
};

struct dec_options : common_key_options
{
    std::vector<std::filesystem::path> paths{};
    std::optional<std::filesystem::path> output_dir{};
};

struct edit_options : common_key_options
{
    std::filesystem::path target_path{};
    std::optional<std::string> editor{};
    bool assume_yes{false};
};

struct info_options
{
    std::filesystem::path target_path{};
};

using command_payload = std::variant<enc_options, dec_options, edit_options, info_options>;

struct parsed_command
{
    command_kind kind{};
    command_payload payload;
};

parsed_command parse_command_line(int argc, char const* const* argv);

std::string build_usage_string();
