cmake_minimum_required(VERSION 3.22)

project(nbias LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CTest)

option(NBIAS_BUILD_WASM "Build the wasm/ Emscripten bindings for nbias_core (requires configuring with emcmake)" OFF)

if(EMSCRIPTEN)
    include(FetchContent)
    FetchContent_Declare(libsodium_src
        GIT_REPOSITORY https://github.com/jedisct1/libsodium.git
        GIT_TAG 1.0.20-RELEASE
    )
    FetchContent_MakeAvailable(libsodium_src)

    set(sodium_wasm_prefix "${libsodium_src_SOURCE_DIR}/libsodium-js-sumo")
    set(sodium_wasm_lib "${sodium_wasm_prefix}/lib/libsodium.a")

    add_custom_command(
        OUTPUT "${sodium_wasm_lib}"
        COMMAND ./dist-build/emscripten.sh --sumo
        WORKING_DIRECTORY "${libsodium_src_SOURCE_DIR}"
        COMMENT "Building libsodium for wasm32 (dist-build/emscripten.sh --sumo)"
        VERBATIM
    )
    add_custom_target(libsodium_wasm_build DEPENDS "${sodium_wasm_lib}")

    file(MAKE_DIRECTORY "${sodium_wasm_prefix}/include")

    add_library(sodium_for_wasm STATIC IMPORTED GLOBAL)
    set_target_properties(sodium_for_wasm PROPERTIES IMPORTED_LOCATION "${sodium_wasm_lib}")
    add_dependencies(sodium_for_wasm libsodium_wasm_build)
    target_include_directories(sodium_for_wasm INTERFACE "${sodium_wasm_prefix}/include")

    set(nbias_sodium_target sodium_for_wasm)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(sodium REQUIRED IMPORTED_TARGET libsodium)
    set(nbias_sodium_target PkgConfig::sodium)
endif()

add_library(nbias_core STATIC
    core/src/vault_codec.cpp
    core/src/kdf_params.cpp
    core/src/crypto_engine.cpp
)

target_include_directories(nbias_core PUBLIC core/include)
target_link_libraries(nbias_core PUBLIC ${nbias_sodium_target})

if(EMSCRIPTEN)
    target_compile_options(nbias_core PUBLIC -fwasm-exceptions)
endif()

if(NBIAS_BUILD_WASM)
    if(NOT EMSCRIPTEN)
        message(FATAL_ERROR "NBIAS_BUILD_WASM requires configuring with 'emcmake cmake' (the Emscripten toolchain).")
    endif()

    add_executable(nbias_wasm wasm/src/bindings.cpp)
    target_link_libraries(nbias_wasm PRIVATE nbias_core)
    set_target_properties(nbias_wasm PROPERTIES OUTPUT_NAME "nbias_core" SUFFIX ".mjs")
    target_link_options(nbias_wasm PRIVATE
        "-fwasm-exceptions"
        "-sMODULARIZE=1"
        "-sEXPORT_ES6=1"
        "-sEXPORT_NAME=createNbiasCoreModule"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sEXPORTED_FUNCTIONS=[\"_malloc\",\"_free\"]"
        "-sEXPORTED_RUNTIME_METHODS=[\"getValue\",\"setValue\",\"UTF8ToString\",\"stringToUTF8\",\"lengthBytesUTF8\",\"HEAPU8\"]"
    )

    if(BUILD_TESTING)
        add_test(
            NAME wasm_smoke
            COMMAND node "${CMAKE_CURRENT_SOURCE_DIR}/wasm/test/smoke_test.mjs" "$<TARGET_FILE_DIR:nbias_wasm>/nbias_core.mjs"
        )
    endif()
endif()

if(NOT EMSCRIPTEN)
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
endif()

if(CMAKE_EXPORT_COMPILE_COMMANDS AND NOT EMSCRIPTEN AND NOT TARGET link_compile_commands)
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

if(BUILD_TESTING AND NOT EMSCRIPTEN)
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
