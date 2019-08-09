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
        ui->tunePane->addRowSeparator(tr("PID"));
        ui->tunePane->addParamRow(mVesc->appConfig(), "app_balance_conf.kp");
        ui->tunePane->addParamRow(mVesc->appConfig(), "app_balance_conf.ki");
        ui->tunePane->addParamRow(mVesc->appConfig(), "app_balance_conf.kd");
        ui->tunePane->addRowSeparator(tr("Main loop"));
        ui->tunePane->addParamRow(mVesc->appConfig(), "app_balance_conf.loop_delay");
        ui->tunePane->addRowSeparator(tr("Gyro Calibration"));
        ui->tunePane->addParamRow(mVesc->appConfig(), "app_balance_conf.pitch_offset");
        ui->tunePane->addParamRow(mVesc->appConfig(), "app_balance_conf.roll_offset");
        ui->tunePane->addParamRow(mVesc->appConfig(), "app_balance_conf.m_acd");
        ui->tunePane->addParamRow(mVesc->appConfig(), "app_balance_conf.m_b");
        ui->tunePane->addParamRow(mVesc->appConfig(), "app_balance_conf.cal_delay");
        ui->tunePane->addParamRow(mVesc->appConfig(), "app_balance_conf.cal_m_acd");
        ui->tunePane->addParamRow(mVesc->appConfig(), "app_balance_conf.cal_m_b");
        ui->tunePane->addRowSeparator(tr("Experimental"));
        ui->tunePane->addParamRow(mVesc->appConfig(), "app_balance_conf.deadzone");
        ui->tunePane->addParamRow(mVesc->appConfig(), "app_balance_conf.current_boost");
        ui->configPane->addRowSeparator(tr("Startup"));
        ui->configPane->addParamRow(mVesc->appConfig(), "app_balance_conf.startup_pitch");
        ui->configPane->addParamRow(mVesc->appConfig(), "app_balance_conf.startup_roll");
        ui->configPane->addParamRow(mVesc->appConfig(), "app_balance_conf.startup_speed");
        ui->configPane->addRowSeparator(tr("Tiltback"));
        ui->configPane->addParamRow(mVesc->appConfig(), "app_balance_conf.tiltback_duty");
        ui->configPane->addParamRow(mVesc->appConfig(), "app_balance_conf.tiltback_angle");
        ui->configPane->addParamRow(mVesc->appConfig(), "app_balance_conf.tiltback_speed");
        ui->configPane->addRowSeparator(tr("Overspeed"));
        ui->configPane->addParamRow(mVesc->appConfig(), "app_balance_conf.overspeed_duty");
        ui->configPane->addRowSeparator(tr("Fault"));
        ui->configPane->addParamRow(mVesc->appConfig(), "app_balance_conf.pitch_fault");
        ui->configPane->addParamRow(mVesc->appConfig(), "app_balance_conf.roll_fault");


        connect(mVesc->commands(), SIGNAL(decodedBalanceReceived(double, double, double, uint32_t, double, double)),
                this, SLOT(appValuesReceived(double, double, double, uint32_t, double, double)));
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
        ui->balancePlot->graph(graphIndex++)->setData(xAxis, mAppPidOutputVec);
        ui->balancePlot->graph(graphIndex++)->setData(xAxis, mAppPitchVec);
        ui->balancePlot->graph(graphIndex++)->setData(xAxis, mAppRollVec);
        ui->balancePlot->graph(graphIndex++)->setData(xAxis, mAppMotorCurrentVec);

        ui->balancePlot->rescaleAxes();

        ui->balancePlot->replot();

        ui->textOutput->setText(QString::number(mAppDiffTimeVec.last()));
    }
}

void PageAppBalance::appValuesReceived(double pid_outpout, double pitch, double roll, uint32_t diff_time, double motor_current, double motor_position) {

    const int maxS = 500;

    appendDoubleAndTrunc(&mAppPidOutputVec, pid_outpout, maxS);
    appendDoubleAndTrunc(&mAppPitchVec, pitch, maxS);
    appendDoubleAndTrunc(&mAppRollVec, roll, maxS);
    appendUint32tAndTrunc(&mAppDiffTimeVec, diff_time, maxS);
    appendDoubleAndTrunc(&mAppMotorCurrentVec, motor_current, maxS);
    appendDoubleAndTrunc(&mAppMotorPositionVec, motor_position, maxS);


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

void PageAppBalance::appendDoubleAndTrunc(QVector<double> *vec, double num, int maxSize)
{
    vec->append(num);

    if(vec->size() > maxSize) {
        vec->remove(0, vec->size() - maxSize);
    }
}

void PageAppBalance::appendUint32tAndTrunc(QVector<uint32_t> *vec, uint32_t num, int maxSize)
{
    vec->append(num);

    if(vec->size() > maxSize) {
        vec->remove(0, vec->size() - maxSize);
    }
}
