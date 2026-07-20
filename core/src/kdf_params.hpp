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

    // Not user-configurable (reference/requirements.md section 9 confirms nbias's robustness
    // target is "not plainly readable", not attacker resistance): each profile maps directly
    // to libsodium's own named crypto_pwhash preset rather than custom-tuned limits.
    kdf_limits select_kdf_limits(kdf_profile profile);
}
