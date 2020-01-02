/*
    Copyright 2017 Benjamin Vedder	benjamin@vedder.se

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

#ifndef QMLUI_H
#define QMLUI_H

#include <QObject>
#include <QQmlApplicationEngine>

#include "vescinterface.h"
#include "utility.h"

class QmlUi : public QObject
{
    Q_OBJECT
public:
    explicit QmlUi(QObject *parent = nullptr);
    bool startQmlUi();
    bool eventFilter(QObject *object, QEvent *e);
    void setVisible(bool visible);

    static VescInterface *vesc();

signals:

public slots:

private:
    QQmlApplicationEngine *mEngine;

    static VescInterface *mVesc;
    static QObject *vescinterface_singletontype_provider(QQmlEngine *engine, QJSEngine *scriptEngine);
    static QObject *utility_singletontype_provider(QQmlEngine *engine, QJSEngine *scriptEngine);

};

#endif // QMLUI_H
