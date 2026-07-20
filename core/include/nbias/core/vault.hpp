#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace nbias::core
{
    using byte_buffer = std::vector<unsigned char>;

    inline constexpr std::uint8_t format_version{0x00};
    inline constexpr std::size_t salt_bytes{16};
    inline constexpr std::size_t nonce_bytes{24};
    inline constexpr std::size_t key_bytes{32};

    enum class auth_method : std::uint8_t
    {
        password = 0,
        no_password = 1,
        symmetric_keyfile = 2,
        asymmetric_keyfile = 3,
    };

    enum class kdf_profile : std::uint8_t
    {
        fast = 0,
        balanced = 1,
        hardened = 2,
    };

    class vault_format_error : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    class vault_auth_error : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    struct vault_header
    {
        std::uint8_t version{format_version};
        auth_method auth{auth_method::no_password};
        kdf_profile profile{kdf_profile::fast};
        std::array<std::uint8_t, salt_bytes> salt{};
        std::array<std::uint8_t, nonce_bytes> nonce{};
        std::string orig_name{};
        std::size_t header_size{};
    };

    struct decrypted_note
    {
        byte_buffer plaintext{};
        std::string orig_name{};
        auth_method auth{auth_method::no_password};
    };

    bool has_vault_header(byte_buffer const& bytes);

    vault_header peek_header(byte_buffer const& vault_bytes);

    byte_buffer encrypt_note(
        byte_buffer const& plaintext,
        std::string const& orig_name,
        auth_method auth,
        kdf_profile profile,
        std::optional<std::string_view> passphrase);

    decrypted_note decrypt_note(
        byte_buffer const& vault_bytes,
        std::optional<std::string_view> passphrase);
}
