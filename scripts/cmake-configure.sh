#!/usr/bin/env bash

# Add bash-utils, needs to use this complex syntax in order to work from any working directory
source "$(dirname "$(realpath "$0")")/bash-utils.sh"

require_command "cmake"

cd "$PROJECT_DIR"

cmake -S "$PROJECT_DIR" -B "$BUILD_DEBUG_DIR" -DCMAKE_BUILD_TYPE=Debug -G Ninja
cmake -S "$PROJECT_DIR" -B "$BUILD_RELEASE_DIR" -DCMAKE_BUILD_TYPE=Release -G Ninja
