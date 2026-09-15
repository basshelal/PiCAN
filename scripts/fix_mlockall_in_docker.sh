#!/bin/bash
set -e

# Docker builds do not have CAP_IPC_LOCK by default, so mlockall will fail.
# This script compiles a tiny shared library that stubs out mlockall to return 0 (success)
# and then runs the provided command with LD_PRELOAD.

DUMMY_C="/tmp/dummy_mlockall.c"
DUMMY_SO="/tmp/libdummy_mlockall.so"

cat << 'EOF' > "$DUMMY_C"
int mlockall(int flags) {
    // Lie and say it succeeded so tests and app can run in unprivileged Docker builds
    return 0;
}
EOF

gcc -shared -fPIC "$DUMMY_C" -o "$DUMMY_SO"

export LD_PRELOAD="$DUMMY_SO"
exec "$@"
