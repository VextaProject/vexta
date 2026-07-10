#!/usr/bin/env bash
set -euo pipefail

if [ "${EUID}" -eq 0 ]; then
    echo "Do not run this entire script as root."
    echo "Run it as a normal user with sudo access."
    exit 1
fi

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
JOBS="${VEXTA_BUILD_JOBS:-$(nproc)}"

echo "========================================"
echo " Vexta Core node installer"
echo "========================================"
echo "Project root: $PROJECT_ROOT"
echo "Build jobs:   $JOBS"
echo

if ! command -v sudo >/dev/null 2>&1; then
    echo "ERROR: sudo is required."
    exit 1
fi

if ! command -v apt-get >/dev/null 2>&1; then
    echo "ERROR: This installer currently supports Ubuntu/Debian systems only."
    exit 1
fi

echo "Installing build dependencies..."
sudo apt-get update
sudo DEBIAN_FRONTEND=noninteractive apt-get install -y \
    build-essential \
    automake \
    autotools-dev \
    autoconf \
    libtool \
    pkg-config \
    python3 \
    git \
    curl \
    ca-certificates \
    libevent-dev \
    libboost-system-dev \
    libboost-filesystem-dev \
    libboost-test-dev \
    libboost-thread-dev \
    libsqlite3-dev \
    libzmq3-dev

cd "$PROJECT_ROOT"

if [ ! -x ./configure ]; then
    echo "Generating configure script..."
    ./autogen.sh
fi

echo "Configuring Vexta Core..."
./configure \
    --without-gui \
    --disable-tests \
    --disable-bench \
    --with-incompatible-bdb

echo "Building Vexta Core..."
make -j"$JOBS"

echo "Installing binaries into /usr/local/bin..."
sudo install -m 0755 src/vextad /usr/local/bin/vextad
sudo install -m 0755 src/vexta-cli /usr/local/bin/vexta-cli
sudo install -m 0755 src/vexta-tx /usr/local/bin/vexta-tx
sudo install -m 0755 src/vexta-util /usr/local/bin/vexta-util
sudo install -m 0755 src/vexta-wallet /usr/local/bin/vexta-wallet

echo "Installing systemd service template..."
sudo install -m 0644 \
    "$PROJECT_ROOT/contrib/vexta/vextad@.service" \
    /etc/systemd/system/vextad@.service

sudo systemctl daemon-reload

echo
echo "Installed versions:"
vextad --version | head -1
vexta-cli --version | head -1

echo
echo "Vexta Core installation completed."
echo "Next steps:"
echo "  mkdir -p \$HOME/.vexta"
echo "  cp contrib/vexta/vexta-mainnet.conf \$HOME/.vexta/vexta.conf"
echo "  nano \$HOME/.vexta/vexta.conf"
echo
echo "Start Vexta manually:"
echo "  vextad -daemon"
echo
echo "Or enable the systemd service:"
echo "  sudo systemctl enable vextad@\$(whoami)"
echo "  sudo systemctl start vextad@\$(whoami)"
echo "  sudo systemctl status vextad@\$(whoami)"
