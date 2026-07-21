#include "test_harness.hpp"

#include "vault_codec.hpp"

using nbias::test::run_case;
using namespace nbias::core;

namespace
{
    std::array<std::uint8_t, salt_bytes> make_salt()
    {
        std::array<std::uint8_t, salt_bytes> salt{};
        for(std::size_t index = 0; index < salt.size(); ++index) {
            salt[index] = static_cast<std::uint8_t>(index);
        }
        return salt;
    }

    std::array<std::uint8_t, nonce_bytes> make_nonce()
    {
        std::array<std::uint8_t, nonce_bytes> nonce{};
        for(std::size_t index = 0; index < nonce.size(); ++index) {
            nonce[index] = static_cast<std::uint8_t>(0xF0 + index);
        }
        return nonce;
    }
}  // namespace

int main()
{
    run_case("serialize_header then parse_header round-trips every field", [] {
        auto salt = make_salt();
        auto nonce = make_nonce();
        auto header_bytes = detail::serialize_header(auth_method::password, kdf_profile::hardened, salt, nonce, "recipe.md");

        auto header = detail::parse_header(header_bytes);
        NBIAS_CHECK(header.version == format_version);
        NBIAS_CHECK(header.auth == auth_method::password);
        NBIAS_CHECK(header.profile == kdf_profile::hardened);
        NBIAS_CHECK(header.salt == salt);
        NBIAS_CHECK(header.nonce == nonce);
        NBIAS_CHECK(header.orig_name == "recipe.md");
        NBIAS_CHECK(header.header_size == header_bytes.size());
    });

    run_case("parse_header rejects a buffer shorter than the fixed header", [] {
        byte_buffer too_short(detail::fixed_header_size - 1, 0);
        bool threw{false};
        try {
            detail::parse_header(too_short);
        }
        catch(vault_format_error const&) {
            threw = true;
        }
        NBIAS_CHECK(threw);
    });

    run_case("parse_header rejects a bad magic", [] {
        auto header_bytes = detail::serialize_header(auth_method::no_password, kdf_profile::fast, make_salt(), make_nonce(), "a.md");
        header_bytes[0] = static_cast<std::uint8_t>('X');

        bool threw{false};
        try {
            detail::parse_header(header_bytes);
        }
        catch(vault_format_error const&) {
            threw = true;
        }
        NBIAS_CHECK(threw);
    });

    run_case("parse_header rejects an unsupported format version", [] {
        auto header_bytes = detail::serialize_header(auth_method::no_password, kdf_profile::fast, make_salt(), make_nonce(), "a.md");
        header_bytes[4] = 0x7F;

        bool threw{false};
        try {
            detail::parse_header(header_bytes);
        }
        catch(vault_format_error const&) {
            threw = true;
        }
        NBIAS_CHECK(threw);
    });

    run_case("parse_header rejects an out-of-range auth_method byte", [] {
        auto header_bytes = detail::serialize_header(auth_method::no_password, kdf_profile::fast, make_salt(), make_nonce(), "a.md");
        header_bytes[5] = 0xFF;

        bool threw{false};
        try {
            detail::parse_header(header_bytes);
        }
        catch(vault_format_error const&) {
            threw = true;
        }
        NBIAS_CHECK(threw);
    });

    run_case("parse_header rejects a non-zero reserved flags byte", [] {
        auto header_bytes = detail::serialize_header(auth_method::no_password, kdf_profile::fast, make_salt(), make_nonce(), "a.md");
        header_bytes[7] = 0x01;

        bool threw{false};
        try {
            detail::parse_header(header_bytes);
        }
        catch(vault_format_error const&) {
            threw = true;
        }
        NBIAS_CHECK(threw);
    });

    run_case("parse_header rejects an orig_name_len longer than the remaining buffer", [] {
        auto header_bytes = detail::serialize_header(auth_method::no_password, kdf_profile::fast, make_salt(), make_nonce(), "a.md");
        header_bytes.resize(detail::fixed_header_size);
        header_bytes[48] = 0xFF;
        header_bytes[49] = 0xFF;

        bool threw{false};
        try {
            detail::parse_header(header_bytes);
        }
        catch(vault_format_error const&) {
            threw = true;
        }
        NBIAS_CHECK(threw);
    });

    run_case("serialize_header rejects an original name over 65535 bytes", [] {
        std::string oversized_name(70000, 'a');

        bool threw{false};
        try {
            detail::serialize_header(auth_method::no_password, kdf_profile::fast, make_salt(), make_nonce(), oversized_name);
        }
        catch(vault_format_error const&) {
            threw = true;
        }
        NBIAS_CHECK(threw);
    });

    return nbias::test::report();
}
