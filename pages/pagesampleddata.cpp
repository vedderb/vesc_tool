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

#include "pagesampleddata.h"
#include "ui_pagesampleddata.h"
#include "digitalfiltering.h"

PageSampledData::PageSampledData(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PageSampledData)
{
    ui->setupUi(this);

    layout()->setContentsMargins(0, 0, 0, 0);
    mVesc = 0;

    mDoReplot = false;
    mDoFilterReplot = false;
    mDoRescale = false;
    mSamplesToWait = 0; // TODO: Use timeout instead?

    mTimer = new QTimer(this);
    mTimer->start(20);

    ui->currentPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    ui->voltagePlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    ui->filterPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    ui->filterResponsePlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

    connect(mTimer, SIGNAL(timeout()),
            this, SLOT(timerSlot()));

    connect(ui->compDelayBox, SIGNAL(toggled(bool)), this, SLOT(replotAll()));
    connect(ui->currentFilterFreqBox, SIGNAL(valueChanged(double)), this, SLOT(replotAll()));
    connect(ui->currentFilterTapBox, SIGNAL(valueChanged(int)), this, SLOT(replotAll()));
    connect(ui->plotModeBox, SIGNAL(currentIndexChanged(int)), this, SLOT(replotAll()));
    connect(ui->filterBox, SIGNAL(currentIndexChanged(int)), this, SLOT(replotAll()));
    connect(ui->filterScatterBox, SIGNAL(toggled(bool)), this, SLOT(replotAll()));
    connect(ui->hammingBox, SIGNAL(toggled(bool)), this, SLOT(replotAll()));
    connect(ui->showCurrent1Box, SIGNAL(toggled(bool)), this, SLOT(replotAll()));
    connect(ui->showCurrent2Box, SIGNAL(toggled(bool)), this, SLOT(replotAll()));
    connect(ui->showCurrent3Box, SIGNAL(toggled(bool)), this, SLOT(replotAll()));
    connect(ui->showMcTotalCurrentBox, SIGNAL(toggled(bool)), this, SLOT(replotAll()));
    connect(ui->showPh1Box, SIGNAL(toggled(bool)), this, SLOT(replotAll()));
    connect(ui->showPh2Box, SIGNAL(toggled(bool)), this, SLOT(replotAll()));
    connect(ui->showPh3Box, SIGNAL(toggled(bool)), this, SLOT(replotAll()));
    connect(ui->showPosCurrentBox, SIGNAL(toggled(bool)), this, SLOT(replotAll()));
    connect(ui->showPosVoltageBox, SIGNAL(toggled(bool)), this, SLOT(replotAll()));
    connect(ui->showVirtualGndBox, SIGNAL(toggled(bool)), this, SLOT(replotAll()));
    connect(ui->showPhaseBox, SIGNAL(toggled(bool)), this, SLOT(replotAll()));
    connect(ui->showPhaseVoltageBox, SIGNAL(toggled(bool)), this, SLOT(replotAll()));
    connect(ui->truncateBox, SIGNAL(toggled(bool)), this, SLOT(replotAll()));

    replotAll();
}

PageSampledData::~PageSampledData()
{
    delete ui;
}

VescInterface *PageSampledData::vesc() const
{
    return mVesc;
}

void PageSampledData::setVesc(VescInterface *vesc)
{
    mVesc = vesc;

    if (mVesc) {
        connect(mVesc->commands(), SIGNAL(samplesReceived(QByteArray)),
                this, SLOT(samplesReceived(QByteArray)));
    }
}

void PageSampledData::timerSlot()
{
    static QVector<double> filter;
    QFont legendFont = font();
    legendFont.setPointSize(9);

    // Plot filters
    if (mDoFilterReplot) {
        if (ui->filterBox->currentIndex() == 2) {
            filter.clear();
            int fLen = (1 << ui->currentFilterTapBox->value());
            for (int i = 0;i < fLen;i++) {
                filter.append(1.0 / (double)fLen);
            }
        } else {
            filter = DigitalFiltering::generateFirFilter(ui->currentFilterFreqBox->value(),
                                                         ui->currentFilterTapBox->value(), ui->hammingBox->isChecked());
        }

        static int last_len = 0;
        bool len_diff = last_len != ui->currentFilterTapBox->value();
        last_len = ui->currentFilterTapBox->value();

        // Plot filter
        QVector<double> filterIndex;
        for(int i = 0;i < filter.size();i++) {
            filterIndex.append((double)i);
        }

        ui->filterPlot->clearGraphs();
        ui->filterResponsePlot->clearGraphs();

        ui->filterPlot->addGraph();
        ui->filterPlot->graph(0)->setData(filterIndex, filter);
        ui->filterPlot->graph(0)->setName("Filter");

        if (ui->filterScatterBox->isChecked()) {
            ui->filterPlot->graph(0)->setLineStyle(QCPGraph::lsLine);
            ui->filterPlot->graph(0)->setScatterStyle(QCPScatterStyle::ssCircle);
        }

        // Plot response
        QVector<double> response = DigitalFiltering::fftWithShift(filter, ui->currentFilterTapBox->value() + 4);

        // Remove positive half
        response.resize(response.size() / 2);

        filterIndex.clear();
        for(int i = 0;i < response.size();i++) {
            filterIndex.append(((double)i / (double)response.size()) * (double)ui->fftFreqBox->value() / 2);
        }

        ui->filterResponsePlot->addGraph();
        ui->filterResponsePlot->graph(0)->setData(filterIndex, response);
        ui->filterResponsePlot->graph(0)->setName("Filter Response");
        ui->filterResponsePlot->graph(0)->setPen(QPen(Qt::red));

        ui->filterPlot->legend->setVisible(true);
        ui->filterPlot->legend->setFont(legendFont);
        ui->filterPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignRight|Qt::AlignBottom);
        ui->filterPlot->legend->setBrush(QBrush(QColor(255,255,255,230)));
        ui->filterPlot->xAxis->setLabel("Index");
        ui->filterPlot->yAxis->setLabel("Value");

        ui->filterResponsePlot->legend->setVisible(true);
        ui->filterResponsePlot->legend->setFont(legendFont);
        ui->filterResponsePlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignRight|Qt::AlignBottom);
        ui->filterResponsePlot->legend->setBrush(QBrush(QColor(255,255,255,230)));
        ui->filterResponsePlot->xAxis->setLabel("Frequency (Hz)");
        ui->filterResponsePlot->yAxis->setLabel("Gain");

        if (len_diff) {
            ui->filterPlot->rescaleAxes();
            ui->filterResponsePlot->rescaleAxes();
        }

        ui->filterPlot->replot();
        ui->filterResponsePlot->replot();

        mDoFilterReplot = false;
    }

    if (mDoReplot) {
        const double f_samp = (ui->fftFreqBox->value() / ui->decimationBox->value());
        int size = curr1Vector.size();

        if(size > 0) {
            QVector<double> position(size);
            for (int i=0;i < size;i++) {
                position[i] = (double)((quint8)statusArray.at(i) & 7);
            }

            QVector<double> position_hall(size);
            for (int i=0;i < size;i++) {
                position_hall[i] = (double)((quint8)(statusArray.at(i) >> 3) & 7) / 1.0;
            }

            QVector<double> phase(size);
            for (int i=0;i < size;i++) {
                phase[i] = (double)((quint8)phaseArray.at(i)) / 250.0 * 360.0;
            }

            // Calculate current and voltages
            QVector<double> curr1 = curr1Vector;
            QVector<double> curr2 = curr2Vector;
            QVector<double> curr3(size);

            QVector<double> ph1 = ph1Vector;
            QVector<double> ph2 = ph2Vector;
            QVector<double> ph3 = ph3Vector;
            QVector<double> vZero = vZeroVector;

            QVector<double> totCurrentMc = currTotVector;
            QVector<double> fSw = fSwVector;

            for (int i=0;i < curr2.size(); i++) {
                curr3[i] = -(curr1[i] + curr2[i]);

                if (ui->truncateBox->isChecked()) {
                    if (!(position[i] == 1 || position[i] == 4)) {
                        ph1[i] = 0;
                    }

                    if (!(position[i] == 2 || position[i] == 5)) {
                        ph2[i] = 0;
                    }

                    if (!(position[i] == 3 || position[i] == 6)) {
                        ph3[i] = 0;
                    }
                }
            }

            // Filter currents
            if (ui->filterBox->currentIndex() > 0) {
                curr1 = DigitalFiltering::filterSignal(curr1, filter, ui->compDelayBox->isChecked());
                curr2 = DigitalFiltering::filterSignal(curr2, filter, ui->compDelayBox->isChecked());
                curr3 = DigitalFiltering::filterSignal(curr3, filter, ui->compDelayBox->isChecked());
            }

            bool showFft = ui->plotModeBox->currentIndex() == 1;
            static bool lastSpectrum = false;
            bool spectrumChanged = showFft != lastSpectrum;
            lastSpectrum = showFft;

            // Filtered x-axis vector for currents
            QVector<double> xAxisCurrDec;
            QVector<double> xAxisCurr;

            // Use DFT
            // TODO: The transform only makes sense with a constant sampling frequency right now. Some
            // weird scaling should be implemented.
            if (showFft) {
                int fftBits = 16;

                curr1 = DigitalFiltering::fftWithShift(curr1, fftBits, true);
                curr2 = DigitalFiltering::fftWithShift(curr2, fftBits, true);
                curr3 = DigitalFiltering::fftWithShift(curr3, fftBits, true);
                totCurrentMc = DigitalFiltering::fftWithShift(totCurrentMc, fftBits, true);

                curr1.resize(curr1.size() / 2);
                curr2.resize(curr2.size() / 2);
                curr3.resize(curr3.size() / 2);
                totCurrentMc.resize(totCurrentMc.size() / 2);

                // Resize x-axis
                xAxisCurrDec.resize(curr1.size());
                xAxisCurr.resize(totCurrentMc.size());

                // Generate Filtered X-axis
                for (int i = 0;i < xAxisCurrDec.size();i++) {
                    xAxisCurrDec[i] = ((double)i / (double)xAxisCurrDec.size()) * (f_samp / 2.0);
                }

                for (int i = 0;i < xAxisCurr.size();i++) {
                    xAxisCurr[i] = ((double)i / (double)xAxisCurr.size()) * (f_samp / 2);
                }
            } else {
                // Resize x-axis
                xAxisCurrDec.resize(curr1.size());
                xAxisCurr.resize(totCurrentMc.size());

                // Generate X axis
                double prev_x = 0.0;
                double rat = (double)fSw.size() / (double)xAxisCurrDec.size();
                for (int i = 0;i < xAxisCurrDec.size();i++) {
                    xAxisCurrDec[i] = prev_x;
                    prev_x += 1.0 / fSw[(int)((double)i * rat)];
                }

                prev_x = 0.0;
                rat = (double)fSw.size() / (double)xAxisCurr.size();
                for (int i = 0;i < xAxisCurr.size();i++) {
                    xAxisCurr[i] = prev_x;
                    prev_x += 1.0 / fSw[(int)((double)i * rat)];
                }
            }

            QVector<double> xAxisVolt(ph1.size());
            double prev_x = 0.0;
            for (int i = 0;i < xAxisVolt.size();i++) {
                xAxisVolt[i] = prev_x;
                prev_x += 1.0 / fSw[i];
            }

            ui->currentPlot->clearGraphs();
            ui->voltagePlot->clearGraphs();

            QPen phasePen;
            phasePen.setStyle(Qt::DotLine);
            phasePen.setColor(Qt::blue);

            QPen phasePen2;
            phasePen2.setStyle(Qt::DotLine);
            phasePen2.setColor(Qt::red);

            int graphIndex = 0;

            if (ui->showCurrent1Box->isChecked()) {
                ui->currentPlot->addGraph();
                ui->currentPlot->graph(graphIndex)->setPen(QPen(Qt::magenta));
                ui->currentPlot->graph(graphIndex)->setData(xAxisCurrDec, curr1);
                ui->currentPlot->graph(graphIndex)->setName("Phase 1 Current");
                graphIndex++;
            }

            if (ui->showCurrent2Box->isChecked()) {
                ui->currentPlot->addGraph();
                ui->currentPlot->graph(graphIndex)->setData(xAxisCurrDec, curr2);
                ui->currentPlot->graph(graphIndex)->setPen(QPen(Qt::red));
                ui->currentPlot->graph(graphIndex)->setName("Phase 2 Current");
                graphIndex++;
            }

            if (ui->showCurrent3Box->isChecked()) {
                ui->currentPlot->addGraph();
                ui->currentPlot->graph(graphIndex)->setData(xAxisCurrDec, curr3);
                ui->currentPlot->graph(graphIndex)->setPen(QPen(Qt::green));
                ui->currentPlot->graph(graphIndex)->setName("Phase 3 Current");
                graphIndex++;
            }

            if (ui->showMcTotalCurrentBox->isChecked()) {
                ui->currentPlot->addGraph();
                ui->currentPlot->graph(graphIndex)->setData(xAxisCurr, totCurrentMc);
                ui->currentPlot->graph(graphIndex)->setPen(QPen(Qt::blue));
                ui->currentPlot->graph(graphIndex)->setName("Total current filtered by MC");
                graphIndex++;
            }

            if (ui->showPosCurrentBox->isChecked() && !showFft) {
                ui->currentPlot->addGraph();
                ui->currentPlot->graph(graphIndex)->setData(xAxisCurr, position);
                ui->currentPlot->graph(graphIndex)->setPen(phasePen);
                ui->currentPlot->graph(graphIndex)->setName("Current position");
                graphIndex++;
            }

            if (ui->showPhaseBox->isChecked()) {
                ui->currentPlot->addGraph(ui->currentPlot->xAxis, ui->currentPlot->yAxis2);
                ui->currentPlot->graph(graphIndex)->setData(xAxisVolt, phase);
                ui->currentPlot->graph(graphIndex)->setPen(QPen(Qt::black));
                ui->currentPlot->graph(graphIndex)->setName("FOC motor phase");
                graphIndex++;
            }

            graphIndex = 0;

            if (ui->showPh1Box->isChecked()) {
                ui->voltagePlot->addGraph();
                ui->voltagePlot->graph(graphIndex)->setData(xAxisVolt, ph1);
                ui->voltagePlot->graph(graphIndex)->setPen(QPen(Qt::blue));
                ui->voltagePlot->graph(graphIndex)->setName("Phase 1 voltage");
                graphIndex++;
            }

            if (ui->showPh2Box->isChecked()) {
                ui->voltagePlot->addGraph();
                ui->voltagePlot->graph(graphIndex)->setData(xAxisVolt, ph2);
                ui->voltagePlot->graph(graphIndex)->setPen(QPen(Qt::red));
                ui->voltagePlot->graph(graphIndex)->setName("Phase 2 voltage");
                graphIndex++;
            }

            if (ui->showPh3Box->isChecked()) {
                ui->voltagePlot->addGraph();
                ui->voltagePlot->graph(graphIndex)->setData(xAxisVolt, ph3);
                ui->voltagePlot->graph(graphIndex)->setPen(QPen(Qt::green));
                ui->voltagePlot->graph(graphIndex)->setName("Phase 3 voltage");
                graphIndex++;
            }

            if (ui->showVirtualGndBox->isChecked()) {
                ui->voltagePlot->addGraph();
                ui->voltagePlot->graph(graphIndex)->setData(xAxisVolt, vZero);
                ui->voltagePlot->graph(graphIndex)->setPen(QPen(Qt::magenta));
                ui->voltagePlot->graph(graphIndex)->setName("Virtual ground");
                graphIndex++;
            }

            if (ui->showPosVoltageBox->isChecked()) {
                ui->voltagePlot->addGraph();
                ui->voltagePlot->graph(graphIndex)->setData(xAxisVolt, position);
                ui->voltagePlot->graph(graphIndex)->setPen(phasePen);
                ui->voltagePlot->graph(graphIndex)->setName("Current position");
                graphIndex++;

                ui->voltagePlot->addGraph();
                ui->voltagePlot->graph(graphIndex)->setData(xAxisVolt, position_hall);
                ui->voltagePlot->graph(graphIndex)->setPen(phasePen2);
                ui->voltagePlot->graph(graphIndex)->setName("Hall position");
                graphIndex++;
            }

            if (ui->showPhaseVoltageBox->isChecked()) {
                ui->voltagePlot->addGraph(ui->voltagePlot->xAxis, ui->voltagePlot->yAxis2);
                ui->voltagePlot->graph(graphIndex)->setData(xAxisVolt, phase);
                ui->voltagePlot->graph(graphIndex)->setPen(QPen(Qt::black));
                ui->voltagePlot->graph(graphIndex)->setName("FOC motor phase");
                graphIndex++;
            }

            // Plot settings
            ui->currentPlot->legend->setVisible(true);
            ui->currentPlot->legend->setFont(legendFont);
            ui->currentPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignRight|Qt::AlignBottom);
            ui->currentPlot->legend->setBrush(QBrush(QColor(255,255,255,230)));
            if (showFft) {
                ui->currentPlot->xAxis->setLabel("Frequency (Hz)");
                ui->currentPlot->yAxis->setLabel("Amplitude");
            } else {
                ui->currentPlot->xAxis->setLabel("Seconds (s)");
                ui->currentPlot->yAxis->setLabel("Amperes (A)");

                if (ui->showPhaseBox->isChecked()) {
                    ui->currentPlot->yAxis2->setLabel("Motor Phase (Degrees)");
                    ui->currentPlot->yAxis2->setVisible(true);
                } else {
                    ui->currentPlot->yAxis2->setVisible(false);
                }
            }

            ui->voltagePlot->legend->setVisible(true);
            ui->voltagePlot->legend->setFont(legendFont);
            ui->voltagePlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignRight|Qt::AlignBottom);
            ui->voltagePlot->legend->setBrush(QBrush(QColor(255,255,255,230)));
            ui->voltagePlot->xAxis->setLabel("Seconds (s)");
            ui->voltagePlot->yAxis->setLabel("Volts (V)");

            if (ui->showPhaseVoltageBox->isChecked()) {
                ui->voltagePlot->yAxis2->setLabel("Motor Phase (Degrees)");
                ui->voltagePlot->yAxis2->setVisible(true);
            } else {
                ui->voltagePlot->yAxis2->setVisible(false);
            }

            if (mDoRescale || spectrumChanged) {
                ui->currentPlot->rescaleAxes();
                ui->voltagePlot->rescaleAxes();
            }

            ui->currentPlot->replot();
            ui->voltagePlot->replot();

        }

        mDoReplot = false;
        mDoRescale = false;
    }
}

void PageSampledData::samplesReceived(QByteArray bytes)
{
    VByteArray vb(bytes);

    tmpCurr1Vector.append(vb.vbPopFrontDouble32Auto());
    tmpCurr2Vector.append(vb.vbPopFrontDouble32Auto());
    tmpPh1Vector.append(vb.vbPopFrontDouble32Auto());
    tmpPh2Vector.append(vb.vbPopFrontDouble32Auto());
    tmpPh3Vector.append(vb.vbPopFrontDouble32Auto());
    tmpVZeroVector.append(vb.vbPopFrontDouble32Auto());
    tmpCurrTotVector.append(vb.vbPopFrontDouble32Auto());
    tmpFSwVector.append(vb.vbPopFrontDouble32Auto());
    tmpStatusArray.append(vb.vbPopFrontInt8());
    tmpPhaseArray.append(vb.vbPopFrontInt8());

    double prog = (double)tmpCurr1Vector.size() / (double)mSamplesToWait;
    ui->sampProgBar->setValue(prog * 100.0);

    if (tmpCurr1Vector.size() == mSamplesToWait) {
        curr1Vector = tmpCurr1Vector;
        curr2Vector = tmpCurr2Vector;
        ph1Vector = tmpPh1Vector;
        ph2Vector = tmpPh2Vector;
        ph3Vector = tmpPh3Vector;
        vZeroVector = tmpVZeroVector;
        currTotVector = tmpCurrTotVector;
        fSwVector = tmpFSwVector;
        statusArray = tmpStatusArray;
        phaseArray = tmpPhaseArray;

        mDoReplot = true;
        mDoFilterReplot = true;
        mDoRescale = true;
    }
}

void PageSampledData::replotAll()
{
    mDoReplot = true;
    mDoFilterReplot = true;
}

void PageSampledData::on_sampleNowButton_clicked()
{
    if (mVesc) {
        clearBuffers();
        mVesc->commands()->samplePrint(DEBUG_SAMPLING_NOW, ui->samplesBox->value(), ui->decimationBox->value());
        mSamplesToWait = ui->samplesBox->value();
    }
}

void PageSampledData::on_sampleStartButton_clicked()
{
    if (mVesc) {
        clearBuffers();
        mVesc->commands()->samplePrint(DEBUG_SAMPLING_START, ui->samplesBox->value(), ui->decimationBox->value());
        mSamplesToWait = ui->samplesBox->value();
    }
}

void PageSampledData::on_sampleTriggerStartButton_clicked()
{
    if (mVesc) {
        clearBuffers();
        mVesc->commands()->samplePrint(DEBUG_SAMPLING_TRIGGER_START, ui->samplesBox->value(), ui->decimationBox->value());
        mSamplesToWait = ui->samplesBox->maximum();
    }
}

void PageSampledData::on_sampleTriggerFaultButton_clicked()
{
    if (mVesc) {
        clearBuffers();
        mVesc->commands()->samplePrint(DEBUG_SAMPLING_TRIGGER_FAULT, ui->samplesBox->value(), ui->decimationBox->value());
        mSamplesToWait = ui->samplesBox->maximum();
    }
}

void PageSampledData::on_sampleTriggerStartNosendButton_clicked()
{
    if (mVesc) {
        clearBuffers();
        mVesc->commands()->samplePrint(DEBUG_SAMPLING_TRIGGER_START_NOSEND, ui->samplesBox->value(), ui->decimationBox->value());
        mSamplesToWait = ui->samplesBox->maximum();
    }
}

void PageSampledData::on_sampleTriggerFaultNosendButton_clicked()
{
    if (mVesc) {
        clearBuffers();
        mVesc->commands()->samplePrint(DEBUG_SAMPLING_TRIGGER_FAULT_NOSEND, ui->samplesBox->value(), ui->decimationBox->value());
        mSamplesToWait = ui->samplesBox->maximum();
    }
}

void PageSampledData::on_sampleLastButton_clicked()
{
    if (mVesc) {
        clearBuffers();
        mVesc->commands()->samplePrint(DEBUG_SAMPLING_SEND_LAST_SAMPLES, ui->samplesBox->value(), ui->decimationBox->value());
        mSamplesToWait = ui->samplesBox->maximum();
    }
}

void PageSampledData::on_sampleStopButton_clicked()
{
    if (mVesc) {
        mVesc->commands()->samplePrint(DEBUG_SAMPLING_OFF, ui->samplesBox->value(), ui->decimationBox->value());
    }
}

void PageSampledData::on_zoomHButton_toggled(bool checked)
{
    (void)checked;
    updateZoom();
}

void PageSampledData::on_zoomVButton_toggled(bool checked)
{
    (void)checked;
    updateZoom();
}

void PageSampledData::on_rescaleButton_clicked()
{
    ui->currentPlot->rescaleAxes();
    ui->currentPlot->replot();

    ui->voltagePlot->rescaleAxes();
    ui->voltagePlot->replot();

    ui->filterPlot->rescaleAxes();
    ui->filterPlot->replot();

    ui->filterResponsePlot->rescaleAxes();
    ui->filterResponsePlot->replot();
}

void PageSampledData::clearBuffers()
{
    mSampleInt = 0;
    tmpCurr1Vector.clear();
    tmpCurr2Vector.clear();
    tmpPh1Vector.clear();
    tmpPh2Vector.clear();
    tmpPh3Vector.clear();
    tmpVZeroVector.clear();
    tmpCurrTotVector.clear();
    tmpFSwVector.clear();
    tmpStatusArray.clear();
    tmpPhaseArray.clear();
}

void PageSampledData::updateZoom()
{
    Qt::Orientations plotOrientations = (Qt::Orientations)
            ((ui->zoomHButton->isChecked() ? Qt::Horizontal : 0) |
             (ui->zoomVButton->isChecked() ? Qt::Vertical : 0));

    ui->currentPlot->axisRect()->setRangeZoom(plotOrientations);
    ui->voltagePlot->axisRect()->setRangeZoom(plotOrientations);
    ui->filterPlot->axisRect()->setRangeZoom(plotOrientations);
    ui->filterResponsePlot->axisRect()->setRangeZoom(plotOrientations);
}

void PageSampledData::on_filterLogScaleBox_toggled(bool checked)
{
    if (checked) {
        ui->filterResponsePlot->yAxis->setScaleType(QCPAxis::stLogarithmic);
    } else {
        ui->filterResponsePlot->yAxis->setScaleType(QCPAxis::stLinear);
    }

    ui->filterResponsePlot->rescaleAxes();
    ui->filterResponsePlot->replot();
}


void PageSampledData::on_plotModeBox_currentIndexChanged(int index)
{
    ui->currentStack->setCurrentIndex(index == 2 ? 1 : 0);
}

void PageSampledData::on_saveDataButton_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("Save CSV"), "",
                                                    tr("CSV Files (*.csv)"));

    if (!fileName.isEmpty()) {
        if (!fileName.toLower().endsWith(".csv")) {
            fileName.append(".csv");
        }

        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly)) {
            QMessageBox::critical(this, "Save CSV File",
                                  "Could not open\n" + fileName + "\nfor writing");
            return;
        }

        QTextStream stream(&file);
        stream.setCodec("UTF-8");

        // Generate Time axis
        QVector<double> timeVec;
        timeVec.resize(fSwVector.size());
        double prev_t = 0.0;
        for (int i = 0;i < timeVec.size();i++) {
            timeVec[i] = prev_t;
            prev_t += 1.0 / fSwVector[i];
        }

        stream << "T;I1;I2;I3;V1;V2;V3;I_tot;V_zero;Phase\n";

        for (int i = 0;i < curr1Vector.size();i++) {
            stream << timeVec.at(i) << ";";
            stream << curr1Vector.at(i) << ";";
            stream << curr2Vector.at(i) << ";";
            stream << -(curr1Vector.at(i) + curr2Vector.at(i)) << ";";
            stream << ph1Vector.at(i) << ";";
            stream << ph2Vector.at(i) << ";";
            stream << ph3Vector.at(i) << ";";
            stream << currTotVector.at(i) << ";";
            stream << vZeroVector.at(i) << ";";
            stream << (double)((quint8)phaseArray.at(i)) / 250.0 * 360.0 << ";";

            if (i < (curr1Vector.size() - 1)) {
                stream << "\n";
            }
        }

        file.close();
    }
}
