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

#ifndef PAGEAPPIMU_H
#define PAGEAPPIMU_H

#include "vescinterface.h"
#include "widgets/vesc3dview.h"

#include <QWidget>
#include <QCheckBox>

namespace Ui {
class PageAppImu;
}

class PageAppImu : public QWidget
{
    Q_OBJECT

public:
    explicit PageAppImu(QWidget *parent = nullptr);
    ~PageAppImu();

    VescInterface *vesc() const;
    void setVesc(VescInterface *vesc);

private slots:
    void timerSlot();
    void valuesReceived(IMU_VALUES values, unsigned int mask);

private:
    Ui::PageAppImu *ui;
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

    QVector<double> mSeconds;

    double mSecondCounter;
    qint64 mLastUpdateTime;

    Vesc3DView *m3dView;
    QCheckBox *mUseYawBox;

    void appendDoubleAndTrunc(QVector<double> *vec, double num, int maxSize);
};

#endif // PAGEAPPIMU_H
