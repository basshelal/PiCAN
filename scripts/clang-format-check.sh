#!/usr/bin/env bash

# Add bash-utils, needs to use this complex syntax in order to work from any working directory
source "$(dirname "$(realpath "$0")")/bash-utils.sh"

require_command "find"
require_command "clang-format"

# TODO this needs to first run find to get all the files, put them into a string and
#  then call clang-format with those, this way we can do more things per file or print
#  all files etc
find "$SRC_DIR" -type f -regex '.*\.\(cppm\|cpp\|c\|hpp\|h\)' -exec \
  clang-format \
  -style=file:"$PROJECT_DIR/.clang-format" \
  --verbose \
  --Werror \
  --dry-run \ # TODO make a script flag or even other script for fixing ie -i flag
{} \;
