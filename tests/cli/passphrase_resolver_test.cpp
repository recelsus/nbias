#include "test_harness.hpp"

#include "passphrase_resolver.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#include <unistd.h>

using nbias::test::run_case;

namespace
{
    std::filesystem::path make_temp_dir()
    {
        auto dir = std::filesystem::temp_directory_path() / ("nbias_passphrase_test_" + std::to_string(::getpid()));
        std::filesystem::create_directories(dir);
        return dir;
    }

    void write_env(std::filesystem::path const& dir, std::string_view content)
    {
        std::ofstream stream(dir / ".env", std::ios::binary);
        stream << content;
    }

    class cwd_guard
    {
    public:
        cwd_guard() : original_(std::filesystem::current_path())
        {
        }

        ~cwd_guard()
        {
            std::filesystem::current_path(original_);
        }

        cwd_guard(cwd_guard const&) = delete;
        cwd_guard& operator=(cwd_guard const&) = delete;

    private:
        std::filesystem::path original_;
    };
}  // namespace

int main()
{
    auto temp_dir = make_temp_dir();

    run_case("explicit --key wins over .env and the environment variable", [&] {
        cwd_guard guard{};
        auto dir = temp_dir / "explicit_wins";
        std::filesystem::create_directories(dir);
        write_env(dir, "NBIAS_KEY=fromenvfile\n");
        std::filesystem::current_path(dir);
        ::setenv("NBIAS_KEY", "fromenvvar", 1);

        auto result = resolve_passphrase_noninteractive(std::string{"explicit"});
        NBIAS_CHECK(result == "explicit");
        ::unsetenv("NBIAS_KEY");
    });

    run_case(".env NBIAS_KEY wins over the environment variable", [&] {
        cwd_guard guard{};
        auto dir = temp_dir / "dotenv_over_env";
        std::filesystem::create_directories(dir);
        write_env(dir, "NBIAS_KEY=fromenvfile\n");
        std::filesystem::current_path(dir);
        ::setenv("NBIAS_KEY", "fromenvvar", 1);

        auto result = resolve_passphrase_noninteractive(std::nullopt);
        NBIAS_CHECK(result == "fromenvfile");
        ::unsetenv("NBIAS_KEY");
    });

    run_case("the environment variable is used when no .env is present", [&] {
        cwd_guard guard{};
        auto dir = temp_dir / "env_var_only";
        std::filesystem::create_directories(dir);
        std::filesystem::current_path(dir);
        ::setenv("NBIAS_KEY", "fromenvvar", 1);

        auto result = resolve_passphrase_noninteractive(std::nullopt);
        NBIAS_CHECK(result == "fromenvvar");
        ::unsetenv("NBIAS_KEY");
    });

    run_case("resolves to nullopt when nothing is configured", [&] {
        cwd_guard guard{};
        auto dir = temp_dir / "nothing_configured";
        std::filesystem::create_directories(dir);
        std::filesystem::current_path(dir);
        ::unsetenv("NBIAS_KEY");

        auto result = resolve_passphrase_noninteractive(std::nullopt);
        NBIAS_CHECK(!result.has_value());
    });

    run_case("as_view exposes the underlying optional<string> as a string_view", [] {
        std::optional<std::string> value{"hunter2"};
        auto view = as_view(value);
        NBIAS_CHECK(view.has_value());
        NBIAS_CHECK(*view == "hunter2");
        NBIAS_CHECK(!as_view(std::nullopt).has_value());
    });

    std::filesystem::remove_all(temp_dir);
    return nbias::test::report();
}
