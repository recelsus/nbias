#include "dec_command.hpp"

#include "cli_defaults.hpp"
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
        if(!stream) {
            throw std::runtime_error("failed to open for writing: " + path.string());
        }
        stream.write(reinterpret_cast<char const*>(content.data()), static_cast<std::streamsize>(content.size()));
        if(!stream) {
            throw std::runtime_error("failed to write: " + path.string());
        }
    }

    nbias::core::decrypted_note decrypt_with_password(
        nbias::core::byte_buffer const& vault_bytes,
        dec_options const& options)
    {
        auto candidate = resolve_passphrase_noninteractive(options.key);
        passphrase_scrubber candidate_scrubber{candidate};
        if(candidate) {
            try {
                return nbias::core::decrypt_note(vault_bytes, as_view(candidate));
            }
            catch(nbias::core::vault_auth_error const&) {
            }
        }

        for(int attempt = 0; attempt < max_interactive_password_attempts; ++attempt) {
            auto passphrase = prompt_passphrase_interactively("password: ");
            string_scrubber attempt_scrubber{passphrase};
            try {
                return nbias::core::decrypt_note(vault_bytes, std::string_view{passphrase});
            }
            catch(nbias::core::vault_auth_error const&) {
                std::cerr << "wrong password\n";
            }
        }

        throw nbias::core::vault_auth_error("too many failed password attempts");
    }

    int decrypt_one(std::filesystem::path const& vault_path, dec_options const& options, env_file const& env)
    {
        if(!std::filesystem::exists(vault_path)) {
            std::cerr << "nbias dec: no such file: " << vault_path.string() << '\n';
            return 1;
        }

        auto vault_bytes = read_file_bytes(vault_path);
        if(!nbias::core::has_vault_header(vault_bytes)) {
            std::cerr << "nbias dec: not a nbias vault: " << vault_path.string() << '\n';
            return 1;
        }

        auto header = nbias::core::peek_header(vault_bytes);

        nbias::core::decrypted_note note{};
        if(header.auth == nbias::core::auth_method::no_password) {
            note = nbias::core::decrypt_note(vault_bytes, std::nullopt);
            if(options.password_protected) {
                std::cout << vault_path.string() << ": this vault has no password\n";
            }
        }
        else {
            note = decrypt_with_password(vault_bytes, options);
        }

        auto output_dir = make_plain_output_dir(vault_path, options.output_dir, env.output_dir, env.input_dir, std::filesystem::current_path());
        auto desired_path = output_dir / sanitize_orig_name(note.orig_name);

        auto write_path = choose_decrypt_write_path(desired_path, note.plaintext);
        write_file_bytes(write_path, note.plaintext);
        std::cout << vault_path.string() << " -> " << write_path.string() << '\n';
        return 0;
    }
}  // namespace

int execute_dec(dec_options const& options)
{
    auto env = load_env_file(std::filesystem::current_path());

    int exit_code{0};
    for(auto const& path : options.paths) {
        try {
            if(decrypt_one(path, options, env) != 0) {
                exit_code = 1;
            }
        }
        catch(std::exception const& problem) {
            std::cerr << "nbias dec: " << path.string() << ": " << problem.what() << '\n';
            exit_code = 1;
        }
    }
    return exit_code;
}
