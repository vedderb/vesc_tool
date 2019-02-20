/*
    Copyright 2019 Benjamin Vedder	benjamin@vedder.se

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

#ifndef DETECTALLFOCDIALOG_H
#define DETECTALLFOCDIALOG_H

#include <QDialog>
#include "vescinterface.h"

namespace Ui {
class DetectAllFocDialog;
}

class DetectAllFocDialog : public QDialog
{
    Q_OBJECT

public:
    class MotorData {
    public:
        MotorData() {
            maxLosses = 0.0;
            openloopErpm = 0.0;
            sensorlessErpm = 0.0;
            poles = 14;
        }

        MotorData(const MotorData &other) {
            maxLosses = other.maxLosses;
            openloopErpm = other.openloopErpm;
            sensorlessErpm = other.sensorlessErpm;
            poles = other.poles;
        }

        MotorData(double maxLosses, double openloopErpm, double sensorlessErpm, int poles) {
            this->maxLosses = maxLosses;
            this->openloopErpm = openloopErpm;
            this->sensorlessErpm = sensorlessErpm;
            this->poles = poles;
        }

        ~MotorData() {}

        double maxLosses;
        double openloopErpm;
        double sensorlessErpm;
        int poles;
    };

    explicit DetectAllFocDialog(VescInterface *vesc, QWidget *parent = 0);
    ~DetectAllFocDialog();

    static void showDialog(VescInterface *vesc, QWidget *parent = 0);

protected:
    void reject();

private slots:
    void updateGearRatio();

    void on_runButton_clicked();
    void on_closeButton_clicked();
    void on_nextMotorButton_clicked();
    void on_prevBattButton_clicked();
    void on_nextBattButton_clicked();
    void on_prevSetupButton_clicked();
    void on_directDriveBox_toggled(bool checked);
    void on_motorList_currentRowChanged(int currentRow);
    void on_prevDirButton_clicked();

private:
    Ui::DetectAllFocDialog *ui;
    VescInterface *mVesc;
    bool mRejectOk;
    int mPulleyMotorOld;
    int mPulleyWheelOld;

};

Q_DECLARE_METATYPE(DetectAllFocDialog::MotorData)

#endif // DETECTALLFOCDIALOG_H
