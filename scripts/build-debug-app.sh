#!/usr/bin/env bash

# Add bash-utils, needs to use this complex syntax in order to work from any working directory
source "$(dirname "$(realpath "$0")")/bash-utils.sh"

require_command "cmake"

cd "$PROJECT_DIR"

cmake --build --preset debug-app -j "$NUM_PROC"
