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

#ifndef PAGERTDATA_H
#define PAGERTDATA_H

#include <QWidget>
#include <QVector>
#include <QTimer>
#include "vescinterface.h"

namespace Ui {
class PageRtData;
}

class PageRtData : public QWidget
{
    Q_OBJECT

public:
    explicit PageRtData(QWidget *parent = 0);
    ~PageRtData();

    VescInterface *vesc() const;
    void setVesc(VescInterface *vesc);

private slots:
    void timerSlot();
    void valuesReceived(MC_VALUES values, unsigned int mask);
    void rotorPosReceived(double pos);
    void plotInitReceived(QString xLabel, QString yLabel);
    void plotDataReceived(double x, double y);
    void plotAddGraphReceived(QString name);
    void plotSetGraphReceived(int graph);

    void on_zoomHButton_toggled(bool checked);
    void on_zoomVButton_toggled(bool checked);
    void on_rescaleButton_clicked();
    void on_posInductanceButton_clicked();
    void on_posObserverButton_clicked();
    void on_posEncoderButton_clicked();
    void on_posPidButton_clicked();
    void on_posPidErrorButton_clicked();
    void on_posEncoderObserverErrorButton_clicked();
    void on_posStopButton_clicked();
    void on_tempShowMosfetBox_toggled(bool checked);
    void on_tempShowMotorBox_toggled(bool checked);
    void on_csvChooseDirButton_clicked();
    void on_csvEnableLogBox_clicked(bool checked);
    void on_csvHelpButton_clicked();
    void on_experimentLoadXmlButton_clicked();
    void on_experimentSaveXmlButton_clicked();
    void on_experimentSavePngButton_clicked();
    void on_experimentSavePdfButton_clicked();

private:
    Ui::PageRtData *ui;
    VescInterface *mVesc;
    QTimer *mTimer;

    QVector<double> mTempMosVec;
    QVector<double> mTempMos1Vec;
    QVector<double> mTempMos2Vec;
    QVector<double> mTempMos3Vec;
    QVector<double> mTempMotorVec;
    QVector<double> mCurrInVec;
    QVector<double> mCurrMotorVec;
    QVector<double> mIdVec;
    QVector<double> mIqVec;
    QVector<double> mDutyVec;
    QVector<double> mRpmVec;
    QVector<double> mPositionVec;
    QVector<double> mSeconds;
    QVector<double> mVdVec;
    QVector<double> mVqVec;

    double mSecondCounter;
    qint64 mLastUpdateTime;

    bool mUpdateValPlot;
    bool mUpdatePosPlot;

    typedef struct {
        QString label;
        QString color;
        QVector<double> xData;
        QVector<double> yData;
    } EXPERIMENT_PLOT;

    QVector<EXPERIMENT_PLOT> mExperimentPlots;
    int mExperimentPlotNow;
    bool mExperimentReplot;

    void appendDoubleAndTrunc(QVector<double> *vec, double num, int maxSize);
    void updateZoom();

};

#endif // PAGERTDATA_H
