#pragma once

#include "nbias/core/vault.hpp"

#include <cstddef>

namespace nbias::core::detail
{
    struct kdf_limits
    {
        unsigned long long opslimit;
        std::size_t memlimit;
    };

    kdf_limits select_kdf_limits(kdf_profile profile);
}  // namespace nbias::core::detail
