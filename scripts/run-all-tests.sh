#!/usr/bin/env bash

# Add bash-utils, needs to use this complex syntax in order to work from any working directory
source "$(dirname "$(realpath "$0")")/bash-utils.sh"

cmake --workflow --preset debug-test
cmake --workflow --preset relinfo-test
cmake --workflow --preset debug-aarch64-test
cmake --workflow --preset relinfo-aarch64-test

