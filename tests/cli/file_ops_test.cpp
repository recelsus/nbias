#include "test_harness.hpp"

#include "file_ops.hpp"

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include <unistd.h>

using nbias::test::run_case;

namespace
{
    std::filesystem::path make_temp_dir()
    {
        auto dir = std::filesystem::temp_directory_path() / ("nbias_file_ops_test_" + std::to_string(::getpid()));
        std::filesystem::create_directories(dir);
        return dir;
    }

    void write_file(std::filesystem::path const& path, std::string_view content)
    {
        std::ofstream stream(path, std::ios::binary);
        stream << content;
    }

    std::vector<unsigned char> to_bytes(std::string_view text)
    {
        return std::vector<unsigned char>(text.begin(), text.end());
    }

    void feed_stdin(std::filesystem::path const& temp_dir, std::string_view answer)
    {
        auto answer_path = temp_dir / "stdin_answer.txt";
        write_file(answer_path, answer);
        if(std::freopen(answer_path.c_str(), "r", stdin) == nullptr) {
            throw std::runtime_error("failed to redirect stdin for the test");
        }
    }
}

int main()
{
    auto temp_dir = make_temp_dir();

    run_case("make_vault_output_path: --output-dir flattens (drops subdirectories)", [] {
        auto result = make_vault_output_path("docs/notes/file.md", std::filesystem::path{"out"}, std::nullopt, "/base");
        NBIAS_CHECK(result == std::filesystem::path{"out/file.md.knty"});
    });

    run_case("make_vault_output_path: .env OUTPUT reproduces subdirectories under base_dir", [] {
        auto result = make_vault_output_path(std::filesystem::path{"/base/docs/notes/file.md"}, std::nullopt, std::string{"encrypted"}, "/base");
        NBIAS_CHECK(result.lexically_normal() == std::filesystem::path{"/base/encrypted/docs/notes/file.md.knty"}.lexically_normal());
    });

    run_case("make_vault_output_path: falls back to the input file's own directory", [] {
        auto result = make_vault_output_path(std::filesystem::path{"/base/docs/file.md"}, std::nullopt, std::nullopt, "/base");
        NBIAS_CHECK(result == std::filesystem::path{"/base/docs/file.md.knty"});
    });

    run_case("make_plain_output_dir: --output-dir flattens", [] {
        auto result = make_plain_output_dir(
            std::filesystem::path{"/base/encrypted/docs/file.md.knty"}, std::filesystem::path{"restored"}, std::string{"encrypted"}, std::string{"restored"}, "/base");
        NBIAS_CHECK(result == std::filesystem::path{"restored"});
    });

    run_case("make_plain_output_dir: .env OUTPUT/INPUT inverts the enc-side subdirectory mapping", [] {
        auto result = make_plain_output_dir(
            std::filesystem::path{"/base/encrypted/docs/notes/file.md.knty"}, std::nullopt, std::string{"encrypted"}, std::string{"restored"}, "/base");
        NBIAS_CHECK(result.lexically_normal() == std::filesystem::path{"/base/restored/docs/notes"}.lexically_normal());
    });

    run_case("make_plain_output_dir: falls back to the vault file's own directory", [] {
        auto result = make_plain_output_dir(std::filesystem::path{"/base/docs/file.md.knty"}, std::nullopt, std::nullopt, std::nullopt, "/base");
        NBIAS_CHECK(result == std::filesystem::path{"/base/docs"});
    });

    run_case("choose_decrypt_write_path: missing target writes directly, no prompt", [&] {
        auto target = temp_dir / "missing.md";
        auto result = choose_decrypt_write_path(target, to_bytes("hello"));
        NBIAS_CHECK(result == target);
    });

    run_case("choose_decrypt_write_path: identical content overwrites silently, no prompt", [&] {
        auto target = temp_dir / "same.md";
        write_file(target, "same content");
        auto result = choose_decrypt_write_path(target, to_bytes("same content"));
        NBIAS_CHECK(result == target);
    });

    run_case("choose_decrypt_write_path: differing content + 'y' overwrites in place", [&] {
        auto target = temp_dir / "differs_yes.md";
        write_file(target, "old content");
        feed_stdin(temp_dir, "y\n");
        auto result = choose_decrypt_write_path(target, to_bytes("new content"));
        NBIAS_CHECK(result == target);
    });

    run_case("choose_decrypt_write_path: differing content + 'N' picks a numbered sibling", [&] {
        auto target = temp_dir / "differs_no.md";
        write_file(target, "old content");
        feed_stdin(temp_dir, "N\n");
        auto result = choose_decrypt_write_path(target, to_bytes("new content"));
        NBIAS_CHECK(result == temp_dir / "differs_no.01.md");
        NBIAS_CHECK(!std::filesystem::exists(result));
    });

    run_case("sanitize_orig_name strips directory components from a plain relative path", [] {
        NBIAS_CHECK(sanitize_orig_name("subdir/file.md") == std::filesystem::path{"file.md"});
    });

    run_case("sanitize_orig_name strips parent-directory traversal", [] {
        NBIAS_CHECK(sanitize_orig_name("../../etc/passwd") == std::filesystem::path{"passwd"});
    });

    run_case("sanitize_orig_name strips a leading absolute path", [] {
        NBIAS_CHECK(sanitize_orig_name("/etc/passwd") == std::filesystem::path{"passwd"});
    });

    run_case("sanitize_orig_name rejects a name that is only '..'", [] {
        bool threw{false};
        try {
            sanitize_orig_name("..");
        }
        catch(std::runtime_error const&) {
            threw = true;
        }
        NBIAS_CHECK(threw);
    });

    run_case("sanitize_orig_name rejects a name that is only '.'", [] {
        bool threw{false};
        try {
            sanitize_orig_name(".");
        }
        catch(std::runtime_error const&) {
            threw = true;
        }
        NBIAS_CHECK(threw);
    });

    run_case("sanitize_orig_name rejects an empty name", [] {
        bool threw{false};
        try {
            sanitize_orig_name("");
        }
        catch(std::runtime_error const&) {
            threw = true;
        }
        NBIAS_CHECK(threw);
    });

    std::filesystem::remove_all(temp_dir);
    return nbias::test::report();
}
