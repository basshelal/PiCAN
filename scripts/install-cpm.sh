#!/usr/bin/env bash

# Add bash-utils, needs to use this complex syntax in order to work from any working directory
source "$(dirname "$(realpath "$0")")/bash-utils.sh"

require_command "wget"

mkdir -p cmake
wget -O "$PROJECT_DIR/cmake/CPM.cmake" "https://github.com/cpm-cmake/CPM.cmake/releases/latest/download/get_cpm.cmake"
