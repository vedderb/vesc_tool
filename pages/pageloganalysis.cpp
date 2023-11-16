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


#include "pageloganalysis.h"
#include "ui_pageloganalysis.h"
#include "utility.h"
#include <QFileDialog>
#include <QMessageBox>
#include <cmath>
#include <QStandardPaths>

PageLogAnalysis::PageLogAnalysis(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PageLogAnalysis)
{
    ui->setupUi(this);
    mVesc = nullptr;

    resetInds();

    ui->centerButton->setIcon(Utility::getIcon("icons/icons8-target-96.png"));
    ui->playButton->setIcon(Utility::getIcon("icons/Circled Play-96.png"));
    ui->logListRefreshButton->setIcon(Utility::getIcon("icons/Refresh-96.png"));
    ui->logListOpenButton->setIcon(Utility::getIcon("icons/Open Folder-96.png"));
    ui->openCurrentButton->setIcon(Utility::getIcon("icons/Open Folder-96.png"));
    ui->openCsvButton->setIcon(Utility::getIcon("icons/Open Folder-96.png"));
    ui->savePlotPdfButton->setIcon(Utility::getIcon("icons/Line Chart-96.png"));
    ui->savePlotPngButton->setIcon(Utility::getIcon("icons/Line Chart-96.png"));
    ui->saveMapPdfButton->setIcon(Utility::getIcon("icons/Waypoint Map-96.png"));
    ui->saveMapPngButton->setIcon(Utility::getIcon("icons/Waypoint Map-96.png"));
    ui->vescLogListRefreshButton->setIcon(Utility::getIcon("icons/Refresh-96.png"));
    ui->vescLogListOpenButton->setIcon(Utility::getIcon("icons/Open Folder-96.png"));
    ui->vescUpButton->setIcon(Utility::getIcon("icons/Upload-96.png"));
    ui->vescSaveAsButton->setIcon(Utility::getIcon("icons/Save as-96.png"));
    ui->vescLogDeleteButton->setIcon(Utility::getIcon("icons/Delete-96.png"));
    ui->saveCsvButton->setIcon(Utility::getIcon("icons/Line Chart-96.png"));

    updateTileServers();

    ui->spanSlider->setMinimum(0);
    ui->spanSlider->setMaximum(10000);
    ui->spanSlider->setValue(10000);

    ui->mapSplitter->setStretchFactor(0, 2);
    ui->mapSplitter->setStretchFactor(1, 1);

    ui->statSplitter->setStretchFactor(0, 6);
    ui->statSplitter->setStretchFactor(1, 1);

    Utility::setPlotColors(ui->plot);
    ui->plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    ui->plot->axisRect()->setRangeZoom(Qt::Orientations());
    ui->plot->axisRect()->setRangeDrag(Qt::Orientations());

    ui->dataTable->setColumnWidth(0, 140);
    ui->dataTable->setColumnWidth(1, 120);
    ui->statTable->setColumnWidth(0, 140);
    ui->logTable->setColumnWidth(0, 250);
    ui->vescLogTable->setColumnWidth(0, 250);

    m3dView = new Vesc3DView(this);
    m3dView->setMinimumWidth(200);
    m3dView->setRollPitchYaw(20, 20, 0);
    m3dView->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
    mUseYawBox = new QCheckBox("Use Yaw (will drift)");
    mUseYawBox->setChecked(true);
    ui->tab_3->layout()->addWidget(mUseYawBox);
    ui->tab_3->layout()->addWidget(m3dView);

    mPlayTimer = new QTimer(this);
    mPlayPosNow = 0.0;
    mPlayTimer->start(100);

    connect(mPlayTimer, &QTimer::timeout, [this]() {
        if (ui->playButton->isChecked() && !mLogTruncated.isEmpty()) {
            mPlayPosNow += double(mPlayTimer->interval()) / 1000.0;

            if (mInd_t_day >= 0) {
                double time = (mLogTruncated.last()[mInd_t_day] - mLogTruncated.first()[mInd_t_day]);
                if (time < 0.0) { // Handle midnight
                    time += 60.0 * 60.0 * 24.0;
                }

                if (mLogTruncated.size() > 0 &&
                        mPlayPosNow <= time) {
                    updateDataAndPlot(mPlayPosNow);
                } else {
                    ui->playButton->setChecked(false);
                }
            }
        }

        if (mVesc) {
            if (!mVesc->isPortConnected()) {
                mVescLastPath.clear();
                if (ui->vescLogTable->rowCount() > 0) {
                    ui->vescLogTable->setRowCount(0);
                }
            }
        }
    });

    mGnssTimer = new QTimer(this);
    mGnssTimer->start(100);
    mGnssMsTodayLast = 0;

    mLogRtFieldUpdatePending = false;
    mLogRtAppendTime = false;
    mLogRtTimer = new QTimer(this);

    connect(mGnssTimer, &QTimer::timeout, [this]() {
        if (mVesc && ui->pollGnssBox->isChecked()) {
            mVesc->commands()->getGnss(0xFFFF);
        }
    });

    QFont legendFont = font();
    legendFont.setPointSize(9);

    ui->plot->legend->setVisible(true);
    ui->plot->legend->setFont(legendFont);
    ui->plot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignRight|Qt::AlignBottom);
    ui->plot->xAxis->setLabel("Seconds (s)");

    mVerticalLine = new QCPCurve(ui->plot->xAxis, ui->plot->yAxis);
    mVerticalLine->removeFromLegend();
    mVerticalLine->setPen(QPen(Utility::getAppQColor("normalText")));

    auto updateMouse = [this](QMouseEvent *event) {
        if (event->modifiers() == Qt::ShiftModifier) {
            ui->plot->axisRect()->setRangeZoom(Qt::Vertical);
            ui->plot->axisRect()->setRangeDrag(Qt::Vertical);
        } else {
            ui->plot->axisRect()->setRangeZoom(Qt::Orientations());
            ui->plot->axisRect()->setRangeDrag(Qt::Orientations());
        }

        if (event->buttons() & Qt::LeftButton) {
            double vx = ui->plot->xAxis->pixelToCoord(event->x());
            updateDataAndPlot(vx);
        }
    };

    connect(ui->map, &MapWidget::infoPointClicked, [this](LocPoint info) {
        if (mInd_t_day >= 0 && !mLogTruncated.isEmpty()) {
            updateDataAndPlot(info.getInfo().toDouble() - mLogTruncated.first()[mInd_t_day]);
        }
    });

    connect(ui->plot, &QCustomPlot::mousePress, [updateMouse](QMouseEvent *event) {
        updateMouse(event);
    });

    connect(ui->plot, &QCustomPlot::mouseMove, [updateMouse](QMouseEvent *event) {
        updateMouse(event);
    });

    connect(ui->plot, &QCustomPlot::mouseWheel, [this](QWheelEvent *event) {
        if (event->modifiers() == Qt::ShiftModifier) {
            ui->plot->axisRect()->setRangeZoom(Qt::Vertical);
            ui->plot->axisRect()->setRangeDrag(Qt::Vertical);
        } else {
            ui->plot->axisRect()->setRangeZoom(Qt::Orientations());
            ui->plot->axisRect()->setRangeDrag(Qt::Orientations());

            double upper = ui->plot->xAxis->range().upper;
            double progress = ui->plot->xAxis->pixelToCoord(event->position().x()) / upper;
            double diff = event->angleDelta().y();
            double d1 = diff * progress;
            double d2 = diff * (1.0 - progress);

            ui->spanSlider->alt_setValue(ui->spanSlider->alt_value() + int(d1));
            ui->spanSlider->setValue(ui->spanSlider->value() - int(d2));
        }
    });

    connect(ui->spanSlider, &SuperSlider::alt_valueChanged, [this](int value) {
        if (value >= ui->spanSlider->value()) {
            ui->spanSlider->setValue(value);
        }
        truncateDataAndPlot(ui->autoZoomBox->isChecked());
    });

    connect(ui->spanSlider, &SuperSlider::valueChanged, [this](int value) {
        if (value <= ui->spanSlider->alt_value()) {
            ui->spanSlider->alt_setValue(value);
        }
        truncateDataAndPlot(ui->autoZoomBox->isChecked());
    });

    connect(ui->dataTable, &QTableWidget::itemSelectionChanged, [this]() {
        updateGraphs();
    });

    connect(ui->filterOutlierBox, &QGroupBox::toggled, [this]() {
        truncateDataAndPlot(ui->autoZoomBox->isChecked());
    });

    connect(ui->pollGnssBox, &QCheckBox::toggled, [this]() {
        if (ui->pollGnssBox->isChecked()) {
            ui->map->setInfoTraceNow(2);
            ui->map->clearInfoTrace();
        }
    });

    connect(ui->filterhAccBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double newVal) {
        (void)newVal;
        if (ui->filterOutlierBox->isChecked()) {
            truncateDataAndPlot(ui->autoZoomBox->isChecked());
        }
    });

    connect(ui->tabWidget, &QTabWidget::currentChanged, [this](int index) {
        (void)index;
        logListRefresh();
    });

    on_gridBox_toggled(ui->gridBox->isChecked());
    logListRefresh();

    QSettings set;
    mLastSaveCsvPath = set.value("pageloganalysis/lastSaveCsvPath", true).toString();
}

PageLogAnalysis::~PageLogAnalysis()
{
    QSettings set;
    set.setValue("pageloganalysis/lastSaveCsvPath", mLastSaveCsvPath);
    set.sync();

    delete ui;
}

VescInterface *PageLogAnalysis::vesc() const
{
    return mVesc;
}

void PageLogAnalysis::setVesc(VescInterface *vesc)
{
    mVesc = vesc;

    if (mVesc) {
        auto updatePlots = [this]() {
            if (mLogRtFieldUpdatePending) {
                return;
            }

            storeSelection();

            resetInds();

            mLogHeader = mLogRtHeader;
            mLog = mLogRt;

            updateInds();
            generateMissingEntries();

            ui->dataTable->setRowCount(0);

            if (mLog.size() == 0) {
                return;
            }

            foreach (auto e, mLogHeader) {
                addDataItem(e.name, !e.isTimeStamp, e.scaleStep, e.scaleMax);
            }

            restoreSelection();

            truncateDataAndPlot(ui->autoZoomBox->isChecked());

            if (mInd_t_day >= 0 && !mLog.isEmpty()) {
                updateDataAndPlot(mLog.last().at(mInd_t_day));
            }
        };

        connect(mVesc->commands(), &Commands::logStart, [this]
                (int fieldNum, double rateHz, bool appendTime, bool appendGnss, bool appendGnssTime) {
            (void)appendGnss; (void)appendGnssTime;

            mLogRtAppendTime = appendTime;

            if (mLogRtAppendTime) {
                fieldNum++;
            }

            mLogRtHeader.resize(fieldNum);

            if (mLogRtAppendTime) {
                LOG_HEADER h;
                h.key = "t_day";
                h.name = "Time";
                h.unit = "s";
                h.isTimeStamp = true;

                for (int i = (mLogRtHeader.size() - 1);i > 0;i--) {
                    mLogRtHeader[i] = mLogRtHeader[i - 1];
                }

                mLogRtHeader[0] = h;
            }

            mLogRtFieldUpdatePending = false;
            mLogRtSamplesNow.resize(fieldNum);
            mLogRtTimer->start(1000.0 / rateHz);
            mLogRt.clear();
        });

        connect(mVesc->commands(), &Commands::logStop, [this, updatePlots] () {
           mLogRtTimer->stop();
           updatePlots();
        });

        connect(mVesc->commands(), &Commands::logConfigField, [this](int fieldInd, LOG_HEADER h) {
            mLogRtFieldUpdatePending = true;

            if (mLogRtHeader.size() <= fieldInd) {
                mLogRtHeader.resize(fieldInd + 1);
            }

            mLogRtHeader[fieldInd] = h;
        });

        connect(mVesc->commands(), &Commands::logSamples, [this](int fieldStart, QVector<double> samples) {
            if (mLogRtAppendTime) {
                fieldStart++;
            }

            for (int i = 0;i < samples.size();i++) {
                int ind = i + fieldStart;
                if (mLogRtSamplesNow.size() > ind) {
                    mLogRtSamplesNow[ind] = samples.at(i);
                }
            }
        });

        connect(mLogRtTimer, &QTimer::timeout, [this, updatePlots]() {
            if (mLogRtAppendTime) {
                mLogRtSamplesNow[0] = (double(QTime::currentTime().msecsSinceStartOfDay()) / 1000.0);
            }

            mLogRt.append(mLogRtSamplesNow);

            if (ui->updateRtBox->isChecked()) {
                updatePlots();
            }
        });

        connect(mVesc->commands(), &Commands::fileProgress, [this]
                (int32_t prog, int32_t tot, double percentage, double bytesPerSec) {
            QTime t(0, 0, 0, 0);;
            t = t.addSecs((tot - prog) / bytesPerSec);
            ui->vescDisplay->setValue(percentage);
            ui->vescDisplay->setText(tr("%1 KB/s, %2").
                                     arg(bytesPerSec / 1024, 0, 'f', 2).
                                     arg(t.toString("hh:mm:ss")));
        });

        connect(mVesc->commands(), &Commands::gnssRx, [this](GNSS_DATA val) {
            if (val.ms_today != mGnssMsTodayLast) {
                mGnssMsTodayLast = val.ms_today;

                if (ui->filterOutlierBox->isChecked() &&
                        (val.hdop * 5.0) > ui->filterhAccBox->value()) {
                    return;
                }

                double llh[3];
                double i_llh[3];
                double xyz[3];

                ui->map->getEnuRef(i_llh);
                llh[0] = val.lat;
                llh[1] = val.lon;
                llh[2] = val.height;

                Utility::llhToEnu(i_llh, llh, xyz);

                LocPoint p;
                p.setXY(xyz[0], xyz[1]);
                p.setRadius(10);
                ui->map->setInfoTraceNow(2);

                LocPoint p2;
                p2.setXY(0, 0);
                p2.setInfo(tr("Hdop %1").arg(val.hdop));

                if (p2.getDistanceTo(p) > 10000) {
                    ui->map->setEnuRef(llh[0], llh[1], llh[2]);
                    p.setXY(0, 0);
                    ui->map->clearAllInfoTraces();
                }

                ui->map->addInfoPoint(p);

                if (ui->followBox->isChecked()) {
                    ui->map->moveView(xyz[0], xyz[1]);
                }
            }
        });
    }
}

void PageLogAnalysis::loadVescLog(QVector<LOG_DATA> log)
{
    storeSelection();

    resetInds();

    mLog.clear();
    mLogTruncated.clear();
    mLogHeader.clear();

    mLogHeader.append(LOG_HEADER("kmh_vesc", "Speed VESC", "km/h"));
    mLogHeader.append(LOG_HEADER("kmh_gnss", "Speed GNSS", "km/h"));
    mLogHeader.append(LOG_HEADER("t_day", "Time", "s", 0, false, true));
    mLogHeader.append(LOG_HEADER("t_day_pos", "Time GNSS", "s", 0, false, true));
    mLogHeader.append(LOG_HEADER("t_trip", "Time of trip", "s", 0, true, true));
    mLogHeader.append(LOG_HEADER("trip_vesc", "Trip VESC", "m", 3, true));
    mLogHeader.append(LOG_HEADER("trip_vesc_abs", "Trip VESC ABS", "m", 3, true));
    mLogHeader.append(LOG_HEADER("trip_gnss", "Trip GNSS", "m", 3, true));
    mLogHeader.append(LOG_HEADER("setup_curr_motor", "Current Motors", "A"));
    mLogHeader.append(LOG_HEADER("setup_curr_battery", "Current Battery", "A"));
    mLogHeader.append(LOG_HEADER("setup_power", "Power", "W", 0));
    mLogHeader.append(LOG_HEADER("erpm", "ERPM", "", 0));
    mLogHeader.append(LOG_HEADER("duty", "Duty", "%", 1));
    mLogHeader.append(LOG_HEADER("fault", "Fault Code", "", 0));
    mLogHeader.append(LOG_HEADER("v_in", "Input Voltage", "V"));
    mLogHeader.append(LOG_HEADER("soc", "Battery Level", "%", 1));
    mLogHeader.append(LOG_HEADER("t_mosfet", "Temp MOSFET", "°C", 1));
    mLogHeader.append(LOG_HEADER("t_motor", "Temp Motor", "°C", 1));
    mLogHeader.append(LOG_HEADER("cnt_ah", "Ah Used", "Ah", 3));
    mLogHeader.append(LOG_HEADER("cnt_ah_chg", "Ah Charged", "Ah", 3));
    mLogHeader.append(LOG_HEADER("cnt_wh", "Wh Used", "Wh", 3));
    mLogHeader.append(LOG_HEADER("cnt_wh_chg", "Wh Charged", "Wh", 3));
    mLogHeader.append(LOG_HEADER("id", "id", "A"));
    mLogHeader.append(LOG_HEADER("iq", "iq", "A"));
    mLogHeader.append(LOG_HEADER("vd", "vd", "V"));
    mLogHeader.append(LOG_HEADER("vq", "vq", "V"));
    mLogHeader.append(LOG_HEADER("t_mosfet_1", "Temp MOSFET 1", "°C", 1));
    mLogHeader.append(LOG_HEADER("t_mosfet_2", "Temp MOSFET 2", "°C", 1));
    mLogHeader.append(LOG_HEADER("t_mosfet_3", "Temp MOSFET 3", "°C", 1));
    mLogHeader.append(LOG_HEADER("position", "Motor Pos", "°", 1));
    mLogHeader.append(LOG_HEADER("roll", "Roll", "°", 1));
    mLogHeader.append(LOG_HEADER("pitch", "Pitch", "°", 1));
    mLogHeader.append(LOG_HEADER("yaw", "Yaw", "°", 1));
    mLogHeader.append(LOG_HEADER("acc_x", "Accel X", "G"));
    mLogHeader.append(LOG_HEADER("acc_y", "Accel Y", "G"));
    mLogHeader.append(LOG_HEADER("acc_z", "Accel Z", "G"));
    mLogHeader.append(LOG_HEADER("gyro_x", "Gyro X", "°/s", 1));
    mLogHeader.append(LOG_HEADER("gyro_y", "Gyro Y", "°/s", 1));
    mLogHeader.append(LOG_HEADER("gyro_z", "Gyro Z", "°/s", 1));
    mLogHeader.append(LOG_HEADER("v1_curr_motor", "V1 Current", "A"));
    mLogHeader.append(LOG_HEADER("v1_curr_battery", "V1 Current Battery", "A"));
    mLogHeader.append(LOG_HEADER("v1_cnt_ah", "V1 Ah Used", "Ah", 3));
    mLogHeader.append(LOG_HEADER("v1_cnt_ah_chg", "V1 Ah Charged", "Ah", 3));
    mLogHeader.append(LOG_HEADER("v1_cnt_wh", "V1 Wh Used", "Wh", 3));
    mLogHeader.append(LOG_HEADER("v1_cnt_wh_chg", "V1 Wh Charged", "Wh", 3));
    mLogHeader.append(LOG_HEADER("gnss_lat", "Latitude", "°", 6));
    mLogHeader.append(LOG_HEADER("gnss_lon", "Longitude", "°", 6));
    mLogHeader.append(LOG_HEADER("gnss_alt", "Altitude", "m"));
    mLogHeader.append(LOG_HEADER("gnss_v_vel", "V. Speed GNSS", "km/h"));
    mLogHeader.append(LOG_HEADER("gnss_h_acc", "H. Accuracy GNSS", "m"));
    mLogHeader.append(LOG_HEADER("gnss_v_acc", "V. Accuracy GNSS", "m"));
    mLogHeader.append(LOG_HEADER("num_vesc", "VESC num", "", 0));

    double i_llh[3];
    for (auto d: log) {
        if (d.posTime >= 0 && (!ui->filterOutlierBox->isChecked() ||
                               d.hAcc < ui->filterhAccBox->value())) {
            i_llh[0] = d.lat;
            i_llh[1] = d.lon;
            i_llh[2] = d.alt;
            ui->map->setEnuRef(i_llh[0], i_llh[1], i_llh[2]);
            break;
        }
    }

    LOG_DATA prevSampleGnss;
    bool prevSampleGnssSet = false;
    double metersGnss = 0.0;

    for (auto d: log) {
        if (d.posTime >= 0 &&
                (!ui->filterOutlierBox->isChecked() ||
                 d.hAcc < ui->filterhAccBox->value())) {
            if (prevSampleGnssSet) {
                double i_llh[3];
                double llh[3];
                double xyz[3];
                ui->map->getEnuRef(i_llh);

                llh[0] = d.lat;
                llh[1] = d.lon;
                llh[2] = d.alt;
                Utility::llhToEnu(i_llh, llh, xyz);

                LocPoint p, p2;
                p.setXY(xyz[0], xyz[1]);
                p.setRadius(10);

                llh[0] = prevSampleGnss.lat;
                llh[1] = prevSampleGnss.lon;
                llh[2] = prevSampleGnss.alt;
                Utility::llhToEnu(i_llh, llh, xyz);

                p2.setXY(xyz[0], xyz[1]);
                p2.setRadius(10);

                metersGnss += p.getDistanceTo(p2);
            }

            prevSampleGnssSet = true;
            prevSampleGnss = d;
        }

        QVector<double> e;
        e.append(d.setupValues.speed * 3.6);
        e.append(d.gVel * 3.6);
        e.append(double(d.valTime) / 1000.0);
        e.append(double(d.posTime) / 1000.0);
        e.append(double(d.valTime) / 1000.0);
        e.append(d.setupValues.tachometer);
        e.append(d.setupValues.tachometer);
        e.append(metersGnss);
        e.append(d.setupValues.current_motor);
        e.append(d.setupValues.current_in);
        e.append(d.setupValues.current_in * d.values.v_in);
        e.append(d.values.rpm);
        e.append(d.values.duty_now * 100);
        e.append(d.values.fault_code);
        e.append(d.values.v_in);
        e.append(d.setupValues.battery_level * 100.0);
        e.append(d.values.temp_mos);
        e.append(d.values.temp_motor);
        e.append(d.setupValues.amp_hours);
        e.append(d.setupValues.amp_hours_charged);
        e.append(d.setupValues.watt_hours);
        e.append(d.setupValues.watt_hours_charged);
        e.append(d.values.id);
        e.append(d.values.iq);
        e.append(d.values.vd);
        e.append(d.values.vq);
        e.append(d.values.temp_mos_1);
        e.append(d.values.temp_mos_2);
        e.append(d.values.temp_mos_3);
        e.append(d.values.position);
        e.append(d.imuValues.roll);
        e.append(d.imuValues.pitch);
        e.append(d.imuValues.yaw);
        e.append(d.imuValues.accX);
        e.append(d.imuValues.accY);
        e.append(d.imuValues.accZ);
        e.append(d.imuValues.gyroX);
        e.append(d.imuValues.gyroY);
        e.append(d.imuValues.gyroZ);
        e.append(d.values.current_motor);
        e.append(d.values.current_in);
        e.append(d.values.amp_hours);
        e.append(d.values.amp_hours_charged);
        e.append(d.values.watt_hours);
        e.append(d.values.watt_hours_charged);
        e.append(d.lat);
        e.append(d.lon);
        e.append(d.alt);
        e.append(d.vVel * 3.6);
        e.append(d.hAcc);
        e.append(d.vAcc);
        e.append(d.setupValues.num_vescs);

        mLog.append(e);
    }

    updateInds();

    ui->dataTable->setRowCount(0);

    if (mLog.size() == 0) {
        return;
    }

    foreach (auto e, mLogHeader) {
        addDataItem(e.name, !e.isTimeStamp, e.scaleStep, e.scaleMax);
    }

    restoreSelection();

    truncateDataAndPlot();
}

void PageLogAnalysis::on_openCsvButton_clicked()
{
    if (mVesc) {
        QString dirPath = QSettings().value("pageloganalysis/lastdir", "").toString();
        QString fileName = QFileDialog::getOpenFileName(this,
                                                        tr("Load CSV File"), dirPath,
                                                        tr("CSV files (*.csv)"));

        if (!fileName.isEmpty()) {
            QSettings().setValue("pageloganalysis/lastdir",
                         QFileInfo(fileName).absolutePath());

            QFile inFile(fileName);
            if (inFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                openLog(inFile.readAll());
            }
        }
    }
}

void PageLogAnalysis::on_openCurrentButton_clicked()
{
    if (mVesc) {
        loadVescLog(mVesc->getRtLogData());
    }
}

void PageLogAnalysis::on_gridBox_toggled(bool checked)
{
    ui->map->setDrawGrid(checked);
}

void PageLogAnalysis::on_tilesHiResButton_toggled(bool checked)
{
    (void)checked;
    updateTileServers();
    ui->map->update();
}

void PageLogAnalysis::on_tilesOsmButton_toggled(bool checked)
{
    (void)checked;
    updateTileServers();
    ui->map->update();
}

void PageLogAnalysis::truncateDataAndPlot(bool zoomGraph)
{
    double start = double(ui->spanSlider->alt_value()) / 10000.0;
    double end = double(ui->spanSlider->value()) / 10000.0;

    ui->map->setInfoTraceNow(0);
    ui->map->clearAllInfoTraces();

    int ind = 0;
    double i_llh[3];
    int posTimeLast = -1;

    ui->map->getEnuRef(i_llh);
    mLogTruncated.clear();

    for (const auto &d: mLog) {
        ind++;
        double prop = double(ind) / double(mLog.size());
        if (prop < start || prop > end) {
            continue;
        }

        mLogTruncated.append(d);
        bool skip = false;

        if (mInd_t_day_pos >= 0 && mInd_gnss_h_acc >= 0) {
            int postime = int(d[mInd_t_day_pos] * 1000.0);
            double h_acc = d[mInd_gnss_h_acc];

            skip = true;
            if (h_acc > 0.0 &&
                    (!ui->filterOutlierBox->isChecked() ||
                     h_acc < ui->filterhAccBox->value()) &&
                    posTimeLast != postime) {
                skip = false;
                posTimeLast = postime;
            }
        }

        if (mInd_gnss_lat < 0 || mInd_gnss_lon < 0) {
            skip = true;
        }

        if (!skip) {
            double llh[3];
            double xyz[3];

            llh[0] = d[mInd_gnss_lat];
            llh[1] = d[mInd_gnss_lon];
            if (mInd_gnss_alt >= 0) {
                llh[2] = d[mInd_gnss_alt];
            } else {
                llh[2] = 0.0;
            }
            Utility::llhToEnu(i_llh, llh, xyz);

            LocPoint p;
            p.setXY(xyz[0], xyz[1]);
            p.setRadius(5);

            if (mInd_t_day >= 0) {
                p.setInfo(QString("%1").arg(d[mInd_t_day]));
            }

            ui->map->addInfoPoint(p, false);
        }
    }

    if (zoomGraph) {
        ui->map->zoomInOnInfoTrace(-1, 0.1);
    }

    ui->map->update();
    updateGraphs();
    updateStats();
}

void PageLogAnalysis::updateGraphs()
{
    auto rows = ui->dataTable->selectionModel()->selectedRows();

    QVector<double> xAxis;
    QVector<QVector<double> > yAxes;
    QVector<QString> names;

    double startTime = -1.0;
    double verticalTime = -1.0;
    LocPoint p, p2;

    double time = 0;
    foreach (const auto &d, mLogTruncated) {
        if (mInd_t_day >= 0) {
            if (startTime < 0) {
                startTime = d[mInd_t_day];
            }

            time = d[mInd_t_day] - startTime;
            if (time < 0) { // Handle midnight
                time += 60 * 60 * 24;
            }
        } else {
            time++;;
        }

        xAxis.append(time);
        int rowInd = 0;

        for (int r = 0;r < rows.size();r++) {
            int row = rows.at(r).row();
            double rowScale = 1.0;
            if(QDoubleSpinBox *sb = qobject_cast<QDoubleSpinBox*>
                    (ui->dataTable->cellWidget(row, 2))) {
                rowScale = sb->value();
            }

            auto entry = d[row];
            const auto &header = mLogHeader[row];

            if (!header.isTimeStamp) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(entry * rowScale);
                names.append(QString("%1 (%2 * %3)").arg(header.name).
                             arg(header.unit).arg(rowScale));
                rowInd++;
            }
        }
    }

    ui->plot->clearGraphs();

    for (int i = 0;i < yAxes.size();i++) {
        QPen pen = QPen(Utility::getAppQColor("plot_graph1"));

        if (i == 1) {
            pen = QPen(Qt::magenta);
        } else if (i == 2) {
            pen = QPen(Utility::getAppQColor("plot_graph2"));
        } else if (i == 3) {
            pen = QPen(Utility::getAppQColor("plot_graph3"));
        } else if (i == 4) {
            pen = QPen(Qt::cyan);
        } else if (i == 5) {
            pen = QPen(Utility::getAppQColor("plot_graph4"));
        }

        ui->plot->addGraph();
        ui->plot->graph(i)->setPen(pen);
        ui->plot->graph(i)->setName(names.at(i));
        ui->plot->graph(i)->setData(xAxis, yAxes.at(i));
    }

    mVerticalLine->setVisible(false);

    if (yAxes.size() > 0) {
        ui->plot->rescaleAxes(true);
    } else if (xAxis.size() >= 2) {
        ui->plot->xAxis->setRangeLower(xAxis.first());
        ui->plot->xAxis->setRangeUpper(xAxis.last());
    }

    if (verticalTime >= 0) {
        QVector<double> x(2) , y(2);
        x[0] = verticalTime; y[0] = ui->plot->yAxis->range().lower;
        x[1] = verticalTime; y[1] = ui->plot->yAxis->range().upper;
        mVerticalLine->setData(x, y);
        mVerticalLine->setVisible(true);
    }

    ui->plot->replotWhenVisible();
}

void PageLogAnalysis::updateStats()
{
    if (mLogTruncated.size() < 2) {
            return;
    }

    auto startSample = mLogTruncated.first();
    auto endSample = mLogTruncated.last();

    int samples = mLogTruncated.size();
    int timeTotMs = 0;

    if (samples < 2) {
        return;
    }

    if (mInd_t_day >= 0) {
        timeTotMs = (endSample[mInd_t_day] - startSample[mInd_t_day]) * 1000.0;
        if (timeTotMs < 0) { // Handle midnight
            timeTotMs += 60 * 60 * 24 * 1000;
        }
    }

    double meters = 0.0;
    double metersAbs = 0.0;
    double metersGnss = 0.0;
    double wh = 0.0;
    double whCharge = 0.0;
    double ah = 0.0;
    double ahCharge = 0.0;

    if (mInd_trip_vesc >= 0) {
        meters = endSample[mInd_trip_vesc] - startSample[mInd_trip_vesc];
    }

    if (mInd_trip_vesc_abs >= 0) {
        metersAbs = endSample[mInd_trip_vesc_abs] - startSample[mInd_trip_vesc_abs];
    }

    if (mInd_trip_gnss >= 0) {
        metersGnss = endSample[mInd_trip_gnss] - startSample[mInd_trip_gnss];
    }

    if (mInd_cnt_wh >= 0) {
        wh = endSample[mInd_cnt_wh] - startSample[mInd_cnt_wh];
    }

    if (mInd_cnt_wh_chg >= 0) {
        whCharge = endSample[mInd_cnt_wh_chg] - startSample[mInd_cnt_wh_chg];
    }

    if (mInd_cnt_ah >= 0) {
        ah = endSample[mInd_cnt_ah] - startSample[mInd_cnt_ah];
    }

    if (mInd_cnt_ah_chg >= 0) {
        ahCharge = endSample[mInd_cnt_ah_chg] - startSample[mInd_cnt_ah_chg];
    }

    ui->statTable->setRowCount(0);
    auto addStatItem = [this](QString name) {
        ui->statTable->setRowCount(ui->statTable->rowCount() + 1);
        ui->statTable->setItem(ui->statTable->rowCount() - 1, 0, new QTableWidgetItem(name));
        ui->statTable->setItem(ui->statTable->rowCount() - 1, 1, new QTableWidgetItem(""));
    };

    addStatItem("Samples");
    addStatItem("Total Time");
    addStatItem("Distance");
    addStatItem("Distance ABS");
    addStatItem("Distance GNSS");
    addStatItem("Wh");
    addStatItem("Wh Charged");
    addStatItem("Ah");
    addStatItem("Ah Charged");
    addStatItem("Avg Speed");
    addStatItem("Avg Speed GNSS");
    addStatItem("Efficiency");
    addStatItem("Efficiency GNSS");
    addStatItem("Avg Sample Rate");

    QTime t(0, 0, 0, 0);
    t = t.addMSecs(timeTotMs);

    ui->statTable->item(0, 1)->setText(QString::number(samples));
    ui->statTable->item(1, 1)->setText(t.toString("hh:mm:ss.zzz"));
    ui->statTable->item(2, 1)->setText(QString::number(meters, 'f', 2) + " m");
    ui->statTable->item(3, 1)->setText(QString::number(metersAbs, 'f', 2) + " m");
    ui->statTable->item(4, 1)->setText(QString::number(metersGnss, 'f', 2) + " m");
    ui->statTable->item(5, 1)->setText(QString::number(wh, 'f', 2) + " Wh");
    ui->statTable->item(6, 1)->setText(QString::number(whCharge, 'f', 2) + " Wh");
    ui->statTable->item(7, 1)->setText(QString::number(ah, 'f', 2) + " Ah");
    ui->statTable->item(8, 1)->setText(QString::number(ahCharge, 'f', 2) + " Ah");
    ui->statTable->item(9, 1)->setText(QString::number(3.6 * metersAbs / (double(timeTotMs) / 1000.0), 'f', 2) + " km/h");
    ui->statTable->item(10, 1)->setText(QString::number(3.6 * metersGnss / (double(timeTotMs) / 1000.0), 'f', 2) + " km/h");
    ui->statTable->item(11, 1)->setText(QString::number((wh - whCharge) / (metersAbs / 1000.0), 'f', 2) + " wh/km");
    ui->statTable->item(12, 1)->setText(QString::number((wh - whCharge) / (metersGnss / 1000.0), 'f', 2) + " wh/km");
    ui->statTable->item(13, 1)->setText(QString::number(double(samples) / (double(timeTotMs) / 1000.0), 'f', 2) + " Hz");
}

void PageLogAnalysis::updateDataAndPlot(double time)
{
    if (mLogTruncated.isEmpty()) {
        return;
    }

    mPlayPosNow = time;

    double upper = ui->plot->xAxis->range().upper;
    double lower = ui->plot->xAxis->range().lower;
    if (time > upper) {
        time = upper;
    } else if (time < lower) {
        time = lower;
    }
    QVector<double> x(2) , y(2);
    x[0] = time; y[0] = ui->plot->yAxis->range().lower;
    x[1] = time; y[1] = ui->plot->yAxis->range().upper;
    mVerticalLine->setData(x, y);
    mVerticalLine->setVisible(true);
    ui->plot->replotWhenVisible();

    auto sample = getLogSample(time);
    auto first = mLogTruncated.first();

    int ind = 0;
    for (int i = 0;i < sample.size();i++) {
        auto value = sample.at(i);
        const auto &header = mLogHeader[i];

        if (header.isRelativeToFirst) {
            value -= first[i];

            if (header.isTimeStamp && value < 0) {
                value += 60 * 60 * 24;
            }
        }

        if (ind != mInd_fault) {
            if (header.isTimeStamp) {
                QTime t(0, 0, 0, 0);
                t = t.addMSecs(value * 1000);
                ui->dataTable->item(ind, 1)->setText(t.toString("hh:mm:ss.zzz"));
            } else {
                ui->dataTable->item(ind, 1)->setText(
                            QString::number(value, 'f', header.precision) + " " + header.unit);
            }
        } else {
            ui->dataTable->item(ind, 1)->setText(Commands::faultToStr(mc_fault_code(round(value))).mid(11));
        }

        ind++;
    }

    bool skip = false;
    if (mInd_gnss_h_acc >= 0) {
        double h_acc = sample[mInd_gnss_h_acc];

        skip = true;
        if (h_acc > 0.0 &&
                (!ui->filterOutlierBox->isChecked() ||
                 h_acc < ui->filterhAccBox->value())) {
            skip = false;
        }
    }

    if (mInd_gnss_lat < 0 || mInd_gnss_lon < 0) {
        skip = true;
    }

    if (!skip) {
        double i_llh[3];
        double llh[3];
        double xyz[3];

        ui->map->getEnuRef(i_llh);
        llh[0] = sample[mInd_gnss_lat];
        llh[1] = sample[mInd_gnss_lon];
        if (mInd_gnss_alt >= 0) {
            llh[2] = sample[mInd_gnss_alt];
        } else {
            llh[2] = 0.0;
        }
        Utility::llhToEnu(i_llh, llh, xyz);

        LocPoint p;
        p.setXY(xyz[0], xyz[1]);
        p.setRadius(10);

        ui->map->setInfoTraceNow(1);
        ui->map->clearInfoTrace();
        ui->map->addInfoPoint(p);

        if (ui->followBox->isChecked()) {
            ui->map->moveView(xyz[0], xyz[1]);
        }
    }

    if (mInd_roll >= 0 && mInd_pitch >= 0 && mInd_yaw >= 0) {
        m3dView->setRollPitchYaw(sample[mInd_roll] * 180.0 / M_PI, sample[mInd_pitch] * 180.0 / M_PI,
                                 mUseYawBox->isChecked() ? sample[mInd_yaw] * 180.0 / M_PI : 0.0);
    }
}

QVector<double> PageLogAnalysis::getLogSample(double time)
{
    QVector<double> d;

    if (!mLogTruncated.isEmpty()) {
        d = mLogTruncated.first();

        if (mInd_t_day >= 0) {
            double startTime = d[mInd_t_day];

            for (auto dn: mLogTruncated) {
                double timeNow = dn[mInd_t_day] - startTime;
                if (timeNow < 0) { // Handle midnight
                    timeNow += 60 * 60 * 24;
                }

                if (timeNow >= time) {
                    d = dn;
                    break;
                }
            }
        }
    }

    return d;
}

void PageLogAnalysis::updateTileServers()
{
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    if (ui->tilesOsmButton->isChecked()) {
        ui->map->osmClient()->setTileServerUrl("http://tile.openstreetmap.org");
        ui->map->osmClient()->setCacheDir(base + "/osm_tiles/osm");
        ui->map->osmClient()->clearCacheMemory();
    } else if (ui->tilesHiResButton->isChecked()) {
        ui->map->osmClient()->setTileServerUrl("http://c.osm.rrze.fau.de/osmhd");
        ui->map->osmClient()->setCacheDir(base + "/osm_tiles/hd");
        ui->map->osmClient()->clearCacheMemory();
    }
}

void PageLogAnalysis::logListRefresh()
{
    ui->logTable->setRowCount(0);
    QSettings set;
    if (set.contains("pageloganalysis/lastdir")) {
        QString dirPath = set.value("pageloganalysis/lastdir").toString();
        QDir dir(dirPath);
        if (dir.exists()) {
            for (QFileInfo f: dir.entryInfoList(QStringList() << "*.csv" << "*.Csv" << "*.CSV",
                                                QDir::Files, QDir::Name)) {
                QTableWidgetItem *itName = new QTableWidgetItem(f.fileName());
                itName->setData(Qt::UserRole, f.absoluteFilePath());
                ui->logTable->setRowCount(ui->logTable->rowCount() + 1);
                ui->logTable->setItem(ui->logTable->rowCount() - 1, 0, itName);
                ui->logTable->setItem(ui->logTable->rowCount() - 1, 1,
                                      new QTableWidgetItem(QString("%1 MB").
                                                           arg(double(f.size())
                                                               / 1024.0 / 1024.0,
                                                               0, 'f', 2)));
            }
        }
    }
}

void PageLogAnalysis::addDataItem(QString name, bool hasScale, double scaleStep, double scaleMax)
{
    ui->dataTable->setRowCount(ui->dataTable->rowCount() + 1);
    auto item1 = new QTableWidgetItem(name);
    ui->dataTable->setItem(ui->dataTable->rowCount() - 1, 0, item1);
    ui->dataTable->setItem(ui->dataTable->rowCount() - 1, 1, new QTableWidgetItem(""));
    if (hasScale) {
        QDoubleSpinBox *sb = new QDoubleSpinBox;
        sb->setSingleStep(scaleStep);
        sb->setValue(1.0);
        sb->setMaximum(scaleMax);
        // Prevent mouse wheel focus to avoid changing the selection
        sb->setFocusPolicy(Qt::StrongFocus);
        ui->dataTable->setCellWidget(ui->dataTable->rowCount() - 1, 2, sb);
        connect(sb, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                [this](double value) {
            (void)value;
            updateGraphs();
        });
    } else {
        ui->dataTable->setItem(ui->dataTable->rowCount() - 1, 2, new QTableWidgetItem("Not Plottable"));
    }
}

void PageLogAnalysis::openLog(QByteArray data)
{
    storeSelection();

    QTextStream in(&data);
    auto tokensLine1 = in.readLine().split(";");
    if (tokensLine1.size() < 1) {
        mVesc->emitStatusMessage("Invalid log file", false);
        return;
    }
    auto entry1 = tokensLine1.first().split(":");
    if (entry1.size() == 1) {
        if (mVesc->loadRtLogFile(data)) {
            on_openCurrentButton_clicked();
        }
    } else {
        resetInds();

        mLog.clear();
        mLogTruncated.clear();
        mLogHeader.clear();

        QVector<double> entryLastData;

        for (auto t: tokensLine1) {
            auto token = t.split(":");
            LOG_HEADER h;
            for (int i = 0;i < t.size();i++) {
                switch (i) {
                case 0: h.key = token.at(i);
                case 1: h.name = token.at(i);
                case 2: h.unit = token.at(i);
                case 3: h.precision = token.at(i).toDouble();
                case 4: h.isRelativeToFirst = token.at(i).toInt();
                case 5: h.isTimeStamp = token.at(i).toInt();
                default: break;
                }
            }
            mLogHeader.append(h);
            entryLastData.append(0.0);
        }

        while (!in.atEnd()) {
            QStringList tokens = in.readLine().split(";");
            QVector<double> entry;
            for (int i = 0;i < tokens.size();i++) {
                if (i >= entryLastData.size()) {
                   break;
                }
                if (!tokens.at(i).isEmpty()) {
                    entryLastData[i] = tokens.at(i).toDouble();
                }
                entry.append(entryLastData.at(i));
            }
            while (entry.size() < entryLastData.size()) {
                entry.append(entryLastData[entry.size()]);
            }
            mLog.append(entry);
        }

        updateInds();

        generateMissingEntries();

        ui->dataTable->setRowCount(0);

        if (mLog.size() == 0) {
            return;
        }

        foreach (auto e, mLogHeader) {
            addDataItem(e.name, !e.isTimeStamp, e.scaleStep, e.scaleMax);
        }

        restoreSelection();

        truncateDataAndPlot();
    }
}

void PageLogAnalysis::generateMissingEntries()
{
    // Create sample array if t_day is missing
    if (mInd_t_day < 0) {
        mLogHeader.append(LOG_HEADER("t_day", "Sample", "", 0));

        for (int i = 0;i < mLog.size();i++) {
            mLog[i].append(i);
        }
    }

    updateInds();

    if (mInd_gnss_lat >= 0 && mInd_gnss_lon >= 0) {

        // Initialize map enu ref
        double i_llh[3] = {57.71495867, 12.89134921, 220.0};
        for (auto d: mLog) {
            double lat = d.at(mInd_gnss_lat);
            double lon = d.at(mInd_gnss_lon);
            double alt = 0;
            if (mInd_gnss_alt >= 0) {
                alt = d.at(mInd_gnss_alt);
            }

            double hacc = 0.0;
            if (mInd_gnss_h_acc >= 0) {
                hacc = d.at(mInd_gnss_h_acc);
            }

            if (hacc > 0.0 && (!ui->filterOutlierBox->isChecked() ||
                             hacc < ui->filterhAccBox->value())) {
                i_llh[0] = lat;
                i_llh[1] = lon;
                i_llh[2] = alt;
                ui->map->setEnuRef(i_llh[0], i_llh[1], i_llh[2]);
                break;
            }
        }

        // Create GNSS trip counter if it is missing
        if (mInd_trip_vesc < 0) {
            mLogHeader.append(LOG_HEADER("trip_gnss", "Trip GNSS", "m", 3, true));

            double prevSampleGnss[3];
            bool prevSampleGnssSet = false;
            double metersGnss = 0.0;

            for (int i = 0;i < mLog.size();i++) {
                double lat = mLog.at(i).at(mInd_gnss_lat);
                double lon = mLog.at(i).at(mInd_gnss_lon);
                double alt = 0;
                if (mInd_gnss_alt >= 0) {
                    alt = mLog.at(i).at(mInd_gnss_alt);
                }

                double hacc = 0.0;
                if (mInd_gnss_h_acc >= 0) {
                    hacc = mLog.at(i).at(mInd_gnss_h_acc);
                }

                if (hacc > 0 && (!ui->filterOutlierBox->isChecked() ||
                                 hacc < ui->filterhAccBox->value())) {
                    if (prevSampleGnssSet) {
                        double i_llh[3];
                        double llh[3];
                        double xyz[3];
                        ui->map->getEnuRef(i_llh);

                        llh[0] = lat;
                        llh[1] = lon;
                        llh[2] = alt;
                        Utility::llhToEnu(i_llh, llh, xyz);

                        LocPoint p, p2;
                        p.setXY(xyz[0], xyz[1]);
                        p.setRadius(10);

                        llh[0] = prevSampleGnss[0];
                        llh[1] = prevSampleGnss[1];
                        llh[2] = prevSampleGnss[2];
                        Utility::llhToEnu(i_llh, llh, xyz);

                        p2.setXY(xyz[0], xyz[1]);
                        p2.setRadius(10);

                        metersGnss += p.getDistanceTo(p2);
                    }

                    prevSampleGnssSet = true;
                    prevSampleGnss[0] = lat;
                    prevSampleGnss[1] = lon;
                    prevSampleGnss[2] = alt;
                }

                mLog[i].append(metersGnss);
            }
        }
    }

    updateInds();
}

void PageLogAnalysis::storeSelection()
{
    mSelection.dataLabels.clear();
    foreach (auto i, ui->dataTable->selectionModel()->selectedRows()) {
        mSelection.dataLabels.append(ui->dataTable->item(i.row(), 0)->text());
    }
    mSelection.scrollPos = ui->dataTable->verticalScrollBar()->value();
}

void PageLogAnalysis::restoreSelection()
{
    ui->dataTable->clearSelection();
    auto modeOld = ui->dataTable->selectionMode();
    ui->dataTable->setSelectionMode(QAbstractItemView::MultiSelection);
    for (int row = 0;row < ui->dataTable->rowCount();row++) {
        bool selected = false;
        foreach (auto i, mSelection.dataLabels) {
            if (ui->dataTable->item(row, 0)->text() == i) {
                selected = true;
                break;
            }
        }

        if (selected) {
            ui->dataTable->selectRow(row);
        }
    }
    ui->dataTable->setSelectionMode(modeOld);
    ui->dataTable->verticalScrollBar()->setValue(mSelection.scrollPos);
}

void PageLogAnalysis::setFileButtonsEnabled(bool en)
{
    ui->vescLogListOpenButton->setEnabled(en);
    ui->vescSaveAsButton->setEnabled(en);
    ui->vescLogListRefreshButton->setEnabled(en);
    ui->vescLogDeleteButton->setEnabled(en);
    ui->vescUpButton->setEnabled(en);
}

void PageLogAnalysis::on_saveMapPdfButton_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("Save PDF"), "",
                                                    tr("PDF Files (*.pdf)"));

    if (!fileName.isEmpty()) {
        if (!fileName.toLower().endsWith(".pdf")) {
            fileName.append(".pdf");
        }

        ui->map->printPdf(fileName,
                          ui->saveWidthBox->value(),
                          ui->saveHeightBox->value());
    }
}

void PageLogAnalysis::on_saveMapPngButton_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("Save Image"), "",
                                                    tr("PNG Files (*.png)"));

    if (!fileName.isEmpty()) {
        if (!fileName.toLower().endsWith(".png")) {
            fileName.append(".png");
        }

        ui->map->printPng(fileName,
                          ui->saveWidthBox->value(),
                          ui->saveHeightBox->value());
    }
}

void PageLogAnalysis::on_savePlotPdfButton_clicked()
{
    Utility::plotSavePdf(ui->plot, ui->saveWidthBox->value(), ui->saveHeightBox->value());
}

void PageLogAnalysis::on_savePlotPngButton_clicked()
{
    Utility::plotSavePng(ui->plot, ui->saveWidthBox->value(), ui->saveHeightBox->value());
}

void PageLogAnalysis::on_centerButton_clicked()
{
    ui->map->zoomInOnInfoTrace(-1, 0.1);
}

void PageLogAnalysis::on_logListOpenButton_clicked()
{
    auto items = ui->logTable->selectedItems();

    if (items.size() > 0) {
        QString fileName = items.
                first()->data(Qt::UserRole).toString();

        QFile inFile(fileName);
        if (inFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            openLog(inFile.readAll());
        }
    } else {
        mVesc->emitMessageDialog("Open Log", "No Log Selected", false);
    }
}

void PageLogAnalysis::on_logListRefreshButton_clicked()
{
    logListRefresh();
}

void PageLogAnalysis::on_logTable_cellDoubleClicked(int row, int column)
{
    (void)row; (void)column;
    on_logListOpenButton_clicked();
}

void PageLogAnalysis::on_vescLogListRefreshButton_clicked()
{
    if (!mVesc->isPortConnected()) {
        mVesc->emitMessageDialog("Refresh", "Not conected", false, false);
        mVescLastPath = "";
        return;
    }

    ui->vescLogTable->setRowCount(0);

    ui->vescLogTab->setEnabled(false);
    auto res = mVesc->commands()->fileBlockList(mVescLastPath);
    ui->vescLogTab->setEnabled(true);

    for (auto f: res) {
        FILE_LIST_ENTRY fe;
        if (f.canConvert<FILE_LIST_ENTRY>()) {
            fe = f.value<FILE_LIST_ENTRY>();
        }

        if (!fe.isDir && !fe.name.toLower().endsWith(".csv")) {
            continue;
        }

        QTableWidgetItem *itName = new QTableWidgetItem(fe.name);
        itName->setData(Qt::UserRole, QVariant::fromValue(fe));
        itName->setIcon(fe.isDir ? Utility::getIcon("icons/Open Folder-96.png") : Utility::getIcon("icons/Line Chart-96.png"));
        ui->vescLogTable->setRowCount(ui->vescLogTable->rowCount() + 1);
        ui->vescLogTable->setItem(ui->vescLogTable->rowCount() - 1, 0, itName);

        if (fe.isDir) {
            ui->vescLogTable->setItem(ui->vescLogTable->rowCount() - 1, 1,
                                      new QTableWidgetItem(QString("Dir, %1 files").arg(fe.size)));
        } else {
            ui->vescLogTable->setItem(ui->vescLogTable->rowCount() - 1, 1,
                                      new QTableWidgetItem(QString("%1 MB").
                                                           arg(double(fe.size)
                                                               / 1024.0 / 1024.0,
                                                               0, 'f', 2)));
        }
    }
}



void PageLogAnalysis::on_vescLogListOpenButton_clicked()
{
    if (!ui->vescLogListOpenButton->isEnabled()) {
        return;
    }

    if (!mVesc->isPortConnected()) {
        mVesc->emitMessageDialog("Open", "Not conected", false, false);
        mVescLastPath = "";
        return;
    }

    auto items = ui->vescLogTable->selectedItems();

    if (items.size() > 0) {
        FILE_LIST_ENTRY fe;
        if (items.first()->data(Qt::UserRole).canConvert<FILE_LIST_ENTRY>()) {
            fe = items.first()->data(Qt::UserRole).value<FILE_LIST_ENTRY>();
        }

        if (fe.isDir) {
            mVescLastPath += "/" + fe.name;
            mVescLastPath.replace("//", "/");
            on_vescLogListRefreshButton_clicked();
        } else {
            setFileButtonsEnabled(false);
            auto data = mVesc->commands()->fileBlockRead(mVescLastPath + "/" + fe.name);
            setFileButtonsEnabled(true);
            if (!data.isEmpty()) {
                openLog(data);
            }
        }
    }
}

void PageLogAnalysis::on_vescUpButton_clicked()
{
    if (!mVesc->isPortConnected()) {
        mVesc->emitMessageDialog("Up", "Not conected", false, false);
        mVescLastPath = "";
        return;
    }

    if (mVescLastPath.lastIndexOf("/") >= 0) {
        mVescLastPath = mVescLastPath.mid(0, mVescLastPath.lastIndexOf("/"));
        on_vescLogListRefreshButton_clicked();
    }
}

void PageLogAnalysis::on_vescLogCancelButton_clicked()
{
    mVesc->commands()->fileBlockCancel();
}

void PageLogAnalysis::on_vescLogTable_cellDoubleClicked(int row, int column)
{
    (void)row; (void)column;
    on_vescLogListOpenButton_clicked();
}

void PageLogAnalysis::on_vescSaveAsButton_clicked()
{
    auto items = ui->vescLogTable->selectedItems();

    if (items.size() <= 0) {
        mVesc->emitMessageDialog("Save File", "No file selected", false);
        return;
    }

    FILE_LIST_ENTRY fe;
    if (items.first()->data(Qt::UserRole).canConvert<FILE_LIST_ENTRY>()) {
        fe = items.first()->data(Qt::UserRole).value<FILE_LIST_ENTRY>();
    }

    if (fe.isDir) {
        mVesc->emitMessageDialog("Save File", "Cannot save directory, only files", false);
    } else {
        qDebug() << items.size();

        if (items.size() == 2) {
            QString fileName = QFileDialog::getSaveFileName(this,
                                                            tr("Save Log File"), "",
                                                            tr("CSV files (*.csv)"));

            if (!fileName.isEmpty()) {
                if (!fileName.endsWith(".csv", Qt::CaseInsensitive)) {
                    fileName += ".csv";
                }

                QFile file(fileName);

                if (!file.open(QIODevice::WriteOnly)) {
                    mVesc->emitMessageDialog("Save File", "Cannot open destination", false);
                    return;
                }

                setFileButtonsEnabled(false);
                auto data = mVesc->commands()->fileBlockRead(mVescLastPath + "/" + fe.name);
                setFileButtonsEnabled(true);

                file.write(data);
                file.close();
            }
        } else {
            QString path = QFileDialog::getExistingDirectory(this,
                                                             tr("Choose Destination"),
                                                             "",
                                                             QFileDialog::ShowDirsOnly |
                                                             QFileDialog::DontResolveSymlinks);
            if (!path.isEmpty()) {
                setFileButtonsEnabled(false);

                bool didCancel = false;
                foreach (auto it, items) {
                    if (didCancel) {
                        break;
                    }

                    if (it->data(Qt::UserRole).canConvert<FILE_LIST_ENTRY>()) {
                        fe = it->data(Qt::UserRole).value<FILE_LIST_ENTRY>();
                        if (!fe.isDir) {
                            QFile file(path + "/" + fe.name);
                            if (file.open(QIODevice::WriteOnly)) {
                                auto data = mVesc->commands()->fileBlockRead(mVescLastPath + "/" + fe.name);
                                didCancel = mVesc->commands()->fileBlockDidCancel();
                                file.write(data);
                                file.close();
                            }
                        }
                    }
                }

                setFileButtonsEnabled(true);
            }
        }
    }
}

void PageLogAnalysis::on_vescLogDeleteButton_clicked()
{
    auto items = ui->vescLogTable->selectedItems();

    if (items.size() <= 0) {
        mVesc->emitMessageDialog("Delete File", "No file selected", false);
        return;
    }

    int ret = QMessageBox::Cancel;
    if (items.size() == 2) {
        FILE_LIST_ENTRY fe;
        if (items.first()->data(Qt::UserRole).canConvert<FILE_LIST_ENTRY>()) {
            fe = items.first()->data(Qt::UserRole).value<FILE_LIST_ENTRY>();
        }

        if (fe.isDir) {
            ret = QMessageBox::warning(this,
                                       tr("Delete Directory"),
                                       tr("This is going to delete %1 and its content permanently. Are you sure?").arg(fe.name),
                                       QMessageBox::Yes | QMessageBox::Cancel);
        } else {
            ret = QMessageBox::warning(this,
                                       tr("Delete File"),
                                       tr("This is going to delete %1 permanently. Are you sure?").arg(fe.name),
                                       QMessageBox::Yes | QMessageBox::Cancel);
        }
    } else {
        ret = QMessageBox::warning(this,
                                   tr("Delete"),
                                   tr("This is going to delete the selected files and directories. Are you sure?"),
                                   QMessageBox::Yes | QMessageBox::Cancel);
    }

    if (ret == QMessageBox::Yes) {
        ui->vescLogTab->setEnabled(false);
        int cnt = 0;
        foreach (auto it, items) {
            if (it->data(Qt::UserRole).canConvert<FILE_LIST_ENTRY>()) {
                auto fe = it->data(Qt::UserRole).value<FILE_LIST_ENTRY>();
                bool ok = mVesc->commands()->fileBlockRemove(mVescLastPath + "/" + fe.name);
                if (ok) {
                    cnt++;
                    mVesc->emitStatusMessage("File deleted", true);
                }
            }
        }
        ui->vescLogTab->setEnabled(true);

        if (cnt > 0) {
            on_vescLogListRefreshButton_clicked();
        }
    }
}

void PageLogAnalysis::on_saveCsvButton_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("Save Log File"), mLastSaveCsvPath,
                                                    tr("CSV files (*.csv)"));

    if (!fileName.isEmpty()) {
        if (!fileName.toLower().endsWith(".csv")) {
            fileName += ".csv";
        }

        QFile file(fileName);

        if (!file.open(QIODevice::WriteOnly)) {
            mVesc->emitMessageDialog("Save File", "Cannot open destination", false);
            return;
        }

        mLastSaveCsvPath = fileName;

        QTextStream os(&file);

        for (int i = 0;i < mLogHeader.size();i++) {
            auto h = mLogHeader.at(i);
            os << h.key << ":"
               << h.name << ":"
               << h.unit << ":"
               << h.precision << ":"
               << h.isRelativeToFirst << ":"
               << h.isTimeStamp;

            if (i < (mLogHeader.size() - 1)) {
                os << ";";
            }
        }

        os << "\n";

        for (int i = 0;i < mLog.size();i++) {
            for (int j = 0;j < mLog.at(i).size();j++) {
                os << Qt::fixed
                   << qSetRealNumberPrecision(mLogHeader.at(j).precision)
                   << mLog.at(i).at(j);

                if (j < (mLog.at(i).size() - 1)) {
                    os << ";";
                }
            }
            os << "\n";
        }

        file.close();
    }
}
