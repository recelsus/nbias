cmake_minimum_required(VERSION 3.22)

project(nbias LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(nbias_sources
    src/main.cpp
    src/cli.cpp
    src/editor.cpp
    src/util.cpp
)

if(APPLE)
    enable_language(OBJCXX)
    list(APPEND nbias_sources src/crypto_commoncrypto.mm)
else()
    list(APPEND nbias_sources src/crypto_openssl.cpp)
endif()

add_executable(nbias ${nbias_sources})

target_include_directories(nbias PRIVATE src)

if(APPLE)
    target_link_libraries(nbias PRIVATE "-framework Security" "-framework Foundation")
endif()

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
