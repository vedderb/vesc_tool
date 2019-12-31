/*
    Copyright 2018 Benjamin Vedder	benjamin@vedder.se

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

#include "pageexperiments.h"
#include "ui_pageexperiments.h"

#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <cmath>

#ifdef HAS_SERIALPORT
#include <QSerialPortInfo>
#endif

PageExperiments::PageExperiments(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PageExperiments)
{
    ui->setupUi(this);
    layout()->setContentsMargins(0, 0, 0, 0);
    mVesc = 0;

    mTimer = new QTimer(this);
    mTimer->start(ui->sampleIntervalBox->value());
    mState = EXPERIMENT_OFF;

    connect(mTimer, SIGNAL(timeout()),
            this, SLOT(timerSlot()));

    ui->plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

    QFont legendFont = font();
    legendFont.setPointSize(9);

    ui->plot->legend->setVisible(true);
    ui->plot->legend->setFont(legendFont);
    ui->plot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop | Qt::AlignLeft);
    ui->plot->legend->setBrush(QBrush(QColor(255,255,255,190)));
    ui->plot->xAxis->setLabel("Seconds (s)");
    ui->plot->yAxis2->setLabel("RPM");
    ui->plot->yAxis2->setVisible(true);

#ifdef HAS_SERIALPORT
    mVictronPort = new QSerialPort(this);
#endif
    mVictronVoltage = 0.0;
    mVictronCurrent = 0.0;
    mVictronTimer.start();

    connect(ui->showCurrentButton, &QPushButton::toggled,
            [=]() {plotSamples(false);});
    connect(ui->showPowerButton, &QPushButton::toggled,
            [=]() {plotSamples(false);});
    connect(ui->showVoltageButton, &QPushButton::toggled,
            [=]() {plotSamples(false);});
    connect(ui->showCurrentInButton, &QPushButton::toggled,
            [=]() {plotSamples(false);});
    connect(ui->showTempMotorButton, &QPushButton::toggled,
            [=]() {plotSamples(false);});
    connect(ui->showTempFetButton, &QPushButton::toggled,
            [=]() {plotSamples(false);});
    connect(ui->showTempFetIndButton, &QPushButton::toggled,
            [=]() {plotSamples(false);});
    connect(ui->showDutyButton, &QPushButton::toggled,
            [=]() {plotSamples(false);});

    connect(ui->showCCurrentButton, &QPushButton::toggled,
            [=]() {plotSamples(false);});
    connect(ui->showCPowerButton, &QPushButton::toggled,
            [=]() {plotSamples(false);});
    connect(ui->showCVoltageButton, &QPushButton::toggled,
            [=]() {plotSamples(false);});
    connect(ui->showCCurrentInButton, &QPushButton::toggled,
            [=]() {plotSamples(false);});
    connect(ui->showCTempMotorButton, &QPushButton::toggled,
            [=]() {plotSamples(false);});
    connect(ui->showCTempFetButton, &QPushButton::toggled,
            [=]() {plotSamples(false);});
    connect(ui->showCTempFetIndButton, &QPushButton::toggled,
            [=]() {plotSamples(false);});
    connect(ui->showCDutyButton, &QPushButton::toggled,
            [=]() {plotSamples(false);});

    connect(ui->compAEdit, &QLineEdit::textChanged,
            [=]() {plotSamples(false);});
    connect(ui->compBEdit, &QLineEdit::textChanged,
            [=]() {plotSamples(false);});

#ifdef HAS_SERIALPORT
    connect(mVictronPort, SIGNAL(readyRead()),
            this, SLOT(victronDataAvailable()));
#endif

    plotSamples(false);
    on_victronRefreshButton_clicked();
}

PageExperiments::~PageExperiments()
{
    delete ui;
}

VescInterface *PageExperiments::vesc() const
{
    return mVesc;
}

void PageExperiments::setVesc(VescInterface *vesc)
{
    mVesc = vesc;

    connect(mVesc->commands(), SIGNAL(valuesReceived(MC_VALUES,uint)),
            this, SLOT(valuesReceived(MC_VALUES,uint)));
}

void PageExperiments::stop()
{
    mVesc->commands()->setCurrent(0);
    mState = EXPERIMENT_OFF;
    ui->progressBar->setValue(100);
}

void PageExperiments::valuesReceived(MC_VALUES values, unsigned int mask)
{
    (void)mask;

    if (mState != EXPERIMENT_OFF) {
#ifdef HAS_SERIALPORT
        if (mVictronPort->isOpen()) {
            values.current_in = mVictronCurrent;
            values.v_in = mVictronVoltage;
        }
#endif

        mTimeVec.append((double)mExperimentTimer.elapsed() / 1000.0);
        mCurrentInVec.append(values.current_in);
        mCurrentMotorVec.append(values.current_motor);
        mPowerVec.append(values.current_in * values.v_in);
        mVoltageVec.append(values.v_in);
        mRpmVec.append(values.rpm);
        mTempFetVec.append(values.temp_mos);
        mTempFet1Vec.append(values.temp_mos_1);
        mTempFet2Vec.append(values.temp_mos_2);
        mTempFet3Vec.append(values.temp_mos_3);
        mTempMotorVec.append(values.temp_motor);
        mDutyVec.append(values.duty_now * 100.0);

        plotSamples(false);
    }
}

void PageExperiments::timerSlot()
{
#ifdef HAS_SERIALPORT
    if (mVictronPort->isOpen() && mVictronTimer.elapsed() >= 5) {
        mVictronTimer.restart();
        static bool current  = false;
        if (current) {
            victronGetCurrent();
        } else {
            victronGetVoltage();
        }
        current = !current;
    }
#endif

    if (mState != EXPERIMENT_OFF) {
        mVesc->commands()->getValues();

        double from = 0.0;
        double to = 0.0;
        double step = 0.0;
        double stepTime = 0.0;

        switch (mState) {
        case EXPERIMENT_DUTY:
            from = ui->dutyFromBox->value();
            to = ui->dutyToBox->value();
            step = ui->dutyStepBox->value();
            stepTime = ui->dutyStepTimeBox->value();
            break;

        case EXPERIMENT_CURRENT:
            from = ui->currentFromBox->value();
            to = ui->currentToBox->value();
            step = ui->currentStepBox->value();
            stepTime = ui->currentStepTimeBox->value();
            break;

        case EXPERIMENT_RPM:
            from = ui->rpmFromBox->value();
            to = ui->rpmToBox->value();
            step = ui->rpmStepBox->value();
            stepTime = ui->rpmStepTimeBox->value();
            break;

        default:
            break;
        }

        if (from > to) {
            step = -step;
        }

        double elapsedSecs = (double)mExperimentTimer.elapsed() / 1000.0;
        double totalSteps = (to - from) / step;
        double totalTime = totalSteps * stepTime;
        double progress = elapsedSecs / totalTime;
        double stepNow = floor(progress * totalSteps);
        double valueNow = from + stepNow * step;

        if (progress >= 1.0) {
            mState = EXPERIMENT_OFF;
            mVesc->commands()->setCurrent(0);
        } else {
            ui->progressBar->setValue(progress * 100);

            switch (mState) {
            case EXPERIMENT_DUTY:
                mVesc->commands()->setDutyCycle(valueNow);
                break;

            case EXPERIMENT_CURRENT:
                mVesc->commands()->setCurrent(valueNow);
                break;

            case EXPERIMENT_RPM:
                mVesc->commands()->setRpm(valueNow);
                break;

            default:
                break;
            }
        }
    }
}

void PageExperiments::on_dutyRunButton_clicked()
{
    mState = EXPERIMENT_DUTY;
    mExperimentTimer.start();
    resetSamples();
}

void PageExperiments::on_currentRunButton_clicked()
{
    mState = EXPERIMENT_CURRENT;
    mExperimentTimer.start();
    resetSamples();
}

void PageExperiments::on_rpmRunButton_clicked()
{
    mState = EXPERIMENT_RPM;
    mExperimentTimer.start();
    resetSamples();
}

void PageExperiments::on_sampleIntervalBox_valueChanged(int arg1)
{
    mTimer->setInterval(arg1);
}

void PageExperiments::on_stopButton_clicked()
{
    stop();
}

void PageExperiments::resetSamples()
{
    mTimeVec.clear();
    mCurrentInVec.clear();
    mCurrentMotorVec.clear();
    mVoltageVec.clear();
    mPowerVec.clear();
    mRpmVec.clear();
    mTempFetVec.clear();
    mTempFet1Vec.clear();
    mTempFet2Vec.clear();
    mTempFet3Vec.clear();
    mTempMotorVec.clear();
    mDutyVec.clear();
}

void PageExperiments::resetCompareSamples()
{
    mCTimeVec.clear();
    mCCurrentInVec.clear();
    mCCurrentMotorVec.clear();
    mCVoltageVec.clear();
    mCPowerVec.clear();
    mCRpmVec.clear();
    mCTempFetVec.clear();
    mCTempFet1Vec.clear();
    mCTempFet2Vec.clear();
    mCTempFet3Vec.clear();
    mCTempMotorVec.clear();
    mCDutyVec.clear();
}

void PageExperiments::updateZoom()
{
    Qt::Orientations plotOrientations = (Qt::Orientations)
            ((ui->zoomHButton->isChecked() ? Qt::Horizontal : 0) |
             (ui->zoomVButton->isChecked() ? Qt::Vertical : 0));

    ui->plot->axisRect()->setRangeZoom(plotOrientations);
}

QVector<double> PageExperiments::createScaledVector(QVector<double> &inVec, double maxValue, QString &scaleStr)
{
    double highest = 0.0;
    for (double d: inVec) {
        if (fabs(d) > highest) {
            highest = fabs(d);
        }
    }

    double scale = 1.0;
    bool scaleChanged = false;
    while ((highest / scale) > maxValue) {
        scale *= 10.0;
        scaleChanged = true;
    }

    QVector<double> res;
    for (double d: inVec) {
        res.append(d / scale);
    }

    scaleStr = "";
    if (scaleChanged) {
        scaleStr = QString(" x%1").arg(scale);
    }

    return res;
}

void PageExperiments::plotSamples(bool exportFormat)
{
    int graphIndex = 0;
    int lineWidth = 0;

    if (exportFormat) {
        lineWidth = 1;
    }

    ui->plot->clearGraphs();

    if (ui->showPowerButton->isChecked()) {
        QString scaleStr;
        QVector<double> scaled = createScaledVector(mPowerVec, 150.0, scaleStr);

        ui->plot->addGraph();
        ui->plot->graph(graphIndex)->setPen(QPen(Qt::blue, lineWidth));
        ui->plot->graph(graphIndex)->setName(ui->compAEdit->text() + " Power (W)" + scaleStr);
        ui->plot->graph(graphIndex)->setData(mTimeVec, scaled);
        graphIndex++;
    }

    if (ui->showCurrentButton->isChecked()) {
        QString scaleStr;
        QVector<double> scaled = createScaledVector(mCurrentMotorVec, 150.0, scaleStr);

        ui->plot->addGraph();
        ui->plot->graph(graphIndex)->setPen(QPen(Qt::magenta, lineWidth));
        ui->plot->graph(graphIndex)->setName(ui->compAEdit->text() + " Current motor (A)" + scaleStr);
        ui->plot->graph(graphIndex)->setData(mTimeVec, scaled);
        graphIndex++;
    }

    if (ui->showCurrentInButton->isChecked()) {
        QString scaleStr;
        QVector<double> scaled = createScaledVector(mCurrentInVec, 150.0, scaleStr);

        ui->plot->addGraph();
        ui->plot->graph(graphIndex)->setPen(QPen(Qt::green, lineWidth));
        ui->plot->graph(graphIndex)->setName(ui->compAEdit->text() + " Current in (A)" + scaleStr);
        ui->plot->graph(graphIndex)->setData(mTimeVec, scaled);
        graphIndex++;
    }

    if (ui->showVoltageButton->isChecked()) {
        ui->plot->addGraph();
        ui->plot->graph(graphIndex)->setPen(QPen(Qt::darkGreen, lineWidth));
        ui->plot->graph(graphIndex)->setName(ui->compAEdit->text() + " Voltage in (V)");
        ui->plot->graph(graphIndex)->setData(mTimeVec, mVoltageVec);
        graphIndex++;
    }

    if (ui->showTempFetButton->isChecked()) {
        ui->plot->addGraph();
        ui->plot->graph(graphIndex)->setPen(QPen(Qt::cyan, lineWidth));
        ui->plot->graph(graphIndex)->setName(ui->compAEdit->text() + " Temp MOSFET (\u00B0C)");
        ui->plot->graph(graphIndex)->setData(mTimeVec, mTempFetVec);
        graphIndex++;
    }

    if (ui->showTempFetIndButton->isChecked()) {
        ui->plot->addGraph();
        ui->plot->graph(graphIndex)->setPen(QPen(QColor("#01DFD7"), lineWidth));
        ui->plot->graph(graphIndex)->setName(ui->compAEdit->text() + " Temp MOSFET 1 (\u00B0C)");
        ui->plot->graph(graphIndex)->setData(mTimeVec, mTempFet1Vec);
        graphIndex++;

        ui->plot->addGraph();
        ui->plot->graph(graphIndex)->setPen(QPen(QColor("#04B4AE"), lineWidth));
        ui->plot->graph(graphIndex)->setName(ui->compAEdit->text() + " Temp MOSFET 2 (\u00B0C)");
        ui->plot->graph(graphIndex)->setData(mTimeVec, mTempFet2Vec);
        graphIndex++;

        ui->plot->addGraph();
        ui->plot->graph(graphIndex)->setPen(QPen(QColor("#088A85"), lineWidth));
        ui->plot->graph(graphIndex)->setName(ui->compAEdit->text() + " Temp MOSFET 3 (\u00B0C)");
        ui->plot->graph(graphIndex)->setData(mTimeVec, mTempFet3Vec);
        graphIndex++;
    }

    if (ui->showTempMotorButton->isChecked()) {
        ui->plot->addGraph();
        ui->plot->graph(graphIndex)->setPen(QPen(Qt::darkGray, lineWidth));
        ui->plot->graph(graphIndex)->setName(ui->compAEdit->text() + " Temp Motor (\u00B0C)");
        ui->plot->graph(graphIndex)->setData(mTimeVec, mTempMotorVec);
        graphIndex++;
    }

    if (ui->showDutyButton->isChecked()) {
        ui->plot->addGraph();
        ui->plot->graph(graphIndex)->setPen(QPen(Qt::darkYellow, lineWidth));
        ui->plot->graph(graphIndex)->setName(ui->compAEdit->text() + " Duty cycle (%)");
        ui->plot->graph(graphIndex)->setData(mTimeVec, mDutyVec);
        graphIndex++;
    }

    ui->plot->addGraph(ui->plot->xAxis, ui->plot->yAxis2);
    ui->plot->graph(graphIndex)->setPen(QPen(Qt::red, lineWidth));
    ui->plot->graph(graphIndex)->setName(ui->compAEdit->text() + " RPM");
    ui->plot->graph(graphIndex)->setData(mTimeVec, mRpmVec);
    graphIndex++;

    if (ui->compareButtons->isEnabled()) {
        Qt::PenStyle penStyle = Qt::DashDotLine;

        if (ui->showCPowerButton->isChecked()) {
            QString scaleStr;
            QVector<double> scaled = createScaledVector(mCPowerVec, 150.0, scaleStr);

            ui->plot->addGraph();
            ui->plot->graph(graphIndex)->setPen(QPen(Qt::blue, lineWidth, penStyle));
            ui->plot->graph(graphIndex)->setName(ui->compBEdit->text() + " Power (W)" + scaleStr);
            ui->plot->graph(graphIndex)->setData(mCTimeVec, scaled);
            graphIndex++;
        }

        if (ui->showCCurrentButton->isChecked()) {
            QString scaleStr;
            QVector<double> scaled = createScaledVector(mCCurrentMotorVec, 150.0, scaleStr);

            ui->plot->addGraph();
            ui->plot->graph(graphIndex)->setPen(QPen(Qt::magenta, lineWidth, penStyle));
            ui->plot->graph(graphIndex)->setName(ui->compBEdit->text() + " Current motor (A)" + scaleStr);
            ui->plot->graph(graphIndex)->setData(mCTimeVec, scaled);
            graphIndex++;
        }

        if (ui->showCCurrentInButton->isChecked()) {
            QString scaleStr;
            QVector<double> scaled = createScaledVector(mCCurrentInVec, 150.0, scaleStr);

            ui->plot->addGraph();
            ui->plot->graph(graphIndex)->setPen(QPen(Qt::green, lineWidth, penStyle));
            ui->plot->graph(graphIndex)->setName(ui->compBEdit->text() + " Current in (A)" + scaleStr);
            ui->plot->graph(graphIndex)->setData(mCTimeVec, scaled);
            graphIndex++;
        }

        if (ui->showCVoltageButton->isChecked()) {
            ui->plot->addGraph();
            ui->plot->graph(graphIndex)->setPen(QPen(Qt::darkGreen, lineWidth, penStyle));
            ui->plot->graph(graphIndex)->setName(ui->compBEdit->text() + " Voltage in (V)");
            ui->plot->graph(graphIndex)->setData(mCTimeVec, mCVoltageVec);
            graphIndex++;
        }

        if (ui->showCTempFetButton->isChecked()) {
            ui->plot->addGraph();
            ui->plot->graph(graphIndex)->setPen(QPen(Qt::cyan, lineWidth, penStyle));
            ui->plot->graph(graphIndex)->setName(ui->compBEdit->text() + " Temp MOSFET (\u00B0C)");
            ui->plot->graph(graphIndex)->setData(mCTimeVec, mCTempFetVec);
            graphIndex++;
        }

        if (ui->showCTempFetIndButton->isChecked()) {
            ui->plot->addGraph();
            ui->plot->graph(graphIndex)->setPen(QPen(QColor("#01DFD7"), lineWidth, penStyle));
            ui->plot->graph(graphIndex)->setName(ui->compBEdit->text() + " Temp MOSFET 1 (\u00B0C)");
            ui->plot->graph(graphIndex)->setData(mCTimeVec, mCTempFet1Vec);
            graphIndex++;

            ui->plot->addGraph();
            ui->plot->graph(graphIndex)->setPen(QPen(QColor("#04B4AE"), lineWidth, penStyle));
            ui->plot->graph(graphIndex)->setName(ui->compBEdit->text() + " Temp MOSFET 2 (\u00B0C)");
            ui->plot->graph(graphIndex)->setData(mCTimeVec, mCTempFet2Vec);
            graphIndex++;

            ui->plot->addGraph();
            ui->plot->graph(graphIndex)->setPen(QPen(QColor("#088A85"), lineWidth, penStyle));
            ui->plot->graph(graphIndex)->setName(ui->compBEdit->text() + " Temp MOSFET 3 (\u00B0C)");
            ui->plot->graph(graphIndex)->setData(mCTimeVec, mCTempFet3Vec);
            graphIndex++;
        }

        if (ui->showCTempMotorButton->isChecked()) {
            ui->plot->addGraph();
            ui->plot->graph(graphIndex)->setPen(QPen(Qt::darkGray, lineWidth, penStyle));
            ui->plot->graph(graphIndex)->setName(ui->compBEdit->text() + " Temp Motor (\u00B0C)");
            ui->plot->graph(graphIndex)->setData(mCTimeVec, mCTempMotorVec);
            graphIndex++;
        }

        if (ui->showCDutyButton->isChecked()) {
            ui->plot->addGraph();
            ui->plot->graph(graphIndex)->setPen(QPen(Qt::darkYellow, lineWidth, penStyle));
            ui->plot->graph(graphIndex)->setName(ui->compBEdit->text() + " Duty cycle (%)");
            ui->plot->graph(graphIndex)->setData(mCTimeVec, mCDutyVec);
            graphIndex++;
        }

        ui->plot->addGraph(ui->plot->xAxis, ui->plot->yAxis2);
        ui->plot->graph(graphIndex)->setPen(QPen(Qt::red, lineWidth, penStyle));
        ui->plot->graph(graphIndex)->setName(ui->compBEdit->text() + " RPM");
        ui->plot->graph(graphIndex)->setData(mCTimeVec, mCRpmVec);
        graphIndex++;
    }

    if (ui->autoscaleButton->isChecked()) {
        ui->plot->rescaleAxes();
    }

    ui->plot->replot();
}

void PageExperiments::victronGetCurrent()
{
#ifdef HAS_SERIALPORT
    if (mVictronPort->isOpen()) {
        QString cmd;
        cmd += ":78FED00D2\n";
        mVictronPort->write(cmd.toLocal8Bit().data());
    }
#endif
}

void PageExperiments::victronGetVoltage()
{
#ifdef HAS_SERIALPORT
    if (mVictronPort->isOpen()) {
        QString cmd;
        cmd += ":78DED00D4\n";
        mVictronPort->write(cmd.toLocal8Bit().data());
    }
#endif
}

void PageExperiments::on_rescaleButton_clicked()
{
    ui->plot->rescaleAxes();
    ui->plot->replot();
}

void PageExperiments::on_zoomHButton_toggled(bool checked)
{
    (void)checked;
    updateZoom();
}

void PageExperiments::on_zoomVButton_toggled(bool checked)
{
    (void)checked;
    updateZoom();
}

void PageExperiments::on_openButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Load CSV File"), "",
                                                    tr("CSV files (*.csv)"));

    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::critical(this, "Load CSV File",
                                  "Could not open\n" + fileName + "\nfor reading");
            return;
        }

        QTextStream stream(&file);

        resetSamples();

        while (!stream.atEnd()) {
            QString line = stream.readLine();
            QStringList tokens = line.split(";");

            if (tokens.size() == 9 || tokens.size() == 12) {
                mTimeVec.append(tokens.at(0).toDouble());
                mPowerVec.append(tokens.at(1).toDouble());
                mCurrentInVec.append(tokens.at(2).toDouble());
                mCurrentMotorVec.append(tokens.at(3).toDouble());
                mVoltageVec.append(tokens.at(4).toDouble());
                mRpmVec.append(tokens.at(5).toDouble());
                mTempFetVec.append(tokens.at(6).toDouble());
                mTempMotorVec.append(tokens.at(7).toDouble());
                mDutyVec.append(tokens.at(8).toDouble());

                if (tokens.size() == 12) {
                    mTempFet1Vec.append(tokens.at(9).toDouble());
                    mTempFet2Vec.append(tokens.at(10).toDouble());
                    mTempFet3Vec.append(tokens.at(11).toDouble());
                } else {
                    mTempFet1Vec.append(-1.0);
                    mTempFet2Vec.append(-1.0);
                    mTempFet3Vec.append(-1.0);
                }
            }
        }

        plotSamples(false);
        file.close();
    }
}

void PageExperiments::on_saveCsvButton_clicked()
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

        for (int i = 0;i < mTimeVec.size();i++) {
            stream << mTimeVec.at(i) << ";";
            stream << mPowerVec.at(i) << ";";
            stream << mCurrentInVec.at(i) << ";";
            stream << mCurrentMotorVec.at(i) << ";";
            stream << mVoltageVec.at(i) << ";";
            stream << mRpmVec.at(i) << ";";
            stream << mTempFetVec.at(i) << ";";
            stream << mTempMotorVec.at(i) << ";";
            stream << mDutyVec.at(i) << ";";
            stream << mTempFet1Vec.at(i) << ";";
            stream << mTempFet2Vec.at(i) << ";";
            stream << mTempFet3Vec.at(i);

            if (i < (mTimeVec.size() - 1)) {
                stream << "\n";
            }
        }

        file.close();
    }
}

void PageExperiments::on_savePdfButton_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("Save PDF"), "",
                                                    tr("PDF Files (*.pdf)"));

    if (!fileName.isEmpty()) {
        if (!fileName.toLower().endsWith(".pdf")) {
            fileName.append(".pdf");
        }

        QString title = ui->titleEdit->text();

        if (!title.isEmpty()) {
            ui->plot->plotLayout()->insertRow(0);
            ui->plot->plotLayout()->
                    addElement(0, 0,
                               new QCPTextElement(ui->plot,
                                                  title,
                                                  QFont("sans", 12, QFont::Bold)));
        }

        plotSamples(true);

        ui->plot->savePdf(fileName,
                          ui->saveWidthBox->value(),
                          ui->saveHeightBox->value());

        if (!title.isEmpty()) {
            delete ui->plot->plotLayout()->element(0, 0);
            ui->plot->plotLayout()->simplify();
        }

        plotSamples(false);
    }
}

void PageExperiments::on_savePngButton_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("Save Image"), "",
                                                    tr("PNG Files (*.png)"));

    if (!fileName.isEmpty()) {
        if (!fileName.toLower().endsWith(".png")) {
            fileName.append(".png");
        }

        QString title = ui->titleEdit->text();

        if (!title.isEmpty()) {
            ui->plot->plotLayout()->insertRow(0);
            ui->plot->plotLayout()->
                    addElement(0, 0,
                               new QCPTextElement(ui->plot,
                                                  title,
                                                  QFont("sans", 12, QFont::Bold)));
        }

        plotSamples(true);

        ui->plot->savePng(fileName,
                          ui->saveWidthBox->value(),
                          ui->saveHeightBox->value());

        if (!title.isEmpty()) {
            delete ui->plot->plotLayout()->element(0, 0);
            ui->plot->plotLayout()->simplify();
        }

        plotSamples(false);
    }
}

void PageExperiments::on_victronRefreshButton_clicked()
{
#ifdef HAS_SERIALPORT
    ui->victronPortBox->clear();

    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    foreach(const QSerialPortInfo &port, ports) {
        ui->victronPortBox->addItem(port.portName(), port.systemLocation());
    }

    ui->victronPortBox->setCurrentIndex(0);
#endif
}

void PageExperiments::on_victronConnectButton_clicked()
{
#ifdef HAS_SERIALPORT
    if(mVictronPort->isOpen()) {
        mVictronPort->close();
    }

    mVictronPort->setPortName(ui->victronPortBox->currentData().toString());
    mVictronPort->open(QIODevice::ReadWrite);

    if(mVictronPort->isOpen()) {
        mVictronPort->setBaudRate(19200);
        mVictronPort->setDataBits(QSerialPort::Data8);
        mVictronPort->setParity(QSerialPort::NoParity);
        mVictronPort->setStopBits(QSerialPort::OneStop);
        mVictronPort->setFlowControl(QSerialPort::NoFlowControl);
    }
#endif
}

void PageExperiments::on_victronDisconnectButton_clicked()
{
#ifdef HAS_SERIALPORT
    mVictronPort->close();
#endif
}

void PageExperiments::victronDataAvailable()
{
#ifdef HAS_SERIALPORT
    while (mVictronPort->bytesAvailable() > 0) {
        for (char c: mVictronPort->readAll()) {
            mVictronData.append(c);

            if (mVictronData.at(mVictronData.size() - 1) == '\n') {
                if (mVictronData.startsWith(':')) {
                    QString data(mVictronData);

                    // Example
                    // :7001000C80076

                    // Check checksum
                    quint8 sum = 7;
                    for (int i = 2;i < data.size();i += 2) {
                        sum += data.mid(i, 2).toInt(0, 16);
                    }

                    if (sum == 0x55) {
                        int cmd = data.mid(2, 2).toInt(0, 16) + data.mid(4, 2).toInt(0, 16) * 256;
                        double val = (double)((qint16)((data.mid(8, 2).toInt(0, 16)) | ((quint16)(data.mid(10, 2).toInt(0, 16)) << 8)));

                        switch (cmd) {
                        case 60813: {
                            mVictronVoltage = val / 100.0;
                            ui->extVoltageLabel->setText(QString("Voltage: %1 V").arg(mVictronVoltage));
                        } break;

                        case 60815: {
                            mVictronCurrent = -val / 10.0;
                            ui->extCurrentLabel->setText(QString("Current: %1 A").arg(mVictronCurrent));
                        } break;

                        default:
                            break;
                        }
                    }
                }

                mVictronData.clear();
            }
        }
    }
#endif
}

void PageExperiments::on_openCompButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Load Comparison CSV File"), "",
                                                    tr("CSV files (*.csv)"));

    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::critical(this, "Load Comparison CSV File",
                                  "Could not open\n" + fileName + "\nfor reading");
            return;
        }

        QTextStream stream(&file);

        resetCompareSamples();

        while (!stream.atEnd()) {
            QString line = stream.readLine();
            QStringList tokens = line.split(";");

            if (tokens.size() == 9 || tokens.size() == 12) {
                mCTimeVec.append(tokens.at(0).toDouble());
                mCPowerVec.append(tokens.at(1).toDouble());
                mCCurrentInVec.append(tokens.at(2).toDouble());
                mCCurrentMotorVec.append(tokens.at(3).toDouble());
                mCVoltageVec.append(tokens.at(4).toDouble());
                mCRpmVec.append(tokens.at(5).toDouble());
                mCTempFetVec.append(tokens.at(6).toDouble());
                mCTempMotorVec.append(tokens.at(7).toDouble());
                mCDutyVec.append(tokens.at(8).toDouble());

                if (tokens.size() == 12) {
                    mCTempFet1Vec.append(tokens.at(9).toDouble());
                    mCTempFet2Vec.append(tokens.at(10).toDouble());
                    mCTempFet3Vec.append(tokens.at(11).toDouble());
                } else {
                    mCTempFet1Vec.append(-1.0);
                    mCTempFet2Vec.append(-1.0);
                    mCTempFet3Vec.append(-1.0);
                }
            }
        }

        ui->compareButtons->setEnabled(true);
        plotSamples(false);
        file.close();
    }
}

void PageExperiments::on_compCloseButton_clicked()
{
    ui->compareButtons->setEnabled(false);
    resetCompareSamples();
    plotSamples(false);
}
