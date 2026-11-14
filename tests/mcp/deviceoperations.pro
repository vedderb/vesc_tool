QT += core gui network testlib
CONFIG += console testcase c++11
CONFIG -= app_bundle
TEMPLATE = app
TARGET = tst_mcpdeviceoperations
INCLUDEPATH += ../.. ../../mcp
SOURCES += tst_mcpdeviceoperations.cpp ../../mcp/mcpdeviceoperations.cpp \
    ../../mcp/mcpwritetransaction.cpp ../../mcp/mcpdatahelpers.cpp ../../vbytearray.cpp ../../packet.cpp
HEADERS += ../../mcp/mcpdeviceoperations.h ../../mcp/mcpwritetransaction.h \
    ../../mcp/mcpdevice.h ../../packet.h
