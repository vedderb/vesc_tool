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
    ui->balancePlot->graph(graphIndex)->setName("Pitch Angle");
    graphIndex++;

    ui->balancePlot->addGraph();
    ui->balancePlot->graph(graphIndex)->setPen(QPen(Qt::cyan));
    ui->balancePlot->graph(graphIndex)->setName("Roll Angle");
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
        reloadParams();

        updateTextOutput();

        connect(mVesc->commands(), SIGNAL(decodedBalanceReceived(BALANCE_VALUES)),
                this, SLOT(appValuesReceived(BALANCE_VALUES)));
    }
}

void PageAppBalance::reloadParams()
{
    if (mVesc) {
        ui->tunePane->clearParams();
        ui->configPane->clearParams();

        ui->tunePane->addParamSubgroup(mVesc->appConfig(), "balance", "tune");
        ui->configPane->addParamSubgroup(mVesc->appConfig(), "balance", "config");
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
        ui->balancePlot->graph(graphIndex++)->setData(xAxis, mAppMAngleVec);
        ui->balancePlot->graph(graphIndex++)->setData(xAxis, mAppCAngleVec);
        ui->balancePlot->graph(graphIndex++)->setData(xAxis, mAppMotorCurrentVec);

        ui->balancePlot->rescaleAxes();

        ui->balancePlot->replot();

        updateTextOutput();
    }
}

void PageAppBalance::appValuesReceived(BALANCE_VALUES values) {

    const int maxS = 250;

    appendDoubleAndTrunc(&mAppPidOutputVec, values.pid_output, maxS);
    appendDoubleAndTrunc(&mAppMAngleVec, values.m_angle, maxS);
    appendDoubleAndTrunc(&mAppCAngleVec, values.c_angle, maxS);
    mAppDiffTime = values.diff_time;
    appendDoubleAndTrunc(&mAppMotorCurrentVec, values.motor_current, maxS);
    appendDoubleAndTrunc(&mAppMotorPositionVec, values.motor_position, maxS);
    mAppState = values.state;
    mAppSwitchValue = values.switch_value;


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

void PageAppBalance::updateTextOutput(){
    QString output = "Loop Time: ";
    output = output + QString::number(mAppDiffTime);

    output = output + "\tState: ";
    if(mAppState == 0){
        output = output + "Calibrating";
    }else if(mAppState == 1){
        output = output + "Running";
    }else if(mAppState == 2){
        output = output + "Fault";
    }else if(mAppState == 3){
        output = output + "Dead";
    }else{
        output = output + "Unknown";
    }

    output = output + "\tSwitch Value: " + QString::number(mAppSwitchValue);

    output = output + "\tMotor Position: ";
    if(mAppMotorPositionVec.empty() == false){
        output = output + QString::number((int)mAppMotorPositionVec.last());
    }else{
        output = output + "Unknown";
    }


    ui->textOutput->setText(output);
}
