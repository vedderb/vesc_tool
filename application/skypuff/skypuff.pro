# Version
VT_VERSION = 4.00
VT_INTRO_VERSION = 1
VT_CONFIG_VERSION = 2

# Set to 0 for stable versions and to test version number for development versions.
VT_IS_TEST_VERSION = 0 # disable warning, but actual test version is 5

VT_ANDROID_VERSION_ARMV7 = 108
VT_ANDROID_VERSION_ARM64 = 109
VT_ANDROID_VERSION_X86 = 110

VT_ANDROID_VERSION = $$VT_ANDROID_VERSION_X86

# Ubuntu 18.04 (should work on raspbian buster too)
# sudo apt install qml-module-qt-labs-folderlistmodel qml-module-qtquick-extras qml-module-qtquick-controls2 qt5-default libqt5quickcontrols2-5 qtquickcontrols2-5-dev qtcreator qtcreator-doc libqt5serialport5-dev build-essential qml-module-qt3d qt3d5-dev qtdeclarative5-dev qtconnectivity5-dev qtmultimedia5-dev qtpositioning5-dev qtpositioning5-dev libqt5gamepad5-dev qml-module-qt-labs-settings qml-module-qt-labs-platform libqt5svg5-dev

DEFINES += VT_VERSION=$$VT_VERSION
DEFINES += VT_INTRO_VERSION=$$VT_INTRO_VERSION
DEFINES += VT_CONFIG_VERSION=$$VT_CONFIG_VERSION
DEFINES += VT_IS_TEST_VERSION=$$VT_IS_TEST_VERSION
QT_LOGGING_RULES="qt.qml.connections=false"
#CONFIG += qtquickcompiler

CONFIG += build_mobile
CONFIG += c++11

QT       += core gui
QT       += widgets
QT       += serialport
QT       += network
QT       += quick
QT       += quickcontrols2
QT       += quickwidgets
QT       += svg
QT       += gui-private
QT       += printsupport
QT       += multimedia

TARGET = skypuff
TEMPLATE = app

# Serial port available
DEFINES += HAS_SERIALPORT

# Bluetooth available
DEFINES += HAS_BLUETOOTH

contains(DEFINES, HAS_SERIALPORT) {
    QT += serialport
}

contains(DEFINES, HAS_BLUETOOTH) {
    QT += bluetooth
}

android: {
    QT += androidextras
    manifest.input = $$PWD/android/AndroidManifest.xml.in
    manifest.output = $$PWD/android/AndroidManifest.xml
    QMAKE_SUBSTITUTES += manifest
}


INCLUDEPATH += ../..

SOURCES += main.cpp \
    skypuff.cpp \
    qmlable_skypuff_types.cpp

HEADERS += \
    missing_types.h \
    skypuff.h \
    app_skypuff.h \
    qmlable_skypuff_types.h
    
contains(DEFINES, HAS_BLUETOOTH) {
    SOURCES += ../../bleuart.cpp
    HEADERS += ../../bleuart.h
}

include(../../application.pri)
include(../../heatshrink/heatshrink.pri)
include(../../widgets/widgets.pri)
include(../../lzokay/lzokay.pri)
include(../../QCodeEditor/qcodeeditor.pri)

RESOURCES += \
    qml.qrc \
    ../../res_config.qrc \

DISTFILES += \
    android/AndroidManifest.xml \
    android/AndroidManifest.xml.in \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradlew \
    android/res/values/libs.xml \
    android/build.gradle \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/src/com/vedder/vesc/VForegroundService.java \
    android/src/com/vedder/vesc/Utils.java

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
