/*
    Copyright 2017 - 2019 Benjamin Vedder	benjamin@vedder.se

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

#include "qmlui.h"
#include "fwhelper.h"

#include <QQuickStyle>
#include <QApplication>
#include <QQuickWindow>

VescInterface *QmlUi::mVesc = nullptr;

QmlUi::QmlUi(QObject *parent) : QObject(parent)
{
    mEngine = new QQmlApplicationEngine(this);
#ifdef DEBUG_BUILD
    qApp->installEventFilter(this);
#endif
}

bool QmlUi::startQmlUi()
{
    qmlRegisterSingletonType<VescInterface>("Vedder.vesc.vescinterface", 1, 0, "VescIf", vescinterface_singletontype_provider);
    qmlRegisterSingletonType<Utility>("Vedder.vesc.utility", 1, 0, "Utility", utility_singletontype_provider);
#ifdef HAS_BLUETOOTH
    qmlRegisterType<BleUart>("Vedder.vesc.bleuart", 1, 0, "BleUart");
#endif
    qmlRegisterType<Commands>("Vedder.vesc.commands", 1, 0, "Commands");
    qmlRegisterType<ConfigParams>("Vedder.vesc.configparams", 1, 0, "ConfigParams");
    qmlRegisterType<FwHelper>("Vedder.vesc.fwhelper", 1, 0, "FwHelper");

    mEngine->load(QUrl(QLatin1String("qrc:/mobile/main.qml")));
    return !mEngine->rootObjects().isEmpty();
}

bool QmlUi::eventFilter(QObject *object, QEvent *e)
{
    Q_UNUSED(object);

    if (e->type() == QEvent::KeyPress || e->type() == QEvent::KeyRelease) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);
        bool isPress = e->type() == QEvent::KeyPress;

        if (isPress && !keyEvent->isAutoRepeat()) {
            switch(keyEvent->key()) {
            case Qt::Key_F5:
                delete mEngine;
                mEngine = new QQmlApplicationEngine(this);
                mEngine->load(QUrl(QLatin1String("mobile/main.qml")));
                return true;

            default:
                break;
            }
        }
    }

    return false;
}

void QmlUi::setVisible(bool visible)
{
    QObject *rootObject = mEngine->rootObjects().first();
    QQuickWindow *window = qobject_cast<QQuickWindow *>(rootObject);
    if (window) {
        window->setVisible(visible);
    }
}

VescInterface *QmlUi::vesc()
{
    return mVesc;
}

QObject *QmlUi::vescinterface_singletontype_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    (void)engine;
    (void)scriptEngine;

    VescInterface *vesc = new VescInterface();
    mVesc = vesc;

    vesc->mcConfig()->loadParamsXml("://res/parameters_mcconf.xml");
    vesc->appConfig()->loadParamsXml("://res/parameters_appconf.xml");
    vesc->infoConfig()->loadParamsXml("://res/info.xml");

    return vesc;
}

QObject *QmlUi::utility_singletontype_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    (void)engine;
    (void)scriptEngine;

    Utility *util = new Utility();

    return util;
}
