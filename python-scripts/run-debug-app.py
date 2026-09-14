#!/usr/bin/env python3
import sys
from utils import run_script, run_cmd, BUILD_DEBUG_DIR

if run_script("build-debug-app.py") == 0:
    run_cmd([str(BUILD_DEBUG_DIR / "src" / "pican-app")])

