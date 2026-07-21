#pragma once

#include <optional>
#include <string>
#include <string_view>

std::optional<std::string> resolve_passphrase_noninteractive(std::optional<std::string> const& explicit_key);

std::string prompt_passphrase_interactively(std::string const& prompt_text);

std::optional<std::string_view> as_view(std::optional<std::string> const& value);

void secure_zero(std::string& value);

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
