/*
    Copyright 2019 - 2023 Benjamin Vedder	benjamin@vedder.se

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

#include "imagewidgetdisp.h"
#include <QPainter>
#include <QPaintEvent>
#include <QDebug>

ImageWidgetDisp::ImageWidgetDisp(QWidget *parent) : QWidget(parent)
{

}

void ImageWidgetDisp::paintEvent(QPaintEvent *event)
{
    (void)event;

    if (!mPixmap.isNull()) {
        QPainter painter(this);
        painter.setRenderHints(QPainter::Antialiasing, false);
        painter.fillRect(rect(), Qt::black);

        int pw = mPixmap.width();
        int ph = mPixmap.height();
        int w = width();
        int h = height();

        if (((double)pw / (double)ph) > ((double)w / (double)h)) {
            painter.drawPixmap(0, (h - (ph * w) / pw) / 2, w, (ph * w) / pw, mPixmap);
        } else {
            painter.drawPixmap((w - (pw * h) / ph) / 2, 0, (pw * h) / ph, h, mPixmap);
        }
    }
}

QPixmap ImageWidgetDisp::pixmap() const
{
    return mPixmap;
}

void ImageWidgetDisp::setPixmap(const QPixmap &pixmap)
{
    mPixmap = pixmap;
    update();
}
