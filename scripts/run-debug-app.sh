#!/usr/bin/env bash

# Add bash-utils, needs to use this complex syntax in order to work from any working directory
source "$(dirname "$(realpath "$0")")/bash-utils.sh"

"$SCRIPTS_DIR"/build-debug-app.sh && \
"$BUILD_DEBUG_DIR"/src/PiCAN
