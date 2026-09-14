#!/usr/bin/env python3
from utils import run_cmd

run_cmd(["sudo", "modprobe", "vcan"])
run_cmd(["sudo", "ip", "link", "add", "dev", "vcan0", "type", "vcan"])
run_cmd(["sudo", "ip", "link", "set", "up", "vcan0"])

