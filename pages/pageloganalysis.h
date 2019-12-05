/*
    Copyright 2019 Benjamin Vedder	benjamin@vedder.se

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


#ifndef PAGELOGANALYSIS_H
#define PAGELOGANALYSIS_H

#include <QWidget>
#include <vescinterface.h>
#include "widgets/qcustomplot.h"
#include "widgets/vesc3dview.h"

namespace Ui {
class PageLogAnalysis;
}

class PageLogAnalysis : public QWidget
{
    Q_OBJECT

public:
    explicit PageLogAnalysis(QWidget *parent = nullptr);
    ~PageLogAnalysis();

    VescInterface *vesc() const;
    void setVesc(VescInterface *vesc);

private slots:
    void on_openCsvButton_clicked();
    void on_openCurrentButton_clicked();
    void on_gridBox_toggled(bool checked);
    void on_tilesHiResButton_toggled(bool checked);
    void on_tilesOsmButton_toggled(bool checked);
    void on_saveMapPdfButton_clicked();
    void on_saveMapPngButton_clicked();
    void on_savePlotPdfButton_clicked();
    void on_savePlotPngButton_clicked();
    void on_centerButton_clicked();
    void on_logListOpenButton_clicked();
    void on_logListRefreshButton_clicked();

private:
    Ui::PageLogAnalysis *ui;
    VescInterface *mVesc;
    QCPCurve *mVerticalLine;
    int mVerticalLineMsLast;
    Vesc3DView *m3dView;
    QCheckBox *mUseYawBox;
    QVector<LOG_DATA> mLogData;
    QVector<LOG_DATA> mLogDataTruncated;
    QTimer *mPlayTimer;
    double mPlayPosNow;

    void truncateDataAndPlot(bool zoomGraph = true);
    void updateGraphs();
    void updateStats();
    void updateDataAndPlot(double time);
    LOG_DATA getLogSample(int timeMs);
    double getDistGnssSample(int timeMs);
    void updateTileServers();
    void logListRefresh();

};

#endif // PAGELOGANALYSIS_H
