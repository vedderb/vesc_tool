/*
    Copyright 2018 Benjamin Vedder	benjamin@vedder.se

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    */

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "vescinterface.h"
#include "utility.h"

QObject *vescinterface_singletontype_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    (void)engine;
    (void)scriptEngine;

    VescInterface *vesc = new VescInterface();
	vesc->fwConfig()->loadParamsXml("://res/config/fw.xml");
    Utility::configLoadLatest(vesc);

    return vesc;
}

QObject *utility_singletontype_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    (void)engine;
    (void)scriptEngine;

    Utility *util = new Utility();

    return util;
}

int main(int argc, char *argv[])
{
    // Settings
    QCoreApplication::setOrganizationName("VESC");
    QCoreApplication::setOrganizationDomain("vesc-project.com");
    QCoreApplication::setApplicationName("VESC Application");

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    
    qmlRegisterSingletonType<VescInterface>("Vedder.vesc.vescinterface", 1, 0, "VescIf", vescinterface_singletontype_provider);
    qmlRegisterSingletonType<Utility>("Vedder.vesc.utility", 1, 0, "Utility", utility_singletontype_provider);
#ifdef HAS_BLUETOOTH
    qmlRegisterType<BleUart>("Vedder.vesc.bleuart", 1, 0, "BleUart");
#endif
    qmlRegisterType<Commands>("Vedder.vesc.commands", 1, 0, "Commands");
    qmlRegisterType<ConfigParams>("Vedder.vesc.configparams", 1, 0, "ConfigParams");
    
    engine.load(QUrl(QLatin1String("qrc:/res/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
