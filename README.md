# ExiTool

This is the source code of VESC Tool. A pre-compiled binary of both the stable release as well as the development release packaged with all the matching firmware for all supported hardware can be downloaded at http://vesc-project.com/

The stable binary is available for **Linux**, **Windows**, **MacOS**, **Android** and **iOS**. The development binary is available for **Linux**, **Windows** and **Android** and is updated every few days.

All binaries can also be downloaded free-of-charge for all platforms except for iOS, which only is available via the Apple App Store as they do not allow any other distribution channel.

## Code Contribution, Distribution and Trademark Usage

VESC is a registered trademark of Benjamin Vedder. Read the [trademark policies](https://vesc-project.com/trademark_policies) for more information.

The "official" binary release of VESC Tool is done via VESC Project only, as that gives users a way to verify that releases, that use the registered VESC trademark, originate from the VESC Project. It is not ok to host a binary release on a different channel and use the VESC trademark for that release.

It is ok to use the github fork function to make contributions to the code. That is because 1) it is the most convenient way to make contributions and 2) the forked repository states clearly that it is a fork and points back to the main repository where the original code can be found. Further, it is easy to see what the code changes are from the forked repository compared to the main repository via github, but that information is lost in a binary release.

**Forks without branding**  

Because the topic came up, here are some words about forking VESC Tool and removing the branding.  

If you make a fork of VESC Tool and remove all traces of the VESC trademark you are not breaking the trademark policies, but we still do not encourage that. The reason is that such forks are 1) confusing to users and 2) divert users away from the VESC Project itself and take away the opportunity to learn about it and to make donations if they choose to. For example, the majority of VESC donations today come from VESC Tool downloads.  

If you see a missing feature and you want to put in some work and make that feature available, we would appreciate if you contribute that back to the main VESC repositories. That way there is only one consistent and compatible release for everyone that is managed by the main authors of the VESC code who make the vast majority of the development. It also gives the main authors, who are the most familiar with the code, a chance to review features to make sure that they are as safe as possible and don't break other parts of the functionality.

## Add Your Hardware to the Binary Release

If you have custom hardware and you want to add support for it in the official release of VESC Tool, you can use the following steps:

1) Go to https://github.com/vedderb/bldc and use the github fork function.  
2) Make your changes, test them and make a pull request to the main repository.  
3) If the pull request gets accepted your hardware will become part of the next official release. It will show up in the binary beta typically after a few days and in the stable version the next time a stable release is made.

## Development

**Note:** These instructions build ExiTool without the BLDC firmwares bundled.

### Linux

Make sure that the required dependencies are installed. There is some advice in the [build_lin](./build_lin) file's comments. If you have Nix installed see below.

```shell
qmake -config release "CONFIG += release_lin build_original exclude_fw"
make -j8
./build/lin/vesc_tool_6.06
```

### Nix

The most easy way to build and run VESC Tool is to just run the provided program:

```shell
nix run
```

This will rebuild the program from scratch on each invokation. To enter a build environment with the dependencies installed for building it manually with QMake, run

```shell
nix develop
```

Then follow the normal build instructions for Linux.

**Note:** The Nix flake's outputs currently only supports x86 Linux.

### Starting QT Creator in Nix

QT Creator allows you to easily build and run the project. It also allows you to edit the page UIs with it's graphical editor. To run it using Nix simply start QT Creator from a shell with the build dependencies:

```shell
nix develop
nix run nixpkgs#qtcreator
```

This makes sure that QT Creator has access to the required dependencies.

## Build Instructions

Notes
- Use Qt 5.15.x. Qt 6 is not supported here.
- For faster setup during development, add `CONFIG+=exclude_fw` to skip bundling firmwares (avoids missing `res/firmwares/res_fw.qrc`).
- On macOS 15 SDK, add `CONFIG+=sdk_no_version_check` to silence SDK warnings.

### macOS (CLI)
```bash
brew install qt@5
export PATH=/opt/homebrew/opt/qt@5/bin:$PATH   # arm64 path; adjust if Intel
mkdir -p build && cd build
qmake CONFIG+=release CONFIG+=release_macos CONFIG+=sdk_no_version_check CONFIG+=exclude_fw ../vesc_tool.pro
make -j$(sysctl -n hw.ncpu)
open macos/ExiTool.app
```

### macOS (Qt Creator)
- Open `vesc_tool.pro` → select a Qt 5.15 macOS kit → Build & Run (Release).

### Linux (Ubuntu/Debian)
Install deps (Ubuntu example):
```bash
sudo apt update && sudo apt install -y \
  qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools \
  qml-module-qt-labs-folderlistmodel qml-module-qtquick-extras \
  qml-module-qtquick-controls2 qtquickcontrols2-5-dev \
  qtdeclarative5-dev qtconnectivity5-dev qtmultimedia5-dev \
  qtpositioning5-dev libqt5serialport5-dev libqt5svg5-dev \
  libqt5gamepad5-dev qml-module-qt-labs-settings qml-module-qt-labs-platform
```
Build:
```bash
mkdir -p build && cd build
qmake -config release "CONFIG+=release_lin exclude_fw" ../vesc_tool.pro
make -j$(nproc)
```
Run the produced binary in `build/lin/`.

### Windows (Qt Creator recommended)
1) Install Qt 5.15.x with MSVC 2019 (or MinGW) kits and C++ toolchain (VS Build Tools or MinGW).
2) Open `vesc_tool.pro` in Qt Creator → select Desktop kit (Qt 5.15) → Build (Release).

CLI (VS 2019 x64 Developer Command Prompt):
```bat
cd C:\path\to\vesc_tool
mkdir build && cd build
qmake CONFIG+=release CONFIG+=release_win CONFIG+=exclude_fw ..\vesc_tool.pro
nmake   REM or: jom -j8
```
Executable will be in `build\win\`.

### Android (Qt Creator recommended)
- Install Android Studio (SDK + Platform Tools) and NDK compatible with Qt 5.15.
- Install Qt 5.15 for Android and set up an Android kit in Qt Creator.
- Open `vesc_tool.pro` → select Android kit → Build (Release) → Deploy/Run.

CLI (advanced; environment must point to Android SDK/NDK and Java):
```bash
mkdir -p build-android && cd build-android
qmake CONFIG+=release CONFIG+=release_android CONFIG+=build_mobile CONFIG+=exclude_fw ../vesc_tool.pro
make -j$(nproc)
# Then package with androiddeployqt via Qt Creator or matching Qt toolchain
```

### iOS (Qt Creator recommended)
- Install Xcode and Qt 5.15 for iOS.
- Open `vesc_tool.pro` in Qt Creator → select iOS kit → Build & Run (device or simulator).

CLI (optional):
```bash
mkdir -p build-ios && cd build-ios
qmake CONFIG+=release CONFIG+=build_mobile ../vesc_tool.pro
make -j$(sysctl -n hw.ncpu)
# Use Qt Creator/Xcode for signing and deployment
```

### Bundling firmwares (optional)
- Remove `CONFIG+=exclude_fw` and ensure `res/firmwares/res_fw.qrc` and referenced binaries are present (often via Git LFS or separate asset download).
