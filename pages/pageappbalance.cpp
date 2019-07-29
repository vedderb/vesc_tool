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

#include "pageappbalance.h"
#include "ui_pageappbalance.h"

PageAppBalance::PageAppBalance(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PageAppBalance)
{
    ui->setupUi(this);
    layout()->setContentsMargins(0, 0, 0, 0);
    mVesc = 0;

    mTimer = new QTimer(this);
    mTimer->start(20);
    mUpdatePlots = false;
    mSecondCounter = 0.0;
    mLastUpdateTime = 0;

    connect(mTimer, SIGNAL(timeout()),
            this, SLOT(timerSlot()));

    ui->balancePlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

    int graphIndex = 0;

    ui->balancePlot->addGraph();
    ui->balancePlot->graph(graphIndex)->setPen(QPen(Qt::red));
    ui->balancePlot->graph(graphIndex)->setName("PID Output");
    graphIndex++;

    ui->balancePlot->addGraph();
    ui->balancePlot->graph(graphIndex)->setPen(QPen(Qt::blue));
    ui->balancePlot->graph(graphIndex)->setName("Pitch");
    graphIndex++;

    ui->balancePlot->addGraph();
    ui->balancePlot->graph(graphIndex)->setPen(QPen(Qt::cyan));
    ui->balancePlot->graph(graphIndex)->setName("Roll");
    graphIndex++;

    ui->balancePlot->addGraph();
    ui->balancePlot->graph(graphIndex)->setPen(QPen(Qt::black));
    ui->balancePlot->graph(graphIndex)->setName("Current");
    graphIndex++;

    QFont legendFont = font();
    legendFont.setPointSize(9);

    ui->balancePlot->legend->setVisible(true);
    ui->balancePlot->legend->setFont(legendFont);
    ui->balancePlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignRight|Qt::AlignBottom);
    ui->balancePlot->legend->setBrush(QBrush(QColor(255,255,255,230)));
    ui->balancePlot->xAxis->setLabel("Seconds (s)");
    ui->balancePlot->yAxis->setLabel("Angle (Deg)/Current (A)");

}

PageAppBalance::~PageAppBalance()
{
    delete ui;
}

VescInterface *PageAppBalance::vesc() const
{
    return mVesc;
}

void PageAppBalance::setVesc(VescInterface *vesc)
{
    mVesc = vesc;

    if (mVesc) {
//        ui->nrfPair->setVesc(mVesc);

        ui->generalTab->addRowSeparator(tr("PID"));
        ui->generalTab->addParamRow(mVesc->appConfig(), "app_balance_conf.kp");
        ui->generalTab->addParamRow(mVesc->appConfig(), "app_balance_conf.ki");
        ui->generalTab->addParamRow(mVesc->appConfig(), "app_balance_conf.kd");
        ui->generalTab->addRowSeparator(tr("Offset"));
        ui->generalTab->addParamRow(mVesc->appConfig(), "app_balance_conf.pitch_offset");
        ui->generalTab->addParamRow(mVesc->appConfig(), "app_balance_conf.roll_offset");
        ui->generalTab->addRowSeparator(tr("Fault"));
        ui->generalTab->addParamRow(mVesc->appConfig(), "app_balance_conf.pitch_fault");
        ui->generalTab->addParamRow(mVesc->appConfig(), "app_balance_conf.roll_fault");
        ui->generalTab->addRowSeparator(tr("Misc"));
        ui->generalTab->addParamRow(mVesc->appConfig(), "app_balance_conf.start_delay");

//        connect(mVesc->commands(), SIGNAL(valuesImuReceived(IMU_VALUES, uint)),
//                this, SLOT(imuValuesReceived(IMU_VALUES, uint)));
//        connect(mVesc->commands(), SIGNAL(valuesReceived(MC_VALUES,unsigned int)),
//                this, SLOT(motorValuesReceived(MC_VALUES, unsigned int)));
        connect(mVesc->commands(), SIGNAL(decodedBalanceReceived(double, double, double, double)),
                this, SLOT(appValuesReceived(double, double, double, double)));
//        connect(mVesc->commands(), SIGNAL(rotorPosReceived(double)),
//                this, SLOT(rotorPosReceived(double)));
    }
}

void PageAppBalance::timerSlot()
{
    if (mUpdatePlots) {

        mUpdatePlots = false;

        int dataSize = mAppPidOutputVec.size();

        QVector<double> xAxis(dataSize);
        for (int i = 0;i < mSeconds.size();i++) {
            xAxis[i] = mSeconds[i];
        }

        int graphIndex = 0;
//        ui->balancePlot->graph(graphIndex++)->setData(xAxis, mPitchVec);
        ui->balancePlot->graph(graphIndex++)->setData(xAxis, mAppPidOutputVec);
        ui->balancePlot->graph(graphIndex++)->setData(xAxis, mAppPitchVec);
        ui->balancePlot->graph(graphIndex++)->setData(xAxis, mAppRollVec);
        ui->balancePlot->graph(graphIndex++)->setData(xAxis, mAppMotorCurrentVec);

        ui->balancePlot->rescaleAxes();

        ui->balancePlot->replot();
    }
}

void PageAppBalance::appValuesReceived(double pid_outpout, double pitch, double roll, double motor_current) {

    const int maxS = 500;

    appendDoubleAndTrunc(&mAppPidOutputVec, pid_outpout, maxS);
    appendDoubleAndTrunc(&mAppPitchVec, pitch, maxS);
    appendDoubleAndTrunc(&mAppRollVec, roll, maxS);
    appendDoubleAndTrunc(&mAppMotorCurrentVec, motor_current, maxS);


    qint64 tNow = QDateTime::currentMSecsSinceEpoch();

    double elapsed = (double)(tNow - mLastUpdateTime) / 1000.0;
    if (elapsed > 1.0) {
        elapsed = 1.0;
    }

    mSecondCounter += elapsed;

    appendDoubleAndTrunc(&mSeconds, mSecondCounter, maxS);

    mLastUpdateTime = tNow;

    mUpdatePlots = true;
}

void PageAppBalance::imuValuesReceived(IMU_VALUES values, unsigned int mask)
{
    (void)mask;

    const int maxS = 500;

    appendDoubleAndTrunc(&mRollVec, values.roll * 180.0 / M_PI, maxS);
    appendDoubleAndTrunc(&mPitchVec, values.pitch * 180.0 / M_PI, maxS);
    appendDoubleAndTrunc(&mYawVec, values.yaw * 180.0 / M_PI, maxS);

    appendDoubleAndTrunc(&mAccXVec, values.accX, maxS);
    appendDoubleAndTrunc(&mAccYVec, values.accY, maxS);
    appendDoubleAndTrunc(&mAccZVec, values.accZ, maxS);

    appendDoubleAndTrunc(&mGyroXVec, values.gyroX, maxS);
    appendDoubleAndTrunc(&mGyroYVec, values.gyroY, maxS);
    appendDoubleAndTrunc(&mGyroZVec, values.gyroZ, maxS);

    appendDoubleAndTrunc(&mMagXVec, values.magX, maxS);
    appendDoubleAndTrunc(&mMagYVec, values.magY, maxS);
    appendDoubleAndTrunc(&mMagZVec, values.magZ, maxS);

    qint64 tNow = QDateTime::currentMSecsSinceEpoch();

    double elapsed = (double)(tNow - mLastUpdateTime) / 1000.0;
    if (elapsed > 1.0) {
        elapsed = 1.0;
    }

    mSecondCounter += elapsed;

    appendDoubleAndTrunc(&mSeconds, mSecondCounter, maxS);

    mLastUpdateTime = tNow;

    mUpdatePlots = true;
}

void PageAppBalance::motorValuesReceived(MC_VALUES values, unsigned int mask)
{
    (void)mask;

    const int maxS = 500;

    appendDoubleAndTrunc(&mTempMosVec, values.temp_mos, maxS);
    appendDoubleAndTrunc(&mTempMos1Vec, values.temp_mos_1, maxS);
    appendDoubleAndTrunc(&mTempMos2Vec, values.temp_mos_2, maxS);
    appendDoubleAndTrunc(&mTempMos3Vec, values.temp_mos_3, maxS);
    appendDoubleAndTrunc(&mTempMotorVec, values.temp_motor, maxS);
    appendDoubleAndTrunc(&mCurrInVec, values.current_in, maxS);
    appendDoubleAndTrunc(&mCurrMotorVec, values.current_motor, maxS);
    appendDoubleAndTrunc(&mIdVec, values.id, maxS);
    appendDoubleAndTrunc(&mIqVec, values.iq, maxS);
    appendDoubleAndTrunc(&mDutyVec, values.duty_now, maxS);
    appendDoubleAndTrunc(&mRpmVec, values.rpm, maxS);

//    qint64 tNow = QDateTime::currentMSecsSinceEpoch();

//    double elapsed = (double)(tNow - mLastUpdateTime) / 1000.0;
//    if (elapsed > 1.0) {
//        elapsed = 1.0;
//    }

//    mSecondCounter += elapsed;

//    appendDoubleAndTrunc(&mSeconds, mSecondCounter, maxS);

//    mLastUpdateTime = tNow;

//    mUpdateValPlot = true;
}

void PageAppBalance::appendDoubleAndTrunc(QVector<double> *vec, double num, int maxSize)
{
    vec->append(num);

    if(vec->size() > maxSize) {
        vec->remove(0, vec->size() - maxSize);
    }
}
