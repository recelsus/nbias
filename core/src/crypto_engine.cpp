#include "nbias/core/vault.hpp"
#include "kdf_params.hpp"
#include "vault_codec.hpp"

#include <sodium.h>

#include <cstring>
#include <mutex>
#include <stdexcept>
#include <utility>

namespace nbias::core
{
    namespace
    {
        void ensure_sodium_ready()
        {
            static std::once_flag once{};
            std::call_once(once, [] {
                if(sodium_init() < 0) {
                    throw std::runtime_error("failed to initialize libsodium");
                }
            });
        }

        // Not a secret: this is the fixed key for auth_method::no_password.
        // Anyone with the nbias binary (or this source) can derive it; the
        // no-password mode is a viewing deterrent, not access control (see
        // requirements.md section 7/9).
        std::array<std::uint8_t, key_bytes> const& embedded_key()
        {
            static constexpr std::array<std::uint8_t, key_bytes> key{
                0xd8, 0xee, 0x38, 0xf9, 0x87, 0x3a, 0x94, 0x3a,
                0x0f, 0x26, 0xe4, 0xf3, 0x64, 0x28, 0x8c, 0xbd,
                0x99, 0xe0, 0xe8, 0x42, 0x06, 0x62, 0x74, 0xb6,
                0x2f, 0x4d, 0x9d, 0xf2, 0x59, 0x33, 0x6b, 0xee,
            };
            return key;
        }

        std::array<std::uint8_t, key_bytes> derive_key_from_passphrase(
            std::string_view passphrase,
            std::array<std::uint8_t, salt_bytes> const& salt,
            kdf_profile profile)
        {
            auto limits = detail::select_kdf_limits(profile);

            std::array<std::uint8_t, key_bytes> key{};
            auto result = crypto_pwhash(
                key.data(), key.size(),
                passphrase.data(), passphrase.size(),
                salt.data(),
                limits.opslimit, limits.memlimit,
                crypto_pwhash_ALG_ARGON2ID13);
            if(result != 0) {
                throw std::runtime_error("key derivation failed (insufficient memory for kdf_profile?)");
            }
            return key;
        }

        std::array<std::uint8_t, key_bytes> resolve_key(
            auth_method auth,
            kdf_profile profile,
            std::array<std::uint8_t, salt_bytes> const& salt,
            std::optional<std::string_view> passphrase)
        {
            if(auth == auth_method::no_password) {
                return embedded_key();
            }
            if(auth != auth_method::password) {
                throw vault_format_error("unsupported auth_method for this build");
            }
            if(!passphrase) {
                throw vault_auth_error("this vault is password protected but no password was supplied");
            }
            return derive_key_from_passphrase(*passphrase, salt, profile);
        }
    }

    bool has_vault_header(byte_buffer const& bytes)
    {
        return bytes.size() >= detail::fixed_header_size
            && std::memcmp(bytes.data(), detail::magic, detail::magic_size) == 0;
    }

    vault_header peek_header(byte_buffer const& vault_bytes)
    {
        return detail::parse_header(vault_bytes);
    }

    byte_buffer encrypt_note(
        byte_buffer const& plaintext,
        std::string const& orig_name,
        auth_method auth,
        kdf_profile profile,
        std::optional<std::string_view> passphrase)
    {
        ensure_sodium_ready();

        std::array<std::uint8_t, salt_bytes> salt{};
        std::array<std::uint8_t, nonce_bytes> nonce{};
        randombytes_buf(salt.data(), salt.size());
        randombytes_buf(nonce.data(), nonce.size());

        auto key = resolve_key(auth, profile, salt, passphrase);
        auto header = detail::serialize_header(auth, profile, salt, nonce, orig_name);

        byte_buffer ciphertext(plaintext.size() + crypto_aead_xchacha20poly1305_ietf_ABYTES);
        unsigned long long ciphertext_len{};
        crypto_aead_xchacha20poly1305_ietf_encrypt(
            ciphertext.data(), &ciphertext_len,
            plaintext.data(), plaintext.size(),
            header.data(), header.size(),
            nullptr,
            nonce.data(), key.data());
        ciphertext.resize(ciphertext_len);

        byte_buffer vault_bytes{};
        vault_bytes.reserve(header.size() + ciphertext.size());
        vault_bytes.insert(vault_bytes.end(), header.begin(), header.end());
        vault_bytes.insert(vault_bytes.end(), ciphertext.begin(), ciphertext.end());
        return vault_bytes;
    }

    decrypted_note decrypt_note(
        byte_buffer const& vault_bytes,
        std::optional<std::string_view> passphrase)
    {
        ensure_sodium_ready();

        auto header = detail::parse_header(vault_bytes);

        auto ciphertext_len = vault_bytes.size() - header.header_size;
        if(ciphertext_len < crypto_aead_xchacha20poly1305_ietf_ABYTES) {
            throw vault_format_error("ciphertext shorter than the minimum AEAD tag size");
        }

        auto key = resolve_key(header.auth, header.profile, header.salt, passphrase);
        auto const* ciphertext = vault_bytes.data() + header.header_size;

        byte_buffer plaintext(ciphertext_len);
        unsigned long long plaintext_len{};
        auto result = crypto_aead_xchacha20poly1305_ietf_decrypt(
            plaintext.data(), &plaintext_len,
            nullptr,
            ciphertext, ciphertext_len,
            vault_bytes.data(), header.header_size,
            header.nonce.data(), key.data());
        if(result != 0) {
            throw vault_auth_error("decryption failed: wrong password or corrupted vault");
        }
        plaintext.resize(plaintext_len);

        return decrypted_note{std::move(plaintext), header.orig_name, header.auth};
    }
}
