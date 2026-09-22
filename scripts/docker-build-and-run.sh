#!/usr/bin/env bash

# Add bash-utils, needs to use this complex syntax in order to work from any working directory
source "$(dirname "$(realpath "$0")")/bash-utils.sh"

docker build -t pican "$PROJECT_DIR" &&
  docker run -t pican:latest
