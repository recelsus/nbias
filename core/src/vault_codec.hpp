#pragma once

#include "nbias/core/vault.hpp"

namespace nbias::core::detail
{
    inline constexpr std::size_t magic_size{4};
    inline constexpr char magic[magic_size] = {'K', 'N', 'T', 'Y'};
    inline constexpr std::size_t fixed_header_size{50};

    byte_buffer serialize_header(
        auth_method auth,
        kdf_profile profile,
        std::array<std::uint8_t, salt_bytes> const& salt,
        std::array<std::uint8_t, nonce_bytes> const& nonce,
        std::string const& orig_name);

    vault_header parse_header(byte_buffer const& vault_bytes);
}  // namespace nbias::core::detail
