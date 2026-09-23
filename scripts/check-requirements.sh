#!/usr/bin/env bash

# Add bash-utils, needs to use this complex syntax in order to work from any working directory
source "$(dirname "$(realpath "$0")")/bash-utils.sh"

require_command "cmake"
require_command "ninja"
require_command "gcc"
require_command "g++"

require_command "uv"
uv sync --quiet
source "$VENV_DIR/bin/activate"

require_command "clang-format"
require_command "clang-tidy"
require_command "run-clang-tidy.py"
require_command "semgrep"
require_command "cppcheck"
