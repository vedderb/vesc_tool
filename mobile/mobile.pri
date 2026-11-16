HEADERS += \
    $$PWD/logreader.h \
    $$PWD/logwriter.h \
    $$PWD/qmlui.h \
    $$PWD/fwhelper.h \
    $$PWD/vesc3ditem.h

SOURCES += \
    $$PWD/logreader.cpp \
    $$PWD/logwriter.cpp \
    $$PWD/qmlui.cpp \
    $$PWD/fwhelper.cpp \
    $$PWD/vesc3ditem.cpp

ios|android {
    HEADERS += $$PWD/ridesession.h $$PWD/ridebridge.h
    SOURCES += $$PWD/ridesession.cpp
}

RESOURCES += \
    $$PWD/qml.qrc

DISTFILES +=

