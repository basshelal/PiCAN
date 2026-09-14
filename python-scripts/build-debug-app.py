#!/usr/bin/env python3

import sys
from utils import require_command, run_cmd, PROJECT_DIR, NUM_PROC

require_command("cmake")
run_cmd(["cmake", "--build", "--preset", "debug-app", "-j", str(NUM_PROC)], cwd=PROJECT_DIR)

