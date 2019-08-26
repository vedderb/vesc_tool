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

#ifndef PAGEAPPIMU_H
#define PAGEAPPIMU_H

#include <QWidget>
#include <QVector>
#include <QTimer>
#include "vescinterface.h"

namespace Ui {
class PageAppImu;
}

class PageAppImu : public QWidget
{
    Q_OBJECT

public:
    explicit PageAppImu(QWidget *parent = 0);
    ~PageAppImu();

    VescInterface *vesc() const;
    void setVesc(VescInterface *vesc);

private slots:
    void timerSlot();
    void appValuesReceived(double pid_outpout, double pitch, double roll, uint32_t diff_time, double motor_current, double motor_position, uint16_t state);

private:
    Ui::PageAppImu *ui;
    VescInterface *mVesc;

    QTimer *mTimer;

    bool mUpdatePlots;

    QVector<double> mAppPidOutputVec;
    QVector<double> mAppPitchVec;
    QVector<double> mAppRollVec;
    uint32_t mAppDiffTime = 0;
    QVector<double> mAppMotorCurrentVec;
    QVector<double> mAppMotorPositionVec;
    uint16_t mAppState;

    QVector<double> mSeconds;

    double mSecondCounter;
    qint64 mLastUpdateTime;

    void appendDoubleAndTrunc(QVector<double> *vec, double num, int maxSize);
    void updateTextOutput();

};

#endif // PAGEAPPIMU_H
