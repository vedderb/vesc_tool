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

#ifndef DETECTBLDC_H
#define DETECTBLDC_H

#include <QWidget>
#include "vescinterface.h"

namespace Ui {
class DetectBldc;
}

class DetectBldc : public QWidget
{
    Q_OBJECT

public:
    explicit DetectBldc(QWidget *parent = 0);
    ~DetectBldc();

    VescInterface *vesc() const;
    void setVesc(VescInterface *vesc);

    void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;

private slots:
    void bldcDetectReceived(bldc_detect param);

    void on_runButton_clicked();
    void on_applyButton_clicked();
    void on_helpButton_clicked();

private:
    Ui::DetectBldc *ui;
    VescInterface *mVesc;
    bldc_detect mResult;
    bool mResultReceived;
    bool mRunning;

};

#endif // DETECTBLDC_H
