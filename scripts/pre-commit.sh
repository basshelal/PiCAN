#!/usr/bin/env bash

# Add bash-utils, needs to use this complex syntax in order to work from any working directory
source "$(dirname "$(realpath "$0")")/../scripts/bash-utils.sh"

echo "Running pre-commit git hook..."

"$SCRIPTS_DIR"/format-check.sh
if [ $? -ne 0 ]; then
    echo_error "Cannot commit: format-check failed"
    exit 1
fi

"$SCRIPTS_DIR"/clang-tidy-check.sh
if [ $? -ne 0 ]; then
    echo_error "Cannot commit: clang-tidy-check failed"
    exit 1
fi

"$SCRIPTS_DIR"/semgrep-check.sh
if [ $? -ne 0 ]; then
    echo_error "Cannot commit: semgrep-check failed"
    exit 1
fi

echo "pre-commit git hook finished successfully"
