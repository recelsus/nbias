#include "edit_command.hpp"

#include "cli_defaults.hpp"
#include "file_ops.hpp"
#include "passphrase_resolver.hpp"

#include <nbias/core/vault.hpp>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string_view>
#include <vector>

#include <unistd.h>

namespace
{
    bool command_exists_on_path(std::string_view name)
    {
        auto const* path_environment = std::getenv("PATH");
        if(path_environment == nullptr) {
            return false;
        }

        std::string_view path{path_environment};
        std::size_t start{0};
        while(start <= path.size()) {
            auto separator = path.find(':', start);
            auto segment = separator == std::string_view::npos ? path.substr(start) : path.substr(start, separator - start);
            if(!segment.empty() && ::access((std::filesystem::path{segment} / name).c_str(), X_OK) == 0) {
                return true;
            }
            if(separator == std::string_view::npos) {
                break;
            }
            start = separator + 1;
        }
        return false;
    }

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

    void write_file_bytes(std::filesystem::path const& path, nbias::core::byte_buffer const& content)
    {
        std::ofstream stream(path, std::ios::binary | std::ios::trunc);
        if(!stream) {
            throw std::runtime_error("failed to open for writing: " + path.string());
        }
        stream.write(reinterpret_cast<char const*>(content.data()), static_cast<std::streamsize>(content.size()));
        if(!stream) {
            throw std::runtime_error("failed to write: " + path.string());
        }
    }

    // Priority: --editor > $VISUAL > $EDITOR > the first of fallback_editor_candidates
    // found on $PATH (GUI editors are typically set via $VISUAL, terminal ones via $EDITOR).
    std::string choose_editor_command(edit_options const& options)
    {
        if(options.editor) {
            return *options.editor;
        }
        if(auto const* visual = std::getenv("VISUAL"); visual != nullptr && visual[0] != '\0') {
            return visual;
        }
        if(auto const* editor = std::getenv("EDITOR"); editor != nullptr && editor[0] != '\0') {
            return editor;
        }
        for(auto candidate : fallback_editor_candidates) {
            if(command_exists_on_path(candidate)) {
                return std::string{candidate};
            }
        }
        throw std::runtime_error("no editor found: set --editor, $VISUAL, $EDITOR, or install one of nvim/vim/vi/nano");
    }

    bool has_vault_extension(std::filesystem::path const& path)
    {
        return path.extension().string() == vault_extension;
    }

    // POSIX-only temporary plaintext file (mkstemp), removed on destruction even if editing
    // fails partway through. Windows temp-file handling is a known open item (requirements.md
    // section 3/10).
    class scoped_temp_file
    {
    public:
        explicit scoped_temp_file(nbias::core::byte_buffer const& content)
        {
            auto pattern = (std::filesystem::temp_directory_path() / "nbias-edit-XXXXXX").string();
            std::vector<char> buffer(pattern.begin(), pattern.end());
            buffer.push_back('\0');

            auto descriptor = mkstemp(buffer.data());
            if(descriptor < 0) {
                throw std::runtime_error("failed to create a temporary file for editing");
            }
            path_ = buffer.data();

            auto bytes_written = ::write(descriptor, content.data(), content.size());
            ::close(descriptor);
            if(bytes_written < 0 || static_cast<std::size_t>(bytes_written) != content.size()) {
                std::filesystem::remove(path_);
                throw std::runtime_error("failed to write plaintext to the temporary file");
            }
        }

        ~scoped_temp_file()
        {
            std::error_code ignored{};
            std::filesystem::remove(path_, ignored);
        }

        scoped_temp_file(scoped_temp_file const&) = delete;
        scoped_temp_file& operator=(scoped_temp_file const&) = delete;

        std::filesystem::path const& path() const
        {
            return path_;
        }

    private:
        std::filesystem::path path_{};
    };

    std::optional<std::string> resolve_edit_passphrase(edit_options const& options)
    {
        auto candidate = resolve_passphrase_noninteractive(options.key);
        if(candidate) {
            return candidate;
        }
        return prompt_passphrase_interactively("password: ");
    }
}

int execute_edit(edit_options const& options)
{
    if(!has_vault_extension(options.target_path) || !std::filesystem::exists(options.target_path)) {
        std::cerr << "nbias edit: " << options.target_path.string()
                  << " is not an existing vault; use 'nbias enc' for first-time encryption\n";
        return 1;
    }

    auto vault_bytes = read_file_bytes(options.target_path);
    if(!nbias::core::has_vault_header(vault_bytes)) {
        std::cerr << "nbias edit: " << options.target_path.string()
                  << " is not a readable nbias vault; use 'nbias enc' for first-time encryption\n";
        return 1;
    }

    auto header = nbias::core::peek_header(vault_bytes);

    std::optional<std::string> passphrase{};
    if(header.auth == nbias::core::auth_method::password) {
        passphrase = resolve_edit_passphrase(options);
    }
    passphrase_scrubber scrubber{passphrase};

    auto note = nbias::core::decrypt_note(vault_bytes, as_view(passphrase));

    scoped_temp_file temp_file{note.plaintext};

    auto invocation = choose_editor_command(options) + " " + temp_file.path().string();
    auto editor_exit_code = std::system(invocation.c_str());
    if(editor_exit_code != 0) {
        std::cerr << "nbias edit: editor exited with a non-zero status; the vault was left untouched\n";
        return 1;
    }

    auto edited_plaintext = read_file_bytes(temp_file.path());
    if(edited_plaintext == note.plaintext) {
        std::cout << "nbias edit: no changes made\n";
        return 0;
    }

    auto new_vault_bytes = nbias::core::encrypt_note(
        edited_plaintext,
        header.orig_name,
        header.auth,
        header.profile,
        as_view(passphrase));

    write_file_bytes(options.target_path, new_vault_bytes);
    std::cout << "nbias edit: re-encrypted " << options.target_path.string() << '\n';
    return 0;
}
