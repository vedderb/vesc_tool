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

#include "vesc3ditem.h"
#include <QtDebug>
#include "utility.h"

Vesc3dItem::Vesc3dItem(QQuickItem *parent) : QQuickPaintedItem(parent)
{
    mVesc3d.setBgColor(Utility::getAppQColor("normalBackground").redF(),
                       Utility::getAppQColor("normalBackground").greenF(),
                       Utility::getAppQColor("normalBackground").blueF(), 1.0);
}

void Vesc3dItem::setRotation(double roll, double pitch, double yaw)
{
    mVesc3d.setRollPitchYaw(roll * 180.0 / M_PI, pitch * 180.0 / M_PI, yaw * 180.0 / M_PI);
    updateImage();
    update();
}

void Vesc3dItem::setRotationQuat(double q0, double q1, double q2, double q3)
{
    mVesc3d.setQuanternions(q0, q1, q2, q3);
    updateImage();
    update();
}

void Vesc3dItem::paint(QPainter *painter)
{
    painter->fillRect(boundingRect(), Qt::transparent);
    if (!mLastCornerImg.isNull()) {
        painter->drawImage(boundingRect(), mLastCornerImg, mLastCornerImg.rect());
    }
}

void Vesc3dItem::updateImage()
{
    if (mVesc3d.size() != size().toSize()) {
        mVesc3d.resize(size().toSize()*1.5);
        mLastCornerImg = mVesc3d.grabFramebuffer();
        // The render seems to be needed after a resize
        mVesc3d.render(&mLastCornerImg, QPoint(), QRegion(), nullptr);
    }

    mLastCornerImg = mVesc3d.grabFramebuffer();
}
