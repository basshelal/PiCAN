#!/usr/bin/env bash

# Add bash-utils, needs to use this complex syntax in order to work from any working directory
source "$(dirname "$(realpath "$0")")/bash-utils.sh"

require_command "clang-format"

echo "Formatting files in place"

DIRS="$SRC_DIR $TEST_DIR"
for dir in $DIRS; do
    if [ -d "$dir" ]; then
        echo "Running clang-format on $dir"

        files="$(find "$dir" -type f \( -name "*.cpp" -o -name "*.hpp" -o -name "*.c" -o -name "*.h" \))"
        clang-format --style="file:$PROJECT_DIR/.clang-format" -i $files
    else
        echo "Warning, directory: $dir not found. Skipping."
    fi
done
