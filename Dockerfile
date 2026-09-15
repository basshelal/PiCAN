# Use a recent Ubuntu image to get recent compilers (Ubuntu 26.04)
FROM ubuntu:26.04

# Avoid tzdata prompts during apt-get install
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies required for building C++ projects, CMake, and autotools (for libbacktrace)
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    ninja-build \
    mold \
    wget \
    curl \
    ca-certificates \
    make \
    autoconf \
    automake \
    libtool \
    libcap2-bin 

# Install uv for Python dependencies (clang-tidy, semgrep, clang-format)
COPY --from=ghcr.io/astral-sh/uv:latest /uv /uvx /bin/

# Set up the working directory
WORKDIR /app

# Copy the project files
COPY . .

# Sync python dependencies to set up formatting & linting tools
RUN uv sync

# Activate the venv and run CMake workflows wrapped with our mlockall fix script
RUN . .venv/bin/activate && cmake --workflow --preset debug
RUN . .venv/bin/activate && cmake --workflow --preset relinfo

# Keep the container running if run interactively, or just act as a successful build check
CMD ["bash"]
