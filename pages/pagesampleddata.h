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

#ifndef PAGESAMPLEDDATA_H
#define PAGESAMPLEDDATA_H

#include <QWidget>
#include <QVector>
#include <QTimer>
#include "vescinterface.h"

namespace Ui {
class PageSampledData;
}

class PageSampledData : public QWidget
{
    Q_OBJECT

public:
    explicit PageSampledData(QWidget *parent = 0);
    ~PageSampledData();

    VescInterface *vesc() const;
    void setVesc(VescInterface *vesc);

private slots:
    void timerSlot();
    void samplesReceived(QByteArray bytes);
    void replotAll();

    void on_sampleNowButton_clicked();
    void on_sampleStartButton_clicked();
    void on_sampleTriggerStartButton_clicked();
    void on_sampleTriggerFaultButton_clicked();
    void on_sampleTriggerStartNosendButton_clicked();
    void on_sampleTriggerFaultNosendButton_clicked();
    void on_sampleLastButton_clicked();
    void on_sampleStopButton_clicked();
    void on_zoomHButton_toggled(bool checked);
    void on_zoomVButton_toggled(bool checked);
    void on_rescaleButton_clicked();
    void on_filterLogScaleBox_toggled(bool checked);
    void on_plotModeBox_currentIndexChanged(int index);
    void on_saveDataButton_clicked();

private:
    Ui::PageSampledData *ui;
    VescInterface *mVesc;
    QTimer *mTimer;

    int mSampleInt;

    QVector<double> curr1Vector;
    QVector<double> curr2Vector;
    QVector<double> ph1Vector;
    QVector<double> ph2Vector;
    QVector<double> ph3Vector;
    QVector<double> vZeroVector;
    QVector<double> currTotVector;
    QVector<double> fSwVector;
    QByteArray statusArray;
    QByteArray phaseArray;

    QVector<double> tmpCurr1Vector;
    QVector<double> tmpCurr2Vector;
    QVector<double> tmpPh1Vector;
    QVector<double> tmpPh2Vector;
    QVector<double> tmpPh3Vector;
    QVector<double> tmpVZeroVector;
    QVector<double> tmpCurrTotVector;
    QVector<double> tmpFSwVector;
    QByteArray tmpStatusArray;
    QByteArray tmpPhaseArray;

    bool mDoReplot;
    bool mDoRescale;
    bool mDoFilterReplot;
    int mSamplesToWait;

    void clearBuffers();
    void updateZoom();

};

#endif // PAGESAMPLEDDATA_H
