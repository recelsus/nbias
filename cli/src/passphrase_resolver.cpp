#include "passphrase_resolver.hpp"

#include "env_file.hpp"

#include <sodium.h>

#include <cstdlib>
#include <filesystem>
#include <iostream>

#include <termios.h>
#include <unistd.h>

std::optional<std::string> resolve_passphrase_noninteractive(std::optional<std::string> const& explicit_key)
{
    if(explicit_key) {
        return explicit_key;
    }

    auto env = load_env_file(std::filesystem::current_path());
    if(env.nbias_key) {
        return env.nbias_key;
    }

    if(auto const* from_environment = std::getenv("NBIAS_KEY"); from_environment != nullptr) {
        return std::string{from_environment};
    }

    return std::nullopt;
}

namespace
{
    class echo_guard
    {
    public:
        echo_guard()
        {
            termios settings{};
            if(tcgetattr(STDIN_FILENO, &settings) != 0) {
                return;
            }
            original_ = settings;
            settings.c_lflag &= ~static_cast<tcflag_t>(ECHO);
            active_ = tcsetattr(STDIN_FILENO, TCSAFLUSH, &settings) == 0;
        }

        ~echo_guard()
        {
            if(active_) {
                tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_);
            }
        }

        echo_guard(echo_guard const&) = delete;
        echo_guard& operator=(echo_guard const&) = delete;

    private:
        termios original_{};
        bool active_{false};
    };
}

std::string prompt_passphrase_interactively(std::string const& prompt_text)
{
    std::cout << prompt_text;
    std::cout.flush();

    std::string passphrase{};
    {
        echo_guard guard{};
        std::getline(std::cin, passphrase);
    }
    std::cout << '\n';
    return passphrase;
}

std::optional<std::string_view> as_view(std::optional<std::string> const& value)
{
    if(value) {
        return std::string_view{*value};
    }
    return std::nullopt;
}

void secure_zero(std::string& value)
{
    if(!value.empty()) {
        sodium_memzero(value.data(), value.size());
    }
}
