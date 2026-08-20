#!/usr/bin/env bash

set -e

echo "=========================================="
echo "   Ubuntu C/C++ Development Environment"
echo "=========================================="

# --------------------------------------------------
# Check OS
# --------------------------------------------------

if [ ! -f /etc/os-release ]; then
    echo "ERROR: /etc/os-release not found."
    exit 1
fi

. /etc/os-release

if [ "$ID" != "ubuntu" ]; then
    echo "ERROR: This script requires Ubuntu."
    exit 1
fi

echo "OS      : $PRETTY_NAME"
echo "Arch    : $(dpkg --print-architecture)"
echo "Kernel  : $(uname -r)"
echo ""

# --------------------------------------------------
# Fix package manager
# --------------------------------------------------

echo "[1/4] Checking package manager..."

 dpkg --configure -a
 apt --fix-broken install -y
 apt update

# --------------------------------------------------
# Core C/C++ development
# --------------------------------------------------

echo "[2/4] Installing C/C++ toolchain..."

 apt install -y \
    build-essential \
    gcc \
    g++ \
    clang \
    cmake \
    ninja-build \
    pkg-config

# --------------------------------------------------
# Development tools
# --------------------------------------------------

echo "[3/4] Installing development tools..."

 apt install -y \
    git \
    openssh-client \
    clang-format \
    clang-tidy \
    gdb \
    valgrind \
    doxygen

# --------------------------------------------------
# Verification
# --------------------------------------------------

echo ""
echo "[4/4] Verifying installation..."
echo ""

printf "%-15s " "GCC:"
gcc --version | head -n 1

printf "%-15s " "G++:"
g++ --version | head -n 1

printf "%-15s " "Clang:"
clang --version | head -n 1

printf "%-15s " "CMake:"
cmake --version | head -n 1

printf "%-15s " "Ninja:"
ninja --version

printf "%-15s " "Make:"
make --version | head -n 1

printf "%-15s " "Git:"
git --version

printf "%-15s " "Clang-format:"
clang-format --version

printf "%-15s " "Clang-tidy:"
clang-tidy --version | head -n 1

printf "%-15s " "GDB:"
gdb --version | head -n 1

printf "%-15s " "Valgrind:"
valgrind --version

printf "%-15s " "Doxygen:"
doxygen --version

printf "%-15s " "Pkg-config:"
pkg-config --version

echo ""
echo "=========================================="
echo "   C/C++ environment ready!"
echo "=========================================="