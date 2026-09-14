#!/usr/bin/env python3
import sys
from utils import run_script

ret1 = run_script("clean-debug.py")
if ret1 != 0:
    sys.exit(ret1)
ret2 = run_script("clean-release.py")
if ret2 != 0:
    sys.exit(ret2)
