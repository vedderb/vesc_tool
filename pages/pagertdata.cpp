/*
    Copyright 2016 - 2022 Benjamin Vedder	benjamin@vedder.se

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

#include "pagertdata.h"
#include "ui_pagertdata.h"
#include <QFileDialog>
#include <QMessageBox>
#include "utility.h"

#include <QXmlStreamWriter>
#include <QXmlStreamReader>

PageRtData::PageRtData(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PageRtData)
{
    ui->setupUi(this);
    layout()->setContentsMargins(0, 0, 0, 0);
    mVesc = nullptr;

    ui->rescaleButton->setIcon(Utility::getIcon("icons/expand_off.png"));

    QIcon mycon = QIcon(Utility::getIcon("icons/expand_off.png"));
    mycon.addPixmap(Utility::getIcon("icons/expand_on.png"), QIcon::Normal, QIcon::On);
    mycon.addPixmap(Utility::getIcon("icons/expand_off.png"), QIcon::Normal, QIcon::Off);
    ui->zoomHButton->setIcon(mycon);

    mycon = QIcon(Utility::getIcon("icons/expand_v_off.png"));
    mycon.addPixmap(Utility::getIcon("icons/expand_v_on.png"), QIcon::Normal, QIcon::On);
    mycon.addPixmap(Utility::getIcon("icons/expand_v_off.png"), QIcon::Normal, QIcon::Off);
    ui->zoomVButton->setIcon(mycon);

    mycon = QIcon(Utility::getIcon("icons/size_off.png"));
    mycon.addPixmap(Utility::getIcon("icons/size_on.png"), QIcon::Normal, QIcon::On);
    mycon.addPixmap(Utility::getIcon("icons/size_off.png"), QIcon::Normal, QIcon::Off);
    ui->autoscaleButton->setIcon(mycon);

    mycon = QIcon(Utility::getIcon("icons/rt_off.png"));
    mycon.addPixmap(Utility::getIcon("icons/rt_on.png"), QIcon::Normal, QIcon::On);
    mycon.addPixmap(Utility::getIcon("icons/rt_off.png"), QIcon::Normal, QIcon::Off);
    ui->logRtButton->setIcon(mycon);

    mTimer = new QTimer(this);
    mTimer->start(20);

    mSecondCounter = 0.0;
    mLastUpdateTime = 0;

    mUpdateValPlot = false;
    mUpdatePosPlot = false;

    QCustomPlot* allPlots[] =
                {ui->currentPlot, ui->tempPlot, ui->focPlot,
                ui->posPlot, ui->rpmPlot};
    for(int j = 0;j < 5; j++) {
        Utility::setPlotColors(allPlots[j]);
        allPlots[j]->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    }

    // Current and duty
    int graphIndex = 0;
    ui->currentPlot->addGraph();

    ui->currentPlot->graph(graphIndex)->setPen(QPen(Utility::getAppQColor("plot_graph1")));
    ui->currentPlot->graph(graphIndex)->setName("Current in");
    graphIndex++;

    ui->currentPlot->addGraph();
    ui->currentPlot->graph(graphIndex)->setPen(QPen(Utility::getAppQColor("plot_graph2")));
    ui->currentPlot->graph(graphIndex)->setName("Current motor");
    graphIndex++;

    ui->currentPlot->addGraph(ui->currentPlot->xAxis, ui->currentPlot->yAxis2);
    ui->currentPlot->graph(graphIndex)->setPen(QPen(Utility::getAppQColor("plot_graph3")));
    ui->currentPlot->graph(graphIndex)->setName("Duty cycle");
    graphIndex++;

    // RPM
    graphIndex = 0;
    ui->rpmPlot->addGraph();
    ui->rpmPlot->graph(graphIndex)->setPen(QPen(Utility::getAppQColor("plot_graph1")));
    ui->rpmPlot->graph(graphIndex)->setName("ERPM");
    graphIndex++;

    // FOC
    graphIndex = 0;
    ui->focPlot->addGraph();
    ui->focPlot->graph(graphIndex)->setPen(QPen(Utility::getAppQColor("plot_graph1")));
    ui->focPlot->graph(graphIndex)->setName("D Current");
    graphIndex++;

    ui->focPlot->addGraph();
    ui->focPlot->graph(graphIndex)->setPen(QPen(Utility::getAppQColor("plot_graph2")));
    ui->focPlot->graph(graphIndex)->setName("Q Current");
    graphIndex++;

    ui->focPlot->addGraph(ui->focPlot->xAxis, ui->focPlot->yAxis2);
    ui->focPlot->graph(graphIndex)->setPen(QPen(Utility::getAppQColor("plot_graph3")));
    ui->focPlot->graph(graphIndex)->setName("D Voltage");
    graphIndex++;

    ui->focPlot->addGraph(ui->focPlot->xAxis, ui->focPlot->yAxis2);
    ui->focPlot->graph(graphIndex)->setPen(QPen(Utility::getAppQColor("plot_graph4")));
    ui->focPlot->graph(graphIndex)->setName("Q Voltage");
    graphIndex++;

    QFont legendFont = font();
    legendFont.setPointSize(9);

    ui->currentPlot->legend->setVisible(true);
    ui->currentPlot->legend->setFont(legendFont);
    ui->currentPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignRight|Qt::AlignBottom);
    ui->currentPlot->xAxis->setLabel("Seconds (s)");
    ui->currentPlot->yAxis->setLabel("Ampere (A)");
    ui->currentPlot->yAxis2->setLabel("Duty Cycle");

    ui->tempPlot->legend->setVisible(true);
    ui->tempPlot->legend->setFont(legendFont);
    ui->tempPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignRight|Qt::AlignBottom);
    ui->tempPlot->xAxis->setLabel("Seconds (s)");
    ui->tempPlot->yAxis->setLabel("Temperature MOSFET (\u00B0C)");
    ui->tempPlot->yAxis2->setLabel("Temperature Motor (\u00B0C)");

    ui->rpmPlot->legend->setVisible(true);
    ui->rpmPlot->legend->setFont(legendFont);
    ui->rpmPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignRight|Qt::AlignBottom);
    ui->rpmPlot->xAxis->setLabel("Seconds (s)");
    ui->rpmPlot->yAxis->setLabel("ERPM");

    ui->focPlot->legend->setVisible(true);
    ui->focPlot->legend->setFont(legendFont);
    ui->focPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignRight|Qt::AlignBottom);
    ui->focPlot->xAxis->setLabel("Seconds (s)");
    ui->focPlot->yAxis->setLabel("Current");
    ui->focPlot->yAxis2->setLabel("Voltage");

    ui->currentPlot->yAxis->setRange(-20, 130);
    ui->currentPlot->yAxis2->setRange(-0.2, 1.3);
    ui->currentPlot->yAxis2->setVisible(true);
    ui->tempPlot->yAxis->setRange(0, 120);
    ui->tempPlot->yAxis2->setRange(0, 120);
    ui->tempPlot->yAxis2->setVisible(true);
    ui->rpmPlot->yAxis->setRange(0, 120);
    ui->focPlot->yAxis->setRange(0, 120);
    ui->focPlot->yAxis2->setRange(0, 120);
    ui->focPlot->yAxis2->setVisible(true);

    ui->posPlot->addGraph();
    ui->posPlot->graph(0)->setPen(QPen(Utility::getAppQColor("plot_graph1")));
    ui->posPlot->graph(0)->setName("Position");

    ui->posPlot->legend->setVisible(true);
    ui->posPlot->legend->setFont(legendFont);
    ui->posPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignRight|Qt::AlignBottom);
    ui->posPlot->xAxis->setLabel("Sample");
    ui->posPlot->yAxis->setLabel("Degrees");

    connect(mTimer, SIGNAL(timeout()),
            this, SLOT(timerSlot()));
}

PageRtData::~PageRtData()
{
    delete ui;
}

VescInterface *PageRtData::vesc() const
{
    return mVesc;
}

void PageRtData::setVesc(VescInterface *vesc)
{
    mVesc = vesc;

    ui->experimentPlot->setVesc(vesc);

    if (mVesc) {
        connect(mVesc->commands(), SIGNAL(valuesReceived(MC_VALUES,uint)),
                this, SLOT(valuesReceived(MC_VALUES,uint)));
        connect(mVesc->commands(), SIGNAL(rotorPosReceived(double)),
                this, SLOT(rotorPosReceived(double)));
    }
}

void PageRtData::timerSlot()
{
    if (mVesc) {
        if (mVesc->isRtLogOpen() != ui->logRtButton->isChecked()) {
            ui->logRtButton->setChecked(mVesc->isRtLogOpen());
        }
    }

    if (mUpdateValPlot) {
        int dataSize = mTempMosVec.size();

        QVector<double> xAxis(dataSize);
        for (int i = 0;i < mSeconds.size();i++) {
            xAxis[i] = mSeconds[i];
        }

        // Current and duty-plot
        int graphIndex = 0;
        ui->currentPlot->graph(graphIndex++)->setData(xAxis, mCurrInVec);
        ui->currentPlot->graph(graphIndex++)->setData(xAxis, mCurrMotorVec);
        ui->currentPlot->graph(graphIndex++)->setData(xAxis, mDutyVec);

        // Temperature plot
        ui->tempPlot->clearGraphs();

        graphIndex = 0;

        ui->tempPlot->addGraph();
        ui->tempPlot->graph(graphIndex)->setPen(QPen(Utility::getAppQColor("plot_graph1")));
        ui->tempPlot->graph(graphIndex)->setName("Temperature MOSFET");
        ui->tempPlot->graph(graphIndex)->setData(xAxis, mTempMosVec);
        ui->tempPlot->graph(graphIndex)->setVisible(ui->tempShowMosfetBox->isChecked());

        graphIndex++;

        if (!mTempMos1Vec.isEmpty() && mTempMos1Vec.last() != 0.0) {
            ui->tempPlot->addGraph();
            ui->tempPlot->graph(graphIndex)->setPen(QPen(Utility::getAppQColor("plot_graph2")));
            ui->tempPlot->graph(graphIndex)->setName("Temperature MOSFET 1");
            ui->tempPlot->graph(graphIndex)->setData(xAxis, mTempMos1Vec);
            ui->tempPlot->graph(graphIndex)->setVisible(ui->tempShowMosfetBox->isChecked());
            graphIndex++;

            ui->tempPlot->addGraph();
            ui->tempPlot->graph(graphIndex)->setPen(QPen(Utility::getAppQColor("plot_graph3")));
            ui->tempPlot->graph(graphIndex)->setName("Temperature MOSFET 2");
            ui->tempPlot->graph(graphIndex)->setData(xAxis, mTempMos2Vec);
            ui->tempPlot->graph(graphIndex)->setVisible(ui->tempShowMosfetBox->isChecked());
            graphIndex++;

            ui->tempPlot->addGraph();
            ui->tempPlot->graph(graphIndex)->setPen(QPen(Utility::getAppQColor("plot_graph4")));
            ui->tempPlot->graph(graphIndex)->setName("Temperature MOSFET 3");
            ui->tempPlot->graph(graphIndex)->setData(xAxis, mTempMos3Vec);
            ui->tempPlot->graph(graphIndex)->setVisible(ui->tempShowMosfetBox->isChecked());
            graphIndex++;
        }

        ui->tempPlot->addGraph(ui->tempPlot->xAxis, ui->tempPlot->yAxis2);
        ui->tempPlot->graph(graphIndex)->setPen(QPen(Utility::getAppQColor("plot_graph5")));
        ui->tempPlot->graph(graphIndex)->setName("Temperature Motor");
        ui->tempPlot->graph(graphIndex)->setData(xAxis, mTempMotorVec);
        ui->tempPlot->graph(graphIndex)->setVisible(ui->tempShowMotorBox->isChecked());
        graphIndex++;

        // RPM plot
        graphIndex = 0;
        ui->rpmPlot->graph(graphIndex++)->setData(xAxis, mRpmVec);

        // FOC plot
        graphIndex = 0;
        ui->focPlot->graph(graphIndex++)->setData(xAxis, mIdVec);
        ui->focPlot->graph(graphIndex++)->setData(xAxis, mIqVec);
        ui->focPlot->graph(graphIndex++)->setData(xAxis, mVdVec);
        ui->focPlot->graph(graphIndex++)->setData(xAxis, mVqVec);

        if (ui->autoscaleButton->isChecked()) {
            ui->currentPlot->rescaleAxes();
            ui->tempPlot->rescaleAxes();
            ui->rpmPlot->rescaleAxes();
            ui->focPlot->rescaleAxes();
        }

        ui->currentPlot->replotWhenVisible();
        ui->tempPlot->replotWhenVisible();
        ui->rpmPlot->replotWhenVisible();
        ui->focPlot->replotWhenVisible();

        mUpdateValPlot = false;
    }

    if (mUpdatePosPlot) {
        QVector<double> xAxis(mPositionVec.size());
        for (int i = 0;i < mPositionVec.size();i++) {
            xAxis[i] = double(i);
        }

        ui->posBar->setValue(int(fabs(mPositionVec.last())));
        ui->posPlot->graph(0)->setData(xAxis, mPositionVec);

        if (ui->autoscaleButton->isChecked()) {
            ui->posPlot->rescaleAxes();
        }

        ui->posPlot->replotWhenVisible();

        mUpdatePosPlot = false;
    }
}

void PageRtData::valuesReceived(MC_VALUES values, unsigned int mask)
{
    (void)mask;
    ui->rtText->setValues(values);

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
    appendDoubleAndTrunc(&mVdVec, values.vd, maxS);
    appendDoubleAndTrunc(&mVqVec, values.vq, maxS);

    qint64 tNow = QDateTime::currentMSecsSinceEpoch();

    double elapsed = double((tNow - mLastUpdateTime)) / 1000.0;
    if (elapsed > 1.0) {
        elapsed = 1.0;
    }

    mSecondCounter += elapsed;

    appendDoubleAndTrunc(&mSeconds, mSecondCounter, maxS);

    mLastUpdateTime = tNow;

    mUpdateValPlot = true;
}

void PageRtData::rotorPosReceived(double pos)
{
    appendDoubleAndTrunc(&mPositionVec, pos, 1500);
    mUpdatePosPlot = true;
}

void PageRtData::appendDoubleAndTrunc(QVector<double> *vec, double num, int maxSize)
{
    vec->append(num);

    if(vec->size() > maxSize) {
        vec->remove(0, vec->size() - maxSize);
    }
}

void PageRtData::updateZoom()
{
    Qt::Orientations plotOrientations = Qt::Orientations(
            ((ui->zoomHButton->isChecked() ? Qt::Horizontal : 0) |
             (ui->zoomVButton->isChecked() ? Qt::Vertical : 0)));

    ui->currentPlot->axisRect()->setRangeZoom(plotOrientations);
    ui->tempPlot->axisRect()->setRangeZoom(plotOrientations);
    ui->rpmPlot->axisRect()->setRangeZoom(plotOrientations);
    ui->focPlot->axisRect()->setRangeZoom(plotOrientations);
    ui->posPlot->axisRect()->setRangeZoom(plotOrientations);
}

void PageRtData::on_zoomHButton_toggled(bool checked)
{
    (void)checked;
    updateZoom();
}

void PageRtData::on_zoomVButton_toggled(bool checked)
{
    (void)checked;
    updateZoom();
}

void PageRtData::on_rescaleButton_clicked()
{
    ui->currentPlot->rescaleAxes();
    ui->tempPlot->rescaleAxes();
    ui->rpmPlot->rescaleAxes();
    ui->focPlot->rescaleAxes();
    ui->posPlot->rescaleAxes();

    ui->currentPlot->replotWhenVisible();
    ui->tempPlot->replotWhenVisible();
    ui->rpmPlot->replotWhenVisible();
    ui->focPlot->replotWhenVisible();
    ui->posPlot->replotWhenVisible();
}

void PageRtData::on_posInductanceButton_clicked()
{
    if (mVesc) {
        mVesc->commands()->setDetect(DISP_POS_MODE_INDUCTANCE);
    }
}

void PageRtData::on_posObserverButton_clicked()
{
    if (mVesc) {
        mVesc->commands()->setDetect(DISP_POS_MODE_OBSERVER);
    }
}

void PageRtData::on_posEncoderButton_clicked()
{
    if (mVesc) {
        mVesc->commands()->setDetect(DISP_POS_MODE_ENCODER);
    }
}

void PageRtData::on_posPidButton_clicked()
{
    if (mVesc) {
        mVesc->commands()->setDetect(DISP_POS_MODE_PID_POS);
    }
}

void PageRtData::on_posPidErrorButton_clicked()
{
    if (mVesc) {
        mVesc->commands()->setDetect(DISP_POS_MODE_PID_POS_ERROR);
    }
}

void PageRtData::on_posEncoderObserverErrorButton_clicked()
{
    if (mVesc) {
        mVesc->commands()->setDetect(DISP_POS_MODE_ENCODER_OBSERVER_ERROR);
    }
}

void PageRtData::on_posStopButton_clicked()
{
    if (mVesc) {
        mVesc->commands()->setDetect(DISP_POS_MODE_NONE);
    }
}

void PageRtData::on_tempShowMosfetBox_toggled(bool checked)
{
    if (ui->tempPlot->graphCount() > 0) {
        ui->tempPlot->graph(0)->setVisible(checked);
    }
}

void PageRtData::on_tempShowMotorBox_toggled(bool checked)
{
    if (ui->tempPlot->graphCount() > 1) {
        ui->tempPlot->graph(1)->setVisible(checked);
    }
}

void PageRtData::on_logRtButton_toggled(bool checked)
{
    QSettings set;
    set.sync();
    if (checked) {
        if (mVesc) {
            mVesc->openRtLogFile(set.value("path_rt_log", "./log").toString());
        }
    } else {
        mVesc->closeRtLogFile();
    }
}

void PageRtData::on_posHallObserverErrorButton_clicked()
{
    if (mVesc) {
        mVesc->commands()->setDetect(DISP_POS_MODE_HALL_OBSERVER_ERROR);
    }
}

