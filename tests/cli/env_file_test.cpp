#include "test_harness.hpp"

#include "env_file.hpp"

#include <fstream>
#include <string>

#include <unistd.h>

using nbias::test::run_case;

namespace
{
    std::filesystem::path make_temp_dir()
    {
        auto dir = std::filesystem::temp_directory_path() / ("nbias_env_file_test_" + std::to_string(::getpid()));
        std::filesystem::create_directories(dir);
        return dir;
    }

    void write_env(std::filesystem::path const& dir, std::string_view content)
    {
        std::ofstream stream(dir / ".env", std::ios::binary);
        stream << content;
    }
}  // namespace

int main()
{
    auto temp_dir = make_temp_dir();

    run_case("load_env_file returns all-nullopt when no .env is present", [&] {
        auto empty_dir = temp_dir / "no_env";
        std::filesystem::create_directories(empty_dir);
        auto env = load_env_file(empty_dir);
        NBIAS_CHECK(!env.output_dir);
        NBIAS_CHECK(!env.input_dir);
        NBIAS_CHECK(!env.nbias_key);
    });

    run_case("load_env_file parses OUTPUT / INPUT / NBIAS_KEY", [&] {
        auto dir = temp_dir / "basic";
        std::filesystem::create_directories(dir);
        write_env(dir, "OUTPUT=encrypted\nINPUT=.\nNBIAS_KEY=hunter2\n");

        auto env = load_env_file(dir);
        NBIAS_CHECK(env.output_dir == "encrypted");
        NBIAS_CHECK(env.input_dir == ".");
        NBIAS_CHECK(env.nbias_key == "hunter2");
    });

    run_case("load_env_file ignores blank lines and # comments", [&] {
        auto dir = temp_dir / "comments";
        std::filesystem::create_directories(dir);
        write_env(dir, "# a comment\n\nOUTPUT=encrypted\n   \n# NBIAS_KEY=should-not-apply\n");

        auto env = load_env_file(dir);
        NBIAS_CHECK(env.output_dir == "encrypted");
        NBIAS_CHECK(!env.nbias_key);
    });

    run_case("load_env_file strips matching surrounding quotes", [&] {
        auto dir = temp_dir / "quotes";
        std::filesystem::create_directories(dir);
        write_env(dir, "NBIAS_KEY=\"hunter2\"\nOUTPUT='encrypted dir'\n");

        auto env = load_env_file(dir);
        NBIAS_CHECK(env.nbias_key == "hunter2");
        NBIAS_CHECK(env.output_dir == "encrypted dir");
    });

    run_case("load_env_file trims surrounding whitespace around key and value", [&] {
        auto dir = temp_dir / "whitespace";
        std::filesystem::create_directories(dir);
        write_env(dir, "  OUTPUT  =   encrypted  \n");

        auto env = load_env_file(dir);
        NBIAS_CHECK(env.output_dir == "encrypted");
    });

    std::filesystem::remove_all(temp_dir);
    return nbias::test::report();
}
