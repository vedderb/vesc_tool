/*
    Copyright 2016 - 2023 Benjamin Vedder	benjamin@vedder.se

    This file is part of VESC Tool.

    VESC Tool is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VESC Tool is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    */

#include "mainwindow.h"
#include "boardsetupwindow.h"
#include "mobile/qmlui.h"
#include "mobile/fwhelper.h"
#include "mobile/vesc3ditem.h"
#include "mobile/logwriter.h"
#include "mobile/logreader.h"
#include "tcpserversimple.h"
#include "pages/pagemotorcomparison.h"
#include "codeloader.h"
#include "configparam.h"
#include "utility.h"
#include "heatshrink/heatshrinkif.h"
#include "minimp3/qminimp3.h"

#include <QApplication>
#include <QStyleFactory>
#include <QSettings>
#include <QDesktopWidget>
#include <QFontDatabase>
#include <QPixmapCache>

#include "tcphub.h"

#ifndef HAS_BLUETOOTH
#include "bleuartdummy.h"
#endif

#ifdef Q_OS_IOS
#include "ios/src/setIosParameters.h"
#endif

#ifdef Q_OS_LINUX
#include <signal.h>
#include <systemcommandexecutor.h>
#endif

#ifndef USE_MOBILE

#include <QProxyStyle>
#include <QtConcurrent/QtConcurrent>

// Disables focus drawing for all widgets
class Style_tweaks : public QProxyStyle
{
public:
    using QProxyStyle::QProxyStyle;
    void drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                       QPainter *painter, const QWidget *widget) const
    {
        if (element == QStyle::PE_FrameFocusRect) return;

        QProxyStyle::drawPrimitive(element, option, painter, widget);
    }
};

static void showHelp()
{
    qDebug() << "Arguments";
    qDebug() << "-h, --help : Show help text";
    qDebug() << "--tcpServer [port] : Connect to VESC and start TCP server on [port]";
    qDebug() << "--loadQml [file] : Load QML UI from file instead of the regular VESC Tool UI";
    qDebug() << "--loadQmlVesc : Load QML UI from the connected VESC instead of the regular VESC Tool UI";
    qDebug() << "--qmlAutoConn : Connect over USB before loading the QML UI";
    qDebug() << "--qmlFullscreen : Run QML UI in fullscreen mode";
    qDebug() << "--qmlOtherScreen : Run QML UI on other screen";
    qDebug() << "--qmlRotation [deg] : Rotate screen by deg degrees";
    qDebug() << "--qmlWindowSize [width:height] : Specify qml window size";
    qDebug() << "--retryConn : Keep trying to reconnect to the VESC when the connection fails";
    qDebug() << "--useMobileUi : Start the mobile UI instead of the full desktop UI";
    qDebug() << "--tcpHub [port] : Start a TCP hub for remote access to connected VESCs";
    qDebug() << "--buildPkg [pkgPath:lispPath:qmlPath:isFullscreen:optMd:optName] : Build VESC Package";
    qDebug() << "--useBoardSetupWindow : Start board setup window instead of the main UI";
    qDebug() << "--xmlConfToCode [xml-file] : Generate C code from XML configuration file (the files are saved in the same directory as the XML)";
    qDebug() << "--vescPort [port] : VESC Port for commands that connect, e.g. /dev/ttyACM0. If this command is left out autoconnect will be used.";
    qDebug() << "--canFwd [canId] : Can ID for CAN forwarding";
    qDebug() << "--getMcConf [confPath] : Connect and read motor configuration and store the XML to confPath.";
    qDebug() << "--setMcConf [confPath] : Connect and write motor configuration XML from confPath.";
    qDebug() << "--getAppConf [confPath] : Connect and read app configuration and store the XML to confPath.";
    qDebug() << "--setAppConf [confPath] : Connect and write app configuration XML from confPath.";
    qDebug() << "--getCustomConf [confPath] : Connect and read custom configuration 1 and store the XML to confPath.";
    qDebug() << "--setCustomConf [confPath] : Connect and write custom configuration 1 XML from confPath.";
    qDebug() << "--debugOutFile [path] : Print debug output to file with path.";
    qDebug() << "--uploadLisp [path] : Upload lisp-script.";
    qDebug() << "--eraseLisp : Erase lisp-script.";
    qDebug() << "--uploadFirmware [path] : Upload firmware-file from path.";
    qDebug() << "--uploadBootloaderBuiltin : Upload bootloader from generic included bootloaders.";
    qDebug() << "--writeFileToSdCard [fileLocal:pathSdcard] : Write file to SD-card.";
    qDebug() << "--packFirmware [fileIn:fileOut] : Pack firmware-file for compatibility with the bootloader. ";
    qDebug() << "--packLisp [fileIn:fileOut] : Pack lisp-file and the included imports.";
    qDebug() << "--bridgeAppData : Send app data (such as data from send-data in lisp) to stdout.";
    qDebug() << "--offscreen : Use offscreen QPA so that X is not required for the CLI-mode.";
    qDebug() << "--downloadPackageArchive : Download package archive to application data directory.";
}

#ifdef Q_OS_LINUX
static void m_cleanup(int sig)
{
    (void)sig;
    qApp->quit();
}
#endif
#endif

QFile m_debug_msg_file;

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    (void)type;
    (void)context;

    if (m_debug_msg_file.isOpen()) {
        m_debug_msg_file.write(msg.toUtf8());
        m_debug_msg_file.write("\n");
        m_debug_msg_file.flush();
    }
}

static void addFonts() {
    QFontDatabase::addApplicationFont("://res/fonts/DejaVuSans.ttf");
    QFontDatabase::addApplicationFont("://res/fonts/DejaVuSans-Bold.ttf");
    QFontDatabase::addApplicationFont("://res/fonts/DejaVuSans-BoldOblique.ttf");
    QFontDatabase::addApplicationFont("://res/fonts/DejaVuSans-Oblique.ttf");
    QFontDatabase::addApplicationFont("://res/fonts/DejaVuSansMono.ttf");
    QFontDatabase::addApplicationFont("://res/fonts/DejaVuSansMono-Bold.ttf");
    QFontDatabase::addApplicationFont("://res/fonts/DejaVuSansMono-BoldOblique.ttf");
    QFontDatabase::addApplicationFont("://res/fonts/DejaVuSansMono-Oblique.ttf");

    QFontDatabase::addApplicationFont("://res/fonts/Roboto/Roboto-Regular.ttf");
    QFontDatabase::addApplicationFont("://res/fonts/Roboto/Roboto-Medium.ttf");
    QFontDatabase::addApplicationFont("://res/fonts/Roboto/Roboto-Bolf.ttf");
    QFontDatabase::addApplicationFont("://res/fonts/Roboto/Roboto-BoldItalic.ttf");
    QFontDatabase::addApplicationFont("://res/fonts/Roboto/Roboto-Italic.ttf");
    QFontDatabase::addApplicationFont(":/res/fonts/Roboto/RobotoMono-VariableFont_wght.ttf");

    QFontDatabase::addApplicationFont("://res/fonts/Exan-Regular.ttf");

    qApp->setFont(QFont("Roboto", 12));
}

int main(int argc, char *argv[])
{
    // Settings
    QCoreApplication::setOrganizationName("VESC");
    QCoreApplication::setOrganizationDomain("vesc-project.com");
    QCoreApplication::setApplicationName("VESC Tool");
    QSettings set;
    bool isDark = set.value("darkMode", true).toBool();
    Utility::setDarkMode(isDark);
    QPixmapCache::setCacheLimit(256000);

    if (isDark) {
        qputenv("QT_QUICK_CONTROLS_CONF", ":/qtquickcontrols2_dark.conf");

        Utility::setAppQColor("lightestBackground", QColor(80,80,80));
        Utility::setAppQColor("lightBackground", QColor(72,72,72));
        Utility::setAppQColor("normalBackground", QColor(48,48,48));
        Utility::setAppQColor("darkBackground", QColor(39,39,39));
        Utility::setAppQColor("plotBackground", QColor(39,39,39));
        Utility::setAppQColor("normalText", QColor(180,180,180));
        Utility::setAppQColor("lightText", QColor(215,215,215));
        Utility::setAppQColor("disabledText", QColor(127,127,127));
        Utility::setAppQColor("lightAccent", QColor(0,161,221));
        Utility::setAppQColor("tertiary1",QColor(229, 207, 51));
        Utility::setAppQColor("tertiary2",QColor(51, 180, 229));
        Utility::setAppQColor("tertiary3",QColor(136, 51, 229));
        Utility::setAppQColor("midAccent", QColor(0,98,153));
        Utility::setAppQColor("darkAccent", QColor(0,69,112));
        Utility::setAppQColor("pink", QColor(219,98,139));
        Utility::setAppQColor("red", QColor(200,52,52));
        Utility::setAppQColor("orange", QColor(206,125,44));
        Utility::setAppQColor("yellow", QColor(210,210,127));
        Utility::setAppQColor("green", QColor(127,200,127));
        Utility::setAppQColor("cyan",QColor(79,203,203));
        Utility::setAppQColor("blue", QColor(77,127,196));
        Utility::setAppQColor("magenta", QColor(157,127,210));
        Utility::setAppQColor("white", QColor(255,255,255));
        Utility::setAppQColor("black", QColor(0,0,0));
    } else {
        qputenv("QT_QUICK_CONTROLS_CONF", ":/qtquickcontrols2.conf");

        Utility::setAppQColor("lightestBackground", QColor(200,200,200));
        Utility::setAppQColor("lightBackground", QColor(225,225,225));
        Utility::setAppQColor("normalBackground", QColor(240,240,240));
        Utility::setAppQColor("darkBackground", QColor(255,255,255));
        Utility::setAppQColor("plotBackground", QColor(250,250,250));
        Utility::setAppQColor("normalText", QColor(60,20,60));
        Utility::setAppQColor("lightText", QColor(33,33,33));
        Utility::setAppQColor("disabledText", QColor(110,110,110));
        Utility::setAppQColor("lightAccent", QColor(0,114,178));
        Utility::setAppQColor("tertiary1",QColor(229, 207, 51));
        Utility::setAppQColor("tertiary2",QColor(51, 180, 229));
        Utility::setAppQColor("tertiary3",QColor(136, 51, 229));
        Utility::setAppQColor("midAccent", QColor(0,114,178));
        Utility::setAppQColor("darkAccent", QColor(0,161,221));
        Utility::setAppQColor("pink", QColor(219,98,139));
        Utility::setAppQColor("red", QColor(200,52,52));
        Utility::setAppQColor("orange", QColor(206,125,44));
        Utility::setAppQColor("yellow", QColor(210,210,127));
        Utility::setAppQColor("green", QColor(127,200,127));
        Utility::setAppQColor("cyan",QColor(79,203,203));
        Utility::setAppQColor("blue", QColor(77,127,196));
        Utility::setAppQColor("magenta", QColor(157,127,210));
        Utility::setAppQColor("white", QColor(255,255,255));
        Utility::setAppQColor("black", QColor(0,0,0));
    }

    // DPI settings
    // TODO: http://www.qcustomplot.com/index.php/support/forum/1344

    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

#ifdef HAS_BLUETOOTH
    qmlRegisterType<BleUart>("Vedder.vesc.bleuart", 1, 0, "BleUart");
#else
    qmlRegisterType<BleUartDummy>("Vedder.vesc.bleuart", 1, 0, "BleUart");
#endif
    qmlRegisterType<Commands>("Vedder.vesc.commands", 1, 0, "Commands");
    qmlRegisterType<ConfigParams>("Vedder.vesc.configparams", 1, 0, "ConfigParams");
    qmlRegisterType<FwHelper>("Vedder.vesc.fwhelper", 1, 0, "FwHelper");
    qmlRegisterType<Esp32Flash>("Vedder.vesc.esp32flash", 1, 0, "Esp32Flash");
    qmlRegisterType<TcpServerSimple>("Vedder.vesc.tcpserversimple", 1, 0, "TcpServerSimple");
    qmlRegisterType<UdpServerSimple>("Vedder.vesc.udpserversimple", 1, 0, "UdpServerSimple");
    qmlRegisterType<Vesc3dItem>("Vedder.vesc.vesc3ditem", 1, 0, "Vesc3dItem");
    qmlRegisterType<LogWriter>("Vedder.vesc.logwriter", 1, 0, "LogWriter");
    qmlRegisterType<LogReader>("Vedder.vesc.logreader", 1, 0, "LogReader");
    qmlRegisterType<TcpHub>("Vedder.vesc.tcphub", 1, 0, "TcpHub");
    qmlRegisterType<CodeLoader>("Vedder.vesc.codeloader", 1, 0, "CodeLoader");
    qmlRegisterType<QMiniMp3>("Vedder.vesc.qminimp3", 1, 0, "QMiniMp3");
#ifdef Q_OS_LINUX
    qmlRegisterType<SystemCommandExecutor>("Vedder.vesc.syscmd", 1, 0, "SysCmd");
#endif

    qRegisterMetaType<VSerialInfo_t>();
    qRegisterMetaType<MCCONF_TEMP>();
    qRegisterMetaType<MC_VALUES>();
    qRegisterMetaType<BMS_VALUES>();
    qRegisterMetaType<FW_RX_PARAMS>();
    qRegisterMetaType<PSW_STATUS>();
    qRegisterMetaType<IO_BOARD_VALUES>();
    qRegisterMetaType<MotorData>();
    qRegisterMetaType<ENCODER_DETECT_RES>();
    qRegisterMetaType<FILE_LIST_ENTRY>();
    qRegisterMetaType<VescPackage>();
    qRegisterMetaType<TCP_HUB_DEVICE>();
    qRegisterMetaType<ConfigParam>();
    qRegisterMetaType<GNSS_DATA>();
    qRegisterMetaType<MiniMp3Dec>();

#ifdef USE_MOBILE
#ifndef DEBUG_BUILD
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
#else
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

#ifdef Q_OS_LINUX
    signal(SIGINT, m_cleanup);
    signal(SIGTERM, m_cleanup);
#endif

    // Parse command line arguments
    QStringList args;
    for (int i = 0;i < argc;i++) {
        args.append(argv[i]);
    }

    bool useTcp = false;
    bool retryConn = false;
    int tcpPort = 65102;
    QString loadQml = "";
    bool qmlAutoConn = false;
    bool qmlFullscreen = false;
    bool loadQmlVesc = false;
    bool qmlOtherScreen = false;
    bool useMobileUi = false;
    bool useBoardSetupWindow = false;
    double qmlRot = 0.0;
    bool isTcpHub = false;
    QStringList pkgArgs;
    QString xmlCodePath = "";
    QString vescPort = "";
    int canFwd = -1;
    QString getMcConfPath = "";
    QString setMcConfPath = "";
    QString getAppConfPath = "";
    QString setAppConfPath = "";
    QString getCustomConfPath = "";
    QString setCustomConfPath = "";
    QSize qmlWindowSize = QSize(-1, -1);
    QString lispPath = "";
    bool eraseLisp = false;
    QString firmwarePath = "";
    bool uploadBootloaderBuiltin = false;
    QString fwPackIn = "";
    QString fwPackOut = "";
    QString fileForSdIn = "";
    QString fileForSdOut = "";
    QString lispPackIn = "";
    QString lispPackOut = "";
    bool bridgeAppData = false;
    bool offscreen = false;
    bool downloadPackageArchive = false;

    // Arguments can be hard-coded in a build like this:
//    qmlWindowSize = QSize(400, 800);
//    loadQmlVesc = true;
//    retryConn = true;

    for (int i = 0;i < args.size();i++) {
        // Skip the program argument
        if (i == 0) {
            continue;
        }

        QString str = args.at(i);

        // Skip path argument
        if (i >= args.size() && args.size() >= 3) {
            break;
        }

        bool dash = str.startsWith("-") && !str.startsWith("--");
        bool found = false;

        if ((dash && str.contains('h')) || str == "--help") {
            showHelp();
            return 0;
        }

        if (str == "--tcpServer") {
            if ((i + 1) < args.size()) {
                i++;
                tcpPort = args.at(i).toInt();
                useTcp = true;
                found = true;
            }
        }

        if (str == "--retryConn") {
            retryConn = true;
            found = true;
        }

        if (str == "--loadQml") {
            if ((i + 1) < args.size()) {
                i++;
                loadQml = args.at(i);
                found = true;
            } else {
                i++;
                qCritical() << "No path to qml UI file";
                return 1;
            }
        }

        if (str == "--loadQmlVesc") {
            loadQmlVesc = true;
            found = true;
        }

        if (str == "--qmlAutoConn") {
            qmlAutoConn = true;
            found = true;
        }

        if (str == "--qmlFullscreen") {
            qmlFullscreen = true;
            found = true;
        }

        if (str == "--qmlOtherScreen") {
            qmlOtherScreen = true;
            found = true;
        }

        if (str == "--useMobileUi") {
            useMobileUi = true;
            found = true;
        }

        if (str == "--useBoardSetupWindow") {
            useBoardSetupWindow = true;
            found = true;
        }

        if (str.startsWith("-qmljsdebugger")) {
            found = true;
        }

        if (str == "--qmlRotation") {
            if ((i + 1) < args.size()) {
                i++;
                qmlRot = args.at(i).toDouble();
                found = true;
            } else {
                i++;
                qCritical() << "No rotation specified";
                return 1;
            }
        }

        if (str == "--qmlWindowSize") {
            if ((i + 1) < args.size()) {
                i++;
                auto p = args.at(i).split(":");
                if (p.size() == 2) {
                    qmlWindowSize.setWidth(p.at(0).toInt());
                    qmlWindowSize.setHeight(p.at(1).toInt());
                } else {
                    qCritical() << "Invalid size specified";
                    return 1;
                }

                found = true;
            } else {
                i++;
                qCritical() << "No size specified";
                return 1;
            }
        }

        if (str == "--tcpHub") {
            if ((i + 1) < args.size()) {
                i++;
                tcpPort = args.at(i).toInt();
                isTcpHub = true;
                found = true;
            }
        }

        if (str == "--buildPkg") {
            if ((i + 1) < args.size()) {
                i++;
                pkgArgs = args.at(i).split(":");
                found = true;
            }
        }

        if (str == "--xmlConfToCode") {
            if ((i + 1) < args.size()) {
                i++;
                xmlCodePath = args.at(i);
                found = true;
            } else {
                i++;
                qCritical() << "No path to xml file";
                return 1;
            }
        }

        if (str == "--vescPort") {
            if ((i + 1) < args.size()) {
                i++;
                vescPort = args.at(i);
                found = true;
            } else {
                i++;
                qCritical() << "No port specified";
                return 1;
            }
        }

        if (str == "--canFwd") {
            if ((i + 1) < args.size()) {
                i++;
                canFwd = args.at(i).toInt(),
                found = true;
            } else {
                i++;
                qCritical() << "No can id specified";
                return 1;
            }
        }

        if (str == "--getMcConf") {
            if ((i + 1) < args.size()) {
                i++;
                getMcConfPath = args.at(i);
                found = true;
            } else {
                i++;
                qCritical() << "No path specified";
                return 1;
            }
        }

        if (str == "--setMcConf") {
            if ((i + 1) < args.size()) {
                i++;
                setMcConfPath = args.at(i);
                found = true;
            } else {
                i++;
                qCritical() << "No path specified";
                return 1;
            }
        }

        if (str == "--getAppConf") {
            if ((i + 1) < args.size()) {
                i++;
                getAppConfPath = args.at(i);
                found = true;
            } else {
                i++;
                qCritical() << "No path specified";
                return 1;
            }
        }

        if (str == "--setAppConf") {
            if ((i + 1) < args.size()) {
                i++;
                setAppConfPath = args.at(i);
                found = true;
            } else {
                i++;
                qCritical() << "No path specified";
                return 1;
            }
        }

        if (str == "--getCustomConf") {
            if ((i + 1) < args.size()) {
                i++;
                getCustomConfPath = args.at(i);
                found = true;
            } else {
                i++;
                qCritical() << "No path specified";
                return 1;
            }
        }

        if (str == "--setCustomConf") {
            if ((i + 1) < args.size()) {
                i++;
                setCustomConfPath = args.at(i);
                found = true;
            } else {
                i++;
                qCritical() << "No path specified";
                return 1;
            }
        }

        if (str == "--uploadLisp") {
            if ((i + 1) < args.size()) {
                i++;
                lispPath = args.at(i);
                found = true;
            } else {
                i++;
                qCritical() << "No path specified";
                return 1;
            }
        }

        if (str == "--eraseLisp") {
            eraseLisp = true;
            found = true;
        }

        if (str == "--uploadFirmware") {
            if ((i + 1) < args.size()) {
                i++;
                firmwarePath = args.at(i);
                found = true;
            } else {
                i++;
                qCritical() << "No path specified";
                return 1;
            }
        }

        if (str == "--uploadBootloaderBuiltin") {
            uploadBootloaderBuiltin = true;
            found = true;
        }

        if (str == "--debugOutFile") {
            if ((i + 1) < args.size()) {
                i++;
                if (!m_debug_msg_file.isOpen()) {
                    m_debug_msg_file.setFileName(args.at(i));
                    if (m_debug_msg_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                        qInstallMessageHandler(myMessageOutput);
                    }
                }
                found = true;
            } else {
                i++;
                qCritical() << "No path specified";
                return 1;
            }
        }

        if (str == "--writeFileToSdCard") {
            if ((i + 1) < args.size()) {
                i++;
                auto p = args.at(i).split(":");
                if (p.size() == 2) {
                    fileForSdIn = p.at(0);
                    fileForSdOut = p.at(1);
                } else {
                    qCritical() << "Invalid paths specified";
                    return 1;
                }

                found = true;
            } else {
                i++;
                qCritical() << "No paths specified";
                return 1;
            }
        }

        if (str == "--packFirmware") {
            if ((i + 1) < args.size()) {
                i++;
                auto p = args.at(i).split(":");
                if (p.size() == 2) {
                    fwPackIn = p.at(0);
                    fwPackOut = p.at(1);
                } else {
                    qCritical() << "Invalid paths specified";
                    return 1;
                }

                found = true;
            } else {
                i++;
                qCritical() << "No paths specified";
                return 1;
            }
        }

        if (str == "--packLisp") {
            if ((i + 1) < args.size()) {
                i++;
                auto p = args.at(i).split(":");
                if (p.size() == 2) {
                    lispPackIn = p.at(0);
                    lispPackOut = p.at(1);
                } else {
                    qCritical() << "Invalid paths specified";
                    return 1;
                }

                found = true;
            } else {
                i++;
                qCritical() << "No paths specified";
                return 1;
            }
        }

        if (str == "--bridgeAppData") {
            bridgeAppData = true;
            found = true;
        }

        if (str == "--offscreen") {
            offscreen = true;
            found = true;
        }

        if (str == "--downloadPackageArchive") {
            downloadPackageArchive = true;
            found = true;
        }

        if (!found) {
            if (dash) {
                qCritical() << "At least one of the flags is invalid:" << str;
            } else {
                qCritical() << "Invalid option:" << str;
            }

            showHelp();
            return 1;
        }
    }

    if (downloadPackageArchive) {
        QCoreApplication appTmp(argc, argv);
        CodeLoader loader;
        qDebug() << "Downloading package archive...";
        loader.downloadPackageArchive();
        qDebug() << "Package archive downloaded!";
    }

    if (!xmlCodePath.isEmpty()) {
        ConfigParams conf;
        if (!conf.loadParamsXml(xmlCodePath)) {
            qCritical() << "Could not parse XML-file" << xmlCodePath;
            return 1;
        }

        QString nameConfig = "device_config";
        if (conf.hasParam("config_name") && conf.getParam("config_name")->type == CFG_T_QSTRING) {
            nameConfig = conf.getParamQString("config_name");
        }

        QFileInfo fi(xmlCodePath);
        xmlCodePath.chop(fi.fileName().length());
        QString pathDefines = xmlCodePath + "conf_default.h";
        QString pathParser = xmlCodePath + "confparser.c";
        QString pathCompressed = xmlCodePath + "confxml.c";

        Utility::createCompressedConfigC(&conf, nameConfig, pathCompressed);
        Utility::createParamParserC(&conf, nameConfig, pathParser);
        conf.saveCDefines(pathDefines, true);

        qDebug() << "Done!";
        return 0;
    }

    if (!fwPackIn.isEmpty()) {
        if (!fwPackIn.endsWith(".bin", Qt::CaseInsensitive)) {
            qWarning() << "Warning: Unexpected file extension for a firmware-file.";
        }

        QFile fIn(fwPackIn);
        if (!fIn.open(QIODevice::ReadOnly)) {
            qWarning() << QString("Could not open %1 for reading.").arg(fwPackIn);
            return 1;
        }

        QFile fOut(fwPackOut);
        if (!fOut.open(QIODevice::WriteOnly)) {
            qWarning() << QString("Could not open %1 for writing.").arg(fwPackOut);
            return 1;
        }

        QByteArray newFirmware = fIn.readAll();
        fIn.close();

        int szTot = newFirmware.size();

        bool useHeatshrink = false;
        if (szTot > 393208 && szTot < 700000) { // If fw is much larger it is probably for the esp32
            useHeatshrink = true;
            qDebug() << "Firmware is big, using heatshrink compression library";
            int szOld = szTot;
            HeatshrinkIf hs;
            newFirmware = hs.encode(newFirmware);
            szTot = newFirmware.size();
            qDebug() << "New size:" << szTot << "(" << 100.0 * (double)szTot / (double)szOld << "%)";

            if (szTot > 393208) {
                qWarning() << "Firmware too big" <<
                            "The firmware you are trying to upload is too large for the bootloader even after compression.";
                return -1;
            }
        }

        if (szTot > 5000000) {
            qWarning() << "Firmware too big" <<
                        "The firmware you are trying to upload is unreasonably "
                        "large, most likely it is an invalid file";
            return -2;
        }

        quint16 crc = Packet::crc16((const unsigned char*)newFirmware.constData(),
                                    uint32_t(newFirmware.size()));
        VByteArray sizeCrc;
        if (useHeatshrink) {
            uint32_t szShift = 0xCC;
            szShift <<= 24;
            szShift |= szTot;
            sizeCrc.vbAppendUint32(szShift);
        } else {
            sizeCrc.vbAppendUint32(szTot);
        }
        sizeCrc.vbAppendUint16(crc);
        newFirmware.prepend(sizeCrc);
        fOut.write(newFirmware);
        fOut.close();

        qDebug() << "Done!";
        return 0;
    }

    if (!lispPackIn.isEmpty()) {
        if (!lispPackIn.endsWith(".lisp", Qt::CaseInsensitive)) {
            qWarning() << "Warning: Unexpected file extension for a lisp-file.";
        }

        QFile fIn(lispPackIn);
        if (!fIn.open(QIODevice::ReadOnly)) {
            qWarning() << QString("Could not open %1 for reading.").arg(lispPackIn);
            return 1;
        }

        QFile fOut(lispPackOut);
        if (!fOut.open(QIODevice::WriteOnly)) {
            qWarning() << QString("Could not open %1 for writing.").arg(lispPackOut);
            return 1;
        }

        CodeLoader loader;
        QFileInfo fi(fIn);
        VByteArray vb = loader.lispPackImports(fIn.readAll(), fi.canonicalPath());
        fIn.close();

        quint16 crc = Packet::crc16((const unsigned char*)vb.constData(), uint32_t(vb.size()));
        VByteArray data;
        data.vbAppendUint32(vb.size() - 2);
        data.vbAppendUint16(crc);
        data.append(vb);

        fOut.write(data);
        fOut.close();

        qDebug() << "Done!";
        return 0;
    }

    if (!pkgArgs.isEmpty()) {
        if (pkgArgs.size() < 4) {
            qWarning() << "Invalid arguments";
            return 1;
        }

        CodeLoader loader;
        QString pkgPath = pkgArgs.at(0);
        lispPath = pkgArgs.at(1);
        QString qmlPath = pkgArgs.at(2);
        bool isFullscreen = pkgArgs.at(3).toInt();

        QString mdPath;
        QString name;

        VescPackage pkg;

        if (pkgArgs.size() >= 6) {
            mdPath = pkgArgs.at(4);
            name = pkgArgs.at(5);

            QFile f(mdPath);
            if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                qWarning() << "Could not open markdown file for reading.";
                return 1;
            }

            QString desc = QString::fromUtf8(f.readAll());
            f.close();

            pkg.name = name;
            pkg.description_md = desc;
            pkg.description = Utility::md2html(desc);
        } else {
            QFile f(pkgPath);
            if (!f.open(QIODevice::ReadOnly)) {
                qWarning() << QString("Could not open %1 for reading.").arg(pkgPath);
                return 1;
            }

            pkg = loader.unpackVescPackage(f.readAll());
            f.close();

            qDebug() << "Opened package" << pkg.name;
        }

        if (!lispPath.isEmpty()) {
            QFile f(lispPath);
            if (!f.open(QIODevice::ReadOnly)) {
                qWarning() << "Could not open lisp file for reading.";
                return 1;
            }

            QFileInfo fi(f);
            pkg.lispData = loader.lispPackImports(f.readAll(), fi.canonicalPath());
            // Empty array means an error. Otherwise, CodeLoader.lispPackImports() always returns data.
            if (pkg.lispData.isEmpty()) {
                qWarning() << "Errors when processing lisp imports.";
                return 1;
            }
            f.close();

            qDebug() << "Read lisp script done";
        }

        if (!qmlPath.isEmpty()) {
            QFile f(qmlPath);
            if (!f.open(QIODevice::ReadOnly)) {
                qWarning() << "Could not open qml file for reading.";
                return 1;
            }

            pkg.qmlFile = f.readAll();
            pkg.qmlIsFullscreen =isFullscreen;
            f.close();

            qDebug() << "Read qml script done";
        }

        QFile file(pkgPath);
        if (!file.open(QIODevice::WriteOnly)) {
            qWarning() << QString("Could not open %1 for writing.").arg(pkgPath);
            return 1;
        }

        file.write(loader.packVescPackage(pkg));
        file.close();

        qDebug() << "Package Saved!";
        return 0;
    }

    double scale = set.value("app_scale_factor", 1.0).toDouble();

#ifdef Q_OS_ANDROID
    scale = 1.0;
#endif

    if (scale > 1.01) {
        qputenv("QT_SCALE_FACTOR", QString::number(scale).toLocal8Bit());
    }
#endif

    QCoreApplication *app;

#ifdef USE_MOBILE
    QApplication *a = new QApplication(argc, argv);
    app = a;

    addFonts();

    QmlUi *qml = new QmlUi;
    qml->startQmlUi();

    // As background running is allowed, make sure to not update the GUI when
    // running in the background.
    QObject::connect(a, &QApplication::applicationStateChanged, [&qml](Qt::ApplicationState state) {
        if(state == Qt::ApplicationHidden) {
            qml->setVisible(false);
        } else {
            qml->setVisible(true);
        }
    });
#else
    VescInterface *vesc = nullptr;
    TcpHub *tcpHub = nullptr;
    MainWindow *w = nullptr;
    BoardSetupWindow *bw = nullptr;
    QmlUi *qmlUi = nullptr;
    QString qmlStr;

    QTimer connTimer;
    connTimer.setInterval(1000);
    QObject::connect(&connTimer, &QTimer::timeout, [&]() {
        if (!vesc->isPortConnected()) {
            if (qmlUi != nullptr) {
                qmlUi->clearQmlCache();

                QTimer::singleShot(10, [&]() {
                    qmlUi->emitReloadCustomGui("qrc:/res/qml/DynamicLoader.qml");
                });
            }

            bool ok = false;
            if (vescPort.isEmpty()) {
                ok = vesc->autoconnect();
            } else {
                ok = vesc->connectSerial(vescPort);
            }

            if (ok) {
                qDebug() << "Connected";
            } else {
                qDebug() << "Could not connect";

                if (!retryConn) {
                    qApp->quit();
                }
            }
        }
    });

    bool isMcConf = !getMcConfPath.isEmpty() || !setMcConfPath.isEmpty();
    bool isAppConf = !getAppConfPath.isEmpty() || !setAppConfPath.isEmpty();
    bool isCustomConf = !getCustomConfPath.isEmpty() || !setCustomConfPath.isEmpty();

    if (isMcConf || isAppConf || isCustomConf || !lispPath.isEmpty() ||
            eraseLisp || !firmwarePath.isEmpty() || uploadBootloaderBuiltin ||
            !fileForSdIn.isEmpty() || bridgeAppData) {
        if (offscreen) {
            qputenv("QT_QPA_PLATFORM", "offscreen");
        }
        app = new QCoreApplication(argc, argv);
        vesc = new VescInterface;
        vesc->setIgnoreCustomConfigs(!isCustomConf);

        vesc->fwConfig()->loadParamsXml("://res/config/fw.xml");
        Utility::configLoadLatest(vesc);

        if (bridgeAppData) {
            QObject::connect(vesc->commands(), &Commands::customAppDataReceived, [] (QByteArray data) {
                fprintf(stdout, "%s", data.constData());
                fflush(stdout);
            });
        }

        QObject::connect(vesc, &VescInterface::statusMessage, [firmwarePath]
                         (const QString &msg, bool isGood) {
            if (isGood) {
                qDebug() << msg;
            } else {
                // Firmware upload tends to end with a serial port error when jumping to the bootloader, do not print it
                if (firmwarePath.isEmpty() || !msg.startsWith("Serial port error")) {
                    qWarning() << msg;
                }
            }
        });

        QObject::connect(vesc, &VescInterface::messageDialog, []
                         (const QString &title, const QString &msg, bool isGood, bool richText) {
            (void)richText;
            if (isGood) {
                qDebug() << title << ":" << msg;
            } else {
                qWarning() << title << ":" << msg;
            }
        });

        QObject::connect(vesc, &VescInterface::fwUploadStatus, []
                         (const QString &status, double progress, bool isOngoing) {
            (void)status;
            (void)isOngoing;

            static double progress_last = 0.0;
            progress *= 100.0;

            if (progress < progress_last) {
                progress_last = 0.0;
            }

            if (progress > 0.5 && (progress - progress_last) >= 1.0) {
                fprintf(stderr, "%s", QString("\rUpload progress: %1%").arg(floor(progress)).toLatin1().data());
                progress_last = progress;
            }
        });

        QObject::connect(vesc->commands(), &Commands::fileProgress, []
                         (int32_t prog, int32_t tot, double percentage, double bytesPerSec) {
            (void)prog;
            (void)tot;

            fprintf(stderr, "%s", QString("\rUpload progress: %1% (%2 kbps)").
                    arg(floor(percentage)).arg(bytesPerSec / 1024).toLatin1().data());
        });

        QTimer::singleShot(10, [&]() {
            int exitCode = 0;
            bool ok = false;
            if (vescPort.isEmpty()) {
                ok = vesc->autoconnect();
            } else {
                ok = vesc->connectSerial(vescPort);
                if (ok) {
                    ok = Utility::waitSignal(vesc, SIGNAL(fwRxChanged(bool, bool)), 1000);
                    if (!ok) {
                        qWarning() << "Could not read firmware version";
                    }
                }
            }

            if (ok) {
                qDebug() << "Connected";
                Utility::sleepWithEventLoop(100);

                if (canFwd >= 0) {
                    vesc->commands()->setSendCan(true, canFwd);
                }

                CodeLoader loader;
                loader.setVesc(vesc);

                if (eraseLisp) {
                    if (loader.lispErase(16)) {
                        qDebug() << "Lisp erase OK!";
                    } else {
                        qWarning() << "Could not erase lisp";
                        exitCode = -10;
                    }
                }

                if (!lispPath.isEmpty()) {
                    QFile f(lispPath);
                    if (f.open(QIODevice::ReadOnly)) {
                        QFileInfo fi(f);
                        VByteArray lispData = loader.lispPackImports(f.readAll(), fi.canonicalPath());
                        f.close();

                        if (!lispData.isEmpty()) {
                            bool ok = loader.lispErase(lispData.size() + 100);
                            if (ok) {
                                ok = loader.lispUpload(lispData);
                            } else {
                                qWarning() << "Could not erase lisp";
                                exitCode = -10;
                            }
                            if (ok) {
                                qDebug() << "Lisp upload OK!";
                                vesc->commands()->lispSetRunning(1);
                                Utility::sleepWithEventLoop(100);
                            } else {
                                qWarning() << "Could not upload lisp";
                                exitCode = -11;
                            }
                        } else {
                            qWarning() << "Empty or invalid lisp-file.";
                            exitCode = -12;
                        }
                    } else {
                        qWarning() << "Could not open lisp file for reading.";
                        exitCode = -13;
                    }
                }

                if (!fileForSdIn.isEmpty()) {
                    QFile f(fileForSdIn);
                    QFileInfo fi(f);
                    if (f.open(QIODevice::ReadOnly)) {
                        QFileInfo fi(f);
                        vesc->commands()->fileBlockMkdir(fileForSdOut);
                        QString target = fileForSdOut + "/" + fi.fileName();
                        if (vesc->commands()->fileBlockWrite(target.replace("//", "/"), f.readAll())) {
                            qDebug() << "Done!";
                        } else {
                            qWarning() << "Could not write file";
                            exitCode = -51;
                        }

                        f.close();
                    } else {
                        qWarning() << "Could not open file for reading.";
                        exitCode = -50;
                    }
                }

                if (isMcConf || isAppConf || isCustomConf) {
                    bool res = vesc->customConfigRxDone();
                    if (!res) {
                        res = Utility::waitSignal(vesc, SIGNAL(customConfigLoadDone()), 4000);
                    }

                    if (res) {
                        if (isMcConf) {
                            ConfigParams *p = vesc->mcConfig();
                            vesc->commands()->getMcconf();
                            res = Utility::waitSignal(p, SIGNAL(updated()), 4000);

                            if (res) {
                                if (!setMcConfPath.isEmpty()) {
                                    res = p->loadXml(setMcConfPath, "MCConfiguration");

                                    if (res) {
                                        vesc->commands()->setMcconf(false);
                                        res = Utility::waitSignal(vesc->commands(), SIGNAL(ackReceived(QString)), 4000);

                                        if (res) {
                                            qDebug() << "Wrote XML from" << setMcConfPath;
                                        } else {
                                            qWarning() << "Could not write config";
                                            exitCode = -4;
                                        }
                                    } else {
                                        qWarning() << "Could not load XML from" << setMcConfPath;
                                        exitCode = -3;
                                    }
                                } else {
                                    res = p->saveXml(getMcConfPath, "MCConfiguration");

                                    if (res) {
                                        qDebug() << "Saved XML to" << getMcConfPath;
                                    } else {
                                        qWarning() << "Could not save XML";
                                        exitCode = -3;
                                    }
                                }
                            } else {
                                qWarning() << "Could not load config";
                                exitCode = -2;
                            }
                        }

                        if (isAppConf) {
                            ConfigParams *p = vesc->appConfig();
                            vesc->commands()->getAppConf();
                            res = Utility::waitSignal(p, SIGNAL(updated()), 4000);

                            if (res) {
                                if (!setAppConfPath.isEmpty()) {
                                    res = p->loadXml(setAppConfPath, "APPConfiguration");

                                    if (res) {
                                        vesc->commands()->setAppConf();
                                        res = Utility::waitSignal(vesc->commands(), SIGNAL(ackReceived(QString)), 4000);

                                        if (res) {
                                            qDebug() << "Wrote XML from" << setAppConfPath;
                                        } else {
                                            qWarning() << "Could not write config";
                                            exitCode = -4;
                                        }
                                    } else {
                                        qWarning() << "Could not load XML from" << setAppConfPath;
                                        exitCode = -3;
                                    }
                                } else {
                                    res = p->saveXml(getAppConfPath, "APPConfiguration");

                                    if (res) {
                                        qDebug() << "Saved XML to" << getAppConfPath;
                                    } else {
                                        qWarning() << "Could not save XML";
                                        exitCode = -3;
                                    }
                                }
                            } else {
                                qWarning() << "Could not load config";
                                exitCode = -2;
                            }
                        }

                        if (isCustomConf) {
                            ConfigParams *p = vesc->customConfig(0);
                            vesc->commands()->customConfigGet(0, false);
                            res = Utility::waitSignal(p, SIGNAL(updated()), 4000);

                            if (res) {
                                if (!setCustomConfPath.isEmpty()) {
                                    res = p->loadXml(setCustomConfPath, "CustomConfiguration");

                                    if (res) {
                                        vesc->commands()->customConfigSet(0, p);
                                        res = Utility::waitSignal(vesc->commands(), SIGNAL(ackReceived(QString)), 4000);

                                        if (res) {
                                            qDebug() << "Wrote XML from" << setCustomConfPath;
                                        } else {
                                            qWarning() << "Could not write config";
                                            exitCode = -4;
                                        }
                                    } else {
                                        qWarning() << "Could not load XML from" << setCustomConfPath;
                                        exitCode = -3;
                                    }
                                } else {
                                    res = p->saveXml(getCustomConfPath, "CustomConfiguration");

                                    if (res) {
                                        qDebug() << "Saved XML to" << getCustomConfPath;
                                    } else {
                                        qWarning() << "Could not save XML";
                                        exitCode = -3;
                                    }
                                }
                            } else {
                                qWarning() << "Could not load config";
                                exitCode = -2;
                            }
                        }
                    } else {
                        qWarning() << "Could not load config";
                        exitCode = -1;
                    }
                }

                if (uploadBootloaderBuiltin) {
                    FW_RX_PARAMS params = vesc->getLastFwRxParams();
                    QString path = "";

                    switch (params.hwType) {
                    case HW_TYPE_VESC:
                        path = "://res/bootloaders/generic.bin";
                        break;

                    case HW_TYPE_VESC_BMS:
                        path = "://res/bootloaders_bms/generic.bin";
                        break;

                    case HW_TYPE_CUSTOM_MODULE:
                        QByteArray endEsp;
                        endEsp.append('\0');
                        endEsp.append('\0');
                        endEsp.append('\0');
                        endEsp.append('\0');

                        if (!params.uuid.endsWith(endEsp)) {
                            if (params.hw == "hm1") {
                                path = "://res/bootloaders_bms/generic.bin";
                            } else {
                                path = "://res/bootloaders_custom_module/stm32g431/stm32g431.bin";
                            }
                        }
                        break;
                    }

                    if (!path.isEmpty()) {
                        QFile f(path);
                        if (f.open(QIODevice::ReadOnly)) {
                            auto fwData = f.readAll();
                            qDebug() << "Erasing old bootloader...";
                            if (vesc->fwUpload(fwData, true, false, false)) {
                                fprintf(stderr, "\r\n");
                                qDebug() << "Bootloader upload OK!";
                            } else {
                                qWarning() << "Bootloader upload failed.";
                                exitCode = -20;
                            }
                        } else {
                            qWarning() << "Could not open bootloader file for reading.";
                            exitCode = -21;
                        }
                    } else {
                        qWarning() << "No included bootloader found.";
                        exitCode = -30;
                    }
                }

                if (!firmwarePath.isEmpty()) {
                    QFile f(firmwarePath);
                    if (f.open(QIODevice::ReadOnly)) {
                        auto fwData = f.readAll();
                        qDebug() << "Erasing firmware buffer...";
                        if (vesc->fwUpload(fwData, false, false, false)) {
                            fprintf(stderr, "\r\n");
                            qDebug() << "Firmware upload OK!";
                        } else {
                            qWarning() << "Firmware upload failed.";
                            exitCode = -20;
                        }
                    } else {
                        qWarning() << "Could not open firmware file for reading.";
                        exitCode = -21;
                    }
                }
            } else {
                qWarning() << "Could not connect";
                exitCode = -1;
            }

            if (!bridgeAppData) {
                qApp->exit(exitCode);
            }
        });
    } else if (useTcp) {
        if (offscreen) {
            qputenv("QT_QPA_PLATFORM", "offscreen");
        }
        app = new QCoreApplication(argc, argv);
        vesc = new VescInterface;
        vesc->fwConfig()->loadParamsXml("://res/config/fw.xml");
        Utility::configLoadLatest(vesc);

        QTimer::singleShot(10, [&]() {
            if (vesc->tcpServerStart(tcpPort)) {
                connTimer.start();
            } else {
                qCritical() << "Could not start TCP server on port" << tcpPort;
                qApp->quit();
            }
        });

        QObject::connect(vesc, &VescInterface::statusMessage, [&](QString msg, bool isGood) {
            if (isGood) {
                qDebug() << msg;
            } else  {
                qWarning() << msg;
            }
        });
    } else if (isTcpHub) {
        if (offscreen) {
            qputenv("QT_QPA_PLATFORM", "offscreen");
        }
        app = new QCoreApplication(argc, argv);
        tcpHub = new TcpHub;
        if (tcpHub->start(tcpPort)) {
            qDebug() << "TcpHub started";
        } else {
            qCritical() << "Could not start TcpHub on port" << tcpPort;
            qApp->quit();
        }
    } else if (downloadPackageArchive) {
        return 0;
    } else {
        QApplication *a = new QApplication(argc, argv);
        app = a;

        addFonts();

        // Style
        qApp->setStyleSheet("QListView::item::selected {background: qlineargradient(x1: 1.0, y1: 0.0, x2: 0, y2: 0, stop: 0 " +
                            Utility::getAppHexColor("lightAccent") +
                            ", stop: 0.4 " + Utility::getAppHexColor("darkAccent") + ");" +
                            " border: none;} ");
        QStyle *myStyle = new Style_tweaks("Fusion");
        a->setStyle(myStyle);

        if (isDark) {
            QPalette darkPalette;
            //QPalette::Inactive
            darkPalette.setColor(QPalette::Window,Utility::getAppQColor("darkBackground"));
            darkPalette.setColor(QPalette::WindowText,Utility::getAppQColor("lightText"));
            darkPalette.setColor(QPalette::Disabled,QPalette::WindowText,Utility::getAppQColor("disabledText"));
            darkPalette.setColor(QPalette::Base,Utility::getAppQColor("normalBackground"));
            darkPalette.setColor(QPalette::AlternateBase,Utility::getAppQColor("lightBackground"));
            darkPalette.setColor(QPalette::ToolTipBase,Utility::getAppQColor("lightestBackground"));
            darkPalette.setColor(QPalette::ToolTipText,Utility::getAppQColor("lightText"));
            darkPalette.setColor(QPalette::Text,Utility::getAppQColor("lightText"));
            darkPalette.setColor(QPalette::Disabled,QPalette::Text,Utility::getAppQColor("disabledText"));
            darkPalette.setColor(QPalette::Dark,QColor(35,35,35));
            darkPalette.setColor(QPalette::Shadow,QColor(20,20,20));
            darkPalette.setColor(QPalette::Button,Utility::getAppQColor("normalBackground"));
            darkPalette.setColor(QPalette::ButtonText,Utility::getAppQColor("lightText"));
            darkPalette.setColor(QPalette::Disabled,QPalette::ButtonText,Utility::getAppQColor("disabledText"));
            darkPalette.setColor(QPalette::Disabled,QPalette::Highlight,Utility::getAppQColor("lightestBackground"));
            darkPalette.setColor(QPalette::HighlightedText,Utility::getAppQColor("white"));
            darkPalette.setColor(QPalette::Inactive,QPalette::Highlight,Utility::getAppQColor("midAccent"));
            darkPalette.setColor(QPalette::Active,QPalette::Highlight,Utility::getAppQColor("darkAccent"));
            darkPalette.setColor(QPalette::Disabled,QPalette::HighlightedText,Utility::getAppQColor("disabledText"));
            darkPalette.setColor(QPalette::Link, QColor(150,150,255));
            darkPalette.setColor(QPalette::LinkVisited, QColor(220,150,255));
            qApp->setPalette(darkPalette);
            qApp->setStyleSheet(
                        "QTabBar::tab:selected, QTabBar::tab:hover {"
                        "    background: #3d3d3d;"
                        "    color: #eeeeee;"
                        "}"
                        "QTabBar::tab:!selected {"
                        "    background: #272727;"
                        "    color: #a5a5a5;"
                        "}"
                        );
        } else {
            QPalette lightPalette = qApp->style()->standardPalette();
            lightPalette.setColor(QPalette::Inactive,QPalette::Highlight,Utility::getAppQColor("darkAccent"));
            lightPalette.setColor(QPalette::Active,QPalette::Highlight,Utility::getAppQColor("midAccent"));
            qApp->setPalette(lightPalette);
        }

        // Register this to not stop on the import statement when reusing components
        // from the mobile UI. In the mobile UI these are provided as singletons, whereas
        // in the desktop GUI they are provided as context properties.
        qmlRegisterType<VescInterface>("Vedder.vesc.vescinterface", 1, 0, "VescIf2");
        qmlRegisterType<Utility>("Vedder.vesc.utility", 1, 0, "Utility2");

        if (!loadQml.isEmpty() || loadQmlVesc) {
            vesc = new VescInterface;
            vesc->fwConfig()->loadParamsXml("://res/config/fw.xml");
            Utility::configLoadLatest(vesc);

            if (loadQmlVesc) {
                QObject::connect(vesc, &VescInterface::qmlLoadDone, [&]() {
                    if (vesc->qmlAppLoaded()) {
                        qmlStr = vesc->qmlApp();
                    } else if (vesc->qmlHwLoaded()) {
                        qmlStr = vesc->qmlHw();
                    } else {
                        qmlStr = "";
                    }

                    qmlUi->clearQmlCache();

                    QTimer::singleShot(10, [&]() {
                        qmlUi->emitReloadCustomGui("qrc:/res/qml/DynamicLoader.qml");
                    });

                    QTimer::singleShot(1000, [&]() {
                        qmlUi->emitReloadQml(qmlStr);
                    });
                });

                connTimer.start();
            } else {
                QFile qmlFile(loadQml);
                if (qmlFile.exists() && qmlFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QFileInfo fi(loadQml);

                    qmlStr = QString::fromUtf8(qmlFile.readAll());
                    qmlFile.close();

                    qmlStr.prepend("import \"qrc:/mobile\";");
                    qmlStr.prepend("import Vedder.vesc.vescinterface 1.0;");
                    qmlStr.prepend("import \"file:/" + fi.canonicalPath() + "\";");

                    QTimer::singleShot(10, [&]() {
                        qmlUi->emitReloadCustomGui("qrc:/res/qml/DynamicLoader.qml");
                    });

                    QTimer::singleShot(1000, [&]() {
                        qmlUi->emitReloadQml(qmlStr);
                        if (qmlAutoConn) {
                            connTimer.start();
                        }
                    });
                } else {
                    qCritical() << "Could not open" << loadQml;
                    delete app;
                    delete vesc;
                    return -1;
                }
            }

            qmlUi = new QmlUi;
            qmlUi->startCustomGui(vesc, "qrc:/res/qml/MainLoader.qml",
                                  qmlWindowSize.width(), qmlWindowSize.height());

            if (qmlFullscreen) {
                qmlUi->emitToggleFullscreen();
            }

            if (qmlOtherScreen) {
                qmlUi->emitMoveToOtherScreen();
            }

            qmlUi->emitRotateScreen(qmlRot);

            QObject::connect(vesc, &VescInterface::statusMessage, [&](QString msg, bool isGood) {
                if (isGood) {
                    qDebug() << msg;
                } else  {
                    qWarning() << msg;
                }
            });
        } else if (useMobileUi) {
            qmlUi = new QmlUi;
            qmlUi->startQmlUi();
        } else if (useBoardSetupWindow){
            bw = new BoardSetupWindow;
            bw->show();
        } else {
            QPixmapCache::setCacheLimit(256000);
            w = new MainWindow;
            w->show();
        }
    }
#endif
#ifdef Q_OS_IOS
    SetIosParams();
#endif

    int res = app->exec();

#ifdef USE_MOBILE
    delete qml;
#else
    if (vesc) {
        delete vesc;
    }

    if (tcpHub) {
        delete tcpHub;
    }

    if (w) {
        delete w;
    }

    if (qmlUi) {
        delete qmlUi;
    }
#endif

    delete app;

    if (m_debug_msg_file.isOpen()) {
        m_debug_msg_file.close();
    }

    return res;
}
