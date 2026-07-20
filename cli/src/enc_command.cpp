#include "enc_command.hpp"

#include "env_file.hpp"
#include "file_ops.hpp"
#include "passphrase_resolver.hpp"

#include <nbias/core/vault.hpp>

#include <fstream>
#include <iostream>
#include <iterator>

namespace
{
    nbias::core::byte_buffer read_file_bytes(std::filesystem::path const& path)
    {
        std::ifstream stream(path, std::ios::binary);
        if(!stream) {
            throw std::runtime_error("cannot open input file: " + path.string());
        }
        return nbias::core::byte_buffer(
            (std::istreambuf_iterator<char>(stream)),
            std::istreambuf_iterator<char>());
    }

    void write_file_bytes(std::filesystem::path const& path, nbias::core::byte_buffer const& content)
    {
        if(auto const& dir = path.parent_path(); !dir.empty()) {
            std::filesystem::create_directories(dir);
        }
        std::ofstream stream(path, std::ios::binary | std::ios::trunc);
        stream.write(reinterpret_cast<char const*>(content.data()), static_cast<std::streamsize>(content.size()));
    }

    bool is_already_encrypted(std::filesystem::path const& path)
    {
        return path.extension().string() == vault_extension;
    }

    bool wants_password(enc_options const& options)
    {
        return options.password_protected && !options.explicit_no_password;
    }

    std::optional<std::string> resolve_enc_passphrase(enc_options const& options)
    {
        if(!wants_password(options)) {
            return std::nullopt;
        }
        auto resolved = resolve_passphrase_noninteractive(options.key);
        if(resolved) {
            return resolved;
        }
        return prompt_passphrase_interactively("password: ");
    }

    int encrypt_one(
        std::filesystem::path const& input_path,
        enc_options const& options,
        std::optional<std::string> const& passphrase,
        env_file const& env)
    {
        if(!std::filesystem::exists(input_path)) {
            std::cerr << "nbias enc: no such file: " << input_path.string() << '\n';
            return 1;
        }
        if(is_already_encrypted(input_path)) {
            std::cerr << "nbias enc: already encrypted, skipping: " << input_path.string() << '\n';
            return 1;
        }

        auto plaintext = read_file_bytes(input_path);
        auto auth = wants_password(options) ? nbias::core::auth_method::password : nbias::core::auth_method::no_password;

        auto vault_bytes = nbias::core::encrypt_note(
            plaintext,
            input_path.filename().string(),
            auth,
            nbias::core::kdf_profile::fast,
            as_view(passphrase));

        auto output_path = make_vault_output_path(input_path, options.output_dir, env.output_dir, std::filesystem::current_path());
        write_file_bytes(output_path, vault_bytes);
        std::cout << input_path.string() << " -> " << output_path.string() << '\n';
        return 0;
    }
}

int execute_enc(enc_options const& options)
{
    auto passphrase = resolve_enc_passphrase(options);
    auto env = load_env_file(std::filesystem::current_path());

    int exit_code{0};
    for(auto const& path : options.paths) {
        try {
            if(encrypt_one(path, options, passphrase, env) != 0) {
                exit_code = 1;
            }
        }
        catch(std::exception const& problem) {
            std::cerr << "nbias enc: " << path.string() << ": " << problem.what() << '\n';
            exit_code = 1;
        }
    }
    return exit_code;
}
