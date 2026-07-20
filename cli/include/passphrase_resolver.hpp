#pragma once

#include <optional>
#include <string>
#include <string_view>

// Resolution order per reference/requirements.md section 6:
//   1. explicit --key/-k value
//   2. .env NBIAS_KEY (current directory)
//   3. NBIAS_KEY environment variable
// Returns nullopt if none of the three are set; callers fall back to an interactive prompt.
std::optional<std::string> resolve_passphrase_noninteractive(std::optional<std::string> const& explicit_key);

// Reads a line from stdin with echo disabled (POSIX only; Windows support is a known gap,
// see requirements.md section 3/10).
std::string prompt_passphrase_interactively(std::string const& prompt_text);

std::optional<std::string_view> as_view(std::optional<std::string> const& value);
