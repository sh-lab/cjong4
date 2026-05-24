cmake_minimum_required(VERSION 3.16)
project(cjong4 LANGUAGES C)

# Use C11
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(BUILD_EXAMPLES "Build example programs" OFF)
option(BUILD_TESTS "Build tests" OFF)

# Collect source files from layered directories.
file(GLOB_RECURSE CJ4_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/manager/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/util/*.c"
)

add_library(cj4 STATIC ${CJ4_SOURCES})

target_include_directories(cj4
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/cjong4/core
        ${CMAKE_CURRENT_SOURCE_DIR}/include/cjong4/manager
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core
        ${CMAKE_CURRENT_SOURCE_DIR}/src/manager
        ${CMAKE_CURRENT_SOURCE_DIR}/src/util
)

# Compiler warnings
if (MSVC)
    target_compile_options(cj4 PRIVATE /W4)
else()
    target_compile_options(cj4 PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(cj4 PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib"
)

if (BUILD_TESTS)
    enable_testing()

    add_executable(cj4_tests
        tests/core/test_core_rules.c
        tests/manager/test_manager_flow.c
    )
    target_include_directories(cj4_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/cjong4/core
        ${CMAKE_CURRENT_SOURCE_DIR}/include/cjong4/manager
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core
        ${CMAKE_CURRENT_SOURCE_DIR}/src/manager
    )
    target_link_libraries(cj4_tests PRIVATE cj4)

    add_test(NAME cj4_tests COMMAND cj4_tests)
endif()

# Install (optional)
install(TARGETS cj4
    ARCHIVE DESTINATION lib
)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION include)

message(STATUS "cjong4 project configured (C ${CMAKE_C_STANDARD})")
