cmake_minimum_required(VERSION 3.30)
project(PiCAN
        VERSION 0.1.0
        LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# TODO improve this mold stuff, it should default to on when found unless specified with an option
find_program(MOLD_EXECUTABLE NAMES mold)
if (MOLD_EXECUTABLE)
    set(MOLD_DEFAULT_STATUS ON)
else ()
    set(MOLD_DEFAULT_STATUS OFF)
    message(STATUS "mold linker not found, defaulting USE_MOLD_LINKER to OFF.")
endif ()

option(USE_MOLD_LINKER "Use mold linker (if available)" ${MOLD_DEFAULT_STATUS})

if (USE_MOLD_LINKER)
    add_link_options(-fuse-ld=mold)
    message("Using mold linker")
else ()
    message("Not using mold linker")
endif ()

add_compile_options(-Wall -Werror -pedantic)

# CPM for package/dependency management, see: https://github.com/cpm-cmake/CPM.cmake
include(cmake/CPM.cmake)

# fmt: String formatting
CPMAddPackage(
        NAME fmt
        GITHUB_REPOSITORY fmtlib/fmt
        GIT_TAG 12.1.0
)

# spdlog: logging
CPMAddPackage(
        NAME spdlog
        GITHUB_REPOSITORY gabime/spdlog
        GIT_TAG v1.16.0
)

# magic_enum: Better enums
CPMAddPackage(
        NAME magic_enum
        GITHUB_REPOSITORY Neargye/magic_enum
        GIT_TAG v0.9.7
)

# GoogleTest: Unit testing
CPMAddPackage(
        NAME googletest
        GITHUB_REPOSITORY google/googletest
        GIT_TAG v1.17.0
        OPTIONS "INSTALL_GTEST OFF" "gtest_force_shared_crt ON"
)

# Qt6, needs to be installed on system using package manager or Qt installer, thus not managed by
# CPM
find_package(Qt6 COMPONENTS Core Gui Qml Quick Widgets REQUIRED)

add_subdirectory(src)

enable_testing()
add_subdirectory(test)

find_program(CLANG_FORMAT_EXE NAMES "clang-format")
if (CLANG_FORMAT_EXE)
    # A. The "Check" Target (For CI/Audits) - Fails if code is messy
    file(GLOB_RECURSE SOURCES_TO_FORMAT
            "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/src//*.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    )

    add_custom_target(
            format-check
            COMMAND ${CLANG_FORMAT_EXE} --dry-run --Werror -i
            ${SOURCES_TO_FORMAT}
            COMMENT "Checking code formatting..."
    )

    # B. The "Fix" Target (For You) - actually changes the files
    add_custom_target(
            format
            COMMAND ${CLANG_FORMAT_EXE} -i
            ${SOURCES_TO_FORMAT}
            COMMENT "Formatting code..."
    )
endif ()

include(ProcessorCount)
ProcessorCount(NUM_CORES)
if (NUM_CORES EQUAL 0)
    set(NUM_CORES 1)
endif ()

find_program(RUN_CLANG_TIDY_EXE NAMES "run-clang-tidy" "run-clang-tidy.py")
if (RUN_CLANG_TIDY_EXE)
    add_custom_target(
            run-clang-tidy
            COMMAND ${RUN_CLANG_TIDY_EXE}
            -p ${CMAKE_BINARY_DIR}
            -j ${NUM_CORES}
            "^${CMAKE_SOURCE_DIR}/src/.*"
            COMMENT "Running clang-tidy in parallel (-j ${NUM_CORES})..."
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            VERBATIM
    )
else ()
    message(FATAL_ERROR "run-clang-tidy not found!")
endif ()

find_program(SEMGREP_EXE NAMES "semgrep")
if (SEMGREP_EXE)
    add_custom_target(
            run-semgrep
            COMMAND ${SEMGREP_EXE}
            -c .semgrep.yaml
            -j ${NUM_CORES}
            --strict
            -q
            src/
            COMMENT "Running semgrep in parallel (-j ${NUM_CORES})..."
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            VERBATIM
    )
else ()
    message(FATAL_ERROR "semgrep not found!")
endif ()