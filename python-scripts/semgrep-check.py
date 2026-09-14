#!/usr/bin/env python3
from utils import require_command, run_cmd, PROJECT_DIR, SRC_DIR, NUM_PROC

require_command("semgrep")
run_cmd([
    "semgrep", "scan",
    "-c", str(PROJECT_DIR / ".semgrep.yaml"),
    "-j", str(NUM_PROC),
    "--strict",
    "--quiet",
    "--error",
    str(SRC_DIR)
], cwd=PROJECT_DIR)

