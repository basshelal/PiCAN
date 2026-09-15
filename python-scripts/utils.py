#!/usr/bin/env python3
import os
import sys
import shutil
import subprocess
from pathlib import Path

# The scripts directory where this utils file is located
SCRIPTS_DIR: Path = Path(__file__).resolve().parent

# The project root dir
PROJECT_DIR: Path = SCRIPTS_DIR.parent

# The src dir in the project root
SRC_DIR: Path = PROJECT_DIR / "src"

# The test dir in the project root
TEST_DIR: Path = PROJECT_DIR / "test"

# The build dir in the project root
BUILD_DIR: Path = PROJECT_DIR / "build"

# The debug build dir
BUILD_DEBUG_DIR: Path = BUILD_DIR / "debug"

# The release build dir
BUILD_RELEASE_DIR: Path = BUILD_DIR / "relinfo"

# Number of processors
NUM_PROC: int = os.cpu_count() or 1

def ensure_build_dirs() -> None:
    BUILD_DEBUG_DIR.mkdir(parents=True, exist_ok=True)
    BUILD_RELEASE_DIR.mkdir(parents=True, exist_ok=True)

if not BUILD_DIR.exists():
    ensure_build_dirs()

def require_command(cmd: str) -> None:
    if shutil.which(cmd) is None:
        print(f"Error: {cmd} could not be found!", file=sys.stderr)
        sys.exit(1)

def echo_error(msg: str) -> None:
    print(f"\033[0;31m{msg}\033[0m", file=sys.stderr)

def run_script(script_name: str, args: list[str] = []) -> int:
    script_path = SCRIPTS_DIR / script_name
    return subprocess.run([sys.executable, str(script_path)] + args).returncode

def run_cmd(cmd: list[str], cwd: Path | None = None, check: bool = True) -> int:
    try:
        result = subprocess.run(cmd, cwd=cwd)
        if check and result.returncode != 0:
            sys.exit(result.returncode)
        return result.returncode
    except FileNotFoundError:
        print(f"Command not found: {cmd[0]}", file=sys.stderr)
        sys.exit(1)
