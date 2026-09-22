cmake_minimum_required(VERSION 3.30)
project(PiCAN
        VERSION 0.1.0
        LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_program(MOLD_PROGRAM NAMES mold)
if (MOLD_PROGRAM)
    set(MOLD_DEFAULT_STATUS ON)
else ()
    set(MOLD_DEFAULT_STATUS OFF)
    message(WARNING "mold linker not found, defaulting USE_MOLD_LINKER to OFF.")
endif ()

option(USE_MOLD_LINKER "Use mold linker (if available)" ${MOLD_DEFAULT_STATUS})

if (USE_MOLD_LINKER)
    add_link_options(-fuse-ld=mold)
    message("Using mold linker")
else ()
    message("Not using mold linker")
endif ()

# TODO review which flags from the below we will need!
# portable warning set

#-Wall -Wextra -Wpedantic -Werror
#-Wconversion -Wsign-conversion -Wold-style-cast -Wcast-qual -Wcast-align
#-Wshadow -Wnon-virtual-dtor -Woverloaded-virtual -Wsuggest-override
#-Wnull-dereference -Wdouble-promotion -Wfloat-equal -Wformat=2
#-Wundef -Wswitch-enum -Wimplicit-fallthrough -Wextra-semi
#-Wzero-as-null-pointer-constant -Wvla -Wmissing-declarations

# GCC-only extras

#-Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wuseless-cast
#-Wmismatched-tags -Wanalyzer-* (via -fanalyzer; strong on C, improving on C++)

# freestanding-ish policy

#-fno-exceptions -fno-rtti -fno-threadsafe-statics
#-fno-unwind-tables -fno-asynchronous-unwind-tables
#-fstack-usage -Wstack-usage=2048 -Wframe-larger-than=1024
#-ffunction-sections -fdata-sections -Wl,--gc-sections

#add_compile_options(
#        -fno-exceptions # No exceptions
#        -fno-rtti       # No RTTI
#)
#add_compile_options(
#        -Wall -Wextra -Wpedantic -Werror #
#        -Wno-interference-size   # Allow use of std::*interference_size alignas variables
#        -Wno-attributes=pican::  # Allow for attributes starting with pican::
#)

add_compile_options(-fno-exceptions -fno-unwind-tables -fno-rtti)
add_compile_options(-Wall -Werror -pedantic -Wno-interference-size -Wno-attributes=pican::)

# CPM for package/dependency management, see: https://github.com/cpm-cmake/CPM.cmake
include(cmake/CPM.cmake)

set(FMT_MODULE ON CACHE BOOL "Build the fmt module" FORCE)
# fmt: String formatting
CPMAddPackage(
        NAME fmt
        GITHUB_REPOSITORY fmtlib/fmt
        GIT_TAG 12.2.0
)

# magic_enum: Better enums
CPMAddPackage(
        NAME magic_enum
        GITHUB_REPOSITORY Neargye/magic_enum
        GIT_TAG v0.9.8
)

# Catch2: Unit testing
CPMAddPackage(
        NAME Catch2
        GITHUB_REPOSITORY catchorg/Catch2
        VERSION 3.15.3
)

# libbacktrace to be built and installed manually
CPMAddPackage(
        NAME libbacktrace
        GITHUB_REPOSITORY ianlancetaylor/libbacktrace
        GIT_TAG master
        DOWNLOAD_ONLY YES
)

if (libbacktrace_ADDED)
    include(ExternalProject)

    set(BACKTRACE_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/libbacktrace-install")
    set(BACKTRACE_STATIC_LIB "${BACKTRACE_INSTALL_DIR}/lib/libbacktrace.a")

    # 1. Build the base configuration command without the host flag
    set(BACKTRACE_CONFIG_CMD
            ${CMAKE_COMMAND} -E env
            "CC=${CMAKE_C_COMPILER}"
            "CFLAGS=${CMAKE_C_FLAGS} -fPIC"
            <SOURCE_DIR>/configure
            --prefix=<INSTALL_DIR>
            --libdir=<INSTALL_DIR>/lib
            --enable-shared=no
            --enable-static=yes
    )

    # Append the --host flag ONLY if we are cross-compiling
    if (CMAKE_CROSSCOMPILING)
        if (CMAKE_C_COMPILER_TARGET)
            # Use the target provided by Yocto
            list(APPEND BACKTRACE_CONFIG_CMD "--host=${CMAKE_C_COMPILER_TARGET}")
        else ()
            # Fallback for the manual aarch64-toolchain.cmake
            list(APPEND BACKTRACE_CONFIG_CMD "--host=aarch64-linux-gnu")
        endif ()
    endif ()

    # Tell CMake how to run ./configure and make
    ExternalProject_Add(libbacktrace_ext
            SOURCE_DIR ${libbacktrace_SOURCE_DIR}
            PREFIX "${CMAKE_CURRENT_BINARY_DIR}/libbacktrace-ext"
            INSTALL_DIR ${BACKTRACE_INSTALL_DIR}

            CONFIGURE_COMMAND ${BACKTRACE_CONFIG_CMD}
            BUILD_COMMAND make
            INSTALL_COMMAND make install

            # Required for Ninja
            BUILD_BYPRODUCTS ${BACKTRACE_STATIC_LIB}
    )

    # Create a modern CMake target that aliases the raw Autotools output
    add_library(libbacktrace STATIC IMPORTED GLOBAL)

    # Ensure CMake knows this library doesn't exist until the ExternalProject finishes
    add_dependencies(libbacktrace libbacktrace_ext)

    # Small fix so that configure step doesn't panic that the directory doesn't exist
    file(MAKE_DIRECTORY ${BACKTRACE_INSTALL_DIR}/include)

    # Expose the include directories and the static archive to your project
    set_target_properties(libbacktrace PROPERTIES
            IMPORTED_LOCATION ${BACKTRACE_STATIC_LIB}
            INTERFACE_INCLUDE_DIRECTORIES ${BACKTRACE_INSTALL_DIR}/include
    )
endif ()

add_subdirectory(src)

enable_testing()
add_subdirectory(test)

# TODO clean this up! I don't want extra CMake targets, instead defer this to scripts
find_program(CLANG_FORMAT_EXE NAMES "clang-format")
if (CLANG_FORMAT_EXE)
    # A. The "Check" Target (For CI/Audits) - Fails if code is messy
    file(GLOB_RECURSE SOURCES_TO_FORMAT
            "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cppm"
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
    message(WARNING "semgrep not found!")
endif ()

# Make git use .githooks/ as its git hooks directory, this way,
# our hooks are tracked in git and can run out of the box after a git clone
find_package(Git)
if (GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
    execute_process(
            COMMAND ${GIT_EXECUTABLE} config core.hooksPath .githooks
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            RESULT_VARIABLE GIT_CONFIG_RESULT
    )

    if (GIT_CONFIG_RESULT EQUAL 0)
        message(STATUS "Git Hooks successfully configured to use '.githooks/'")
    else ()
        message(WARNING "Failed to configure Git Hooks.")
    endif ()
endif ()
