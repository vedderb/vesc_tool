cmake_minimum_required(VERSION 3.16)

# ---------------------------------------------------------------------------
# Version & Git commit
# ---------------------------------------------------------------------------
set(VT_VERSION       "7.00")
set(VT_INTRO_VERSION "1")
set(VT_CONFIG_VERSION "4")
# Set to 0 for stable releases, test version number for development builds
set(VT_IS_TEST_VERSION "2")

execute_process(
    COMMAND git rev-parse --short=8 HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE VT_GIT_COMMIT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(NOT VT_GIT_COMMIT)
    set(VT_GIT_COMMIT "00000000")
endif()

# Android version codes
set(VT_ANDROID_VERSION_ARMV7 190)
set(VT_ANDROID_VERSION_ARM64 191)
set(VT_ANDROID_VERSION_X86   192)

project(vesc_tool VERSION ${VT_VERSION} LANGUAGES C CXX)

# ---------------------------------------------------------------------------
# C++ standard
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Qt6 – find all potentially-needed components up-front
# ---------------------------------------------------------------------------
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Core set of Qt modules always required
set(QT_COMPONENTS
    Core
    Gui
    Widgets
    Network
    Quick
    QuickControls2
    QuickWidgets
    Svg
    PrintSupport
    OpenGL
    OpenGLWidgets
    Core5Compat
)

# ---------------------------------------------------------------------------
# User-toggleable options (mirror the old qmake CONFIG / DEFINES)
# ---------------------------------------------------------------------------
option(HAS_BLUETOOTH   "Enable Bluetooth support"     ON)
option(HAS_SERIALPORT  "Enable serial port support"    ON)
option(HAS_GAMEPAD     "Enable gamepad support"        ON)
option(HAS_CANBUS      "Enable CAN bus (serialbus)"    OFF)
option(HAS_POS         "Enable positioning support"    ON)
option(BUILD_MOBILE    "Build the mobile (QML) GUI"    OFF)
option(EXCLUDE_FW      "Exclude built-in firmwares"    ON)

# Build tier (pick one; default = neutral)
set(BUILD_TIER "neutral" CACHE STRING "Build tier: original, platinum, gold, silver, bronze, free, neutral")
set_property(CACHE BUILD_TIER PROPERTY STRINGS original platinum gold silver bronze free neutral)

# Platform-specific defaults
if(WIN32)
    set(HAS_POS OFF)
endif()
if(ANDROID)
    set(HAS_SERIALPORT OFF)
    # HAS_GAMEPAD stays ON via QtGamepadLegacy
endif()
if(IOS)
    set(HAS_SERIALPORT OFF)
    set(BUILD_MOBILE ON)
endif()

# Append optional Qt components
if(HAS_SERIALPORT)
    list(APPEND QT_COMPONENTS SerialPort)
endif()
if(HAS_CANBUS)
    list(APPEND QT_COMPONENTS SerialBus)
endif()
if(HAS_BLUETOOTH)
    list(APPEND QT_COMPONENTS Bluetooth)
endif()
if(HAS_POS)
    list(APPEND QT_COMPONENTS Positioning)
endif()
find_package(Qt6 REQUIRED COMPONENTS ${QT_COMPONENTS})
# We need GuiPrivate for some internals
find_package(Qt6 REQUIRED COMPONENTS GuiPrivate)

# Qt6 dropped QtGamepad – use QtGamepadLegacy (pumphaus/qtgamepadlegacy).
# Build & install it into your Qt prefix first, then it becomes available
# as the "GamepadLegacy" component.
if(HAS_GAMEPAD)
    find_package(Qt6 QUIET COMPONENTS GamepadLegacy)
    if(NOT TARGET Qt6::GamepadLegacy)
        message(WARNING
            "Qt6::GamepadLegacy not found – gamepad support disabled.\n"
            "  To enable it, build & install https://github.com/pumphaus/qtgamepadlegacy\n"
            "  into your Qt prefix (${Qt6_DIR}/../../..) and re-run CMake.")
        set(HAS_GAMEPAD OFF)
    else()
        message(STATUS "Found Qt6::GamepadLegacy – gamepad support enabled.")
    endif()
endif()

# ---------------------------------------------------------------------------
# Compile definitions shared across the whole target
# ---------------------------------------------------------------------------
set(VT_DEFINITIONS
    VT_VERSION=${VT_VERSION}
    VT_INTRO_VERSION=${VT_INTRO_VERSION}
    VT_CONFIG_VERSION=${VT_CONFIG_VERSION}
    VT_IS_TEST_VERSION=${VT_IS_TEST_VERSION}
    VT_GIT_COMMIT=${VT_GIT_COMMIT}
)

if(HAS_BLUETOOTH)
    list(APPEND VT_DEFINITIONS HAS_BLUETOOTH)
endif()
if(HAS_SERIALPORT)
    list(APPEND VT_DEFINITIONS HAS_SERIALPORT)
endif()
if(HAS_GAMEPAD)
    list(APPEND VT_DEFINITIONS HAS_GAMEPAD)
endif()
if(HAS_CANBUS)
    list(APPEND VT_DEFINITIONS HAS_CANBUS)
endif()
if(HAS_POS)
    list(APPEND VT_DEFINITIONS HAS_POS)
endif()
if(BUILD_MOBILE)
    list(APPEND VT_DEFINITIONS USE_MOBILE)
endif()
if(WIN32)
    list(APPEND VT_DEFINITIONS _USE_MATH_DEFINES)
endif()

# Build-tier defines & resources
if(BUILD_TIER STREQUAL "original")
    list(APPEND VT_DEFINITIONS VER_ORIGINAL)
    set(TIER_QRC res_original.qrc)
elseif(BUILD_TIER STREQUAL "platinum")
    list(APPEND VT_DEFINITIONS VER_PLATINUM)
    set(TIER_QRC res_platinum.qrc)
elseif(BUILD_TIER STREQUAL "gold")
    list(APPEND VT_DEFINITIONS VER_GOLD)
    set(TIER_QRC res_gold.qrc)
elseif(BUILD_TIER STREQUAL "silver")
    list(APPEND VT_DEFINITIONS VER_SILVER)
    set(TIER_QRC res_silver.qrc)
elseif(BUILD_TIER STREQUAL "bronze")
    list(APPEND VT_DEFINITIONS VER_BRONZE)
    set(TIER_QRC res_bronze.qrc)
elseif(BUILD_TIER STREQUAL "free")
    list(APPEND VT_DEFINITIONS VER_FREE)
    set(TIER_QRC res_free.qrc)
else()
    list(APPEND VT_DEFINITIONS VER_NEUTRAL)
    set(TIER_QRC res_neutral.qrc)
endif()

# ---------------------------------------------------------------------------
# Sources – root
# ---------------------------------------------------------------------------
set(ROOT_SOURCES
    main.cpp
    bleuartdummy.cpp
    codeloader.cpp
    mainwindow.cpp
    boardsetupwindow.cpp
    packet.cpp
    preferences.cpp
    tcphub.cpp
    udpserversimple.cpp
    vbytearray.cpp
    commands.cpp
    configparams.cpp
    configparam.cpp
    vescinterface.cpp
    parametereditor.cpp
    digitalfiltering.cpp
    setupwizardapp.cpp
    setupwizardmotor.cpp
    startupwizard.cpp
    utility.cpp
    tcpserversimple.cpp
    hexfile.cpp
)

set(ROOT_HEADERS
    mainwindow.h
    bleuartdummy.h
    codeloader.h
    boardsetupwindow.h
    packet.h
    preferences.h
    tcphub.h
    udpserversimple.h
    vbytearray.h
    commands.h
    datatypes.h
    configparams.h
    configparam.h
    vescinterface.h
    parametereditor.h
    digitalfiltering.h
    setupwizardapp.h
    setupwizardmotor.h
    startupwizard.h
    utility.h
    tcpserversimple.h
    hexfile.h
)

set(ROOT_FORMS
    mainwindow.ui
    boardsetupwindow.ui
    parametereditor.ui
    preferences.ui
)

# Platform-specific root sources
if(UNIX AND NOT IOS)
    list(APPEND ROOT_HEADERS systemcommandexecutor.h)
endif()
if(HAS_BLUETOOTH)
    list(APPEND ROOT_SOURCES bleuart.cpp)
    list(APPEND ROOT_HEADERS bleuart.h)
endif()

# ---------------------------------------------------------------------------
# Sources – pages/
# ---------------------------------------------------------------------------
set(PAGES_SOURCES
    pages/pageapppas.cpp
    pages/pagebms.cpp
    pages/pagecananalyzer.cpp
    pages/pageconnection.cpp
    pages/pagecustomconfig.cpp
    pages/pagedisplaytool.cpp
    pages/pageespprog.cpp
    pages/pagelisp.cpp
    pages/pagemotor.cpp
    pages/pagedebugprint.cpp
    pages/pagebldc.cpp
    pages/pageappgeneral.cpp
    pages/pagedc.cpp
    pages/pagefoc.cpp
    pages/pagecontrollers.cpp
    pages/pageappppm.cpp
    pages/pageappadc.cpp
    pages/pageappuart.cpp
    pages/pageappnunchuk.cpp
    pages/pageappnrf.cpp
    pages/pagemotorcomparison.cpp
    pages/pagescripting.cpp
    pages/pageterminal.cpp
    pages/pagefirmware.cpp
    pages/pagertdata.cpp
    pages/pagesampleddata.cpp
    pages/pagevescpackage.cpp
    pages/pagewelcome.cpp
    pages/pagemotorsettings.cpp
    pages/pageappsettings.cpp
    pages/pagedataanalysis.cpp
    pages/pagemotorinfo.cpp
    pages/pagesetupcalculators.cpp
    pages/pagegpd.cpp
    pages/pageexperiments.cpp
    pages/pageimu.cpp
    pages/pageswdprog.cpp
    pages/pageappimu.cpp
    pages/pageloganalysis.cpp
)

set(PAGES_HEADERS
    pages/pageapppas.h
    pages/pagebms.h
    pages/pagecananalyzer.h
    pages/pageconnection.h
    pages/pagecustomconfig.h
    pages/pagedisplaytool.h
    pages/pageespprog.h
    pages/pagelisp.h
    pages/pagemotor.h
    pages/pagedebugprint.h
    pages/pagebldc.h
    pages/pageappgeneral.h
    pages/pagedc.h
    pages/pagefoc.h
    pages/pagecontrollers.h
    pages/pageappppm.h
    pages/pageappadc.h
    pages/pageappuart.h
    pages/pageappnunchuk.h
    pages/pageappnrf.h
    pages/pagemotorcomparison.h
    pages/pagescripting.h
    pages/pageterminal.h
    pages/pagefirmware.h
    pages/pagertdata.h
    pages/pagesampleddata.h
    pages/pagevescpackage.h
    pages/pagewelcome.h
    pages/pagemotorsettings.h
    pages/pageappsettings.h
    pages/pagedataanalysis.h
    pages/pagemotorinfo.h
    pages/pagesetupcalculators.h
    pages/pagegpd.h
    pages/pageexperiments.h
    pages/pageimu.h
    pages/pageswdprog.h
    pages/pageappimu.h
    pages/pageloganalysis.h
)

set(PAGES_FORMS
    pages/pageapppas.ui
    pages/pagebms.ui
    pages/pagecananalyzer.ui
    pages/pageconnection.ui
    pages/pagecustomconfig.ui
    pages/pagedisplaytool.ui
    pages/pageespprog.ui
    pages/pagelisp.ui
    pages/pagemotor.ui
    pages/pagedebugprint.ui
    pages/pagebldc.ui
    pages/pageappgeneral.ui
    pages/pagedc.ui
    pages/pagefoc.ui
    pages/pagecontrollers.ui
    pages/pageappppm.ui
    pages/pageappadc.ui
    pages/pageappuart.ui
    pages/pageappnunchuk.ui
    pages/pageappnrf.ui
    pages/pagemotorcomparison.ui
    pages/pagescripting.ui
    pages/pageterminal.ui
    pages/pagefirmware.ui
    pages/pagertdata.ui
    pages/pagesampleddata.ui
    pages/pagevescpackage.ui
    pages/pagewelcome.ui
    pages/pagemotorsettings.ui
    pages/pageappsettings.ui
    pages/pagedataanalysis.ui
    pages/pagemotorinfo.ui
    pages/pagesetupcalculators.ui
    pages/pagegpd.ui
    pages/pageexperiments.ui
    pages/pageimu.ui
    pages/pageswdprog.ui
    pages/pageappimu.ui
    pages/pageloganalysis.ui
)

# ---------------------------------------------------------------------------
# Sources – widgets/
# ---------------------------------------------------------------------------
set(WIDGETS_SOURCES
    widgets/batttempplot.cpp
    widgets/canlistitem.cpp
    widgets/experimentplot.cpp
    widgets/parameditbitfield.cpp
    widgets/parameditbool.cpp
    widgets/parameditdouble.cpp
    widgets/parameditenum.cpp
    widgets/parameditint.cpp
    widgets/displaybar.cpp
    widgets/displaypercentage.cpp
    widgets/helpdialog.cpp
    widgets/mrichtextedit.cpp
    widgets/mtextedit.cpp
    widgets/pagelistitem.cpp
    widgets/paramtable.cpp
    widgets/qcustomplot.cpp
    widgets/detectbldc.cpp
    widgets/batterycalculator.cpp
    widgets/detectfoc.cpp
    widgets/detectfocencoder.cpp
    widgets/detectfochall.cpp
    widgets/ppmmap.cpp
    widgets/adcmap.cpp
    widgets/rtdatatext.cpp
    widgets/nrfpair.cpp
    widgets/scripteditor.cpp
    widgets/vtextbrowser.cpp
    widgets/imagewidget.cpp
    widgets/parameditstring.cpp
    widgets/paramdialog.cpp
    widgets/aspectimglabel.cpp
    widgets/historylineedit.cpp
    widgets/detectallfocdialog.cpp
    widgets/dirsetup.cpp
    widgets/vesc3dview.cpp
    widgets/superslider.cpp
)

set(WIDGETS_HEADERS
    widgets/batttempplot.h
    widgets/canlistitem.h
    widgets/experimentplot.h
    widgets/parameditbitfield.h
    widgets/parameditbool.h
    widgets/parameditdouble.h
    widgets/parameditenum.h
    widgets/parameditint.h
    widgets/displaybar.h
    widgets/displaypercentage.h
    widgets/helpdialog.h
    widgets/mrichtextedit.h
    widgets/mtextedit.h
    widgets/pagelistitem.h
    widgets/paramtable.h
    widgets/qcustomplot.h
    widgets/detectbldc.h
    widgets/batterycalculator.h
    widgets/detectfoc.h
    widgets/detectfocencoder.h
    widgets/detectfochall.h
    widgets/ppmmap.h
    widgets/adcmap.h
    widgets/rtdatatext.h
    widgets/nrfpair.h
    widgets/scripteditor.h
    widgets/vtextbrowser.h
    widgets/imagewidget.h
    widgets/parameditstring.h
    widgets/paramdialog.h
    widgets/aspectimglabel.h
    widgets/historylineedit.h
    widgets/detectallfocdialog.h
    widgets/dirsetup.h
    widgets/vesc3dview.h
    widgets/superslider.h
)

set(WIDGETS_FORMS
    widgets/experimentplot.ui
    widgets/parameditbitfield.ui
    widgets/parameditbool.ui
    widgets/parameditdouble.ui
    widgets/parameditenum.ui
    widgets/parameditint.ui
    widgets/mrichtextedit.ui
    widgets/helpdialog.ui
    widgets/detectbldc.ui
    widgets/batterycalculator.ui
    widgets/detectfoc.ui
    widgets/detectfocencoder.ui
    widgets/detectfochall.ui
    widgets/ppmmap.ui
    widgets/adcmap.ui
    widgets/nrfpair.ui
    widgets/parameditstring.ui
    widgets/paramdialog.ui
    widgets/detectallfocdialog.ui
    widgets/dirsetup.ui
    widgets/scripteditor.ui
)

# ---------------------------------------------------------------------------
# Sources – mobile/
# ---------------------------------------------------------------------------
set(MOBILE_SOURCES
    mobile/logreader.cpp
    mobile/logwriter.cpp
    mobile/qmlui.cpp
    mobile/fwhelper.cpp
    mobile/vesc3ditem.cpp
)

set(MOBILE_HEADERS
    mobile/logreader.h
    mobile/logwriter.h
    mobile/qmlui.h
    mobile/fwhelper.h
    mobile/vesc3ditem.h
)

# ---------------------------------------------------------------------------
# Sources – map/
# ---------------------------------------------------------------------------
set(MAP_SOURCES
    map/carinfo.cpp
    map/copterinfo.cpp
    map/locpoint.cpp
    map/mapwidget.cpp
    map/osmclient.cpp
    map/osmtile.cpp
    map/perspectivepixmap.cpp
)

set(MAP_HEADERS
    map/carinfo.h
    map/copterinfo.h
    map/locpoint.h
    map/mapwidget.h
    map/osmclient.h
    map/osmtile.h
    map/perspectivepixmap.h
)

# ---------------------------------------------------------------------------
# Sources – lzokay/
# ---------------------------------------------------------------------------
set(LZOKAY_SOURCES lzokay/lzokay.cpp)
set(LZOKAY_HEADERS lzokay/lzokay.hpp)

# ---------------------------------------------------------------------------
# Sources – heatshrink/ (mixed C and C++)
# ---------------------------------------------------------------------------
set(HEATSHRINK_SOURCES
    heatshrink/heatshrink_decoder.c
    heatshrink/heatshrink_encoder.c
    heatshrink/heatshrinkif.cpp
)

set(HEATSHRINK_HEADERS
    heatshrink/heatshrink_common.h
    heatshrink/heatshrink_config.h
    heatshrink/heatshrink_decoder.h
    heatshrink/heatshrink_encoder.h
    heatshrink/heatshrinkif.h
)

# ---------------------------------------------------------------------------
# Sources – QCodeEditor/
# ---------------------------------------------------------------------------
set(QCODEEDITOR_SOURCES
    QCodeEditor/src/internal/LispHighlighter.cpp
    QCodeEditor/src/internal/QCodeEditor.cpp
    QCodeEditor/src/internal/QCXXHighlighter.cpp
    QCodeEditor/src/internal/QFramedTextAttribute.cpp
    QCodeEditor/src/internal/QGLSLCompleter.cpp
    QCodeEditor/src/internal/QGLSLHighlighter.cpp
    QCodeEditor/src/internal/QJSONHighlighter.cpp
    QCodeEditor/src/internal/QLanguage.cpp
    QCodeEditor/src/internal/QLineNumberArea.cpp
    QCodeEditor/src/internal/QLispCompleter.cpp
    QCodeEditor/src/internal/QLuaCompleter.cpp
    QCodeEditor/src/internal/QLuaHighlighter.cpp
    QCodeEditor/src/internal/QPythonCompleter.cpp
    QCodeEditor/src/internal/QPythonHighlighter.cpp
    QCodeEditor/src/internal/QStyleSyntaxHighlighter.cpp
    QCodeEditor/src/internal/QSyntaxStyle.cpp
    QCodeEditor/src/internal/QXMLHighlighter.cpp
    QCodeEditor/src/internal/QmlHighlighter.cpp
    QCodeEditor/src/internal/QVescCompleter.cpp
)

set(QCODEEDITOR_HEADERS
    QCodeEditor/include/internal/LispHighlighter.hpp
    QCodeEditor/include/internal/QCodeEditor.hpp
    QCodeEditor/include/internal/QCXXHighlighter.hpp
    QCodeEditor/include/internal/QFramedTextAttribute.hpp
    QCodeEditor/include/internal/QGLSLCompleter.hpp
    QCodeEditor/include/internal/QGLSLHighlighter.hpp
    QCodeEditor/include/internal/QHighlightBlockRule.hpp
    QCodeEditor/include/internal/QHighlightRule.hpp
    QCodeEditor/include/internal/QJSONHighlighter.hpp
    QCodeEditor/include/internal/QLanguage.hpp
    QCodeEditor/include/internal/QLineNumberArea.hpp
    QCodeEditor/include/internal/QLispCompleter.hpp
    QCodeEditor/include/internal/QLuaCompleter.hpp
    QCodeEditor/include/internal/QLuaHighlighter.hpp
    QCodeEditor/include/internal/QPythonCompleter.hpp
    QCodeEditor/include/internal/QPythonHighlighter.hpp
    QCodeEditor/include/internal/QStyleSyntaxHighlighter.hpp
    QCodeEditor/include/internal/QSyntaxStyle.hpp
    QCodeEditor/include/internal/QXMLHighlighter.hpp
    QCodeEditor/include/internal/QmlHighlighter.hpp
    QCodeEditor/include/internal/QVescCompleter.hpp
)

# ---------------------------------------------------------------------------
# Sources – esp32/ (mixed C and C++)
# ---------------------------------------------------------------------------
set(ESP32_SOURCES
    esp32/esp32flash.cpp
    esp32/esp_loader.c
    esp32/esp_targets.c
    esp32/md5_hash.c
    esp32/serial_comm.c
)

set(ESP32_HEADERS
    esp32/esp32flash.h
    esp32/esp_loader.h
    esp32/esp_targets.h
    esp32/md5_hash.h
    esp32/serial_comm.h
    esp32/serial_comm_prv.h
    esp32/serial_io.h
)

# ---------------------------------------------------------------------------
# Sources – display_tool/
# ---------------------------------------------------------------------------
set(DISPLAY_TOOL_SOURCES
    display_tool/dispeditor.cpp
    display_tool/displayedit.cpp
    display_tool/imagewidgetdisp.cpp
)

set(DISPLAY_TOOL_HEADERS
    display_tool/dispeditor.h
    display_tool/displayedit.h
    display_tool/imagewidgetdisp.h
)

set(DISPLAY_TOOL_FORMS
    display_tool/dispeditor.ui
)

# ---------------------------------------------------------------------------
# Sources – qmarkdowntextedit/
# ---------------------------------------------------------------------------
set(QMARKDOWNTEXTEDIT_SOURCES
    qmarkdowntextedit/markdownhighlighter.cpp
    qmarkdowntextedit/qmarkdowntextedit.cpp
    qmarkdowntextedit/qownlanguagedata.cpp
    qmarkdowntextedit/qplaintexteditsearchwidget.cpp
)

set(QMARKDOWNTEXTEDIT_HEADERS
    qmarkdowntextedit/linenumberarea.h
    qmarkdowntextedit/markdownhighlighter.h
    qmarkdowntextedit/qmarkdowntextedit.h
    qmarkdowntextedit/qownlanguagedata.h
    qmarkdowntextedit/qplaintexteditsearchwidget.h
)

set(QMARKDOWNTEXTEDIT_FORMS
    qmarkdowntextedit/qplaintexteditsearchwidget.ui
)

# ---------------------------------------------------------------------------
# Sources – maddy/ (header-only markdown parser)
# ---------------------------------------------------------------------------
set(MADDY_HEADERS
    maddy/blockparser.h
    maddy/breaklineparser.h
    maddy/checklistparser.h
    maddy/codeblockparser.h
    maddy/emphasizedparser.h
    maddy/headlineparser.h
    maddy/horizontallineparser.h
    maddy/htmlparser.h
    maddy/imageparser.h
    maddy/inlinecodeparser.h
    maddy/italicparser.h
    maddy/latexblockparser.h
    maddy/orderedlistparser.h
    maddy/paragraphparser.h
    maddy/parserconfig.h
    maddy/parser.h
    maddy/quoteparser.h
    maddy/strikethroughparser.h
    maddy/strongparser.h
    maddy/tableparser.h
    maddy/unorderedlistparser.h
    maddy/lineparser.h
    maddy/linkparser.h
)

# ---------------------------------------------------------------------------
# Sources – minimp3/
# ---------------------------------------------------------------------------
set(MINIMP3_SOURCES minimp3/qminimp3.cpp)
set(MINIMP3_HEADERS
    minimp3/minimp3.h
    minimp3/minimp3_ex.h
    minimp3/qminimp3.h
)

# ---------------------------------------------------------------------------
# Sources – iOS-specific
# ---------------------------------------------------------------------------
if(IOS)
    set(IOS_SOURCES ios/src/setIosParameters.mm)
    set(IOS_HEADERS ios/src/setIosParameters.h)
else()
    set(IOS_SOURCES)
    set(IOS_HEADERS)
endif()

# ---------------------------------------------------------------------------
# Resource files (.qrc)
# ---------------------------------------------------------------------------
set(RESOURCE_FILES
    res.qrc
    res_custom_module.qrc
    res_lisp.qrc
    res_qml.qrc
    res/config/res_config.qrc
    res_fw_bms.qrc
    mobile/qml.qrc
    qmarkdowntextedit/media.qrc
    QCodeEditor/resources/qcodeeditor_resources.qrc
    ${TIER_QRC}
)

if(NOT EXCLUDE_FW)
    list(APPEND RESOURCE_FILES res/firmwares/res_fw.qrc)
endif()

# ---------------------------------------------------------------------------
# Collect all sources
# ---------------------------------------------------------------------------
set(ALL_SOURCES
    ${ROOT_SOURCES}
    ${PAGES_SOURCES}
    ${WIDGETS_SOURCES}
    ${MOBILE_SOURCES}
    ${MAP_SOURCES}
    ${LZOKAY_SOURCES}
    ${HEATSHRINK_SOURCES}
    ${QCODEEDITOR_SOURCES}
    ${ESP32_SOURCES}
    ${DISPLAY_TOOL_SOURCES}
    ${QMARKDOWNTEXTEDIT_SOURCES}
    ${MINIMP3_SOURCES}
    ${IOS_SOURCES}
)

set(ALL_HEADERS
    ${ROOT_HEADERS}
    ${PAGES_HEADERS}
    ${WIDGETS_HEADERS}
    ${MOBILE_HEADERS}
    ${MAP_HEADERS}
    ${LZOKAY_HEADERS}
    ${HEATSHRINK_HEADERS}
    ${QCODEEDITOR_HEADERS}
    ${ESP32_HEADERS}
    ${DISPLAY_TOOL_HEADERS}
    ${QMARKDOWNTEXTEDIT_HEADERS}
    ${MADDY_HEADERS}
    ${MINIMP3_HEADERS}
    ${IOS_HEADERS}
)

set(ALL_FORMS
    ${ROOT_FORMS}
    ${PAGES_FORMS}
    ${WIDGETS_FORMS}
    ${DISPLAY_TOOL_FORMS}
    ${QMARKDOWNTEXTEDIT_FORMS}
)

# ---------------------------------------------------------------------------
# Target name (mirrors old qmake logic)
# ---------------------------------------------------------------------------
if(IOS OR APPLE)
    set(APP_TARGET "VESC Tool")
elseif(ANDROID)
    set(APP_TARGET "vesc_tool")
else()
    set(APP_TARGET "vesc_tool_${VT_VERSION}")
endif()

# ---------------------------------------------------------------------------
# Create the executable
# ---------------------------------------------------------------------------
qt_add_executable(${APP_TARGET}
    ${ALL_SOURCES}
    ${ALL_HEADERS}
    ${ALL_FORMS}
    ${RESOURCE_FILES}
)

# ---------------------------------------------------------------------------
# Compile definitions
# ---------------------------------------------------------------------------
target_compile_definitions(${APP_TARGET} PRIVATE ${VT_DEFINITIONS})

# ---------------------------------------------------------------------------
# Include directories
# ---------------------------------------------------------------------------
target_include_directories(${APP_TARGET} PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/pages
    ${CMAKE_SOURCE_DIR}/widgets
    ${CMAKE_SOURCE_DIR}/mobile
    ${CMAKE_SOURCE_DIR}/map
    ${CMAKE_SOURCE_DIR}/lzokay
    ${CMAKE_SOURCE_DIR}/heatshrink
    ${CMAKE_SOURCE_DIR}/QCodeEditor/include
    ${CMAKE_SOURCE_DIR}/esp32
    ${CMAKE_SOURCE_DIR}/display_tool
    ${CMAKE_SOURCE_DIR}/qmarkdowntextedit
    ${CMAKE_SOURCE_DIR}/maddy
    ${CMAKE_SOURCE_DIR}/minimp3
    # AUTOUIC needs to find generated ui_*.h files
    ${CMAKE_CURRENT_BINARY_DIR}
)

# ---------------------------------------------------------------------------
# Link Qt libraries
# ---------------------------------------------------------------------------
set(QT_LINK_LIBS
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::QuickWidgets
    Qt6::Svg
    Qt6::GuiPrivate
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    Qt6::Core5Compat
)

if(NOT IOS)
    list(APPEND QT_LINK_LIBS Qt6::PrintSupport)
endif()

if(HAS_SERIALPORT)
    list(APPEND QT_LINK_LIBS Qt6::SerialPort)
endif()
if(HAS_CANBUS)
    list(APPEND QT_LINK_LIBS Qt6::SerialBus)
endif()
if(HAS_BLUETOOTH)
    list(APPEND QT_LINK_LIBS Qt6::Bluetooth)
endif()
if(HAS_POS)
    list(APPEND QT_LINK_LIBS Qt6::Positioning)
endif()
if(HAS_GAMEPAD)
    list(APPEND QT_LINK_LIBS Qt6::GamepadLegacy)
endif()

target_link_libraries(${APP_TARGET} PRIVATE ${QT_LINK_LIBS})

# ---------------------------------------------------------------------------
# Compiler flags
# ---------------------------------------------------------------------------
if(NOT MSVC)
    target_compile_options(${APP_TARGET} PRIVATE
        -Wno-deprecated-copy
    )
endif()

if(IOS)
    target_compile_options(${APP_TARGET} PRIVATE
        $<$<CONFIG:Debug>:-Wall>
    )
endif()

# ---------------------------------------------------------------------------
# AUTOUIC search paths (so AUTOUIC can find .ui files in subdirectories)
# ---------------------------------------------------------------------------
set_target_properties(${APP_TARGET} PROPERTIES
    AUTOUIC_SEARCH_PATHS
        "${CMAKE_SOURCE_DIR};${CMAKE_SOURCE_DIR}/pages;${CMAKE_SOURCE_DIR}/widgets;${CMAKE_SOURCE_DIR}/display_tool;${CMAKE_SOURCE_DIR}/qmarkdowntextedit"
)

# ---------------------------------------------------------------------------
# Android-specific configuration
# ---------------------------------------------------------------------------
if(ANDROID)
    # Page size alignment (required for some Android ABIs)
    target_link_options(${APP_TARGET} PRIVATE
        -Wl,-z,max-page-size=16384
        -Wl,-z,common-page-size=16384
    )

    # Determine android version code
    if(ANDROID_ABI STREQUAL "x86")
        set(VT_ANDROID_VERSION ${VT_ANDROID_VERSION_X86})
    elseif(ANDROID_ABI STREQUAL "arm64-v8a")
        set(VT_ANDROID_VERSION ${VT_ANDROID_VERSION_ARM64})
    elseif(ANDROID_ABI STREQUAL "armeabi-v7a")
        set(VT_ANDROID_VERSION ${VT_ANDROID_VERSION_ARMV7})
    else()
        set(VT_ANDROID_VERSION ${VT_ANDROID_VERSION_X86})
    endif()

    # Generate AndroidManifest.xml from template
    configure_file(
        ${CMAKE_SOURCE_DIR}/android/AndroidManifest.xml.in
        ${CMAKE_SOURCE_DIR}/android/AndroidManifest.xml
        @ONLY
    )

    set_target_properties(${APP_TARGET} PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/android"
    )
endif()

# ---------------------------------------------------------------------------
# Linux-specific (static libstdc++ for portable binaries)
# ---------------------------------------------------------------------------
if(UNIX AND NOT APPLE AND NOT ANDROID)
    option(STATIC_LIBCXX "Statically link libstdc++ and libgcc" OFF)
    if(STATIC_LIBCXX)
        target_link_options(${APP_TARGET} PRIVATE
            -static-libstdc++ -static-libgcc
        )
    endif()
endif()

# ---------------------------------------------------------------------------
# macOS-specific
# ---------------------------------------------------------------------------
if(APPLE AND NOT IOS)
    set_target_properties(${APP_TARGET} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/macos/Info.plist"
        MACOSX_BUNDLE_ICON_FILE "appIcon"
    )

    # Install the icon into the bundle
    set(MACOS_ICON "${CMAKE_SOURCE_DIR}/macos/appIcon.icns")
    set_source_files_properties(${MACOS_ICON} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
    target_sources(${APP_TARGET} PRIVATE ${MACOS_ICON})

    # Universal binary (x86_64 + arm64)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "macOS architectures")
endif()

# ---------------------------------------------------------------------------
# iOS-specific
# ---------------------------------------------------------------------------
if(IOS)
    set_target_properties(${APP_TARGET} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/ios/Info.plist"
    )

    target_compile_definitions(${APP_TARGET} PRIVATE QT_NO_PRINTER)

    # Asset catalog
    set_target_properties(${APP_TARGET} PROPERTIES
        XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon"
    )

    # Launch screen storyboard
    set(IOS_STORYBOARD "${CMAKE_SOURCE_DIR}/ios/MyLaunchScreen.storyboard")
    if(EXISTS ${IOS_STORYBOARD})
        set_source_files_properties(${IOS_STORYBOARD} PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources"
        )
        target_sources(${APP_TARGET} PRIVATE ${IOS_STORYBOARD})
    endif()

    # iTunes artwork
    file(GLOB IOS_ARTWORK "${CMAKE_SOURCE_DIR}/ios/iTunesArtwork*")
    if(IOS_ARTWORK)
        set_source_files_properties(${IOS_ARTWORK} PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources"
        )
        target_sources(${APP_TARGET} PRIVATE ${IOS_ARTWORK})
    endif()

    # Target device family: 1=iPhone, 2=iPad, 1,2=Universal
    set_target_properties(${APP_TARGET} PROPERTIES
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
        XCODE_ATTRIBUTE_GCC_WARN_64_TO_32_BIT_CONVERSION "NO"
    )
endif()

# ---------------------------------------------------------------------------
# Windows-specific
# ---------------------------------------------------------------------------
if(WIN32)
    set_target_properties(${APP_TARGET} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# ---------------------------------------------------------------------------
# Install rules (basic)
# ---------------------------------------------------------------------------
include(GNUInstallDirs)
install(TARGETS ${APP_TARGET}
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
