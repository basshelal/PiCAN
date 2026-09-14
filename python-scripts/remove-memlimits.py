#!/usr/bin/env python3
import subprocess
from utils import run_cmd

try:
    pid = subprocess.check_output(["pidof", "clion"], text=True).strip()
except subprocess.CalledProcessError:
    print("Could not find pid of clion")
    return

run_cmd(["sudo", "prlimit", f"--pid={pid}", "--memlock=unlimited"])

# print memlock status
try:
    output = subprocess.check_output(["prlimit", f"--pid={pid}"], text=True)
    for line in output.splitlines():
        if "MEMLOCK" in line:
            print(line)
except subprocess.CalledProcessError:
    pass

