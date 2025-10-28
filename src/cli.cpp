#include "cli.hpp"

#include <algorithm>
#include <charconv>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <string>
#include <string_view>
#include <vector>

namespace
{
    [[noreturn]] void throw_cli_error(const std::string& message)
    {
        throw std::invalid_argument(message);
    }

    std::size_t parse_size_value(std::string_view text)
    {
        std::size_t value{};
        auto* begin = text.data();
        auto* end = text.data() + text.size();
        auto result = std::from_chars(begin, end, value);
        if(result.ec != std::errc{} || result.ptr != end) {
            throw_cli_error("invalid integer value: " + std::string{text});
        }
        return value;
    }

    delete_source_mode parse_delete_source_value(std::string_view text)
    {
        if(text == "ask") {
            return delete_source_mode::ask;
        }
        if(text == "yes") {
            return delete_source_mode::yes;
        }
        if(text == "no") {
            return delete_source_mode::no;
        }
        throw_cli_error("invalid delete-source value: " + std::string{text});
    }

    kdf_profile parse_kdf_profile(std::string_view text)
    {
        if(text == "fast") {
            return kdf_profile::fast;
        }
        if(text == "balanced") {
            return kdf_profile::balanced;
        }
        if(text == "hardened") {
            return kdf_profile::hardened;
        }
        throw_cli_error("invalid kdf profile: " + std::string{text});
    }

    tmpfs_mode parse_tmpfs_mode(std::string_view text)
    {
        if(text == "prefer") {
            return tmpfs_mode::prefer;
        }
        if(text == "never") {
            return tmpfs_mode::never;
        }
        throw_cli_error("invalid tmpfs mode: " + std::string{text});
    }

    void assign_common_option(common_options& common, std::string_view option, std::string const& value)
    {
        if(option == "--passphrase") {
            common.passphrase = value;
            return;
        }
        if(option == "--key-hex") {
            common.key_hex = value;
            return;
        }
        throw_cli_error("unknown option: " + std::string{option});
    }

    void assign_flag(common_options& common, std::string_view option)
    {
        if(option == "--verbose") {
            common.verbose = true;
            return;
        }
        throw_cli_error("unknown flag: " + std::string{option});
    }

    parsed_command parse_edit_command(std::vector<std::string> const& args)
    {
        edit_options options{};
        bool path_set = false;
        for(std::size_t index = 2; index < args.size(); ++index) {
            auto const& token = args[index];
            if(token == "--passphrase" || token == "--key-hex") {
                if(index + 1 >= args.size()) {
                    throw_cli_error(token + " expects value");
                }
                assign_common_option(options, token, args[index + 1]);
                ++index;
                continue;
            }
            if(token == "--editor") {
                if(index + 1 >= args.size()) {
                    throw_cli_error("--editor expects value");
                }
                options.editor = args[index + 1];
                ++index;
                continue;
            }
            if(token == "--ext") {
                if(index + 1 >= args.size()) {
                    throw_cli_error("--ext expects value");
                }
                options.vault_extension = args[index + 1];
                ++index;
                continue;
            }
            if(token == "--allow-ext") {
                if(index + 1 >= args.size()) {
                    throw_cli_error("--allow-ext expects value");
                }
                options.allow_extensions = args[index + 1];
                ++index;
                continue;
            }
            if(token == "--max-bytes") {
                if(index + 1 >= args.size()) {
                    throw_cli_error("--max-bytes expects value");
                }
                options.max_bytes = parse_size_value(args[index + 1]);
                ++index;
                continue;
            }
            if(token == "--yes" || token == "-y") {
                options.assume_yes = true;
                continue;
            }
            if(token == "--delete-source") {
                if(index + 1 >= args.size()) {
                    throw_cli_error("--delete-source expects value");
                }
                options.delete_source = parse_delete_source_value(args[index + 1]);
                ++index;
                continue;
            }
            if(token == "--orig-ext") {
                if(index + 1 >= args.size()) {
                    throw_cli_error("--orig-ext expects value");
                }
                options.orig_extension = args[index + 1];
                ++index;
                continue;
            }
            if(token == "--kdf-profile") {
                if(index + 1 >= args.size()) {
                    throw_cli_error("--kdf-profile expects value");
                }
                options.profile = parse_kdf_profile(args[index + 1]);
                ++index;
                continue;
            }
            if(token == "--tmpfs") {
                if(index + 1 >= args.size()) {
                    throw_cli_error("--tmpfs expects value");
                }
                options.tmpfs = parse_tmpfs_mode(args[index + 1]);
                ++index;
                continue;
            }
            if(token == "--verbose") {
                assign_flag(options, token);
                continue;
            }
            if(!token.empty() && token.front() == '-') {
                throw_cli_error("unknown option: " + token);
            }
            if(!path_set) {
                options.target_path = token;
                path_set = true;
                continue;
            }
            throw_cli_error("unexpected argument: " + token);
        }

        if(!path_set) {
            throw_cli_error("edit requires target path");
        }

        parsed_command command{};
        command.kind = command_kind::edit;
        command.payload = options;
        return command;
    }

    parsed_command parse_encrypt_command(std::vector<std::string> const& args)
    {
        encrypt_options options{};
        bool input_set = false;
        bool output_set = false;
        for(std::size_t index = 1; index < args.size(); ++index) {
            auto const& token = args[index];
            if(token == "--passphrase" || token == "--key-hex") {
                if(index + 1 >= args.size()) {
                    throw_cli_error(token + " expects value");
                }
                assign_common_option(options, token, args[index + 1]);
                ++index;
                continue;
            }
            if(token == "-i" || token == "--input") {
                if(index + 1 >= args.size()) {
                    throw_cli_error(token + " expects value");
                }
                options.input_path = args[index + 1];
                input_set = true;
                ++index;
                continue;
            }
            if(token == "-o" || token == "--output") {
                if(index + 1 >= args.size()) {
                    throw_cli_error(token + " expects value");
                }
                options.output_path = args[index + 1];
                output_set = true;
                ++index;
                continue;
            }
            if(token == "--orig-ext") {
                if(index + 1 >= args.size()) {
                    throw_cli_error("--orig-ext expects value");
                }
                options.orig_extension = args[index + 1];
                ++index;
                continue;
            }
            if(token == "--kdf-profile") {
                if(index + 1 >= args.size()) {
                    throw_cli_error("--kdf-profile expects value");
                }
                options.profile = parse_kdf_profile(args[index + 1]);
                ++index;
                continue;
            }
            if(token == "--verbose") {
                assign_flag(options, token);
                continue;
            }
            throw_cli_error("unknown option: " + token);
        }

        if(!input_set) {
            throw_cli_error("encrypt requires input path");
        }
        if(!output_set) {
            throw_cli_error("encrypt requires output path");
        }

        parsed_command command{};
        command.kind = command_kind::encrypt;
        command.payload = options;
        return command;
    }

    parsed_command parse_decrypt_command(std::vector<std::string> const& args)
    {
        decrypt_options options{};
        bool input_set = false;
        bool output_set = false;
        for(std::size_t index = 1; index < args.size(); ++index) {
            auto const& token = args[index];
            if(token == "--passphrase" || token == "--key-hex") {
                if(index + 1 >= args.size()) {
                    throw_cli_error(token + " expects value");
                }
                assign_common_option(options, token, args[index + 1]);
                ++index;
                continue;
            }
            if(token == "-i" || token == "--input") {
                if(index + 1 >= args.size()) {
                    throw_cli_error(token + " expects value");
                }
                options.input_path = args[index + 1];
                input_set = true;
                ++index;
                continue;
            }
            if(token == "-o" || token == "--output") {
                if(index + 1 >= args.size()) {
                    throw_cli_error(token + " expects value");
                }
                options.output_path = args[index + 1];
                output_set = true;
                ++index;
                continue;
            }
            if(token == "--verbose") {
                assign_flag(options, token);
                continue;
            }
            throw_cli_error("unknown option: " + token);
        }

        if(!input_set) {
            throw_cli_error("decrypt requires input path");
        }
        if(!output_set) {
            throw_cli_error("decrypt requires output path");
        }

        parsed_command command{};
        command.kind = command_kind::decrypt;
        command.payload = options;
        return command;
    }

    parsed_command parse_info_command(std::vector<std::string> const& args)
    {
        info_options options{};
        bool path_set = false;
        for(std::size_t index = 1; index < args.size(); ++index) {
            auto const& token = args[index];
            if(token == "--verbose") {
                assign_flag(options, token);
                continue;
            }
            if(token == "--passphrase" || token == "--key-hex") {
                if(index + 1 >= args.size()) {
                    throw_cli_error(token + " expects value");
                }
                assign_common_option(options, token, args[index + 1]);
                ++index;
                continue;
            }
            if(!token.empty() && token.front() == '-') {
                throw_cli_error("unknown option: " + token);
            }
            if(!path_set) {
                options.input_path = token;
                path_set = true;
                continue;
            }
            throw_cli_error("unexpected argument: " + token);
        }

        if(!path_set) {
            throw_cli_error("info requires target path");
        }

        parsed_command command{};
        command.kind = command_kind::info;
        command.payload = options;
        return command;
    }
}

parsed_command parse_command_line(int argc, char const* const* argv)
{
    if(argc < 2) {
        throw_cli_error("nbias expects a command");
    }

    std::vector<std::string> args;
    args.reserve(static_cast<std::size_t>(argc));
    for(int i = 0; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }

    auto const& command = args[1];
    if(command == "edit") {
        if(argc < 3) {
            throw_cli_error("edit requires target path");
        }
        return parse_edit_command(args);
    }
    if(command == "encrypt") {
        return parse_encrypt_command(args);
    }
    if(command == "decrypt") {
        return parse_decrypt_command(args);
    }
    if(command == "info") {
        return parse_info_command(args);
    }
    if(command == "--help" || command == "-h" || command == "help") {
        throw_cli_error(build_usage_string());
    }

    throw_cli_error("unknown command: " + command);
}

std::string build_usage_string()
{
    std::ostringstream out;
    out << "Usage: nbias <command> [options]\n\n";
    out << "Commands:\n";
    out << "  edit <path>           Edit note or vault\n";
    out << "  encrypt -i <in> -o <out>  Encrypt plaintext\n";
    out << "  decrypt -i <in> -o <out>  Decrypt vault\n";
    out << "  info <vault>          Show vault meta\n";
    out << "\n";
    out << "Run 'nbias <command> --help' for command details.\n";
    return out.str();
}
