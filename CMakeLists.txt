cmake_minimum_required(VERSION 3.22)

project(nbias LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(sodium REQUIRED IMPORTED_TARGET libsodium)

add_library(nbias_core STATIC
    core/src/vault_codec.cpp
    core/src/kdf_params.cpp
    core/src/crypto_engine.cpp
)

target_include_directories(nbias_core PUBLIC core/include)
target_link_libraries(nbias_core PUBLIC PkgConfig::sodium)

add_executable(nbias
    cli/src/main.cpp
    cli/src/cli_options.cpp
    cli/src/env_file.cpp
    cli/src/passphrase_resolver.cpp
    cli/src/file_ops.cpp
    cli/src/enc_command.cpp
    cli/src/dec_command.cpp
    cli/src/edit_command.cpp
)

target_include_directories(nbias PRIVATE cli/include)
target_link_libraries(nbias PRIVATE nbias_core)

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
