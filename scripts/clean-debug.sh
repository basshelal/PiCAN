#!/usr/bin/env bash

# Add bash-utils, needs to use this complex syntax in order to work from any working directory
source "$(dirname "$(realpath "$0")")/bash-utils.sh"

echo "Deleting $BUILD_DEBUG_DIR"
rm -rf "$BUILD_DEBUG_DIR"
