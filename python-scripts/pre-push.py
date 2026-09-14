#!/usr/bin/env python3
import sys
from utils import run_script, echo_error

print("Running pre-push git hook...")

if run_script("format-check.py") != 0:
    echo_error("Cannot push: format-check failed")
    sys.exit(1)

if run_script("clang-tidy-check.py") != 0:
    echo_error("Cannot push: clang-tidy-check failed")
    sys.exit(1)

if run_script("semgrep-check.py") != 0:
    echo_error("Cannot push: semgrep-check failed")
    sys.exit(1)

if run_script("build-debug-app.py") != 0:
    echo_error("Cannot push: build-debug-app failed")
    sys.exit(1)

if run_script("build-debug-test.py") != 0:
    echo_error("Cannot push: build-debug-test failed")
    sys.exit(1)

if run_script("run-debug-test.py") != 0:
    echo_error("Cannot push: run-debug-test failed")
    sys.exit(1)

print("pre-push git hook finished successfully")

