#!/usr/bin/env python3
import shutil
from utils import BUILD_DEBUG_DIR

print(f"Deleting {BUILD_DEBUG_DIR}")
if BUILD_DEBUG_DIR.exists():
    shutil.rmtree(BUILD_DEBUG_DIR)

