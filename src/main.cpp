#include "cli.hpp"
#include "editor.hpp"
#include "crypto.hpp"

#include <exception>
#include <iostream>
#include <type_traits>
#include <variant>

namespace
{
    int dispatch_command(parsed_command const& command)
    {
        return std::visit(
            [&command](auto const& options) -> int
            {
                if constexpr(std::is_same_v<std::decay_t<decltype(options)>, edit_options>) {
                    return execute_edit(options);
                }
                if constexpr(std::is_same_v<std::decay_t<decltype(options)>, encrypt_options>) {
                    return execute_encrypt(options);
                }
                if constexpr(std::is_same_v<std::decay_t<decltype(options)>, decrypt_options>) {
                    return execute_decrypt(options);
                }
                if constexpr(std::is_same_v<std::decay_t<decltype(options)>, info_options>) {
                    return execute_info(options);
                }
            },
            command.payload);
    }
}

int main(int argc, char* argv[])
{
    try {
        auto command = parse_command_line(argc, argv);
        return dispatch_command(command);
    }
    catch(std::invalid_argument const& problem) {
        std::cerr << problem.what() << '\n';
        return 1;
    }
    catch(std::exception const& problem) {
        std::cerr << "fatal: " << problem.what() << '\n';
        return 1;
    }
}
