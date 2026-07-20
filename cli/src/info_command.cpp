#include "info_command.hpp"

#include <nbias/core/vault.hpp>

#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

namespace
{
    nbias::core::byte_buffer read_file_bytes(std::filesystem::path const& path)
    {
        std::ifstream stream(path, std::ios::binary);
        if(!stream) {
            throw std::runtime_error("cannot open file: " + path.string());
        }
        return nbias::core::byte_buffer(
            (std::istreambuf_iterator<char>(stream)),
            std::istreambuf_iterator<char>());
    }

    std::string_view describe_auth(nbias::core::auth_method auth)
    {
        switch(auth) {
        case nbias::core::auth_method::password:
            return "password";
        case nbias::core::auth_method::no_password:
            return "no_password (embedded key)";
        case nbias::core::auth_method::symmetric_keyfile:
            return "symmetric_keyfile (reserved, unimplemented)";
        case nbias::core::auth_method::asymmetric_keyfile:
            return "asymmetric_keyfile (reserved, unimplemented)";
        }
        return "unknown";
    }

    std::string_view describe_profile(nbias::core::kdf_profile profile)
    {
        switch(profile) {
        case nbias::core::kdf_profile::fast:
            return "fast";
        case nbias::core::kdf_profile::balanced:
            return "balanced";
        case nbias::core::kdf_profile::hardened:
            return "hardened";
        }
        return "unknown";
    }
}

int execute_info(info_options const& options)
{
    if(!std::filesystem::exists(options.target_path)) {
        std::cerr << "nbias info: no such file: " << options.target_path.string() << '\n';
        return 1;
    }

    auto vault_bytes = read_file_bytes(options.target_path);
    if(!nbias::core::has_vault_header(vault_bytes)) {
        std::cerr << "nbias info: not a nbias vault: " << options.target_path.string() << '\n';
        return 1;
    }

    auto header = nbias::core::peek_header(vault_bytes);

    std::cout << "path:           " << options.target_path.string() << '\n';
    std::cout << "format version: " << static_cast<int>(header.version) << '\n';
    std::cout << "auth method:    " << describe_auth(header.auth) << '\n';
    if(header.auth == nbias::core::auth_method::password) {
        std::cout << "kdf profile:    " << describe_profile(header.profile) << '\n';
    }
    std::cout << "original name:  " << header.orig_name << '\n';
    return 0;
}
