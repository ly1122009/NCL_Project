#!/bin/bash

# Dừng ngay nếu có lỗi
set -e

echo "========================================================="
echo "   SETUP ENVIRONMENT FOR EMBEDDED LINUX (C/C++)          "
echo "========================================================="

# 1. Nhận diện môi trường
IS_TERMUX=false
if [ -d "/data/data/com.termux/files/usr" ]; then
    IS_TERMUX=true
    echo "[+] Phát hiện môi trường: Termux (Android)"
    SUDO=""
else
    echo "[+] Phát hiện môi trường: WSL2 / Linux Standard"
    if [ "$(id -u)" -ne 0 ]; then
        SUDO="sudo"
    else
        SUDO=""
    fi
fi

# 2. Cài đặt trên Termux
if [ "$IS_TERMUX" = true ]; then
    echo "[+] Đang cập nhật gói và cài đặt công cụ cho Termux..."
    pkg update -y && pkg upgrade -y
    
    # Toolchain cơ bản
    pkg install -y clang make cmake ninja pkg-config git gdb \
                   libusb libftdi openocd minicom python-pip

    echo "---------------------------------------------------------"
    echo "LƯU Ý TERMUX:"
    echo " - Cross-compiler (arm-linux-gnueabi) không hỗ trợ sẵn qua 'pkg'."
    echo " - Bạn nên biên dịch trực tiếp (native) hoặc dùng Clang target:"
    echo "   VD: clang++ --target=aarch64-linux-gnu ..."
    echo "---------------------------------------------------------"

# 3. Cài đặt trên WSL2 / Debian / Ubuntu
else
    echo "[+] Đang cập nhật hệ thống và cài đặt cho WSL2/Linux..."
    $SUDO apt-get update -y
    $SUDO apt-get upgrade -y

    echo "[+] 1. Cài đặt Build Tools cơ bản..."
    $SUDO apt-get install -y \
        build-essential \
        cmake \
        ninja-build \
        git \
        pkg-config \
        autoconf \
        automake \
        libtool \
        ccache

    echo "[+] 2. Cài đặt Cross-Compiler (ARM32 & ARM64)..."
    $SUDO apt-get install -y \
        gcc-arm-linux-gnueabihf \
        g++-arm-linux-gnueabihf \
        gcc-aarch64-linux-gnu \
        g++-aarch64-linux-gnu

    echo "[+] 3. Cài đặt dependencies để build Kernel / U-Boot / Yocto / Buildroot..."
    $SUDO apt-get install -y \
        bison \
        flex \
        libncurses5-dev \
        libncursesw5-dev \
        libssl-dev \
        bc \
        rsync \
        cpio \
        unzip \
        u-boot-tools \
        lzop \
        device-tree-compiler

    echo "[+] 4. Cài đặt công cụ Debug & Giả lập (QEMU, GDB, OpenOCD)..."
    $SUDO apt-get install -y \
        gdb-multiarch \
        qemu-system-arm \
        qemu-system-x86 \
        qemu-user-static \
        openocd \
        minicom \
        screen \
        net-tools \
        iproute2 \
        libgpiod-dev \
        gpiod
fi

echo "========================================================="
echo " SUCCESS: Môi trường Embedded Linux C/C++ đã sẵn sàng!   "
echo "========================================================="