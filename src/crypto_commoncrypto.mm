#include "crypto.hpp"

#include "util.hpp"

#include <iostream>

namespace
{
    void log_common(common_options const& options)
    {
        std::cout << "  verbose: " << (options.verbose ? "true" : "false") << '\n';
        if(options.passphrase) {
            std::cout << "  passphrase: [supplied]" << '\n';
        }
        if(options.key_hex) {
            std::cout << "  key_hex: [supplied]" << '\n';
        }
    }
}

int execute_encrypt(encrypt_options const& options)
{
    std::cout << "[nbias] encrypt stub\n";
    std::cout << "  input: " << options.input_path << '\n';
    std::cout << "  output: " << options.output_path << '\n';
    if(options.orig_extension) {
        std::cout << "  orig_ext: " << *options.orig_extension << '\n';
    }
    std::cout << "  kdf_profile: " << to_string(options.profile) << '\n';
    log_common(options);
    return 0;
}

int execute_decrypt(decrypt_options const& options)
{
    std::cout << "[nbias] decrypt stub\n";
    std::cout << "  input: " << options.input_path << '\n';
    std::cout << "  output: " << options.output_path << '\n';
    log_common(options);
    return 0;
}

int execute_info(info_options const& options)
{
    std::cout << "[nbias] info stub\n";
    std::cout << "  input: " << options.input_path << '\n';
    log_common(options);
    return 0;
}

