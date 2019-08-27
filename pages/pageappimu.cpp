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

#include "pageappimu.h"
#include "ui_pageappimu.h"

PageAppImu::PageAppImu(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PageAppImu)
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

    ui->rpyPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

    int graphIndex = 0;

    ui->rpyPlot->addGraph();
    ui->rpyPlot->graph(graphIndex)->setPen(QPen(Qt::blue));
    ui->rpyPlot->graph(graphIndex)->setName("Roll");
    graphIndex++;

    ui->rpyPlot->addGraph();
    ui->rpyPlot->graph(graphIndex)->setPen(QPen(Qt::red));
    ui->rpyPlot->graph(graphIndex)->setName("Pitch");
    graphIndex++;

    ui->rpyPlot->addGraph();
    ui->rpyPlot->graph(graphIndex)->setPen(QPen(Qt::green));
    ui->rpyPlot->graph(graphIndex)->setName("Yaw");
    graphIndex++;

    QFont legendFont = font();
    legendFont.setPointSize(9);

    ui->rpyPlot->legend->setVisible(true);
    ui->rpyPlot->legend->setFont(legendFont);
    ui->rpyPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignRight|Qt::AlignBottom);
    ui->rpyPlot->legend->setBrush(QBrush(QColor(255,255,255,230)));
    ui->rpyPlot->xAxis->setLabel("Seconds (s)");
    ui->rpyPlot->yAxis->setLabel("Angle (Deg)");

}

PageAppImu::~PageAppImu()
{
    delete ui;
}

VescInterface *PageAppImu::vesc() const
{
    return mVesc;
}

void PageAppImu::setVesc(VescInterface *vesc)
{
    mVesc = vesc;

    if (mVesc) {
        ui->configPane->addRowSeparator(tr("Input Source"));
        ui->configPane->addParamRow(mVesc->appConfig(), "app_imu_conf.use_peripheral");
        ui->configPane->addRowSeparator(tr("Axes"));
        ui->configPane->addParamRow(mVesc->appConfig(), "app_imu_conf.pitch_axis");
        ui->configPane->addParamRow(mVesc->appConfig(), "app_imu_conf.roll_axis");
        ui->configPane->addParamRow(mVesc->appConfig(), "app_imu_conf.yaw_axis");
        ui->configPane->addParamRow(mVesc->appConfig(), "app_imu_conf.flip");
        ui->configPane->addRowSeparator(tr("AHRS"));
        ui->configPane->addParamRow(mVesc->appConfig(), "app_imu_conf.hertz");
        ui->configPane->addParamRow(mVesc->appConfig(), "app_imu_conf.m_acd");
        ui->configPane->addParamRow(mVesc->appConfig(), "app_imu_conf.m_b");
        ui->configPane->addParamRow(mVesc->appConfig(), "app_imu_conf.cal_delay");
        ui->configPane->addParamRow(mVesc->appConfig(), "app_imu_conf.cal_m_acd");
        ui->configPane->addParamRow(mVesc->appConfig(), "app_imu_conf.cal_m_b");

        connect(mVesc->commands(), SIGNAL(valuesImuReceived(IMU_VALUES,uint)),
                this, SLOT(valuesReceived(IMU_VALUES,uint)));
    }
}

void PageAppImu::timerSlot()
{
    if (mUpdatePlots) {
        mUpdatePlots = false;

        int dataSize = mRollVec.size();

        QVector<double> xAxis(dataSize);
        for (int i = 0;i < mSeconds.size();i++) {
            xAxis[i] = mSeconds[i];
        }

        int graphIndex = 0;
        ui->rpyPlot->graph(graphIndex++)->setData(xAxis, mRollVec);
        ui->rpyPlot->graph(graphIndex++)->setData(xAxis, mPitchVec);
        ui->rpyPlot->graph(graphIndex++)->setData(xAxis, mYawVec);

        ui->rpyPlot->rescaleAxes();

        ui->rpyPlot->replot();
    }
}

void PageAppImu::valuesReceived(IMU_VALUES values, unsigned int mask)
{
    (void)mask;

    const int maxS = 500;

    appendDoubleAndTrunc(&mRollVec, values.roll * 180.0 / M_PI, maxS);
    appendDoubleAndTrunc(&mPitchVec, values.pitch * 180.0 / M_PI, maxS);
    appendDoubleAndTrunc(&mYawVec, values.yaw * 180.0 / M_PI, maxS);

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

void PageAppImu::appendDoubleAndTrunc(QVector<double> *vec, double num, int maxSize)
{
    vec->append(num);

    if(vec->size() > maxSize) {
        vec->remove(0, vec->size() - maxSize);
    }
}
