/*
    Copyright 2016 - 2017 Benjamin Vedder	benjamin@vedder.se

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

#include "aspectimglabel.h"

AspectImgLabel::AspectImgLabel(Qt::Orientation orientation, QWidget *parent) :
    QLabel(parent), mOrientation(orientation)
{

}

void AspectImgLabel::resizeEvent(QResizeEvent *event)
{
    QLabel::resizeEvent(event);

    const QPixmap *pix = pixmap();

    if (pix) {
        int wLabel = width();
        int hLabel = height();
        int wImg = pix->width();
        int hImg = pix->height();

        if (mOrientation == Qt::Horizontal) {
            setMaximumHeight((wLabel * hImg) / wImg);
        } else if (hLabel != hImg) {
            setMaximumWidth((hLabel * wImg) / hImg);
        }
    }
}
