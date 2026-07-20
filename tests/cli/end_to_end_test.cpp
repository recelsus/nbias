#include "test_harness.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include <unistd.h>

using nbias::test::run_case;

namespace
{
    std::filesystem::path make_temp_dir()
    {
        auto dir = std::filesystem::temp_directory_path() / ("nbias_e2e_test_" + std::to_string(::getpid()));
        std::filesystem::create_directories(dir);
        return dir;
    }

    void write_file(std::filesystem::path const& path, std::string_view content)
    {
        std::ofstream stream(path, std::ios::binary);
        stream << content;
    }

    std::string read_file(std::filesystem::path const& path)
    {
        std::ifstream stream(path, std::ios::binary);
        return std::string((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    }

    int run_shell(std::filesystem::path const& cwd, std::string const& command_line)
    {
        auto full_command = "cd '" + cwd.string() + "' && " + command_line;
        return std::system(full_command.c_str());
    }

    void make_executable_script(std::filesystem::path const& path, std::string_view body)
    {
        write_file(path, body);
        std::filesystem::permissions(path, std::filesystem::perms::owner_all);
    }
}

int main()
{
    // A real $VISUAL/$EDITOR inherited from the developer's shell would otherwise take
    // priority over the fake per-command EDITOR below and launch a real, blocking editor.
    ::unsetenv("VISUAL");
    ::unsetenv("EDITOR");

    auto temp_dir = make_temp_dir();
    std::string const binary{NBIAS_BINARY_PATH};

    run_case("enc then dec round-trips a plaintext file with no password", [&] {
        auto dir = temp_dir / "no_password";
        std::filesystem::create_directories(dir);
        write_file(dir / "note.md", "hello nbias, this is a secret recipe\n");

        NBIAS_CHECK(run_shell(dir, "'" + binary + "' enc note.md") == 0);
        NBIAS_CHECK(std::filesystem::exists(dir / "note.md.knty"));

        NBIAS_CHECK(run_shell(dir, "'" + binary + "' dec note.md.knty --output-dir restored") == 0);
        NBIAS_CHECK(read_file(dir / "restored" / "note.md") == "hello nbias, this is a secret recipe\n");
    });

    run_case("enc then dec round-trips a password-protected file", [&] {
        auto dir = temp_dir / "with_password";
        std::filesystem::create_directories(dir);
        write_file(dir / "note.md", "line one\nline two\n");

        NBIAS_CHECK(run_shell(dir, "'" + binary + "' enc note.md -K --key hunter2 --output-dir pw_out") == 0);
        NBIAS_CHECK(run_shell(dir, "'" + binary + "' dec pw_out/note.md.knty --key hunter2 --output-dir pw_restored") == 0);
        NBIAS_CHECK(read_file(dir / "pw_restored" / "note.md") == "line one\nline two\n");
    });

    run_case("dec with the wrong password fails", [&] {
        auto dir = temp_dir / "wrong_password";
        std::filesystem::create_directories(dir);
        write_file(dir / "note.md", "top secret\n");

        NBIAS_CHECK(run_shell(dir, "'" + binary + "' enc note.md -K --key correct --output-dir pw_out") == 0);
        NBIAS_CHECK(run_shell(dir, "'" + binary + "' dec pw_out/note.md.knty --key wrong --output-dir out < /dev/null") != 0);
    });

    run_case("edit re-encrypts through the same core engine enc/dec share", [&] {
        auto dir = temp_dir / "edit_roundtrip";
        std::filesystem::create_directories(dir);
        write_file(dir / "note.md", "original line\n");
        make_executable_script(dir / "append.sh", "#!/bin/sh\necho \"appended by test\" >> \"$1\"\n");

        NBIAS_CHECK(run_shell(dir, "'" + binary + "' enc note.md -K --key hunter2 --output-dir vault") == 0);
        NBIAS_CHECK(run_shell(dir, "EDITOR='" + (dir / "append.sh").string() + "' '" + binary + "' edit vault/note.md.knty --key hunter2") == 0);
        NBIAS_CHECK(run_shell(dir, "'" + binary + "' dec vault/note.md.knty --key hunter2 --output-dir restored") == 0);
        NBIAS_CHECK(read_file(dir / "restored" / "note.md") == "original line\nappended by test\n");
    });

    run_case("info reports auth method without needing a password", [&] {
        auto dir = temp_dir / "info_check";
        std::filesystem::create_directories(dir);
        write_file(dir / "note.md", "content\n");

        NBIAS_CHECK(run_shell(dir, "'" + binary + "' enc note.md -K --key hunter2 --output-dir vault") == 0);
        NBIAS_CHECK(run_shell(dir, "'" + binary + "' info vault/note.md.knty > info_output.txt") == 0);
        auto output = read_file(dir / "info_output.txt");
        NBIAS_CHECK(output.find("auth method:") != std::string::npos);
        NBIAS_CHECK(output.find("password") != std::string::npos);
    });

    std::filesystem::remove_all(temp_dir);
    return nbias::test::report();
}
