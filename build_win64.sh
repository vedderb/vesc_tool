#!/usr/bin/env bash
set -euo pipefail

# ==============================================================================
# VESC Tool Windows x86_64 Cross-Build & Single-File Portable EXE Script
# ==============================================================================

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE}")" && pwd)"
BUILD_DIR="$REPO_DIR/build-win6"
APP_VERSION="8.00"
DIST_NAME="vesc_tool_${APP_VERSION}_win64"
DIST_DIR="$REPO_DIR/$DIST_NAME"
ZIP_FILE="$REPO_DIR/${DIST_NAME}.zip"
SINGLE_EXE="$REPO_DIR/${DIST_NAME}.exe"

QT_VER="6.12.0"
QT_HOST="/opt/qt-host/$QT_VER/gcc_64"
QT_WIN="/opt/qt-win/$QT_VER/msvc2022_64"
XWIN_DIR="/opt/xwin"
LLVM_BIN="/usr/lib/llvm-21/bin"
CLANG_INC="$("$LLVM_BIN/clang++" -print-resource-dir)/include"
MKSPEC_DIR="$QT_HOST/mkspecs/linux-win32-clang-msvc"

echo "=== 1. Verifying Toolchain & Directories ==="
for dir in "$QT_HOST" "$QT_WIN" "$XWIN_DIR" "$LLVM_BIN" "$CLANG_INC"; do
  if [ ! -d "$dir" ]; then
    echo "ERROR: Required directory not found: $dir"
    exit 1
  fi
done
if ! command -v makensis >/dev/null 2>&1; then
  echo "ERROR: makensis not found. Install it with: sudo apt-get install -y nsis"
  exit 1
fi

echo "=== 2. Cleaning Previous Build & Packaging Artifacts ==="
rm -rf "$BUILD_DIR" "$DIST_DIR" "$ZIP_FILE" "$SINGLE_EXE"

echo "=== 3. Writing Custom Qt Mkspec ($MKSPEC_DIR) ==="
mkdir -p "$MKSPEC_DIR"
cp -f "$QT_HOST/mkspecs/win32-clang-msvc/qplatformdefs.h" "$MKSPEC_DIR/"

cat << EOF > "$MKSPEC_DIR/qmake.conf"
MAKEFILE_GENERATOR      = UNIX
CONFIG                 += incremental flat precompile_header autogen_precompile_source cross_compile
QMAKE_INCREMENTAL_STYLE = sublib

QMAKE_PLATFORM          = win32
QMAKE_COMPILER          = gcc clang llvm

XWIN_DIR                = $XWIN_DIR
LLVM_BIN                = $LLVM_BIN
CLANG_INC               = $CLANG_INC

TARGET_FLAGS            = --target=x86_64-pc-windows-msvc -msse2 -fms-runtime-lib=dll \\
                          -isystem \$\$CLANG_INC \\
                          -isystem \$\$XWIN_DIR/crt/include \\
                          -isystem \$\$XWIN_DIR/sdk/include/ucrt \\
                          -isystem \$\$XWIN_DIR/sdk/include/um \\
                          -isystem \$\$XWIN_DIR/sdk/include/shared \\
                          -DWIN32 -D_WIN32 -DWIN64 -D_WIN64 -DUNICODE -D_UNICODE \\
                          -DQT_DLL -D_CRT_SECURE_NO_WARNINGS

QMAKE_CC                = \$\$LLVM_BIN/clang
QMAKE_CXX               = \$\$LLVM_BIN/clang++

QMAKE_CFLAGS            = \$\$TARGET_FLAGS -fms-extensions -fms-compatibility -fdelayed-template-parsing
QMAKE_CFLAGS_DEBUG      = -g -O0
QMAKE_CFLAGS_RELEASE    = -O2 -DNDEBUG
QMAKE_CFLAGS_SHLIB      =
QMAKE_CFLAGS_STATIC_LIB =
QMAKE_CFLAGS_YACC       = -Wno-unused -Wno-parentheses
QMAKE_CFLAGS_HIDESYMS   =
QMAKE_CFLAGS_SPLIT_SECTIONS = -ffunction-sections

QMAKE_CXXFLAGS          = \$\$QMAKE_CFLAGS -std=c++17
QMAKE_CXXFLAGS_DEBUG    = \$\$QMAKE_CFLAGS_DEBUG
QMAKE_CXXFLAGS_RELEASE  = \$\$QMAKE_CFLAGS_RELEASE
QMAKE_CXXFLAGS_SHLIB    =
QMAKE_CXXFLAGS_STATIC_LIB =
QMAKE_CXXFLAGS_YACC     = \$\$QMAKE_CFLAGS_YACC
QMAKE_CXXFLAGS_HIDESYMS =
QMAKE_CXXFLAGS_SPLIT_SECTIONS = \$\$QMAKE_CFLAGS_SPLIT_SECTIONS
QMAKE_CXXFLAGS_CXX11    = -std=c++11
QMAKE_CXXFLAGS_CXX14    = -std=c++14
QMAKE_CXXFLAGS_CXX17    = -std=c++17
QMAKE_CXXFLAGS_CXX2A    = -std=c++20

QMAKE_LFLAGS_CXX11      =
QMAKE_LFLAGS_CXX14      =
QMAKE_LFLAGS_CXX17      =
QMAKE_LFLAGS_CXX2A      =

QMAKE_LINK              = \$\$LLVM_BIN/clang++
QMAKE_LINK_SHLIB        = \$\$LLVM_BIN/clang++
QMAKE_LINK_C            = \$\$LLVM_BIN/clang
QMAKE_LINK_C_SHLIB      = \$\$LLVM_BIN/clang

QMAKE_LFLAGS            = --target=x86_64-pc-windows-msvc -fuse-ld=lld -fms-runtime-lib=dll \\
                          -Wl,/nodefaultlib:libcmt,/nodefaultlib:libucrt,/nodefaultlib:libvcruntime \\
                          -L\$\$XWIN_DIR/crt/lib/x86_64 \\
                          -L\$\$XWIN_DIR/sdk/lib/um/x86_64 \\
                          -L\$\$XWIN_DIR/sdk/lib/ucrt/x86_64
QMAKE_LFLAGS_DEBUG      = -g
QMAKE_LFLAGS_RELEASE    =
QMAKE_LFLAGS_APP        =
QMAKE_LFLAGS_SHLIB      = -shared
QMAKE_LFLAGS_PLUGIN     = -shared
QMAKE_LFLAGS_WINDOWS    = -Wl,/subsystem:windows
QMAKE_LFLAGS_CONSOLE    = -Wl,/subsystem:console
QMAKE_LFLAGS_GCSECTIONS = -Wl,/opt:ref

QMAKE_LIB               = \$\$LLVM_BIN/llvm-ar cqs
QMAKE_RC                = \$\$LLVM_BIN/llvm-rc
QMAKE_LIBS              = -lucrt -lvcruntime -lmsvcrt -lkernel32 -luser32 -lgdi32 -lwinspool -lshell32 -lole32 -loleaut32 -luuid -lcomdlg32 -ladvapi32
QMAKE_LIBS_QT_ENTRY     = -lQt6EntryPoint

QMAKE_EXTENSION_SHLIB   = dll
QMAKE_PREFIX_SHLIB      =
QMAKE_EXTENSION_STATICLIB = lib
QMAKE_PREFIX_STATICLIB  =
QMAKE_EXTENSION_EXE     = exe

QMAKE_COPY              = cp -f
QMAKE_COPY_DIR          = cp -rf
QMAKE_MOVE              = mv -f
QMAKE_DEL_FILE          = rm -f
QMAKE_DEL_DIR           = rmdir
QMAKE_MKDIR             = mkdir -p
QMAKE_CHK_DIR_EXISTS    = test -d
QMAKE_NULL_DEVICE       = /dev/null

QMAKE_MOC               = $QT_HOST/libexec/moc
QMAKE_UIC               = $QT_HOST/libexec/uic
QMAKE_RCC               = $QT_HOST/libexec/rcc
QMAKE_QSB               = $QT_HOST/bin/qsb

load(qt_config)
EOF

echo "=== 4. Preparing Build Directory & Running qmake6 ==="
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cat << EOF > "$BUILD_DIR/qt-win6.conf"
[Paths]
Prefix = $QT_WIN
ArchData = $QT_WIN
Data = $QT_WIN
Headers = $QT_WIN/include
Libraries = $QT_WIN/lib
LibraryExecutables = $QT_WIN/bin
Binaries = $QT_WIN/bin
Plugins = $QT_WIN/plugins
QmlImports = $QT_WIN/qml
HostPrefix = $QT_HOST
HostBinaries = $QT_HOST/bin
HostLibexecs = $QT_HOST/libexec
HostData = $QT_WIN
EOF

"$QT_HOST/bin/qmake6" "$REPO_DIR/vesc_tool.pro" \
  -spec "$MKSPEC_DIR" \
  -qtconf "$BUILD_DIR/qt-win6.conf" \
  CONFIG+=release

echo "=== 5. Patching ELF-only Linker Flags & Compiling ==="
sed -i 's/-Wl,-z,max-page-size=16384//g; s/-Wl,-z,common-page-size=16384//g; s/-z max-page-size=16384//g; s/-z common-page-size=16384//g' Makefile
make -j"$(nproc)"

BUILT_EXE="$(find "$BUILD_DIR" -maxdepth 1 -type f \( -name "vesc_tool*.exe" -o -name "vesc_tool_*" \) ! -name "*.o" ! -name "*.cpp" ! -name "*.h" | head -n 1)"
if [ -z "$BUILT_EXE" ]; then
  echo "ERROR: Built executable not found in $BUILD_DIR"
  exit 1
fi
EXE_BASENAME="$(basename "$BUILT_EXE" .exe).exe"
echo "Built binary: $BUILT_EXE -> $EXE_BASENAME"

echo "=== 6. Packaging Windows Distribution Folder ==="
mkdir -p "$DIST_DIR/plugins" "$DIST_DIR/qml"
cp "$BUILT_EXE" "$DIST_DIR/$EXE_BASENAME"

# Copy required Qt plugins
for plugin_dir in platforms styles imageformats iconengines tls networkinformation geoservices position; do
  if [ -d "$QT_WIN/plugins/$plugin_dir" ]; then
    cp -rf "$QT_WIN/plugins/$plugin_dir" "$DIST_DIR/plugins/"
  fi
done

# Copy required QML modules
for qml_mod in QtQuick QtQml Qt5Compat QtCore Qt Qt3D QtLocation QtPositioning; do
  if [ -d "$QT_WIN/qml/$qml_mod" ]; then
    cp -rf "$QT_WIN/qml/$qml_mod" "$DIST_DIR/qml/"
  fi
done

# Remove debug symbols (.pdb) and debug DLLs (*d.dll)
find "$DIST_DIR/plugins" "$DIST_DIR/qml" -name "*.pdb" -delete
find "$DIST_DIR/plugins" "$DIST_DIR/qml" -name "*d.dll" ! -name "Qt6Quick3D*.dll" ! -name "qtquick3d*.dll" -delete

# Resolve all direct and transitive Qt6 DLL dependencies across .exe, plugins/, and qml/
for pass in 1 2 3 4; do
  find "$DIST_DIR" \( -name "*.dll" -o -name "*.exe" \) | while read -r bin; do
    "$LLVM_BIN/llvm-objdump" -p "$bin" 2>/dev/null | awk '/DLL Name:/ {print $3}' | while read -r dll; do
      if [ -f "$QT_WIN/bin/$dll" ] && [ ! -f "$DIST_DIR/$dll" ]; then
        cp -v "$QT_WIN/bin/$dll" "$DIST_DIR/"
      fi
    done
  done
done

# Copy runtime shader compiler if available
[ -f "$QT_WIN/bin/d3dcompiler_47.dll" ] && cp -f "$QT_WIN/bin/d3dcompiler_47.dll" "$DIST_DIR/"

# Write qt.conf for relative plugin/QML resolution on Windows
cat << 'EOF' > "$DIST_DIR/qt.conf"
[Paths]
Plugins = plugins
QmlImports = qml
EOF

# Copy MSVC runtime DLLs from xwin
find "$XWIN_DIR" -type f \( -iname "vcruntime140*.dll" -o -iname "msvcp140*.dll" -o -iname "concrt140.dll" \) -exec cp -fv {} "$DIST_DIR/" \;

echo "=== 7. Creating Zip Archive ==="
cd "$REPO_DIR"
zip -qr "$ZIP_FILE" "$DIST_NAME/"

echo "=== 8. Building Single-File Portable Executable ($SINGLE_EXE) ==="
PORTABLE_NSI="$BUILD_DIR/portable.nsi"
ICON_OPT=""
if [ -f "$REPO_DIR/res/icons/vesc_tool.ico" ]; then
  ICON_OPT="Icon \"$REPO_DIR/res/icons/vesc_tool.ico\""
elif [ -f "$REPO_DIR/res/vesc_tool.ico" ]; then
  ICON_OPT="Icon \"$REPO_DIR/res/vesc_tool.ico\""
fi

cat << EOF > "$PORTABLE_NSI"
Unicode True
Name "VESC Tool $APP_VERSION"
OutFile "$SINGLE_EXE"
RequestExecutionLevel user
SilentInstall silent
SetCompressor /SOLID lzma
$ICON_OPT

Section "Run"
  StrCpy \$INSTDIR "\$LOCALAPPDATA\\VESC_Tool_${APP_VERSION}"
  CreateDirectory "\$INSTDIR"
  SetOutPath "\$INSTDIR"
  SetOverwrite ifnewer
  File /r "$DIST_DIR\\*.*"

  ExecWait '"\$INSTDIR\\$EXE_BASENAME"'
SectionEnd
EOF

makensis -V2 "$PORTABLE_NSI"

echo "=== 9. Cleaning Build Artifacts ==="
rm -rf "$BUILD_DIR" "$DIST_DIR"

echo "=== Done! ==="
echo "Single-File EXE : $SINGLE_EXE"
echo "Portable Zip    : $ZIP_FILE"
echo "Folder          : $DIST_DIR"

