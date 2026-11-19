# LispBM for the Virtual ESC, one copy per simulated device. The include
# paths are for C only, so that the LispBM headers do not shadow others,
# except the platform headers that platform_mutex.cpp needs.
L = $$PWD/lispBM
QMAKE_CFLAGS += -I$$L/include -I$$L/include/extensions -I$$L/src/extensions \
    -I$$L/utils -I$$L/platform/qt5/include -I$$L/repl
QMAKE_CXXFLAGS += -I$$L/platform/qt5/include

SOURCES += \
    $$PWD/vl_node0.c \
    $$PWD/vl_node1.c \
    $$PWD/vl_node2.c \
    $$PWD/vl_node3.c \
    $$PWD/vl_node4.c \
    $$L/platform/qt5/src/platform_mutex.cpp

HEADERS += $$PWD/vl_api.h
