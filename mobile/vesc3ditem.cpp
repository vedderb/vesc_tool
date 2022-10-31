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
#include <QApplication>



Vesc3dItem::Vesc3dItem(QQuickItem *parent) : QQuickPaintedItem(parent)
{
    mVesc3d.setBgColor(Utility::getAppQColor("normalBackground").redF(),
                       Utility::getAppQColor("normalBackground").greenF(),
                       Utility::getAppQColor("normalBackground").blueF(), 1.0);
    setRenderTarget(QQuickPaintedItem::FramebufferObject);
    setAntialiasing(true);
    setOpaquePainting(true);
}

void Vesc3dItem::setRotation(double roll, double pitch, double yaw)
{
    if(abs(roll - mRoll) > 0.005 || abs(pitch - mPitch) > 0.005 ||  abs(yaw - mYaw) > 0.005) {
        mVesc3d.setRollPitchYaw(roll * 180.0 / M_PI, pitch * 180.0 / M_PI, yaw * 180.0 / M_PI);
        updateImage();
        update();
        mRoll = roll; mPitch = pitch; mYaw = yaw;
    }
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
    if (qApp->applicationState() == Qt::ApplicationState::ApplicationActive) {
        double scale = 1.0;

        if (mVesc3d.size() != size().toSize()) {
            mVesc3d.resize(size().toSize() * scale);
            mVesc3d.render(&mLastCornerImg, QPoint(), QRegion(), QWidget::RenderFlags());
        }

        mLastCornerImg = mVesc3d.grabFramebuffer();
    }
}
