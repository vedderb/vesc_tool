HEADERS += \
    $$PWD/esp32flash.h \
    $$PWD/esp_loader.h \
    $$PWD/esp_targets.h \
    $$PWD/md5_hash.h \
    $$PWD/serial_comm_prv.h \
    $$PWD/esp_loader_io.h \
    $$PWD/protocol.h

SOURCES += \
    $$PWD/esp32flash.cpp \
    $$PWD/esp_loader.c \
    $$PWD/esp_targets.c \
    $$PWD/md5_hash.c \
    $$PWD/protocol_uart.c \
    $$PWD/protocol_serial.c \
    $$PWD/esp_stubs.c \
    $$PWD/slip.c

