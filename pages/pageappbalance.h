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

#ifndef PAGEAPPBALANCE_H
#define PAGEAPPBALANCE_H

#include <QWidget>
#include <QVector>
#include <QTimer>
#include "vescinterface.h"

namespace Ui {
class PageAppBalance;
}

class PageAppBalance : public QWidget
{
    Q_OBJECT

public:
    explicit PageAppBalance(QWidget *parent = 0);
    ~PageAppBalance();

    VescInterface *vesc() const;
    void setVesc(VescInterface *vesc);

private slots:
    void timerSlot();
    void imuValuesReceived(IMU_VALUES values, unsigned int mask);
    void motorValuesReceived(MC_VALUES values, unsigned int mask);
    void appValuesReceived(double pid_outpout, double pitch, double roll, double motor_current);

private:
    Ui::PageAppBalance *ui;
    VescInterface *mVesc;

    QTimer *mTimer;

    bool mUpdatePlots;

    QVector<double> mRollVec;
    QVector<double> mPitchVec;
    QVector<double> mYawVec;

    QVector<double> mAccXVec;
    QVector<double> mAccYVec;
    QVector<double> mAccZVec;

    QVector<double> mGyroXVec;
    QVector<double> mGyroYVec;
    QVector<double> mGyroZVec;

    QVector<double> mMagXVec;
    QVector<double> mMagYVec;
    QVector<double> mMagZVec;

    QVector<double> mTempMosVec;
    QVector<double> mTempMos1Vec;
    QVector<double> mTempMos2Vec;
    QVector<double> mTempMos3Vec;
    QVector<double> mTempMotorVec;
    QVector<double> mCurrInVec;
    QVector<double> mCurrMotorVec;
    QVector<double> mIdVec;
    QVector<double> mIqVec;
    QVector<double> mDutyVec;
    QVector<double> mRpmVec;

    QVector<double> mAppPidOutputVec;
    QVector<double> mAppPitchVec;
    QVector<double> mAppRollVec;
    QVector<double> mAppMotorCurrentVec;

    QVector<double> mSeconds;

    double mSecondCounter;
    qint64 mLastUpdateTime;

    void appendDoubleAndTrunc(QVector<double> *vec, double num, int maxSize);

};

#endif // PAGEAPPNRF_H
