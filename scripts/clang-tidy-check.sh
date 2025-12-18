#!/usr/bin/env bash

# Add bash-utils, needs to use this complex syntax in order to work from any working directory
source "$(dirname "$(realpath "$0")")/bash-utils.sh"

require_command "run-clang-tidy"

run-clang-tidy -config-file "$PROJECT_DIR/.clang-tidy" \
-j "$NUM_PROC" \
-use-color true \
-quiet \
-p "$BUILD_DEBUG_DIR/" "^""$SRC_DIR/.*"
