#!/usr/bin/env bash

sudo prlimit --pid $(pidof clion) --memlock=unlimited
prlimit --pid $(pidof clion) | grep "MEMLOCK"