#include "test_harness.hpp"

#include <nbias/core/vault.hpp>

using nbias::test::run_case;
using namespace nbias::core;

namespace
{
    byte_buffer to_bytes(std::string_view text)
    {
        return byte_buffer(text.begin(), text.end());
    }
}

int main()
{
    run_case("encrypt_note then decrypt_note recovers the plaintext (no password)", [] {
        auto plaintext = to_bytes("hello nbias, this is a secret recipe");
        auto vault = encrypt_note(plaintext, "recipe.md", auth_method::no_password, kdf_profile::fast, std::nullopt);

        NBIAS_CHECK(has_vault_header(vault));

        auto note = decrypt_note(vault, std::nullopt);
        NBIAS_CHECK(note.plaintext == plaintext);
        NBIAS_CHECK(note.orig_name == "recipe.md");
        NBIAS_CHECK(note.auth == auth_method::no_password);
    });

    run_case("encrypt_note then decrypt_note recovers the plaintext (correct password)", [] {
        auto plaintext = to_bytes("line1\nline2\n");
        auto vault = encrypt_note(plaintext, "diary.md", auth_method::password, kdf_profile::fast, std::string_view{"hunter2"});

        auto note = decrypt_note(vault, std::string_view{"hunter2"});
        NBIAS_CHECK(note.plaintext == plaintext);
        NBIAS_CHECK(note.auth == auth_method::password);
    });

    run_case("decrypt_note rejects a wrong password", [] {
        auto plaintext = to_bytes("top secret miso ratio");
        auto vault = encrypt_note(plaintext, "recipe.md", auth_method::password, kdf_profile::fast, std::string_view{"correct-password"});

        bool threw{false};
        try {
            decrypt_note(vault, std::string_view{"wrong-password"});
        }
        catch(vault_auth_error const&) {
            threw = true;
        }
        NBIAS_CHECK(threw);
    });

    run_case("decrypt_note requires a password when the vault is password-protected", [] {
        auto plaintext = to_bytes("needs a password");
        auto vault = encrypt_note(plaintext, "recipe.md", auth_method::password, kdf_profile::fast, std::string_view{"hunter2"});

        bool threw{false};
        try {
            decrypt_note(vault, std::nullopt);
        }
        catch(vault_auth_error const&) {
            threw = true;
        }
        NBIAS_CHECK(threw);
    });

    run_case("two encryptions of the same plaintext use different salt/nonce (ciphertext differs)", [] {
        auto plaintext = to_bytes("same content, twice");
        auto first = encrypt_note(plaintext, "a.md", auth_method::password, kdf_profile::fast, std::string_view{"hunter2"});
        auto second = encrypt_note(plaintext, "a.md", auth_method::password, kdf_profile::fast, std::string_view{"hunter2"});

        NBIAS_CHECK(first != second);

        auto first_header = peek_header(first);
        auto second_header = peek_header(second);
        NBIAS_CHECK(first_header.salt != second_header.salt);
        NBIAS_CHECK(first_header.nonce != second_header.nonce);
    });

    run_case("tampering with the header is detected via the AEAD tag (AAD covers the header)", [] {
        auto plaintext = to_bytes("tamper me not");
        auto vault = encrypt_note(plaintext, "a.md", auth_method::no_password, kdf_profile::fast, std::nullopt);

        // Flip a byte inside orig_name, which is part of the AAD-covered header.
        vault[50] ^= 0xFF;

        bool threw{false};
        try {
            decrypt_note(vault, std::nullopt);
        }
        catch(vault_auth_error const&) {
            threw = true;
        }
        NBIAS_CHECK(threw);
    });

    run_case("tampering with the ciphertext body is detected via the AEAD tag", [] {
        auto plaintext = to_bytes("tamper me not either");
        auto vault = encrypt_note(plaintext, "a.md", auth_method::no_password, kdf_profile::fast, std::nullopt);

        // Flip the last byte, inside the ciphertext/tag region rather than the header.
        vault.back() ^= 0xFF;

        bool threw{false};
        try {
            decrypt_note(vault, std::nullopt);
        }
        catch(vault_auth_error const&) {
            threw = true;
        }
        NBIAS_CHECK(threw);
    });

    run_case("decrypt_note rejects a vault truncated shorter than the minimum AEAD tag size", [] {
        auto plaintext = to_bytes("short");
        auto vault = encrypt_note(plaintext, "a.md", auth_method::no_password, kdf_profile::fast, std::nullopt);
        auto header = peek_header(vault);

        // Keep the header intact but leave fewer than 16 (ABYTES) ciphertext/tag bytes.
        vault.resize(header.header_size + 4);

        bool threw{false};
        try {
            decrypt_note(vault, std::nullopt);
        }
        catch(vault_format_error const&) {
            threw = true;
        }
        NBIAS_CHECK(threw);
    });

    run_case("has_vault_header rejects non-vault bytes", [] {
        auto not_a_vault = to_bytes("just some plain text file, not a vault at all");
        NBIAS_CHECK(!has_vault_header(not_a_vault));
    });

    return nbias::test::report();
}
