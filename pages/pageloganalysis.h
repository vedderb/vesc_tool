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
#include <QCheckBox>
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

    void loadVescLog(QVector<LOG_DATA> log);

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
    void on_logTable_cellDoubleClicked(int row, int column);
    void on_vescLogListRefreshButton_clicked();
    void on_vescLogListOpenButton_clicked();
    void on_vescUpButton_clicked();
    void on_vescLogCancelButton_clicked();
    void on_vescLogTable_cellDoubleClicked(int row, int column);
    void on_vescSaveAsButton_clicked();
    void on_vescLogDeleteButton_clicked();
    void on_saveCsvButton_clicked();

private:
    Ui::PageLogAnalysis *ui;
    VescInterface *mVesc;
    QCPCurve *mVerticalLine;
    Vesc3DView *m3dView;
    QCheckBox *mUseYawBox;
    QTimer *mPlayTimer;
    QTimer *mGnssTimer;
    double mPlayPosNow;
    QString mVescLastPath;
    qint32 mGnssMsTodayLast;
    QString mLastSaveCsvPath;

    QVector<LOG_HEADER> mLogHeader;
    QVector<QVector<double> > mLog;
    QVector<QVector<double> > mLogTruncated;

    QVector<LOG_HEADER> mLogRtHeader;
    QVector<QVector<double> > mLogRt;
    QVector<double> mLogRtSamplesNow;
    QTimer *mLogRtTimer;
    bool mLogRtAppendTime;
    bool mLogRtFieldUpdatePending;

    // Lightweight pre-calculated offsets in the log. These
    // need to be looked up a lot and finding them in the
    // header each time slows down the responsiveness.
    int mInd_t_day;
    int mInd_t_day_pos;
    int mInd_gnss_h_acc;
    int mInd_gnss_lat;
    int mInd_gnss_lon;
    int mInd_gnss_alt;
    int mInd_trip_vesc;
    int mInd_trip_vesc_abs;
    int mInd_trip_gnss;
    int mInd_cnt_wh;
    int mInd_cnt_wh_chg;
    int mInd_cnt_ah;
    int mInd_cnt_ah_chg;
    int mInd_roll;
    int mInd_pitch;
    int mInd_yaw;
    int mInd_fault;

    struct SelectoData {
        QStringList dataLabels;
        int scrollPos;
    };

    SelectoData mSelection;

    void resetInds() {
        mInd_t_day = -1;
        mInd_t_day_pos = -1;
        mInd_gnss_h_acc = -1;
        mInd_gnss_lat = -1;
        mInd_gnss_lon = -1;
        mInd_gnss_alt = -1;
        mInd_trip_vesc = -1;
        mInd_trip_vesc_abs = -1;
        mInd_trip_gnss = -1;
        mInd_cnt_wh = -1;
        mInd_cnt_wh_chg = -1;
        mInd_cnt_ah = -1;
        mInd_cnt_ah_chg = -1;
        mInd_roll = -1;
        mInd_pitch = -1;
        mInd_yaw = -1;
        mInd_fault = -1;
    }

    void updateInds() {
        if (!mLogHeader.isEmpty()) {
            for (int i = 0;i < mLogHeader.size();i++) {
                auto e = mLogHeader.at(i);
                if (e.key == "t_day") mInd_t_day = i;
                else if (e.key == "t_day_pos") mInd_t_day_pos = i;
                else if (e.key == "gnss_h_acc") mInd_gnss_h_acc = i;
                else if (e.key == "gnss_lat") mInd_gnss_lat = i;
                else if (e.key == "gnss_lon") mInd_gnss_lon = i;
                else if (e.key == "gnss_alt") mInd_gnss_alt = i;
                else if (e.key == "trip_vesc") mInd_trip_vesc = i;
                else if (e.key == "trip_vesc_abs") mInd_trip_vesc_abs = i;
                else if (e.key == "trip_gnss") mInd_trip_gnss = i;
                else if (e.key == "cnt_wh") mInd_cnt_wh = i;
                else if (e.key == "cnt_wh_chg") mInd_cnt_wh_chg = i;
                else if (e.key == "cnt_ah") mInd_cnt_ah = i;
                else if (e.key == "cnt_ah_chg") mInd_cnt_ah_chg = i;
                else if (e.key == "roll") mInd_roll = i;
                else if (e.key == "pitch") mInd_pitch = i;
                else if (e.key == "yaw") mInd_yaw = i;
                else if (e.key == "fault") mInd_fault = i;
            }
        }
    }

    void truncateDataAndPlot(bool zoomGraph = true);
    void updateGraphs();
    void updateStats();
    void updateDataAndPlot(double time);
    QVector<double> getLogSample(double time);
    void updateTileServers();
    void logListRefresh();
    void addDataItem(QString name, bool hasScale = true,
                     double scaleStep = 0.1, double scaleMax = 99.99);
    void openLog(QByteArray data);
    void generateMissingEntries();

    void storeSelection();
    void restoreSelection();
    void setFileButtonsEnabled(bool en);

};

#endif // PAGELOGANALYSIS_H
