/*
    Copyright 2020 Benjamin Vedder	benjamin@vedder.se

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

#include "batttempplot.h"
#include "utility.h"
#include <QFont>
#include <QPainter>
#include <QPaintEvent>
#include <QDebug>

BattTempPlot::BattTempPlot(QWidget *parent) : QWidget(parent)
{
    mBatt = BATT_NULL;
}

void BattTempPlot::setValues(BMS_VALUES &val)
{
    mVal = val;
    update();
}

void BattTempPlot::unload()
{
    mBattImg = QPixmap();
    mBatt = BATT_NULL;
    update();
}

void BattTempPlot::load12s7p()
{
    if (mBatt != BATT_12S7P) {
        mBattImg.load("://res/images/12s7p_pcb.png");
        mBatt = BATT_12S7P;
    }
}

void BattTempPlot::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    // Paint begins here
    painter.fillRect(event->rect(), QBrush(Qt::transparent));

    double mWMap = width();
    double mHMap = height();

    if (!mBattImg.isNull()) {
        mWMap = mBattImg.width();
        mHMap = mBattImg.height();
    }

    double xs = event->rect().width();
    double ys = event->rect().height();
    QTransform drawTrans;

    if (ys > (xs / (mWMap / mHMap))) {
        ys = xs / (mWMap / mHMap);
    } else {
        xs = ys * (mWMap / mHMap);
    }

    drawTrans.scale(xs / mWMap, ys / mHMap);
    painter.setTransform(drawTrans);

    if (mBatt == BATT_12S7P) {
        painter.drawPixmap(0, 0, int(mWMap), int(mHMap), mBattImg);

        QFont font;
        font.setFamily("DejaVu Sans Mono");
        font.setPointSize(72);
        painter.setFont(font);
        painter.setPen(Utility::getAppQColor("green"));

        if (mVal.temps.size() >= 6) {
            painter.drawText(2985, 1840, QString("%1 °C").arg(mVal.temp_ic, 0, 'f', 1, QChar('0')));
            painter.drawText(2985, 1467, QString("%1 °C").arg(mVal.temps.at(0), 0, 'f', 1, QChar('0')));
            painter.drawText(30, 890, QString("%1 °C").arg(mVal.temps.at(1), 0, 'f', 1, QChar('0')));
            painter.drawText(2985, 260, QString("%1 °C").arg(mVal.temps.at(2), 0, 'f', 1, QChar('0')));
            painter.drawText(2985, 600, QString("%1 °C").arg(mVal.temps.at(3), 0, 'f', 1, QChar('0')));
            painter.drawText(2985, 945, QString("%1 °C").arg(mVal.temp_hum_sensor, 0, 'f', 1, QChar('0')));
            painter.drawText(2985, 1050, QString("%1 %RH").arg(mVal.humidity, 0, 'f', 1, QChar('0')));
            painter.drawText(30, 1506, QString("%1 °C").arg(mVal.temps.at(5), 0, 'f', 1, QChar('0')));
            painter.drawText(30, 1794, QString("%1 °C").arg(mVal.temps.at(4), 0, 'f', 1, QChar('0')));
        }
    } else {
        painter.drawText(10, height() / 2, "No battery loaded...");
    }
}
