#include "test_harness.hpp"

#include "cli_options.hpp"

#include <stdexcept>
#include <vector>

using nbias::test::run_case;

namespace
{
    parsed_command parse(std::vector<std::string> const& args)
    {
        std::vector<std::string> full_args{"nbias"};
        full_args.insert(full_args.end(), args.begin(), args.end());

        std::vector<char const*> argv{};
        argv.reserve(full_args.size());
        for(auto const& arg : full_args) {
            argv.push_back(arg.c_str());
        }
        return parse_command_line(static_cast<int>(argv.size()), argv.data());
    }

    bool throws_invalid_argument(std::vector<std::string> const& args)
    {
        try {
            parse(args);
        }
        catch(std::invalid_argument const&) {
            return true;
        }
        return false;
    }
}

int main()
{
    run_case("enc with a single path defaults to no password and fast kdf", [] {
        auto command = parse({"enc", "a.md"});
        NBIAS_CHECK(command.kind == command_kind::enc);
        auto const& options = std::get<enc_options>(command.payload);
        NBIAS_CHECK(options.paths.size() == 1);
        NBIAS_CHECK(options.paths[0] == "a.md");
        NBIAS_CHECK(!options.password_protected);
        NBIAS_CHECK(options.profile == nbias::core::kdf_profile::fast);
    });

    run_case("enc accepts multiple paths and --output-dir", [] {
        auto command = parse({"enc", "a.md", "b.md", "--output-dir", "out"});
        auto const& options = std::get<enc_options>(command.payload);
        NBIAS_CHECK(options.paths.size() == 2);
        NBIAS_CHECK(options.output_dir == std::filesystem::path{"out"});
    });

    run_case("enc -K --key sets password_protected and the key value", [] {
        auto command = parse({"enc", "a.md", "-K", "--key", "hunter2"});
        auto const& options = std::get<enc_options>(command.payload);
        NBIAS_CHECK(options.password_protected);
        NBIAS_CHECK(options.key == "hunter2");
    });

    run_case("enc --kdf-profile selects the requested profile", [] {
        auto command = parse({"enc", "a.md", "--kdf-profile", "hardened"});
        auto const& options = std::get<enc_options>(command.payload);
        NBIAS_CHECK(options.profile == nbias::core::kdf_profile::hardened);
    });

    run_case("enc --kdf-profile rejects an unknown profile name", [] {
        NBIAS_CHECK(throws_invalid_argument({"enc", "a.md", "--kdf-profile", "bogus"}));
    });

    run_case("enc rejects -K combined with --no-password", [] {
        NBIAS_CHECK(throws_invalid_argument({"enc", "a.md", "-K", "--no-password"}));
    });

    run_case("enc rejects --key combined with --no-password", [] {
        NBIAS_CHECK(throws_invalid_argument({"enc", "a.md", "--key", "hunter2", "--no-password"}));
    });

    run_case("enc requires at least one path", [] {
        NBIAS_CHECK(throws_invalid_argument({"enc"}));
    });

    run_case("dec does not accept --kdf-profile (enc-only option)", [] {
        NBIAS_CHECK(throws_invalid_argument({"dec", "a.md.knty", "--kdf-profile", "fast"}));
    });

    run_case("edit parses --editor and --yes", [] {
        auto command = parse({"edit", "a.md.knty", "--editor", "vim", "--yes"});
        NBIAS_CHECK(command.kind == command_kind::edit);
        auto const& options = std::get<edit_options>(command.payload);
        NBIAS_CHECK(options.target_path == "a.md.knty");
        NBIAS_CHECK(options.editor == "vim");
        NBIAS_CHECK(options.assume_yes);
    });

    run_case("edit rejects more than one positional argument", [] {
        NBIAS_CHECK(throws_invalid_argument({"edit", "a.md.knty", "b.md.knty"}));
    });

    run_case("info parses its single target path", [] {
        auto command = parse({"info", "a.md.knty"});
        NBIAS_CHECK(command.kind == command_kind::info);
        auto const& options = std::get<info_options>(command.payload);
        NBIAS_CHECK(options.target_path == "a.md.knty");
    });

    run_case("an unknown command is rejected", [] {
        NBIAS_CHECK(throws_invalid_argument({"bogus"}));
    });

    return nbias::test::report();
}
