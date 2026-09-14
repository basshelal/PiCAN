#!/usr/bin/env python3
from pathlib import Path
from utils import require_command, run_cmd, PROJECT_DIR, SRC_DIR, TEST_DIR

require_command("clang-format")
print("Formatting files in place")

dirs = [SRC_DIR, TEST_DIR]

for d in dirs:
    if d.is_dir():
        print(f"Running clang-format on {d}")
        files: list[str] = []
        for ext in ("*.cpp", "*.hpp", "*.c", "*.h"):
            files.extend(str(p) for p in d.rglob(ext) if p.is_file())
        
        if files:
            run_cmd([
                "clang-format",
                f"--style=file:{PROJECT_DIR / '.clang-format'}",
                "-i"
            ] + files)
    else:
        print(f"Warning, directory: {d} not found. Skipping.")

