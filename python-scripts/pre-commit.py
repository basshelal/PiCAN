#!/usr/bin/env python3
import sys
from utils import run_script, echo_error

print("Running pre-commit git hook...")

if run_script("format-check.py") != 0:
    echo_error("Cannot commit: format-check failed")
    sys.exit(1)

if run_script("clang-tidy-check.py") != 0:
    echo_error("Cannot commit: clang-tidy-check failed")
    sys.exit(1)

if run_script("semgrep-check.py") != 0:
    echo_error("Cannot commit: semgrep-check failed")
    sys.exit(1)

print("pre-commit git hook finished successfully")

