#include "cli_options.hpp"

#include <sstream>
#include <stdexcept>
#include <vector>

namespace
{
    [[noreturn]] void throw_cli_error(std::string const& message)
    {
        throw std::invalid_argument(message);
    }

    bool is_option_token(std::string const& token)
    {
        return !token.empty() && token.front() == '-';
    }

    void assign_key_option(common_key_options& options, std::string const& value)
    {
        options.key = value;
        options.password_protected = true;
    }

    parsed_command parse_enc_or_dec_command(command_kind kind, std::vector<std::string> const& args)
    {
        enc_options enc{};
        dec_options dec{};
        auto& paths = kind == command_kind::enc ? enc.paths : dec.paths;
        auto& output_dir = kind == command_kind::enc ? enc.output_dir : dec.output_dir;
        auto& common = kind == command_kind::enc
            ? static_cast<common_key_options&>(enc)
            : static_cast<common_key_options&>(dec);

        for(std::size_t index = 2; index < args.size(); ++index) {
            auto const& token = args[index];
            if(token == "--output-dir") {
                if(index + 1 >= args.size()) {
                    throw_cli_error("--output-dir expects a value");
                }
                output_dir = args[index + 1];
                ++index;
                continue;
            }
            if(token == "-K") {
                common.password_protected = true;
                continue;
            }
            if(token == "--no-password") {
                common.explicit_no_password = true;
                continue;
            }
            if(token == "--key" || token == "-k") {
                if(index + 1 >= args.size()) {
                    throw_cli_error(token + " expects a value");
                }
                assign_key_option(common, args[index + 1]);
                ++index;
                continue;
            }
            if(is_option_token(token)) {
                throw_cli_error("unknown option: " + token);
            }
            paths.emplace_back(token);
        }

        if(paths.empty()) {
            throw_cli_error((kind == command_kind::enc ? std::string{"enc"} : std::string{"dec"}) + " requires at least one path");
        }

        parsed_command command{};
        command.kind = kind;
        command.payload = kind == command_kind::enc ? command_payload{enc} : command_payload{dec};
        return command;
    }

    parsed_command parse_edit_command(std::vector<std::string> const& args)
    {
        edit_options options{};
        bool path_set{false};

        for(std::size_t index = 2; index < args.size(); ++index) {
            auto const& token = args[index];
            if(token == "--editor") {
                if(index + 1 >= args.size()) {
                    throw_cli_error("--editor expects a value");
                }
                options.editor = args[index + 1];
                ++index;
                continue;
            }
            if(token == "-K") {
                options.password_protected = true;
                continue;
            }
            if(token == "--key" || token == "-k") {
                if(index + 1 >= args.size()) {
                    throw_cli_error(token + " expects a value");
                }
                assign_key_option(options, args[index + 1]);
                ++index;
                continue;
            }
            if(token == "--yes" || token == "-y") {
                options.assume_yes = true;
                continue;
            }
            if(is_option_token(token)) {
                throw_cli_error("unknown option: " + token);
            }
            if(path_set) {
                throw_cli_error("unexpected argument: " + token);
            }
            options.target_path = token;
            path_set = true;
        }

        if(!path_set) {
            throw_cli_error("edit requires a target path");
        }

        parsed_command command{};
        command.kind = command_kind::edit;
        command.payload = options;
        return command;
    }
}

parsed_command parse_command_line(int argc, char const* const* argv)
{
    if(argc < 2) {
        throw_cli_error(build_usage_string());
    }

    std::vector<std::string> args{};
    args.reserve(static_cast<std::size_t>(argc));
    for(int index = 0; index < argc; ++index) {
        args.emplace_back(argv[index]);
    }

    auto const& command = args[1];
    if(command == "enc") {
        return parse_enc_or_dec_command(command_kind::enc, args);
    }
    if(command == "dec") {
        return parse_enc_or_dec_command(command_kind::dec, args);
    }
    if(command == "edit") {
        return parse_edit_command(args);
    }
    if(command == "--help" || command == "-h" || command == "help") {
        throw_cli_error(build_usage_string());
    }

    throw_cli_error("unknown command: " + command);
}

std::string build_usage_string()
{
    std::ostringstream out{};
    out << "Usage: nbias <command> [options]\n\n";
    out << "Commands:\n";
    out << "  enc <path...> [--output-dir <dir>] [-K] [--key|-k <value>] [--no-password]\n";
    out << "  dec <path...> [--output-dir <dir>] [-K] [--key|-k <value>]\n";
    out << "  edit <path.knty> [--editor <cmd>] [-K] [--key|-k <value>] [--yes]\n";
    return out.str();
}
