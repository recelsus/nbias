#include "cli_options.hpp"
#include "dec_command.hpp"
#include "edit_command.hpp"
#include "enc_command.hpp"

#include <exception>
#include <iostream>
#include <type_traits>
#include <variant>

namespace
{
    int dispatch_command(parsed_command const& command)
    {
        return std::visit(
            [](auto const& options) -> int
            {
                if constexpr(std::is_same_v<std::decay_t<decltype(options)>, enc_options>) {
                    return execute_enc(options);
                }
                if constexpr(std::is_same_v<std::decay_t<decltype(options)>, dec_options>) {
                    return execute_dec(options);
                }
                if constexpr(std::is_same_v<std::decay_t<decltype(options)>, edit_options>) {
                    return execute_edit(options);
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
