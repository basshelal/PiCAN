#!/usr/bin/env python3
import sys
from utils import require_command, run_cmd, PROJECT_DIR, BUILD_DEBUG_DIR, BUILD_RELEASE_DIR

require_command("cmake")

run_cmd([
    "cmake", "-S", str(PROJECT_DIR), "-B", str(BUILD_DEBUG_DIR),
    "-DCMAKE_BUILD_TYPE=Debug", "-G", "Ninja"
], cwd=PROJECT_DIR)

run_cmd([
    "cmake", "-S", str(PROJECT_DIR), "-B", str(BUILD_RELEASE_DIR),
    "-DCMAKE_BUILD_TYPE=Release", "-G", "Ninja"
], cwd=PROJECT_DIR)