#-------------------------------------------------
#
# Project created by QtCreator 2016-08-12T21:55:19
#
#-------------------------------------------------

# Version
VT_VERSION = 6.06
VT_INTRO_VERSION = 1
VT_CONFIG_VERSION = 4

# Set to 0 for stable versions and to test version number for development versions.
VT_IS_TEST_VERSION = 2

# GIT commit
VT_GIT_COMMIT = $$system(git rev-parse --short=8 HEAD)

VT_ANDROID_VERSION_ARMV7 = 162
VT_ANDROID_VERSION_ARM64 = 163
VT_ANDROID_VERSION_X86 = 164

VT_ANDROID_VERSION = $$VT_ANDROID_VERSION_X86

# Ubuntu 18.04 (should work on raspbian buster too)
# sudo apt install qml-module-qt-labs-folderlistmodel qml-module-qtquick-extras qml-module-qtquick-controls2 qt5-default libqt5quickcontrols2-5 qtquickcontrols2-5-dev qtcreator qtcreator-doc libqt5serialport5-dev build-essential qml-module-qt3d qt3d5-dev qtdeclarative5-dev qtconnectivity5-dev qtmultimedia5-dev qtpositioning5-dev qtpositioning5-dev libqt5gamepad5-dev qml-module-qt-labs-settings qml-module-qt-labs-platform libqt5svg5-dev

DEFINES += VT_VERSION=$$VT_VERSION
DEFINES += VT_INTRO_VERSION=$$VT_INTRO_VERSION
DEFINES += VT_CONFIG_VERSION=$$VT_CONFIG_VERSION
DEFINES += VT_IS_TEST_VERSION=$$VT_IS_TEST_VERSION
DEFINES += VT_GIT_COMMIT=$$VT_GIT_COMMIT
QT_LOGGING_RULES="qt.qml.connections=false"
#CONFIG += qtquickcompiler

CONFIG += c++11
CONFIG += resources_big
ios: {
    QMAKE_CXXFLAGS_DEBUG += -Wall
}

!win32-msvc*: { !android: {
    QMAKE_CXXFLAGS += -Wno-deprecated-copy
}}

# Build mobile GUI
#CONFIG += build_mobile

# Exclude built-in firmwares
#CONFIG += exclude_fw

ios: {
    CONFIG    += build_mobile
    DEFINES   += QT_NO_PRINTER
}

# Debug build (e.g. F5 to reload QML files)
#DEFINES += DEBUG_BUILD

# If BLE disconnects on ubuntu after about 90 seconds the reason is most likely that the connection interval is incompatible. This can be fixed with:
# sudo bash -c 'echo 6 > /sys/kernel/debug/bluetooth/hci0/conn_min_interval'

# Clear old bluetooth devices
# sudo rm -rf /var/lib/bluetooth/*
# sudo service bluetooth restart

# Bluetooth available
DEFINES += HAS_BLUETOOTH

# CAN bus available
# Adding serialbus to Qt seems to break the serial port on static builds. TODO: Figure out why.
#DEFINES += HAS_CANBUS

# Positioning
DEFINES += HAS_POS

!ios: {
    QT       += printsupport
!android: {
    # Serial port available
    DEFINES += HAS_SERIALPORT
    DEFINES += HAS_GAMEPAD
}
}

win32: {
    DEFINES += _USE_MATH_DEFINES
}

# https://stackoverflow.com/questions/61444320/what-are-the-configure-options-for-qt-that-enable-dead-keys-usage
unix: {
!ios: {
    QTPLUGIN += composeplatforminputcontextplugin
}
}

# Options
#CONFIG += build_original
#CONFIG += build_platinum
#CONFIG += build_gold
#CONFIG += build_silver
#CONFIG += build_bronze
#CONFIG += build_free

QT       += core gui
QT       += widgets
QT       += network
QT       += quick
QT       += quickcontrols2
QT       += quickwidgets
QT       += svg
QT       += gui-private

contains(DEFINES, HAS_SERIALPORT) {
    QT       += serialport
}

contains(DEFINES, HAS_CANBUS) {
    QT       += serialbus
}

contains(DEFINES, HAS_BLUETOOTH) {
    QT       += bluetooth
}

contains(DEFINES, HAS_POS) {
    QT       += positioning
}

contains(DEFINES, HAS_GAMEPAD) {
    QT       += gamepad
}

android: QT += androidextras

ios | macx: {
    TARGET = "VESC Tool"
}else: {
    android:{
        TARGET = "vesc_tool"
    }else:{

        TARGET = vesc_tool_$$VT_VERSION
    }
}

ANDROID_VERSION = 1

android:contains(QT_ARCH, i386) {
    VT_ANDROID_VERSION = $$VT_ANDROID_VERSION_X86
}

contains(ANDROID_TARGET_ARCH, arm64-v8a) {
    VT_ANDROID_VERSION = $$VT_ANDROID_VERSION_ARM64
}

contains(ANDROID_TARGET_ARCH, armeabi-v7a) {
    VT_ANDROID_VERSION = $$VT_ANDROID_VERSION_ARMV7
}

android: {
    manifest.input = $$PWD/android/AndroidManifest.xml.in
    manifest.output = $$PWD/android/AndroidManifest.xml
    QMAKE_SUBSTITUTES += manifest
}

TEMPLATE = app

release_win {
    DESTDIR = build/win
    OBJECTS_DIR = build/win/obj
    MOC_DIR = build/win/obj
    RCC_DIR = build/win/obj
    UI_DIR = build/win/obj
}

release_lin {
    # http://micro.nicholaswilson.me.uk/post/31855915892/rules-of-static-linking-libstdc-libc-libgcc
    # http://insanecoding.blogspot.se/2012/07/creating-portable-linux-binaries.html
    QMAKE_LFLAGS += -static-libstdc++ -static-libgcc
    DESTDIR = build/lin
    OBJECTS_DIR = build/lin/obj
    MOC_DIR = build/lin/obj
    RCC_DIR = build/lin/obj
    UI_DIR = build/lin/obj
}

release_macos {
    # brew install qt
    DESTDIR = build/macos
    OBJECTS_DIR = build/macos/obj
    MOC_DIR = build/macos/obj
    RCC_DIR = build/macos/obj
    UI_DIR = build/macos/obj
}

release_android {
    DESTDIR = build/android
    OBJECTS_DIR = build/android/obj
    MOC_DIR = build/android/obj
    RCC_DIR = build/android/obj
    UI_DIR = build/android/obj
}

build_mobile {
    DEFINES += USE_MOBILE
}

SOURCES += main.cpp\
    bleuartdummy.cpp \
    codeloader.cpp \
    mainwindow.cpp \
    boardsetupwindow.cpp \
    packet.cpp \
    preferences.cpp \
    tcphub.cpp \
    udpserversimple.cpp \
    vbytearray.cpp \
    commands.cpp \
    configparams.cpp \
    configparam.cpp \
    vescinterface.cpp \
    parametereditor.cpp \
    digitalfiltering.cpp \
    setupwizardapp.cpp \
    setupwizardmotor.cpp \
    startupwizard.cpp \
    utility.cpp \
    tcpserversimple.cpp \
    hexfile.cpp

HEADERS  += mainwindow.h \
    bleuartdummy.h \
    codeloader.h \
    boardsetupwindow.h \
    packet.h \
    preferences.h \
    tcphub.h \
    udpserversimple.h \
    vbytearray.h \
    commands.h \
    datatypes.h \
    configparams.h \
    configparam.h \
    vescinterface.h \
    parametereditor.h \
    digitalfiltering.h \
    setupwizardapp.h \
    setupwizardmotor.h \
    startupwizard.h \
    utility.h \
    tcpserversimple.h \
    hexfile.h

unix: {
!ios: {
    HEADERS += systemcommandexecutor.h
}
}

FORMS    += mainwindow.ui \
    boardsetupwindow.ui \
    parametereditor.ui \
    preferences.ui

contains(DEFINES, HAS_BLUETOOTH) {
    SOURCES += bleuart.cpp
    HEADERS += bleuart.h
}

include(pages/pages.pri)
include(widgets/widgets.pri)
include(mobile/mobile.pri)
include(map/map.pri)
include(lzokay/lzokay.pri)
include(heatshrink/heatshrink.pri)
include(QCodeEditor/qcodeeditor.pri)
include(esp32/esp32.pri)
include(display_tool/display_tool.pri)
include(qmarkdowntextedit/qmarkdowntextedit.pri)
include(maddy/maddy.pri)
include(minimp3/minimp3.pri)

RESOURCES += res.qrc \
    res_custom_module.qrc \
    res_lisp.qrc \
    res_qml.qrc
RESOURCES += res_config.qrc

RESOURCES += res_fw_bms.qrc
RESOURCES += res/firmwares_esp/res_fw_esp.qrc

!exclude_fw {
    RESOURCES += res/firmwares/res_fw.qrc
}

build_original {
    RESOURCES += res_original.qrc
    DEFINES += VER_ORIGINAL
} else:build_platinum {
    RESOURCES += res_platinum.qrc
    DEFINES += VER_PLATINUM
} else:build_gold {
    RESOURCES += res_gold.qrc
    DEFINES += VER_GOLD
} else:build_silver {
    RESOURCES += res_silver.qrc
    DEFINES += VER_SILVER
} else:build_bronze {
    RESOURCES += res_bronze.qrc
    DEFINES += VER_BRONZE
} else:build_free {
    RESOURCES += res_free.qrc
    DEFINES += VER_FREE
} else {
    RESOURCES += res_neutral.qrc
    DEFINES += VER_NEUTRAL
}

DISTFILES += \
    android/AndroidManifest.xml \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradlew \
    android/gradlew.bat \
    android/res/values/libs.xml \
    android/build.gradle \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/src/com/vedder/vesc/VForegroundService.java \
    android/src/com/vedder/vesc/Utils.java

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

macx-clang:contains(QMAKE_HOST.arch, arm.*): {
   QMAKE_APPLE_DEVICE_ARCHS=arm64
}

macx {
    ICON        =  macos/appIcon.icns
    QMAKE_INFO_PLIST = macos/Info.plist
    DISTFILES += macos/Info.plist
    QMAKE_CFLAGS_RELEASE = $$QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO
    QMAKE_CXXFLAGS_RELEASE = $$QMAKE_CXXFLAGS_RELEASE_WITH_DEBUGINFO
    QMAKE_OBJECTIVE_CFLAGS_RELEASE = $$QMAKE_OBJECTIVE_CFLAGS_RELEASE_WITH_DEBUGINFO
    QMAKE_LFLAGS_RELEASE = $$QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO
}

ios {
    QMAKE_INFO_PLIST = ios/Info.plist
    HEADERS += ios/src/setIosParameters.h
    SOURCES += ios/src/setIosParameters.mm
    DISTFILES += ios/Info.plist \
                 ios/*.storyboard
    QMAKE_ASSET_CATALOGS = $$PWD/ios/Images.xcassets
    QMAKE_ASSET_CATALOGS_APP_ICON = "AppIcon"

    ios_artwork.files = $$files($$PWD/ios/iTunesArtwork*.png)
    QMAKE_BUNDLE_DATA += ios_artwork
    app_launch_images.files = $$files($$PWD/ios/LaunchImage*.png)
    QMAKE_BUNDLE_DATA += app_launch_images
    app_launch_screen.files = $$files($$PWD/ios/MyLaunchScreen.storyboard)
    QMAKE_BUNDLE_DATA += app_launch_screen

    #QMAKE_IOS_DEPLOYMENT_TARGET = 11.0

    disable_warning.name = GCC_WARN_64_TO_32_BIT_CONVERSION
    disable_warning.value = NO

    QMAKE_MAC_XCODE_SETTINGS += disable_warning

    # Note for devices: 1=iPhone, 2=iPad, 1,2=Universal.
    CONFIG -= warn_on
    QMAKE_APPLE_TARGETED_DEVICE_FAMILY = 1,2
}
CONFIG -= warn_on

contains(ANDROID_TARGET_ARCH,) {
    ANDROID_ABIS = \
        armeabi-v7a
}
