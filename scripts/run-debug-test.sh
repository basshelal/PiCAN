#!/usr/bin/env bash

# Add bash-utils, needs to use this complex syntax in order to work from any working directory
source "$(dirname "$(realpath "$0")")/bash-utils.sh"

"$SCRIPTS_DIR"/build-debug-test.sh && \
"$BUILD_DEBUG_DIR"/test/pican-test

# TODO need a way to launch with memlock removed, use: prlimit --memlock=unlimited your_command