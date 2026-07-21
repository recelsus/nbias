#include "vault_codec.hpp"

#include <algorithm>
#include <cstring>

namespace nbias::core::detail
{
    namespace
    {
        void append_u16_le(byte_buffer& out, std::uint16_t value)
        {
            out.push_back(static_cast<std::uint8_t>(value & 0xFF));
            out.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));
        }

        std::uint16_t read_u16_le(std::uint8_t const* data)
        {
            return static_cast<std::uint16_t>(data[0]) | static_cast<std::uint16_t>(data[1] << 8);
        }

        bool is_known_auth_method(std::uint8_t value)
        {
            return value <= static_cast<std::uint8_t>(auth_method::asymmetric_keyfile);
        }

        bool is_known_kdf_profile(std::uint8_t value)
        {
            return value <= static_cast<std::uint8_t>(kdf_profile::hardened);
        }
    }  // namespace

    byte_buffer serialize_header(
        auth_method auth,
        kdf_profile profile,
        std::array<std::uint8_t, salt_bytes> const& salt,
        std::array<std::uint8_t, nonce_bytes> const& nonce,
        std::string const& orig_name)
    {
        if(orig_name.size() > 0xFFFF) {
            throw vault_format_error("original file name too long");
        }

        byte_buffer out{};
        out.reserve(fixed_header_size + orig_name.size());
        out.insert(out.end(), magic, magic + magic_size);
        out.push_back(format_version);
        out.push_back(static_cast<std::uint8_t>(auth));
        out.push_back(static_cast<std::uint8_t>(profile));
        out.push_back(0);
        out.insert(out.end(), salt.begin(), salt.end());
        out.insert(out.end(), nonce.begin(), nonce.end());
        append_u16_le(out, static_cast<std::uint16_t>(orig_name.size()));
        out.insert(out.end(), orig_name.begin(), orig_name.end());
        return out;
    }

    vault_header parse_header(byte_buffer const& vault_bytes)
    {
        if(vault_bytes.size() < fixed_header_size) {
            throw vault_format_error("vault file too small to contain a header");
        }
        if(std::memcmp(vault_bytes.data(), magic, magic_size) != 0) {
            throw vault_format_error("not a nbias vault file (bad magic)");
        }

        vault_header header{};
        header.version = vault_bytes[4];
        if(header.version != format_version) {
            throw vault_format_error("unsupported vault format version");
        }

        auto auth_value = vault_bytes[5];
        if(!is_known_auth_method(auth_value)) {
            throw vault_format_error("unknown auth_method in vault header");
        }
        header.auth = static_cast<auth_method>(auth_value);

        auto profile_value = vault_bytes[6];
        if(!is_known_kdf_profile(profile_value)) {
            throw vault_format_error("unknown kdf_profile in vault header");
        }
        header.profile = static_cast<kdf_profile>(profile_value);

        if(vault_bytes[7] != 0) {
            throw vault_format_error("unknown flag bits set in vault header");
        }

        std::copy_n(vault_bytes.begin() + 8, salt_bytes, header.salt.begin());
        std::copy_n(vault_bytes.begin() + 24, nonce_bytes, header.nonce.begin());

        auto orig_name_len = read_u16_le(vault_bytes.data() + 48);
        std::size_t orig_name_begin{fixed_header_size};
        if(vault_bytes.size() < orig_name_begin + orig_name_len) {
            throw vault_format_error("vault header declares a name longer than the file");
        }
        header.orig_name.assign(
            reinterpret_cast<char const*>(vault_bytes.data() + orig_name_begin),
            orig_name_len);
        header.header_size = orig_name_begin + orig_name_len;

        return header;
    }
}  // namespace nbias::core::detail
