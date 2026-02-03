#!/usr/bin/env bash

# Utilities and helpers for all scripts needs to be sourced to add to your bash script

# The path of the calling script (not this utils file)
export THIS_SCRIPT_PATH=$(realpath "$0")

# The directory of the calling script
export THIS_SCRIPT_DIR=$(dirname "$THIS_SCRIPT_PATH")

# The scripts directory where this utils file is located
export SCRIPTS_DIR=$(dirname "$(realpath "${BASH_SOURCE[0]}")")

# The project root dir
export PROJECT_DIR=$(realpath "$SCRIPTS_DIR/../")

# The src dir in the project root
export SRC_DIR=$(realpath "$PROJECT_DIR/src")

# The test dir in the project root
export TEST_DIR=$(realpath "$PROJECT_DIR/test")

# The build dir in the project root
export BUILD_DIR=$(realpath "$PROJECT_DIR/build")

# The debug build dir
export BUILD_DEBUG_DIR=$(realpath "$BUILD_DIR/debug")

# The release build dir
export BUILD_RELEASE_DIR=$(realpath "$BUILD_DIR/release")

# Number of processors
export NUM_PROC=$(nproc)

require_command() {
    local cmd="$1"
    if ! command -v "$cmd" &> /dev/null; then
        echo "Error: "$cmd" could not be found!" >&2
        exit 1
    fi
}

echo_error() {
    printf "\033[0;31m%s\n\033[0m" "$*" >&2
}