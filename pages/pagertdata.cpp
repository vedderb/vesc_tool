/*
    Copyright 2016 - 2019 Benjamin Vedder	benjamin@vedder.se

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
#include "widgets/helpdialog.h"

#include <QXmlStreamWriter>
#include <QXmlStreamReader>

PageRtData::PageRtData(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PageRtData)
{
    ui->setupUi(this);
    layout()->setContentsMargins(0, 0, 0, 0);
    mVesc = 0;

    mTimer = new QTimer(this);
    mTimer->start(20);

    mSecondCounter = 0.0;
    mLastUpdateTime = 0;

    mUpdateValPlot = false;
    mUpdatePosPlot = false;

    mExperimentReplot = false;
    mExperimentPlotNow = 0;

    ui->currentPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    ui->tempPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    ui->rpmPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    ui->focPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    ui->posPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    ui->experimentPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

    // Current and duty
    int graphIndex = 0;
    ui->currentPlot->addGraph();
    ui->currentPlot->graph(graphIndex)->setPen(QPen(Qt::blue));
    ui->currentPlot->graph(graphIndex)->setName("Current in");
    graphIndex++;

    ui->currentPlot->addGraph();
    ui->currentPlot->graph(graphIndex)->setPen(QPen(Qt::red));
    ui->currentPlot->graph(graphIndex)->setName("Current motor");
    graphIndex++;

    ui->currentPlot->addGraph(ui->currentPlot->xAxis, ui->currentPlot->yAxis2);
    ui->currentPlot->graph(graphIndex)->setPen(QPen(Qt::green));
    ui->currentPlot->graph(graphIndex)->setName("Duty cycle");
    graphIndex++;

    // RPM
    graphIndex = 0;
    ui->rpmPlot->addGraph();
    ui->rpmPlot->graph(graphIndex)->setPen(QPen(Qt::blue));
    ui->rpmPlot->graph(graphIndex)->setName("ERPM");
    graphIndex++;

    // FOC
    graphIndex = 0;
    ui->focPlot->addGraph();
    ui->focPlot->graph(graphIndex)->setPen(QPen(Qt::blue));
    ui->focPlot->graph(graphIndex)->setName("D Current");
    graphIndex++;

    ui->focPlot->addGraph();
    ui->focPlot->graph(graphIndex)->setPen(QPen(Qt::red));
    ui->focPlot->graph(graphIndex)->setName("Q Current");
    graphIndex++;

    ui->focPlot->addGraph(ui->focPlot->xAxis, ui->focPlot->yAxis2);
    ui->focPlot->graph(graphIndex)->setPen(QPen(Qt::green));
    ui->focPlot->graph(graphIndex)->setName("D Voltage");
    graphIndex++;

    ui->focPlot->addGraph(ui->focPlot->xAxis, ui->focPlot->yAxis2);
    ui->focPlot->graph(graphIndex)->setPen(QPen(Qt::darkGreen));
    ui->focPlot->graph(graphIndex)->setName("Q Voltage");
    graphIndex++;

    QFont legendFont = font();
    legendFont.setPointSize(9);

    ui->currentPlot->legend->setVisible(true);
    ui->currentPlot->legend->setFont(legendFont);
    ui->currentPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignRight|Qt::AlignBottom);
    ui->currentPlot->legend->setBrush(QBrush(QColor(255,255,255,230)));
    ui->currentPlot->xAxis->setLabel("Seconds (s)");
    ui->currentPlot->yAxis->setLabel("Ampere (A)");
    ui->currentPlot->yAxis2->setLabel("Duty Cycle");

    ui->tempPlot->legend->setVisible(true);
    ui->tempPlot->legend->setFont(legendFont);
    ui->tempPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignRight|Qt::AlignBottom);
    ui->tempPlot->legend->setBrush(QBrush(QColor(255,255,255,230)));
    ui->tempPlot->xAxis->setLabel("Seconds (s)");
    ui->tempPlot->yAxis->setLabel("Temperature MOSFET (\u00B0C)");
    ui->tempPlot->yAxis2->setLabel("Temperature Motor (\u00B0C)");

    ui->rpmPlot->legend->setVisible(true);
    ui->rpmPlot->legend->setFont(legendFont);
    ui->rpmPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignRight|Qt::AlignBottom);
    ui->rpmPlot->legend->setBrush(QBrush(QColor(255,255,255,230)));
    ui->rpmPlot->xAxis->setLabel("Seconds (s)");
    ui->rpmPlot->yAxis->setLabel("ERPM");

    ui->focPlot->legend->setVisible(true);
    ui->focPlot->legend->setFont(legendFont);
    ui->focPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignRight|Qt::AlignBottom);
    ui->focPlot->legend->setBrush(QBrush(QColor(255,255,255,230)));
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

    ui->posPlot->addGraph();
    ui->posPlot->graph(0)->setPen(QPen(Qt::blue));
    ui->posPlot->graph(0)->setName("Position");
    ui->posPlot->legend->setVisible(true);
    ui->posPlot->legend->setFont(legendFont);
    ui->posPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignRight|Qt::AlignBottom);
    ui->posPlot->legend->setBrush(QBrush(QColor(255,255,255,230)));
    ui->posPlot->xAxis->setLabel("Sample");
    ui->posPlot->yAxis->setLabel("Degrees");

    // Experiment
    ui->experimentPlot->xAxis->grid()->setSubGridVisible(true);
    ui->experimentPlot->yAxis->grid()->setSubGridVisible(true);

    connect(ui->experimentGraph1Button, &QPushButton::toggled,
            [=]() {mExperimentReplot = true;});
    connect(ui->experimentGraph2Button, &QPushButton::toggled,
            [=]() {mExperimentReplot = true;});
    connect(ui->experimentGraph3Button, &QPushButton::toggled,
            [=]() {mExperimentReplot = true;});
    connect(ui->experimentGraph4Button, &QPushButton::toggled,
            [=]() {mExperimentReplot = true;});
    connect(ui->experimentGraph5Button, &QPushButton::toggled,
            [=]() {mExperimentReplot = true;});
    connect(ui->experimentScatterButton, &QPushButton::toggled,
            [=]() {mExperimentReplot = true;});

    connect(ui->experimentHZoomButton, &QPushButton::toggled,
            [=]() {
        Qt::Orientations plotOrientations = Qt::Orientations(
                ((ui->experimentHZoomButton->isChecked() ? Qt::Horizontal : 0) |
                 (ui->experimentVZoomButton->isChecked() ? Qt::Vertical : 0)));
        ui->experimentPlot->axisRect()->setRangeZoom(plotOrientations);
    });

    connect(ui->experimentVZoomButton, &QPushButton::toggled,
            [=]() {
        Qt::Orientations plotOrientations = Qt::Orientations(
                ((ui->experimentHZoomButton->isChecked() ? Qt::Horizontal : 0) |
                 (ui->experimentVZoomButton->isChecked() ? Qt::Vertical : 0)));
        ui->experimentPlot->axisRect()->setRangeZoom(plotOrientations);
    });

    connect(mTimer, SIGNAL(timeout()),
            this, SLOT(timerSlot()));

    QSettings set;
    if (set.contains("pagertdata/lastcsvfile")) {
        ui->csvFileEdit->setText(set.value("pagertdata/lastcsvfile").toString());
    }
}

PageRtData::~PageRtData()
{
    QSettings set;
    set.setValue("pagertdata/lastcsvfile", ui->csvFileEdit->text());
    delete ui;
}

VescInterface *PageRtData::vesc() const
{
    return mVesc;
}

void PageRtData::setVesc(VescInterface *vesc)
{
    mVesc = vesc;

    if (mVesc) {
        connect(mVesc->commands(), SIGNAL(valuesReceived(MC_VALUES,unsigned int)),
                this, SLOT(valuesReceived(MC_VALUES, unsigned int)));
        connect(mVesc->commands(), SIGNAL(rotorPosReceived(double)),
                this, SLOT(rotorPosReceived(double)));
        connect(mVesc->commands(), SIGNAL(plotInitReceived(QString,QString)),
                this, SLOT(plotInitReceived(QString,QString)));
        connect(mVesc->commands(), SIGNAL(plotDataReceived(double,double)),
                this, SLOT(plotDataReceived(double,double)));
        connect(mVesc->commands(), SIGNAL(plotAddGraphReceived(QString)),
                this, SLOT(plotAddGraphReceived(QString)));
        connect(mVesc->commands(), SIGNAL(plotSetGraphReceived(int)),
                this, SLOT(plotSetGraphReceived(int)));
    }
}

void PageRtData::timerSlot()
{
    if (mVesc) {
        if (mVesc->isRtLogOpen() != ui->csvEnableLogBox->isChecked()) {
            ui->csvEnableLogBox->setChecked(mVesc->isRtLogOpen());
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
        ui->tempPlot->graph(graphIndex)->setPen(QPen(Qt::blue));
        ui->tempPlot->graph(graphIndex)->setName("Temperature MOSFET");
        ui->tempPlot->graph(graphIndex)->setData(xAxis, mTempMosVec);
        graphIndex++;

        if (!mTempMos1Vec.isEmpty() && mTempMos1Vec.last() != 0.0) {
            ui->tempPlot->addGraph();
            ui->tempPlot->graph(graphIndex)->setPen(QPen(Qt::green));
            ui->tempPlot->graph(graphIndex)->setName("Temperature MOSFET 1");
            ui->tempPlot->graph(graphIndex)->setData(xAxis, mTempMos1Vec);
            graphIndex++;

            ui->tempPlot->addGraph();
            ui->tempPlot->graph(graphIndex)->setPen(QPen(Qt::darkGreen));
            ui->tempPlot->graph(graphIndex)->setName("Temperature MOSFET 2");
            ui->tempPlot->graph(graphIndex)->setData(xAxis, mTempMos2Vec);
            graphIndex++;

            ui->tempPlot->addGraph();
            ui->tempPlot->graph(graphIndex)->setPen(QPen(Qt::cyan));
            ui->tempPlot->graph(graphIndex)->setName("Temperature MOSFET 3");
            ui->tempPlot->graph(graphIndex)->setData(xAxis, mTempMos3Vec);
            graphIndex++;
        }

        ui->tempPlot->addGraph(ui->tempPlot->xAxis, ui->tempPlot->yAxis2);
        ui->tempPlot->graph(graphIndex)->setPen(QPen(Qt::magenta));
        ui->tempPlot->graph(graphIndex)->setName("Temperature Motor");
        ui->tempPlot->graph(graphIndex)->setData(xAxis, mTempMotorVec);
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

        ui->currentPlot->replot();
        ui->tempPlot->replot();
        ui->rpmPlot->replot();
        ui->focPlot->replot();

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

        ui->posPlot->replot();

        mUpdatePosPlot = false;
    }

    if (mExperimentReplot) {
        ui->experimentPlot->clearGraphs();

        for (int i = 0;i < mExperimentPlots.size();i++) {
            switch (i) {
            case 0: if (!ui->experimentGraph1Button->isChecked()) {continue;} break;
            case 1: if (!ui->experimentGraph2Button->isChecked()) {continue;} break;
            case 2: if (!ui->experimentGraph3Button->isChecked()) {continue;} break;
            case 3: if (!ui->experimentGraph4Button->isChecked()) {continue;} break;
            case 4: if (!ui->experimentGraph5Button->isChecked()) {continue;} break;
            default: break;
            }

            ui->experimentPlot->addGraph();
            ui->experimentPlot->graph()->setData(mExperimentPlots.at(i).xData, mExperimentPlots.at(i).yData);
            ui->experimentPlot->graph()->setName(mExperimentPlots.at(i).label);
            ui->experimentPlot->graph()->setPen(QPen(mExperimentPlots.at(i).color));
            if (ui->experimentScatterButton->isChecked()) {
                ui->experimentPlot->graph()->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 5));
            }
        }

        ui->experimentPlot->legend->setVisible(mExperimentPlots.size() > 1);

        if (ui->experimentAutoScaleButton->isChecked()) {
            ui->experimentPlot->rescaleAxes();
        }

        ui->experimentPlot->replot();
        mExperimentReplot = false;
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

void PageRtData::plotInitReceived(QString xLabel, QString yLabel)
{
    mExperimentPlots.clear();

    ui->experimentPlot->clearGraphs();
    ui->experimentPlot->xAxis->setLabel(xLabel);
    ui->experimentPlot->yAxis->setLabel(yLabel);

    mExperimentReplot = true;
}

void PageRtData::plotDataReceived(double x, double y)
{
    if (mExperimentPlots.size() <= mExperimentPlotNow) {
        mExperimentPlots.resize(mExperimentPlotNow + 1);
    }

    mExperimentPlots[mExperimentPlotNow].xData.append(x);
    mExperimentPlots[mExperimentPlotNow].yData.append(y);
    mExperimentReplot = true;
}

void PageRtData::plotAddGraphReceived(QString name)
{
    mExperimentPlots.resize(mExperimentPlots.size() + 1);
    mExperimentPlots.last().label = name;

    if (mExperimentPlots.size() == 1) {
        mExperimentPlots.last().color = "blue";
    } else if (mExperimentPlots.size() == 2) {
        mExperimentPlots.last().color = "red";
    } else if (mExperimentPlots.size() == 3) {
        mExperimentPlots.last().color = "magenta";
    } else if (mExperimentPlots.size() == 4) {
        mExperimentPlots.last().color = "darkgreen";
    } else if (mExperimentPlots.size() == 5) {
        mExperimentPlots.last().color = "cyan";
    } else {
        mExperimentPlots.last().color = "blue";
    }

    mExperimentReplot = true;
}

void PageRtData::plotSetGraphReceived(int graph)
{
    mExperimentPlotNow = graph;
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

    ui->currentPlot->replot();
    ui->tempPlot->replot();
    ui->rpmPlot->replot();
    ui->focPlot->replot();
    ui->posPlot->replot();
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
    ui->tempPlot->graph(0)->setVisible(checked);
}

void PageRtData::on_tempShowMotorBox_toggled(bool checked)
{
    ui->tempPlot->graph(1)->setVisible(checked);
}

void PageRtData::on_csvChooseDirButton_clicked()
{
    ui->csvFileEdit->setText(QFileDialog::getExistingDirectory(this,
                                                               "Choose CSV output directory"));
}

void PageRtData::on_csvEnableLogBox_clicked(bool checked)
{
    if (checked) {
        if (mVesc) {
            mVesc->openRtLogFile(ui->csvFileEdit->text());
        }
    } else {
        mVesc->closeRtLogFile();
    }
}

void PageRtData::on_csvHelpButton_clicked()
{
    HelpDialog::showHelp(this, mVesc->infoConfig(), "help_rt_logging");
}

void PageRtData::on_experimentLoadXmlButton_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Load Plot"), "",
                                                    tr("Xml files (*.xml)"));

    if (!filename.isEmpty()) {
        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::critical(this, "Load Plot",
                                  "Could not open\n" + filename + "\nfor reading");
            return;
        }

        QXmlStreamReader stream(&file);

        // Look for plot tag
        bool plots_found = false;
        while (stream.readNextStartElement()) {
            if (stream.name() == "plot") {
                plots_found = true;
                break;
            }
        }

        if (plots_found) {
            mExperimentPlots.clear();

            while (stream.readNextStartElement()) {
                QString name = stream.name().toString();

                if (name == "xlabel") {
                    ui->experimentPlot->xAxis->setLabel(stream.readElementText());
                } else if (name == "ylabel") {
                    ui->experimentPlot->yAxis->setLabel(stream.readElementText());
                } else if (name == "graph") {
                    EXPERIMENT_PLOT p;

                    while (stream.readNextStartElement()) {
                        QString name2 = stream.name().toString();

                        if (name2 == "label") {
                            p.label = stream.readElementText();
                        } else if (name2 == "color") {
                            p.color = stream.readElementText();
                        } else if (name2 == "point") {
                            while (stream.readNextStartElement()) {
                                QString name3 = stream.name().toString();

                                if (name3 == "x") {
                                    p.xData.append(stream.readElementText().toDouble());
                                } else if (name3 == "y") {
                                    p.yData.append(stream.readElementText().toDouble());
                                } else {
                                    qWarning() << ": Unknown XML element :" << name2;
                                    stream.skipCurrentElement();
                                }
                            }
                        } else {
                            qWarning() << ": Unknown XML element :" << name2;
                            stream.skipCurrentElement();
                        }

                        if (stream.hasError()) {
                            qWarning() << " : XML ERROR :" << stream.errorString();
                        }
                    }

                    mExperimentPlots.append(p);
                }

                if (stream.hasError()) {
                    qWarning() << "XML ERROR :" << stream.errorString();
                    qWarning() << stream.lineNumber() << stream.columnNumber();
                }
            }

            mExperimentReplot = true;

            file.close();
            if (mVesc) {
                mVesc->emitStatusMessage("Loaded plot", true);
            }
        } else {
            QMessageBox::critical(this, "Load Plot",
                                  "plot tag not found in " + filename);
        }
    }
}

void PageRtData::on_experimentSaveXmlButton_clicked()
{
    QString filename = QFileDialog::getSaveFileName(this,
                                                    tr("Save Plot"), "",
                                                    tr("Xml files (*.xml)"));

    if (filename.isEmpty()) {
        return;
    }

    if (!filename.toLower().endsWith(".xml")) {
        filename.append(".xml");
    }

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, "Save Plot",
                              "Could not open\n" + filename + "\nfor writing");
        return;
    }

    QXmlStreamWriter stream(&file);
    stream.setCodec("UTF-8");
    stream.setAutoFormatting(true);
    stream.writeStartDocument();

    stream.writeStartElement("plot");
    stream.writeTextElement("xlabel", ui->experimentPlot->xAxis->label());
    stream.writeTextElement("ylabel", ui->experimentPlot->yAxis->label());

    for (EXPERIMENT_PLOT p: mExperimentPlots) {
        stream.writeStartElement("graph");
        stream.writeTextElement("label", p.label);
        stream.writeTextElement("color", p.color);
        for (int i = 0;i < p.xData.size();i++) {
            stream.writeStartElement("point");
            stream.writeTextElement("x", QString::number(p.xData.at(i)));
            stream.writeTextElement("y", QString::number(p.yData.at(i)));
            stream.writeEndElement();
        }
        stream.writeEndElement();
    }

    stream.writeEndDocument();
    file.close();
}

void PageRtData::on_experimentSavePngButton_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("Save Image"), "",
                                                    tr("PNG Files (*.png)"));

    if (!fileName.isEmpty()) {
        if (!fileName.toLower().endsWith(".png")) {
            fileName.append(".png");
        }

        ui->experimentPlot->savePng(fileName,
                                    ui->experimentWBox->value(),
                                    ui->experimentHBox->value(),
                                    ui->experimentScaleBox->value());
    }
}

void PageRtData::on_experimentSavePdfButton_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("Save PDF"), "",
                                                    tr("PDF Files (*.pdf)"));

    if (!fileName.isEmpty()) {
        if (!fileName.toLower().endsWith(".pdf")) {
            fileName.append(".pdf");
        }

        ui->experimentPlot->savePdf(fileName,
                                    ui->experimentWBox->value(),
                                    ui->experimentHBox->value());
    }
}
