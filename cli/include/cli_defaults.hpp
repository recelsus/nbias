#pragma once

#include <array>
#include <string_view>

inline constexpr int max_interactive_password_attempts{3};

inline constexpr int numbered_suffix_digit_width{2};
inline constexpr int numbered_suffix_max_sequence{99};

inline constexpr std::array<std::string_view, 4> fallback_editor_candidates{"nvim", "vim", "vi", "nano"};
