#!/usr/bin/env bash

# Add bash-utils, needs to use this complex syntax in order to work from any working directory
source "$(dirname "$(realpath "$0")")/bash-utils.sh"

require_command "find"
require_command "clang-format"

SRC_FILES=$(find "$SRC_DIR" -type f -regex '.*\.\(cppm\|cpp\|c\|hpp\|h\)')

clang-format \
  -style=file:"$PROJECT_DIR/.clang-format" \
  --verbose \
  --Werror \
  --dry-run \
  $SRC_FILES

# TODO does not do automatic fixes, either make that behavior a flag, or the default