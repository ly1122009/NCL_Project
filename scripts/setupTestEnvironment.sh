#!/bin/bash

# Dừng script ngay nếu có lỗi
set -e

echo "=== [1/4] Phát hiện môi trường & Cài đặt công cụ ==="

# Kiểm tra nếu là Termux
if [ -d "/data/data/com.termux/files/usr" ]; then
    ENV_TYPE="TERMUX"
    echo "--> Môi trường: Termux"
    pkg update -y
    pkg install -y cmake clang git make
    PREFIX_PATH="$PREFIX"
    SUDO=""
else
    ENV_TYPE="LINUX/WSL2"
    echo "--> Môi trường: WSL2 / Linux"
    
    # Kiểm tra sudo
    if [ "$(id -u)" -eq 0 ]; then
        SUDO=""
    else
        SUDO="sudo"
    fi

    # Cài đặt package theo distro
    if command -v apt-get &> /dev/null; then
        $SUDO apt-get update
        $SUDO apt-get install -y build-essential cmake git
    elif command -v dnf &> /dev/null; then
        $SUDO dnf groupinstall -y "Development Tools"
        $SUDO dnf install -y cmake git gcc-c++
    elif command -v pacman &> /dev/null; then
        $SUDO pacman -Sy --noconfirm base-devel cmake git
    fi
    PREFIX_PATH="/usr/local"
fi

WORK_DIR="/tmp/gtest_build"
# Tải vào $HOME nếu /tmp không khả dụng (đặc trưng một số bản Termux)
[ "$ENV_TYPE" = "TERMUX" ] && WORK_DIR="$HOME/gtest_build"

echo "=== [2/4] Tải source code Google Test ==="
rm -rf "$WORK_DIR"
git clone https://github.com/google/googletest.git "$WORK_DIR"

echo "=== [3/4] Biên dịch và cài đặt ==="
cd "$WORK_DIR"
mkdir build && cd build

# Cấu hình CMake với tiền tố cài đặt phù hợp
cmake .. -DCMAKE_INSTALL_PREFIX="$PREFIX_PATH"

# Lấy số nhân CPU để biên dịch nhanh
NPROC=$(nproc 2>/dev/null || echo 2)
make -j"$NPROC"

# Cài đặt vào hệ thống
$SUDO make install

echo "=== [4/4] Dọn dẹp ==="
rm -rf "$WORK_DIR"

echo "=========================================="
echo " SUCCESS: Google Test đã cài thành công!"
echo " Môi trường: $ENV_TYPE"
echo "=========================================="