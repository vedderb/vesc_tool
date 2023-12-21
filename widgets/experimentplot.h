/*
    Copyright 2022 Benjamin Vedder	benjamin@vedder.se

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

#ifndef EXPERIMENTPLOT_H
#define EXPERIMENTPLOT_H

#include <QWidget>
#include <QTimer>
#include "vescinterface.h"

namespace Ui {
class ExperimentPlot;
}

class ExperimentPlot : public QWidget
{
    Q_OBJECT

public:
    explicit ExperimentPlot(QWidget *parent = nullptr);
    ~ExperimentPlot();

    VescInterface *vesc() const;
    void setVesc(VescInterface *newVesc);

private slots:
    void on_experimentLoadXmlButton_clicked();
    void on_experimentSaveXmlButton_clicked();
    void on_experimentSavePngButton_clicked();
    void on_experimentSavePdfButton_clicked();
    void on_experimentClearDataButton_clicked();
    void on_experimentSaveCsvButton_clicked();

    void plotInitReceived(QString xLabel, QString yLabel);
    void plotDataReceived(double x, double y);
    void plotAddGraphReceived(QString name);
    void plotSetGraphReceived(int graph);

private:
    typedef struct {
        QString label;
        QColor color;
        QVector<double> xData;
        QVector<double> yData;
    } EXPERIMENT_PLOT;

    QVector<EXPERIMENT_PLOT> mExperimentPlots;
    int mExperimentPlotNow;
    bool mExperimentReplot;

    Ui::ExperimentPlot *ui;
    VescInterface *mVesc;
    QTimer *mTimer;

};

#endif // EXPERIMENTPLOT_H
