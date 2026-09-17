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
    libcap2-bin \
    g++-aarch64-linux-gnu \
    crossbuild-essential-arm64 \
    libc6-dev-arm64-cross \
    libc6-arm64-cross \
    linux-libc-dev-arm64-cross \
    qemu-user && \
    rm -rf /var/lib/apt/lists/*

# Install uv for Python dependencies (clang-tidy, semgrep, clang-format)
COPY --from=ghcr.io/astral-sh/uv:latest /uv /uvx /bin/

# Fix Git "dubious ownership" warnings which frequently break CPM in Docker
RUN git config --global --add safe.directory '*'

# Set up the working directory
WORKDIR /app

# Copy the project files ignoring everything in .dockerignore
COPY . .

# Sync python dependencies to set up formatting & linting tools
RUN uv sync

# Activate our venv
ENV PATH="/app/.venv/bin:$PATH"

# TODO these need to be configure and build ONLY, have the CMD do the test running, confusing because it splits our
#  workflow
RUN cmake --preset debug-config && cmake --build --preset debug-test
RUN cmake --preset relinfo-config && cmake --build --preset relinfo-test
RUN cmake --preset debug-aarch64-config && cmake --build --preset debug-aarch64-test
RUN cmake --preset relinfo-aarch64-config && cmake --build --preset relinfo-aarch64-test

# Run all tests
CMD ["scripts/run-all-tests.sh"]
