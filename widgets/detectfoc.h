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

#ifndef DETECTFOC_H
#define DETECTFOC_H

#include <QWidget>
#include "vescinterface.h"

namespace Ui {
class DetectFoc;
}

class DetectFoc : public QWidget
{
    Q_OBJECT

public:
    explicit DetectFoc(QWidget *parent = nullptr);
    ~DetectFoc();

    VescInterface *vesc() const;
    void setVesc(VescInterface *vesc);
    bool allValuesOk() const;
    bool lastOkValuesApplied() const;

private slots:
    void motorRLReceived(double r, double l);
    void motorLinkageReceived(double flux_linkage);
    void paramChangedDouble(QObject *src, QString name, double newParam);

    void on_rlButton_clicked();
    void on_lambdaButton_clicked();
    void on_helpButton_clicked();
    void on_applyAllButton_clicked();
    void on_calcKpKiButton_clicked();
    void on_calcGainButton_clicked();
    void on_calcApplyLocalButton_clicked();

private:
    Ui::DetectFoc *ui;
    VescInterface *mVesc;
    bool mLastCalcOk;
    bool mAllValuesOk;
    bool mLastOkValuesApplied;
    bool mRunning;

    void updateColors();

};

#endif // DETECTFOC_H
