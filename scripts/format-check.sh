#!/usr/bin/env bash

# Add bash-utils, needs to use this complex syntax in order to work from any working directory
source "$(dirname "$(realpath "$0")")/bash-utils.sh"

require_command "clang-format"

echo "Checking format of files"

DIRS="$SRC_DIR $TEST_DIR"
success="true"
for dir in $DIRS; do
    if [ -d "$dir" ]; then
        echo "Running clang-format on $dir"

        files="$(find "$dir" -type f \( -name "*.cpp" -o -name "*.hpp" -o -name "*.c" -o -name "*.h" \))"
        clang-format --style="file:$PROJECT_DIR/.clang-format" --dry-run --Werror -i $files
        if [[ ! "$?" == "0" ]];
        then
          success="false"
        fi
    else
        echo "Warning, directory: $dir not found. Skipping."
    fi
done

if [ "$success" == "false" ]; then
    echo "Files need formatting!" 1>&2
    exit 1
fi
