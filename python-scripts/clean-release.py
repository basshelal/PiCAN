#!/usr/bin/env python3
import shutil
from utils import BUILD_RELEASE_DIR

print(f"Deleting {BUILD_RELEASE_DIR}")
if BUILD_RELEASE_DIR.exists():
    shutil.rmtree(BUILD_RELEASE_DIR)

