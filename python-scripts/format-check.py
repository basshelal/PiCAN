#!/usr/bin/env python3
import sys
from pathlib import Path
from utils import require_command, run_cmd, PROJECT_DIR, SRC_DIR, TEST_DIR, echo_error

require_command("clang-format")
print("Checking format of files")

dirs = [SRC_DIR, TEST_DIR]
success = True

for d in dirs:
    if d.is_dir():
        print(f"Running clang-format on {d}")
        files: list[str] = []
        for ext in ("*.cpp", "*.hpp", "*.c", "*.h"):
            files.extend(str(p) for p in d.rglob(ext) if p.is_file())
        
        if files:
            ret = run_cmd([
                "clang-format",
                f"--style=file:{PROJECT_DIR / '.clang-format'}",
                "--dry-run", "--Werror", "-i"
            ] + files, check=False)
            if ret != 0:
                success = False
    else:
        print(f"Warning, directory: {d} not found. Skipping.")

if not success:
    echo_error("Files need formatting!")
    sys.exit(1)

