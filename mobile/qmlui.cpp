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

#include <QQuickStyle>
#include <QApplication>
#include <QQuickWindow>
#include <QQmlContext>

VescInterface *QmlUi::mVesc = nullptr;

QmlUi::QmlUi(QObject *parent) : QObject(parent)
{
    mEngine = nullptr;
#ifdef DEBUG_BUILD
    qApp->installEventFilter(this);
#endif
}

bool QmlUi::startQmlUi()
{
    if (!mEngine) {
        mEngine = new QQmlApplicationEngine(this);
    }

    qmlRegisterSingletonType<VescInterface>("Vedder.vesc.vescinterface", 1, 0, "VescIf", vescinterface_singletontype_provider);
    qmlRegisterSingletonType<Utility>("Vedder.vesc.utility", 1, 0, "Utility", utility_singletontype_provider);

    mEngine->load(QUrl(QLatin1String("qrc:/mobile/main.qml")));
    return !mEngine->rootObjects().isEmpty();
}

bool QmlUi::eventFilter(QObject *object, QEvent *e)
{
    (void)object;

    if (e->type() == QEvent::KeyPress || e->type() == QEvent::KeyRelease) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);
        bool isPress = e->type() == QEvent::KeyPress;

        if (isPress && !keyEvent->isAutoRepeat()) {
            switch(keyEvent->key()) {
            case Qt::Key_F5:
                delete mEngine;
                mEngine = new QQmlApplicationEngine(this);
//                mEngine->load(QUrl(QLatin1String("mobile/main.qml")));
                mEngine->load(QUrl(QLatin1String("/home/benjamin/Nextcloud/Dokument/QtCreator/vesc_tool/mobile/main.qml")));
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
    if (mEngine) {
        QObject *rootObject = mEngine->rootObjects().first();
        QQuickWindow *window = qobject_cast<QQuickWindow *>(rootObject);
        if (window) {
            window->setVisible(visible);
        }
    }
}

void QmlUi::startCustomGui(VescInterface *vesc, QString qmlFile, int width, int height)
{
    if (mEngine) {
        mEngine->deleteLater();
        mEngine = nullptr;
    }

    mEngine = new QQmlApplicationEngine(this);
    mEngine->rootContext()->setContextProperty("QmlUi", this);
    mEngine->rootContext()->setContextProperty("VescIf", vesc);
    mEngine->rootContext()->setContextProperty("Utility", &mUtil);
    mEngine->load(QUrl(qmlFile));

    auto objs = mEngine->rootObjects();
    if (!objs.isEmpty()) {
        if (width > 0) {
            objs.first()->setProperty("width", width);
        }

        if (height > 0) {
            objs.first()->setProperty("height", height);
        }
    }

    if (!mImportPathList.isEmpty()) {
        mEngine->setImportPathList(mImportPathList);
    }
}

void QmlUi::stopCustomGui()
{
    if (mEngine) {
        mEngine->deleteLater();
        mEngine = nullptr;
    }
}

bool QmlUi::isCustomGuiRunning()
{
    return mEngine != nullptr;
}

void QmlUi::emitReloadCustomGui(QString fileName)
{
    emit reloadFile(fileName);
}

void QmlUi::emitReloadQml(QString str)
{
    emit reloadQml(str);
}

void QmlUi::emitToggleFullscreen()
{
    emit toggleFullscreen();
}

void QmlUi::emitMoveToOtherScreen()
{
    emit moveToOtherScreen();
}

void QmlUi::emitMoveToFirstScreen()
{
    emit moveToFirstScreen();
}

void QmlUi::emitRotateScreen(double rot)
{
    emit rotateScreen(rot);
}

void QmlUi::clearQmlCache()
{
    if (mEngine) {
        mEngine->clearComponentCache();
    }
}

void QmlUi::setImportPathList(QStringList paths)
{
    mImportPathList = paths;

    if (mEngine) {
        mEngine->setImportPathList(paths);
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
    vesc->fwConfig()->loadParamsXml("://res/config/fw.xml");
    Utility::configLoadLatest(vesc);

    return vesc;
}

QObject *QmlUi::utility_singletontype_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    (void)engine;
    (void)scriptEngine;

    Utility *util = new Utility();

    return util;
}
