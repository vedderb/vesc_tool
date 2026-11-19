# LispBM for the Virtual ESC, one copy per simulated device. The include
# paths are for C only, so that the LispBM headers do not shadow others.
QMAKE_CFLAGS += -I$$PWD/include -I$$PWD/include/extensions -I$$PWD/src/extensions \
    -I$$PWD/utils -I$$PWD/platform -I$$PWD/repl

SOURCES += \
    $$PWD/vl_node0.c \
    $$PWD/vl_node1.c \
    $$PWD/vl_node2.c \
    $$PWD/vl_node3.c \
    $$PWD/vl_node4.c \
    $$PWD/platform/platform_mutex.cpp

HEADERS += $$PWD/vl_api.h
