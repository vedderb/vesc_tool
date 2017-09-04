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

#include "displaybar.h"
#include <QPainter>
#include <cmath>

DisplayBar::DisplayBar(QWidget *parent) : QWidget(parent)
{
    mName = "Current";
    mRange = 60.0;
    mVal = 0.0;
    mUnit = " A";
    mDecimals = 2;
}

void DisplayBar::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();
    double f_disp = 0.3;
    double f_val = 1.0 - f_disp;

    painter.fillRect(event->rect(), Qt::transparent);
    painter.setBrush(Qt::black);
    painter.drawRoundedRect(event->rect(), 5.0, 5.0);

    painter.setBrush(QBrush(Qt::red));
    painter.fillRect(w / 2 - 1, 0, 2, h, Qt::darkRed);
    painter.fillRect(0, h * f_disp - 1, w, 2, Qt::darkGreen);

    QPen pen;
    QFont font;

    // Name
    pen.setColor(Qt::white);
    font.setFamily("Monospace");
    font.setBold(true);
    font.setPixelSize(h * f_val - 2);
    painter.setPen(pen);
    painter.setFont(font);
    painter.drawText(QRect(0, h * f_disp + 1, w / 2 - 2, h * f_val - 1),
                     Qt::AlignCenter, mName);

    // Value
    pen.setColor(Qt::white);
    font.setFamily("Monospace");
    font.setBold(true);
    font.setPixelSize(h * f_val - 2);
    painter.setPen(pen);
    painter.setFont(font);
    QString str = QString("%1%2").arg(mVal, 0, 'f', mDecimals).arg(mUnit);
    painter.drawText(QRect(w / 2 + 1, h * f_disp + 1, w / 2 - 2, h * f_val - 1),
                     Qt::AlignCenter, str);

    double xsp = (double)w / 2.0 + 1.0;
    double xsm = (double)w / 2.0 - 1.0;
    double valw = (mVal / mRange) * ((double)w / 2.0 - 2.0);
    double valh = (double)h * f_disp - 2.0;

    if (fabs(valw) > 0.1) {
        if (valw >= 0.0) {
            painter.setBrush(Qt::green);
            painter.drawRect(xsp, 1, valw, valh);
        } else {
            painter.setBrush(Qt::red);
            painter.drawRect(xsm + valw, 1, -valw, valh);
        }
    }
}

int DisplayBar::decimals() const
{
    return mDecimals;
}

void DisplayBar::setDecimals(int decimals)
{
    mDecimals = decimals;
}

QString DisplayBar::unit() const
{
    return mUnit;
}

void DisplayBar::setUnit(const QString &unit)
{
    mUnit = unit;
}

double DisplayBar::val() const
{
    return mVal;
}

void DisplayBar::setVal(double val)
{
    mVal = val;
    update();
}

double DisplayBar::range() const
{
    return mRange;
}

void DisplayBar::setRange(double range)
{
    mRange = range;
    update();
}

QString DisplayBar::name() const
{
    return mName;
}

void DisplayBar::setName(const QString &name)
{
    mName = name;
    update();
}
