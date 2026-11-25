#!/usr/bin/env bash
set -euo pipefail

# ==============================================================================
# Windows x86_64 Cross-Compilation Toolchain Installer for Ubuntu/Debian
# Installs: LLVM/Clang 21 + lld, xwin (MSVC CRT + Windows SDK),
#           Qt 6.12.0 Linux Host Tools, and Qt 6.12.0 Windows MSVC 2022 Target
# ==============================================================================

QT_VER="6.12.0"
LLVM_VER="21"
QT_HOST_DIR="/opt/qt-host"
QT_WIN_DIR="/opt/qt-win"
XWIN_DIR="/opt/xwin"

# Qt 6 addon modules required by VESC Tool
QT_MODULES=(
  qtserialport
  qt5compat
  qtshadertools
  qtlocation
  qtpositioning
  qtconnectivity
)

if [ "$EUID" -ne 0 ]; then
  echo "Please run with sudo: sudo $0"
  exit 1
fi

echo "=== 1. Installing Base System Dependencies ==="
apt-get update
apt-get install -y --no-install-recommends \
  ca-certificates \
  curl \
  gnupg \
  lsb-release \
  software-properties-common \
  build-essential \
  make \
  cmake \
  ninja-build \
  zip \
  unzip \
  p7zip-full \
  python3 \
  python3-pip \
  python3-venv \
  libgl1-mesa-dev \
  libxkbcommon-x11-0 \
  libdbus-1-3 \
  libfontconfig1 \
  nsis

echo "=== 2. Installing LLVM / Clang $LLVM_VER & LLD ==="
if ! command -v "clang++-$LLVM_VER" >/dev/null 2>&1; then
  curl -fsSL https://llvm.org -o /tmp/llvm.sh
  chmod +x /tmp/llvm.sh
  /tmp/llvm.sh "$LLVM_VER"
  rm -f /tmp/llvm.sh
fi
apt-get install -y "clang-$LLVM_VER" "lld-$LLVM_VER" "llvm-$LLVM_VER"

echo "=== 3. Installing aqtinstall & xwin in Isolated Tools Env ==="
TOOLS_VENV="/opt/win64-build-tools"
python3 -m venv "$TOOLS_VENV"
"$TOOLS_VENV/bin/pip" install --upgrade pip
"$TOOLS_VENV/bin/pip" install aqtinstall

# Download prebuilt xwin binary (avoids long Rust/cargo compile times)
XWIN_VER="0.6.7"
XWIN_ARCH="x86_64-unknown-linux-musl"
if [ ! -x "$TOOLS_VENV/bin/xwin" ]; then
  curl -fsSL "https://github.com{XWIN_VER}/xwin-${XWIN_VER}-${XWIN_ARCH}.tar.gz" \
    -o /tmp/xwin.tar.gz
  tar -xzf /tmp/xwin.tar.gz -C /tmp
  mv "/tmp/xwin-${XWIN_VER}-${XWIN_ARCH}/xwin" "$TOOLS_VENV/bin/xwin"
  chmod +x "$TOOLS_VENV/bin/xwin"
  rm -rf /tmp/xwin.tar.gz "/tmp/xwin-${XWIN_VER}-${XWIN_ARCH}"
fi

echo "=== 4. Downloading & Extracting MSVC CRT + Windows SDK to $XWIN_DIR ==="
if [ ! -d "$XWIN_DIR/crt" ] || [ ! -d "$XWIN_DIR/sdk" ]; then
  mkdir -p "$XWIN_DIR"
  "$TOOLS_VENV/bin/xwin" --accept-license --cache-dir /tmp/xwin-cache splat --output "$XWIN_DIR"
  rm -rf /tmp/xwin-cache
else
  echo "Skipping xwin splat ($XWIN_DIR already exists)."
fi

echo "=== 5. Installing Qt $QT_VER Linux Host Tools to $QT_HOST_DIR ==="
mkdir -p "$QT_HOST_DIR"
if [ ! -x "$QT_HOST_DIR/$QT_VER/gcc_64/bin/qmake6" ]; then
  "$TOOLS_VENV/bin/aqt" install-qt linux desktop "$QT_VER" linux_gcc_64 -O "$QT_HOST_DIR"
else
  echo "Skipping Qt host install ($QT_HOST_DIR/$QT_VER/gcc_64 already exists)."
fi

echo "=== 6. Installing Qt $QT_VER Windows Target (msvc2022_64) to $QT_WIN_DIR ==="
mkdir -p "$QT_WIN_DIR"
"$TOOLS_VENV/bin/aqt" install-qt windows desktop "$QT_VER" win64_msvc2022_64 \
  -O "$QT_WIN_DIR" \
  -m "${QT_MODULES[@]}"

# Install OpenSSL 3 runtime DLLs for Qt TLS backend
"$TOOLS_VENV/bin/aqt" install-tool windows desktop tools_opensslv3_x64 -O "$QT_WIN_DIR"

echo "=== 7. Fixing Permissions for Non-Root Builds ==="
chmod -R a+rX "$XWIN_DIR" "$QT_HOST_DIR" "$QT_WIN_DIR" "$TOOLS_VENV"

echo "=============================================================================="
echo "Toolchain installation complete!"
echo "  LLVM/Clang : /usr/lib/llvm-$LLVM_VER/bin"
echo "  MSVC / SDK : $XWIN_DIR"
echo "  Qt Host    : $QT_HOST_DIR/$QT_VER/gcc_64"
echo "  Qt Windows : $QT_WIN_DIR/$QT_VER/msvc2022_64"
echo "=============================================================================="

