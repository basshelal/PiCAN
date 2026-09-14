#!/usr/bin/env python3
import sys
from utils import run_script, run_cmd, BUILD_DEBUG_DIR

if run_script("build-debug-test.py") == 0:
    # TODO need a way to launch with memlock removed, use: prlimit --memlock=unlimited your_command
    run_cmd([str(BUILD_DEBUG_DIR / "test" / "pican-test")])

