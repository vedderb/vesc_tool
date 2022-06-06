/*
    Copyright 2016 - 2021 Benjamin Vedder	benjamin@vedder.se

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
#include "mobile/qmlui.h"
#include "mobile/fwhelper.h"
#include "mobile/vesc3ditem.h"
#include "mobile/logwriter.h"
#include "mobile/logreader.h"
#include "tcpserversimple.h"
#include "pages/pagemotorcomparison.h"

#include <QApplication>
#include <QStyleFactory>
#include <QSettings>
#include <QDesktopWidget>
#include <QFontDatabase>

#ifdef Q_OS_IOS
#include "ios/src/setIosParameters.h"
#endif

#ifdef Q_OS_LINUX
#include <signal.h>
#endif

#ifndef USE_MOBILE

#include <QProxyStyle>

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
    qDebug() << "--tcpServer [port] : Autoconnect to VESC and start TCP server on [port]";
    qDebug() << "--loadQml [file] : Load QML UI from file instead of the regular VESC Tool UI";
    qDebug() << "--loadQmlVesc : Load QML UI from the connected VESC instead of the regular VESC Tool UI";
    qDebug() << "--qmlAutoConn : Autoconnect over USB before loading the QML UI";
    qDebug() << "--qmlFullscreen : Run QML UI in fullscreen mode";
    qDebug() << "--qmlOtherScreen : Run QML UI on other screen";
    qDebug() << "--qmlRotation [deg] : Rotate screen by deg degrees";
    qDebug() << "--retryConn : Keep trying to reconnect to the VESC when the connection fails";
    qDebug() << "--useMobileUi : Start the mobile UI instead of the full desktop UI";

}

#ifdef Q_OS_LINUX
static void m_cleanup(int sig)
{
    (void)sig;
    qApp->quit();
}
#endif
#endif

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
#endif
    qmlRegisterType<Commands>("Vedder.vesc.commands", 1, 0, "Commands");
    qmlRegisterType<ConfigParams>("Vedder.vesc.configparams", 1, 0, "ConfigParams");
    qmlRegisterType<FwHelper>("Vedder.vesc.fwhelper", 1, 0, "FwHelper");
    qmlRegisterType<TcpServerSimple>("Vedder.vesc.tcpserversimple", 1, 0, "TcpServerSimple");
    qmlRegisterType<UdpServerSimple>("Vedder.vesc.udpserversimple", 1, 0, "UdpServerSimple");
    qmlRegisterType<Vesc3dItem>("Vedder.vesc.vesc3ditem", 1, 0, "Vesc3dItem");
    qmlRegisterType<LogWriter>("Vedder.vesc.logwriter", 1, 0, "LogWriter");
    qmlRegisterType<LogReader>("Vedder.vesc.logreader", 1, 0, "LogReader");

    qRegisterMetaType<MCCONF_TEMP>();
    qRegisterMetaType<MC_VALUES>();
    qRegisterMetaType<BMS_VALUES>();
    qRegisterMetaType<FW_RX_PARAMS>();
    qRegisterMetaType<PSW_STATUS>();
    qRegisterMetaType<IO_BOARD_VALUES>();
    qRegisterMetaType<MotorData>();

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
    double qmlRot = 0.0;

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
            found = true;
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
    MainWindow *w = nullptr;
    QmlUi *qmlUi = nullptr;
    QString qmlStr;

    QTimer connTimer;
    connTimer.setInterval(1000);
    QObject::connect(&connTimer, &QTimer::timeout, [&]() {
        if (!vesc->isPortConnected()) {
            bool ok = vesc->autoconnect();

            if (ok) {
                qDebug() << "Connected";
            } else {
                qDebug() << "Could not autoconnect";

                if (!retryConn) {
                    qApp->quit();
                }
            }
        }
    });

    if (useTcp) {
        qputenv("QT_QPA_PLATFORM", "offscreen");

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
        qmlRegisterType<VescInterface>("Vedder.vesc.utility", 1, 0, "Utility2");

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
            qmlUi->startCustomGui(vesc);

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
        } else {
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

    if (w) {
        delete w;
    }

    if (qmlUi) {
        delete qmlUi;
    }
#endif

    delete app;

    return res;
}
