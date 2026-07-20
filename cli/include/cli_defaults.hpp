#pragma once

#include <array>
#include <string_view>

// Internal policy constants: not exposed as CLI flags, but shared across command
// implementations so the values stay in one place. Provisional defaults for the open
// items tracked in reference/requirements.md sections 2/3/6.

inline constexpr int max_interactive_password_attempts{3};

inline constexpr int numbered_suffix_digit_width{2};
inline constexpr int numbered_suffix_max_sequence{99};

// Tried in order once neither --editor nor $VISUAL/$EDITOR resolved to anything (see
// choose_editor_command in edit_command.cpp): the first one found on $PATH wins.
inline constexpr std::array<std::string_view, 4> fallback_editor_candidates{"nvim", "vim", "vi", "nano"};
