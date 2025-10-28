#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

enum class command_kind
{
    edit,
    encrypt,
    decrypt,
    info,
};

enum class delete_source_mode
{
    ask,
    yes,
    no,
};

enum class kdf_profile
{
    fast,
    balanced,
    hardened,
};

enum class tmpfs_mode
{
    prefer,
    never,
};

struct common_options
{
    bool verbose{false};
    std::optional<std::string> passphrase{};
    std::optional<std::string> key_hex{};
};

struct edit_options : common_options
{
    std::filesystem::path target_path{};
    std::optional<std::string> editor{};
    std::string vault_extension{"nbv"};
    std::optional<std::string> allow_extensions{};
    std::optional<std::size_t> max_bytes{};
    bool assume_yes{false};
    delete_source_mode delete_source{delete_source_mode::ask};
    std::optional<std::string> orig_extension{};
    kdf_profile profile{kdf_profile::balanced};
    tmpfs_mode tmpfs{tmpfs_mode::prefer};
};

struct encrypt_options : common_options
{
    std::filesystem::path input_path{};
    std::filesystem::path output_path{};
    std::optional<std::string> orig_extension{};
    kdf_profile profile{kdf_profile::balanced};
};

struct decrypt_options : common_options
{
    std::filesystem::path input_path{};
    std::filesystem::path output_path{};
};

struct info_options : common_options
{
    std::filesystem::path input_path{};
};

using command_payload = std::variant<edit_options, encrypt_options, decrypt_options, info_options>;

struct parsed_command
{
    command_kind kind{};
    command_payload payload;
};

parsed_command parse_command_line(int argc, char const* const* argv);

std::string build_usage_string();

