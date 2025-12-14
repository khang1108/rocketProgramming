#!/bin/bash
# install.sh - Install Qt6 and build project

# Function to install Qt6
install_qt() {
    if command -v pacman &> /dev/null; then
        sudo pacman -S --noconfirm qt6-base qt6-multimedia
    elif command -v apt &> /dev/null; then
        sudo apt update && sudo apt install -y qt6-base-dev qt6-multimedia-dev
    elif command -v dnf &> /dev/null; then
        sudo dnf install -y qt6-qtbase-devel qt6-qtmultimedia-devel
    else
        echo "Package manager not supported"
        exit 1
    fi
}

# Check Qt6
if ! cmake --find-package -DNAME=Qt6 -DCOMPILER_ID=GNU -DLANGUAGE=CXX &> /dev/null; then
    echo "Qt6 not found. Installing..."
    install_qt
fi

# Build
mkdir -p build && cd build
cmake ..
make -j$(nproc)

echo "Installation complete! Run:"
echo "  cd build/bin"
echo "  ./server 8554"