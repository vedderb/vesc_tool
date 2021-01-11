/*
    Copyright 2016 - 2019 Benjamin Vedder	benjamin@vedder.se

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

#include <QApplication>
#include <QStyleFactory>
#include <QSettings>
#include <QDesktopWidget>
#include <QFontDatabase>

#ifdef Q_OS_LINUX
#include <signal.h>
#endif

#ifndef USE_MOBILE
static void showHelp()
{
    qDebug() << "Arguments";
    qDebug() << "-h, --help : Show help text";
    qDebug() << "--tcpServer [port] : Autoconnect to VESC and start TCP server on [port]";
}

#ifdef Q_OS_LINUX
static void m_cleanup(int sig)
{
    (void)sig;
    qApp->quit();
}
#endif
#endif

int main(int argc, char *argv[])
{
    // Settings
    QCoreApplication::setOrganizationName("VESC");
    QCoreApplication::setOrganizationDomain("vesc-project.com");
    QCoreApplication::setApplicationName("VESC Tool");

    // DPI settings
    // TODO: http://www.qcustomplot.com/index.php/support/forum/1344

    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

#ifdef USE_MOBILE
#ifndef DEBUG_BUILD
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
#else
    QCoreApplication::setAttribute(Qt::AA_Use96Dpi);

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
    int tcpPort = 65102;

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

    QSettings set;
    bool scaleAuto = true;
    double scale = 1.0;

    if (set.contains("app_scale_auto")) {
        scaleAuto = set.value("app_scale_auto").toBool();
    } else {
        set.setValue("app_scale_auto", scaleAuto);
    }

    if (scaleAuto) {
        QApplication tmp(argc, argv);
        QRect rec = tmp.desktop()->screenGeometry();
        int height = rec.height();
        int width = rec.width();
        double ptFont = tmp.font().pointSizeF();
        if (ptFont < 0.0) {
            ptFont = tmp.font().pixelSize();
        }

        if (width > 3000 && height > 1700) {
            scale = 1.5;
        } else {
            if (ptFont > 11.0) {
                scale = ptFont / 11.0;
            }
        }

        set.setValue("app_scale_factor", scale);
    } else if (set.contains("app_scale_factor")) {
        scale = set.value("app_scale_factor").toDouble();
    }

    set.setValue("app_scale_factor", scale);

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

    // Fonts
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

    qApp->setFont(QFont("Roboto", 12));

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

    if (useTcp) {
        app = new QCoreApplication(argc, argv);

        vesc = new VescInterface;
        vesc->fwConfig()->loadParamsXml("://res/config/fw.xml");
        Utility::configLoadLatest(vesc);

        QTimer::singleShot(1000, [&]() {
            bool ok = vesc->autoconnect();

            if (ok) {
                ok = vesc->tcpServerStart(tcpPort);

                if (!ok) {
                    qCritical() << "Could not start TCP server on port" << tcpPort;
                    qApp->quit();
                }
            } else {
                qCritical() << "Could not autoconnect to VESC";
                qApp->quit();
            }
        });

        QObject::connect(vesc, &VescInterface::statusMessage, [](QString msg, bool isGood) {
            if (isGood) {
                qDebug() << msg;
            } else  {
                qWarning() << msg;
                qWarning() << "Closing...";
                qApp->quit();
            }
        });
    } else {
        QApplication *a = new QApplication(argc, argv);
        app = a;

        // Fonts
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

        qApp->setFont(QFont("Roboto", 12));

        // Style
        a->setStyleSheet("");
        a->setStyle(QStyleFactory::create("Fusion"));

        w = new MainWindow;
        w->show();
    }
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
#endif

    delete app;

    return res;
}
