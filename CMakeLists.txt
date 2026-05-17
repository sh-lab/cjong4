cmake_minimum_required(VERSION 3.16)
project(cjong4 LANGUAGES C)

# Use C11
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(BUILD_EXAMPLES "Build example programs" OFF)
option(BUILD_TESTS "Build tests" OFF)

# Collect source files from src/
file(GLOB CJ4_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

add_library(cj4 STATIC ${CJ4_SOURCES})

target_include_directories(cj4
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/cjong4
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

# Install (optional)
install(TARGETS cj4
    ARCHIVE DESTINATION lib
)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION include)

message(STATUS "cjong4 project configured (C ${CMAKE_C_STANDARD})")
