cmake_minimum_required(VERSION 3.16)
project(cjong4 VERSION 3.1.0 LANGUAGES C)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# Use C11
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(BUILD_TESTS "Build tests" OFF)

# Collect source files from layered directories.
file(GLOB_RECURSE CJ4_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/manager/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/util/*.c"
)

add_library(cj4 STATIC ${CJ4_SOURCES})
add_library(cjong4::cj4 ALIAS cj4)

target_include_directories(cj4
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
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
        tests/test_main.c
        tests/test_support.c
        tests/core/test_location.c
        tests/core/test_rules.c
        tests/core/test_riichi_furiten.c
        tests/core/test_calls_kan.c
        tests/core/test_round_progression.c
        tests/core/test_scoring_settlement.c
        tests/core/test_abortive_draw.c
        tests/manager/test_player_view.c
        tests/manager/test_manager_actions.c
        tests/manager/test_manager_step.c
    )
    target_include_directories(cj4_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/cjong4/core
        ${CMAKE_CURRENT_SOURCE_DIR}/include/cjong4/manager
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core
        ${CMAKE_CURRENT_SOURCE_DIR}/src/manager
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )
    target_link_libraries(cj4_tests PRIVATE cj4)

    add_test(NAME cj4_tests COMMAND cj4_tests)
endif()

install(TARGETS cj4
    EXPORT cjong4Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

set(CJ4_CMAKE_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/cjong4")

install(EXPORT cjong4Targets
    FILE cjong4Targets.cmake
    NAMESPACE cjong4::
    DESTINATION ${CJ4_CMAKE_INSTALL_DIR}
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cjong4Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cjong4Config.cmake"
    INSTALL_DESTINATION ${CJ4_CMAKE_INSTALL_DIR}
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/cjong4ConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/cjong4Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/cjong4ConfigVersion.cmake"
    DESTINATION ${CJ4_CMAKE_INSTALL_DIR}
)

message(STATUS "cjong4 project configured (C ${CMAKE_C_STANDARD})")
