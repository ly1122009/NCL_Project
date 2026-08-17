#!/usr/bin/env bash

set -e

echo "=========================================="
echo "       GoogleTest Setup"
echo "=========================================="

# --------------------------------------------------
# Configuration
# --------------------------------------------------

GTEST_VERSION="v1.17.0"
INSTALL_DIR="/opt/googletest"

# --------------------------------------------------
# Check dependencies
# --------------------------------------------------

echo "[1/5] Checking dependencies..."

command -v git >/dev/null 2>&1 || {
    echo "ERROR: git is not installed."
    exit 1
}

command -v cmake >/dev/null 2>&1 || {
    echo "ERROR: cmake is not installed."
    exit 1
}

command -v g++ >/dev/null 2>&1 || {
    echo "ERROR: g++ is not installed."
    exit 1
}

# --------------------------------------------------
# Clone GoogleTest
# --------------------------------------------------

echo "[2/5] Downloading GoogleTest ${GTEST_VERSION}..."

if [ -d "$INSTALL_DIR" ]; then
    echo "GoogleTest directory already exists:"
    echo "  $INSTALL_DIR"

    read -r -p "Remove and reinstall? [y/N] " answer

    if [[ "$answer" =~ ^[Yy]$ ]]; then
        sudo rm -rf "$INSTALL_DIR"
    else
        echo "Using existing installation."
    fi
fi

if [ ! -d "$INSTALL_DIR" ]; then
    sudo git clone \
        --depth 1 \
        --branch "$GTEST_VERSION" \
        https://github.com/google/googletest.git \
        "$INSTALL_DIR"
fi

# --------------------------------------------------
# Build
# --------------------------------------------------

echo "[3/5] Building GoogleTest..."

sudo rm -rf "$INSTALL_DIR/build"

sudo cmake \
    -S "$INSTALL_DIR" \
    -B "$INSTALL_DIR/build" \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_GMOCK=ON \
    -DINSTALL_GTEST=ON

sudo cmake \
    --build "$INSTALL_DIR/build"

# --------------------------------------------------
# Install
# --------------------------------------------------

echo "[4/5] Installing GoogleTest..."

sudo cmake \
    --install "$INSTALL_DIR/build"

# --------------------------------------------------
# Verify
# --------------------------------------------------

echo "[5/5] Verifying installation..."

echo ""
echo "GoogleTest files:"
ls -l /usr/local/lib/ 2>/dev/null | grep -E 'gtest|gmock' || true

echo ""
echo "CMake package:"
find /usr/local/lib/cmake \
    -maxdepth 2 \
    -type d \
    \( -name "GTest" -o -name "GTest*" \) \
    2>/dev/null || true

echo ""
echo "=========================================="
echo "       GoogleTest setup complete"
echo "=========================================="

echo ""
echo "Version:"
grep -E 'GOOGLETEST_VERSION' \
    "$INSTALL_DIR/CMakeLists.txt" \
    2>/dev/null || true

echo ""
echo "Installed to:"
echo "  $INSTALL_DIR"

echo ""
echo "CMake can now use:"
echo ""
echo "  find_package(GTest CONFIG REQUIRED)"
echo ""
echo "=========================================="
