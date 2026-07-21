#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

inline constexpr std::string_view vault_extension{".knty"};

// Reads a single line from stdin and reports whether it was "y"/"Y".
bool read_yes_no_answer();

// enc output path: --output-dir (flat) > .env OUTPUT (reproduces the input's subdirectory
// structure relative to base_dir) > same directory as the input file.
std::filesystem::path make_vault_output_path(
    std::filesystem::path const& input_path,
    std::optional<std::filesystem::path> const& output_dir_override,
    std::optional<std::string> const& env_output_dir,
    std::filesystem::path const& base_dir);

// dec output directory: --output-dir (flat) > .env INPUT (inverts the enc-side OUTPUT mapping:
// strips the .env OUTPUT prefix from the vault's directory, then reproduces what's left under
// INPUT) > same directory as the vault file. The restored filename itself comes from the vault
// header's orig_name, not from this helper.
std::filesystem::path make_plain_output_dir(
    std::filesystem::path const& vault_path,
    std::optional<std::filesystem::path> const& output_dir_override,
    std::optional<std::string> const& env_output_dir,
    std::optional<std::string> const& env_input_dir,
    std::filesystem::path const& base_dir);

// Implements the collision rule from requirements.md section 2: identical content overwrites
// silently, differing content prompts y/N, and declining picks a numbered sibling name.
std::filesystem::path choose_decrypt_write_path(
    std::filesystem::path const& desired_path,
    std::vector<unsigned char> const& new_content);

// Reduces a vault header's orig_name to a single path component safe to join under an output
// directory. A vault's header is AEAD-authenticated, but that only proves it matches what its
// author wrote into it — a maliciously crafted vault can still carry a traversal-style
// orig_name (e.g. "../../.bashrc", "/etc/passwd"), so this must be applied before writing.
// Throws std::runtime_error if nothing safe remains after stripping directory components.
std::filesystem::path sanitize_orig_name(std::string const& orig_name);
