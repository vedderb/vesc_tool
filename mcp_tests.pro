# Build with the real app sources/resources to test the actual Commands boundary.
# Keep the shadow build directory outside the checkout.
include(vesc_tool.pro)
QT += testlib
CONFIG += console testcase
TARGET = tst_mcpapplication
SOURCES -= main.cpp
SOURCES += tests/mcp/tst_mcpapplication.cpp
