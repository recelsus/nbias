#include "util.hpp"

#include <stdexcept>

std::string to_string(delete_source_mode mode)
{
    switch(mode) {
    case delete_source_mode::ask:
        return "ask";
    case delete_source_mode::yes:
        return "yes";
    case delete_source_mode::no:
        return "no";
    }
    throw std::logic_error("invalid delete_source_mode");
}

std::string to_string(kdf_profile profile)
{
    switch(profile) {
    case kdf_profile::fast:
        return "fast";
    case kdf_profile::balanced:
        return "balanced";
    case kdf_profile::hardened:
        return "hardened";
    }
    throw std::logic_error("invalid kdf_profile");
}

std::string to_string(tmpfs_mode mode)
{
    switch(mode) {
    case tmpfs_mode::prefer:
        return "prefer";
    case tmpfs_mode::never:
        return "never";
    }
    throw std::logic_error("invalid tmpfs_mode");
}

