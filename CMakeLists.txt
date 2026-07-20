cmake_minimum_required(VERSION 3.22)

project(nbias LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CTest)

find_package(PkgConfig REQUIRED)
pkg_check_modules(sodium REQUIRED IMPORTED_TARGET libsodium)

add_library(nbias_core STATIC
    core/src/vault_codec.cpp
    core/src/kdf_params.cpp
    core/src/crypto_engine.cpp
)

target_include_directories(nbias_core PUBLIC core/include)
target_link_libraries(nbias_core PUBLIC PkgConfig::sodium)

add_library(nbias_cli_support STATIC
    cli/src/cli_options.cpp
    cli/src/env_file.cpp
    cli/src/passphrase_resolver.cpp
    cli/src/file_ops.cpp
)

target_include_directories(nbias_cli_support PUBLIC cli/include)
target_link_libraries(nbias_cli_support PUBLIC nbias_core)

add_executable(nbias
    cli/src/main.cpp
    cli/src/enc_command.cpp
    cli/src/dec_command.cpp
    cli/src/edit_command.cpp
    cli/src/info_command.cpp
)

target_link_libraries(nbias PRIVATE nbias_cli_support)

if(CMAKE_EXPORT_COMPILE_COMMANDS AND NOT TARGET link_compile_commands)
    set(link_compile_commands_script "${CMAKE_BINARY_DIR}/link_compile_commands.cmake")
    file(WRITE ${link_compile_commands_script}
        "if(EXISTS \"${CMAKE_BINARY_DIR}/compile_commands.json\")\n"
        "  if(WIN32)\n"
        "    file(COPY \"${CMAKE_BINARY_DIR}/compile_commands.json\" DESTINATION \"${CMAKE_SOURCE_DIR}\")\n"
        "  else()\n"
        "    file(REMOVE \"${CMAKE_SOURCE_DIR}/compile_commands.json\")\n"
        "    file(CREATE_LINK \"${CMAKE_BINARY_DIR}/compile_commands.json\" \"${CMAKE_SOURCE_DIR}/compile_commands.json\" SYMBOLIC)\n"
        "  endif()\n"
        "endif()\n"
    )

    add_custom_target(link_compile_commands ALL
        COMMAND ${CMAKE_COMMAND} -P ${link_compile_commands_script}
        COMMENT "Sync compile_commands.json to the project root"
    )
endif()

if(BUILD_TESTING)
    add_executable(core_vault_codec_test tests/core/vault_codec_test.cpp)
    target_include_directories(core_vault_codec_test PRIVATE core/src tests/support)
    target_link_libraries(core_vault_codec_test PRIVATE nbias_core)
    add_test(NAME core_vault_codec COMMAND core_vault_codec_test)

    add_executable(core_crypto_engine_test tests/core/crypto_engine_test.cpp)
    target_include_directories(core_crypto_engine_test PRIVATE tests/support)
    target_link_libraries(core_crypto_engine_test PRIVATE nbias_core)
    add_test(NAME core_crypto_engine COMMAND core_crypto_engine_test)

    add_executable(cli_file_ops_test tests/cli/file_ops_test.cpp)
    target_include_directories(cli_file_ops_test PRIVATE tests/support)
    target_link_libraries(cli_file_ops_test PRIVATE nbias_cli_support)
    add_test(NAME cli_file_ops COMMAND cli_file_ops_test)

    add_executable(cli_env_file_test tests/cli/env_file_test.cpp)
    target_include_directories(cli_env_file_test PRIVATE tests/support)
    target_link_libraries(cli_env_file_test PRIVATE nbias_cli_support)
    add_test(NAME cli_env_file COMMAND cli_env_file_test)

    add_executable(cli_passphrase_resolver_test tests/cli/passphrase_resolver_test.cpp)
    target_include_directories(cli_passphrase_resolver_test PRIVATE tests/support)
    target_link_libraries(cli_passphrase_resolver_test PRIVATE nbias_cli_support)
    add_test(NAME cli_passphrase_resolver COMMAND cli_passphrase_resolver_test)

    add_executable(cli_options_test tests/cli/cli_options_test.cpp)
    target_include_directories(cli_options_test PRIVATE tests/support)
    target_link_libraries(cli_options_test PRIVATE nbias_cli_support)
    add_test(NAME cli_options COMMAND cli_options_test)

    add_executable(cli_end_to_end_test tests/cli/end_to_end_test.cpp)
    target_include_directories(cli_end_to_end_test PRIVATE tests/support)
    target_compile_definitions(cli_end_to_end_test PRIVATE NBIAS_BINARY_PATH="$<TARGET_FILE:nbias>")
    add_test(NAME cli_end_to_end COMMAND cli_end_to_end_test)
endif()
