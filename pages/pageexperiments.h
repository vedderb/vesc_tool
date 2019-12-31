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

#ifndef PAGEEXPERIMENTS_H
#define PAGEEXPERIMENTS_H

#include <QWidget>
#include <QTimer>
#include <QElapsedTimer>
#include "vescinterface.h"

#ifdef HAS_SERIALPORT
#include <QSerialPortInfo>
#endif

namespace Ui {
class PageExperiments;
}

class PageExperiments : public QWidget
{
    Q_OBJECT

public:
    explicit PageExperiments(QWidget *parent = 0);
    ~PageExperiments();

    VescInterface *vesc() const;
    void setVesc(VescInterface *vesc);
    void stop();

private slots:
    void valuesReceived(MC_VALUES values, unsigned int mask);
    void timerSlot();
    void victronDataAvailable();

    void on_dutyRunButton_clicked();
    void on_currentRunButton_clicked();
    void on_rpmRunButton_clicked();
    void on_sampleIntervalBox_valueChanged(int arg1);
    void on_stopButton_clicked();
    void on_rescaleButton_clicked();
    void on_zoomHButton_toggled(bool checked);
    void on_zoomVButton_toggled(bool checked);
    void on_openButton_clicked();
    void on_saveCsvButton_clicked();
    void on_savePdfButton_clicked();
    void on_savePngButton_clicked();
    void on_victronRefreshButton_clicked();
    void on_victronConnectButton_clicked();
    void on_victronDisconnectButton_clicked();
    void on_openCompButton_clicked();
    void on_compCloseButton_clicked();

private:
    typedef enum {
        EXPERIMENT_OFF = 0,
        EXPERIMENT_DUTY,
        EXPERIMENT_CURRENT,
        EXPERIMENT_RPM
    } experiment_state;

    Ui::PageExperiments *ui;
    VescInterface *mVesc;
    experiment_state mState;
    QVector<double> mTimeVec;
    QVector<double> mCurrentInVec;
    QVector<double> mCurrentMotorVec;
    QVector<double> mVoltageVec;
    QVector<double> mPowerVec;
    QVector<double> mRpmVec;
    QVector<double> mTempFetVec;
    QVector<double> mTempFet1Vec;
    QVector<double> mTempFet2Vec;
    QVector<double> mTempFet3Vec;
    QVector<double> mTempMotorVec;
    QVector<double> mDutyVec;

    QVector<double> mCTimeVec;
    QVector<double> mCCurrentInVec;
    QVector<double> mCCurrentMotorVec;
    QVector<double> mCVoltageVec;
    QVector<double> mCPowerVec;
    QVector<double> mCRpmVec;
    QVector<double> mCTempFetVec;
    QVector<double> mCTempFet1Vec;
    QVector<double> mCTempFet2Vec;
    QVector<double> mCTempFet3Vec;
    QVector<double> mCTempMotorVec;
    QVector<double> mCDutyVec;

    QTimer *mTimer;
    QElapsedTimer mExperimentTimer;

    // Victron Energy BMV700
    QElapsedTimer mVictronTimer;
#ifdef HAS_SERIALPORT
    QSerialPort *mVictronPort;
#endif
    QByteArray mVictronData;
    double mVictronVoltage;
    double mVictronCurrent;

    void resetSamples();
    void resetCompareSamples();
    void updateZoom();
    QVector<double> createScaledVector(QVector<double> &inVec, double maxValue, QString &scaleStr);
    void plotSamples(bool exportFormat);

    void victronGetCurrent();
    void victronGetVoltage();

};

#endif // PAGEEXPERIMENTS_H
