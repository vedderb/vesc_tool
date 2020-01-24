VT_VERSION = 0.95
DEFINES += VT_VERSION=$$VT_VERSION

CONFIG += c++11

QT += core
QT += gui
QT += widgets
QT += serialport
QT += network
QT += printsupport
QT += quick

TARGET = application
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

INCLUDEPATH += ../../

SOURCES += main.cpp\

HEADERS  += 
    
contains(DEFINES, HAS_BLUETOOTH) {
    SOURCES += ../../bleuart.cpp
    HEADERS += ../../bleuart.h
}

include(../../application.pri)
include(../../widgets/widgets.pri)
include(../../lzokay/lzokay.pri)

RESOURCES += \
    qml.qrc \
    ../../res_config.qrc

