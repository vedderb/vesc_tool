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

#ifndef PAGEBMS_H
#define PAGEBMS_H

#include <QWidget>
#include <QTimer>
#include "vescinterface.h"
#include "widgets/qcustomplot.h"

namespace Ui {
class PageBms;
}

class PageBms : public QWidget
{
    Q_OBJECT

public:
    explicit PageBms(QWidget *parent = nullptr);
    ~PageBms();

    VescInterface *vesc() const;
    void setVesc(VescInterface *vesc);

private slots:
    void bmsValuesRx(BMS_VALUES val);

    void on_resetAhButton_clicked();
    void on_resetWhButton_clicked();
    void on_balOnButton_clicked();
    void on_balOffButton_clicked();
    void on_zeroCurrentButton_clicked();
    void on_chgEnButton_clicked();
    void on_chgDisButton_clicked();

private:
    Ui::PageBms *ui;
    VescInterface *mVesc;
    QVector<QCPBars*> mCellBars;
    QVector<QCPBars*> mTempBars;

    void reloadCellBars(int cells);
    void reloadTempBars(int sensors);

};

#endif // PAGEBMS_H
