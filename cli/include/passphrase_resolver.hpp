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

// Best-effort: overwrites the string's buffer with zeros. Not a guarantee against every copy
// a std::string may have been through (reallocations, small-string-optimization moves, etc.),
// but shrinks the window a plaintext password sits in memory after it's no longer needed.
void secure_zero(std::string& value);

// RAII wrapper: calls secure_zero on the referenced optional<string> (if it holds a value)
// when the guard goes out of scope, regardless of how the scope was exited.
class passphrase_scrubber
{
public:
    explicit passphrase_scrubber(std::optional<std::string>& value) : value_{value}
    {
    }

    ~passphrase_scrubber()
    {
        if(value_) {
            secure_zero(*value_);
        }
    }

    passphrase_scrubber(passphrase_scrubber const&) = delete;
    passphrase_scrubber& operator=(passphrase_scrubber const&) = delete;

private:
    std::optional<std::string>& value_;
};

// Same as passphrase_scrubber, but for a plain (always-populated) std::string.
class string_scrubber
{
public:
    explicit string_scrubber(std::string& value) : value_{value}
    {
    }

    ~string_scrubber()
    {
        secure_zero(value_);
    }

    string_scrubber(string_scrubber const&) = delete;
    string_scrubber& operator=(string_scrubber const&) = delete;

private:
    std::string& value_;
};
