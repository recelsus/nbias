#include "editor.hpp"

#include "util.hpp"

#include <iostream>

int execute_edit(edit_options const& options)
{
    std::cout << "[nbias] edit stub\n";
    std::cout << "  target: " << options.target_path << '\n';
    if(options.editor) {
        std::cout << "  editor: " << *options.editor << '\n';
    }
    std::cout << "  vault_ext: " << options.vault_extension << '\n';
    if(options.allow_extensions) {
        std::cout << "  allow_ext: " << *options.allow_extensions << '\n';
    }
    if(options.max_bytes) {
        std::cout << "  max_bytes: " << *options.max_bytes << '\n';
    }
    std::cout << "  assume_yes: " << (options.assume_yes ? "true" : "false") << '\n';
    std::cout << "  delete_source: " << to_string(options.delete_source) << '\n';
    if(options.orig_extension) {
        std::cout << "  orig_ext: " << *options.orig_extension << '\n';
    }
    std::cout << "  kdf_profile: " << to_string(options.profile) << '\n';
    std::cout << "  tmpfs: " << to_string(options.tmpfs) << '\n';
    std::cout << "  verbose: " << (options.verbose ? "true" : "false") << '\n';
    if(options.passphrase) {
        std::cout << "  passphrase: [supplied]" << '\n';
    }
    if(options.key_hex) {
        std::cout << "  key_hex: [supplied]" << '\n';
    }
    return 0;
}

