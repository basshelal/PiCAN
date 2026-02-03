#!/usr/bin/env bash

# Add bash-utils, needs to use this complex syntax in order to work from any working directory
source "$(dirname "$(realpath "$0")")/../scripts/bash-utils.sh"

echo "Running pre-push git hook..."

"$SCRIPTS_DIR"/format-check.sh
if [ $? -ne 0 ]; then
    echo_error "Cannot push: format-check failed"
    exit 1
fi

"$SCRIPTS_DIR"/clang-tidy-check.sh
if [ $? -ne 0 ]; then
    echo_error "Cannot push: clang-tidy-check failed"
    exit 1
fi

"$SCRIPTS_DIR"/semgrep-check.sh
if [ $? -ne 0 ]; then
    echo_error "Cannot push: semgrep-check failed"
    exit 1
fi

"$SCRIPTS_DIR"/build-debug-app.sh
if [ $? -ne 0 ]; then
    echo_error "Cannot push: build-debug-app failed"
    exit 1
fi

"$SCRIPTS_DIR"/build-debug-test.sh
if [ $? -ne 0 ]; then
    echo_error "Cannot push: build-debug-test failed"
    exit 1
fi

"$SCRIPTS_DIR"/run-debug-test.sh
if [ $? -ne 0 ]; then
    echo_error "Cannot push: run-debug-test failed"
    exit 1
fi

echo "pre-push git hook finished successfully"
