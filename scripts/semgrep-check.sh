#!/usr/bin/env bash

# Add bash-utils, needs to use this complex syntax in order to work from any working directory
source "$(dirname "$(realpath "$0")")/bash-utils.sh"

require_command "semgrep"

semgrep scan -c "$PROJECT_DIR/.semgrep.yaml" \
-j "$NUM_PROC" \
--strict \
--quiet \
--error \
"$SRC_DIR"
