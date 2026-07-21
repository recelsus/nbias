#include <nbias/core/vault.hpp>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <optional>
#include <string>
#include <string_view>

#include <emscripten.h>

namespace
{
    std::string g_last_error{};

    void set_last_error(std::string message)
    {
        g_last_error = std::move(message);
    }

    enum nbias_wasm_status : int
    {
        nbias_wasm_ok = 0,
        nbias_wasm_format_error = 1,
        nbias_wasm_auth_error = 2,
        nbias_wasm_internal_error = 3,
    };

    unsigned char* dup_to_malloc(nbias::core::byte_buffer const& data)
    {
        auto* buffer = static_cast<unsigned char*>(std::malloc(data.empty() ? 1 : data.size()));
        if(buffer != nullptr && !data.empty()) {
            std::memcpy(buffer, data.data(), data.size());
        }
        return buffer;
    }

    char* dup_string_to_malloc(std::string const& text)
    {
        auto* buffer = static_cast<char*>(std::malloc(text.size() + 1));
        if(buffer != nullptr) {
            std::memcpy(buffer, text.c_str(), text.size() + 1);
        }
        return buffer;
    }

    std::optional<std::string_view> to_passphrase_view(char const* passphrase)
    {
        if(passphrase == nullptr) {
            return std::nullopt;
        }
        return std::string_view{passphrase};
    }
}  // namespace

extern "C"
{
    EMSCRIPTEN_KEEPALIVE
    void nbias_wasm_free_buffer(void* ptr)
    {
        std::free(ptr);
    }

    EMSCRIPTEN_KEEPALIVE
    char const* nbias_wasm_last_error_message()
    {
        return g_last_error.c_str();
    }

    EMSCRIPTEN_KEEPALIVE
    int nbias_wasm_has_vault_header(unsigned char const* bytes, std::size_t len)
    {
        nbias::core::byte_buffer buffer(bytes, bytes + len);
        return nbias::core::has_vault_header(buffer) ? 1 : 0;
    }

    EMSCRIPTEN_KEEPALIVE
    int nbias_wasm_peek_header(
        unsigned char const* vault_bytes, std::size_t vault_len,
        std::uint8_t* out_version,
        std::uint8_t* out_auth_method,
        std::uint8_t* out_kdf_profile,
        char** out_orig_name)
    {
        try {
            nbias::core::byte_buffer buffer(vault_bytes, vault_bytes + vault_len);
            auto header = nbias::core::peek_header(buffer);

            *out_version = header.version;
            *out_auth_method = static_cast<std::uint8_t>(header.auth);
            *out_kdf_profile = static_cast<std::uint8_t>(header.profile);
            *out_orig_name = dup_string_to_malloc(header.orig_name);
            return nbias_wasm_ok;
        }
        catch(nbias::core::vault_format_error const& error) {
            set_last_error(error.what());
            return nbias_wasm_format_error;
        }
        catch(std::exception const& error) {
            set_last_error(error.what());
            return nbias_wasm_internal_error;
        }
    }

    EMSCRIPTEN_KEEPALIVE
    int nbias_wasm_encrypt_note(
        unsigned char const* plaintext, std::size_t plaintext_len,
        char const* orig_name,
        std::uint8_t auth_method,
        std::uint8_t kdf_profile,
        char const* passphrase,
        unsigned char** out_vault_bytes,
        std::size_t* out_vault_len)
    {
        try {
            nbias::core::byte_buffer plaintext_buffer(plaintext, plaintext + plaintext_len);

            auto vault = nbias::core::encrypt_note(
                plaintext_buffer,
                std::string{orig_name},
                static_cast<nbias::core::auth_method>(auth_method),
                static_cast<nbias::core::kdf_profile>(kdf_profile),
                to_passphrase_view(passphrase));

            *out_vault_bytes = dup_to_malloc(vault);
            *out_vault_len = vault.size();
            return nbias_wasm_ok;
        }
        catch(nbias::core::vault_auth_error const& error) {
            set_last_error(error.what());
            return nbias_wasm_auth_error;
        }
        catch(nbias::core::vault_format_error const& error) {
            set_last_error(error.what());
            return nbias_wasm_format_error;
        }
        catch(std::exception const& error) {
            set_last_error(error.what());
            return nbias_wasm_internal_error;
        }
    }

    EMSCRIPTEN_KEEPALIVE
    int nbias_wasm_decrypt_note(
        unsigned char const* vault_bytes, std::size_t vault_len,
        char const* passphrase,
        unsigned char** out_plaintext, std::size_t* out_plaintext_len,
        char** out_orig_name,
        std::uint8_t* out_auth_method)
    {
        try {
            nbias::core::byte_buffer buffer(vault_bytes, vault_bytes + vault_len);

            auto note = nbias::core::decrypt_note(buffer, to_passphrase_view(passphrase));

            *out_plaintext = dup_to_malloc(note.plaintext);
            *out_plaintext_len = note.plaintext.size();
            *out_orig_name = dup_string_to_malloc(note.orig_name);
            *out_auth_method = static_cast<std::uint8_t>(note.auth);
            return nbias_wasm_ok;
        }
        catch(nbias::core::vault_auth_error const& error) {
            set_last_error(error.what());
            return nbias_wasm_auth_error;
        }
        catch(nbias::core::vault_format_error const& error) {
            set_last_error(error.what());
            return nbias_wasm_format_error;
        }
        catch(std::exception const& error) {
            set_last_error(error.what());
            return nbias_wasm_internal_error;
        }
    }
}

int main()
{
    return 0;
}
