/*
    Copyright 2021 Benjamin Vedder	benjamin@vedder.se

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

#ifndef VESC3DITEM_H
#define VESC3DITEM_H

#include <QObject>
#include <QtQuick>

#include "widgets/vesc3dview.h"

class Vesc3dItem : public QQuickPaintedItem
{
    Q_OBJECT

public:
    explicit Vesc3dItem(QQuickItem *parent = nullptr);

    Q_INVOKABLE void setRotation(double roll, double pitch, double yaw);
    Q_INVOKABLE void setRotationQuat(double q0, double q1, double q2, double q3);

    void paint(QPainter *painter) override;

signals:

private slots:


private:
    Vesc3DView mVesc3d;
    QImage mLastCornerImg;

    void updateImage();

};

#endif // VESC3DITEM_H
