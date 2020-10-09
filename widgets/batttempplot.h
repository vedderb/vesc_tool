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

#ifndef BATTTEMPPLOT_H
#define BATTTEMPPLOT_H

#include <QWidget>
#include <QPixmap>
#include "datatypes.h"

class BattTempPlot : public QWidget
{
    Q_OBJECT
public:
    explicit BattTempPlot(QWidget *parent = nullptr);
    void setValues(BMS_VALUES &val);
    void unload();
    void load12s7p();

signals:

public slots:

protected:
    void paintEvent(QPaintEvent *event);

private:
    typedef enum {
        BATT_NULL = 0,
        BATT_12S7P
    } BATT_NOW;

    QPixmap mBattImg;
    BMS_VALUES mVal;
    BATT_NOW mBatt;

};

#endif // BATTTEMPPLOT_H
