#!/usr/bin/env python3
from utils import require_command, run_cmd, PROJECT_DIR, BUILD_DEBUG_DIR, SRC_DIR, NUM_PROC

require_command("run-clang-tidy")
run_cmd([
    "run-clang-tidy",
    "-config-file", str(PROJECT_DIR / ".clang-tidy"),
    "-j", str(NUM_PROC),
    "-use-color", "true",
    "-quiet",
    "-p", f"{BUILD_DEBUG_DIR}/",
    f"^{SRC_DIR}/.*"
], cwd=PROJECT_DIR)

