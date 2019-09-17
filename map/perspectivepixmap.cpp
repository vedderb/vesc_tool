/*
    Copyright 2016 Benjamin Vedder	benjamin@vedder.se

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

#include "perspectivepixmap.h"
#include <cmath>

PerspectivePixmap::PerspectivePixmap()
{

}

void PerspectivePixmap::setPixmap(QPixmap pixmap)
{
    mPixMap = pixmap;
}

void PerspectivePixmap::setXOffset(double offset)
{
    mXOffset = offset;
}

void PerspectivePixmap::setYOffset(double offset)
{
    mYOffset = offset;
}

void PerspectivePixmap::setScale(double scale)
{
    mScale = scale;
}

QPixmap PerspectivePixmap::getPixmap()
{
    return mPixMap;
}

double PerspectivePixmap::getXOffset()
{
    return mXOffset;
}

double PerspectivePixmap::getYOffset()
{
    return mYOffset;
}

double PerspectivePixmap::getScale()
{
    return mScale;
}

void PerspectivePixmap::drawUsingPainter(QPainter &painter)
{
    if (!mPixMap.isNull()) {
        qreal opOld = painter.opacity();
        QTransform transOld = painter.transform();
        QTransform trans = painter.transform();
        trans.scale(1, -1);
        painter.setTransform(trans);
        painter.setOpacity(0.8);
        painter.drawPixmap(-mXOffset / mScale, -mYOffset / mScale,
                           mPixMap.width() / mScale, mPixMap.height() / mScale, mPixMap);
        painter.setOpacity(opOld);
        painter.setTransform(transOld);
    }
}
