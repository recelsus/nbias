#include "kdf_params.hpp"

#include <sodium.h>

#include <stdexcept>

namespace nbias::core::detail
{
    namespace
    {
        constexpr unsigned long long fast_opslimit{crypto_pwhash_OPSLIMIT_INTERACTIVE};
        constexpr std::size_t fast_memlimit{crypto_pwhash_MEMLIMIT_INTERACTIVE};

        constexpr unsigned long long balanced_opslimit{crypto_pwhash_OPSLIMIT_MODERATE};
        constexpr std::size_t balanced_memlimit{crypto_pwhash_MEMLIMIT_MODERATE};

        constexpr unsigned long long hardened_opslimit{crypto_pwhash_OPSLIMIT_SENSITIVE};
        constexpr std::size_t hardened_memlimit{crypto_pwhash_MEMLIMIT_SENSITIVE};
    }  // namespace

    kdf_limits select_kdf_limits(kdf_profile profile)
    {
        switch(profile) {
        case kdf_profile::fast:
            return {fast_opslimit, fast_memlimit};
        case kdf_profile::balanced:
            return {balanced_opslimit, balanced_memlimit};
        case kdf_profile::hardened:
            return {hardened_opslimit, hardened_memlimit};
        }
        throw std::logic_error("invalid kdf_profile");
    }
}  // namespace nbias::core::detail
