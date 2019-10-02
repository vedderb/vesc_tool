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
#include <cmath>

PageLogAnalysis::PageLogAnalysis(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PageLogAnalysis)
{
    ui->setupUi(this);
    mVesc = nullptr;

    updateTileServers();

    ui->spanSlider->setMinimum(0);
    ui->spanSlider->setMaximum(10000);
    ui->spanSlider->setValue(10000);

    ui->mapSplitter->setStretchFactor(0, 4);
    ui->mapSplitter->setStretchFactor(1, 1);

    ui->statSplitter->setStretchFactor(0, 6);
    ui->statSplitter->setStretchFactor(1, 1);

    ui->plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    ui->plot->axisRect()->setRangeZoom(nullptr);
    ui->plot->axisRect()->setRangeDrag(nullptr);

    ui->dataTable->setColumnWidth(0, 140);
    ui->dataTable->setColumnWidth(1, 120);
    ui->statTable->setColumnWidth(0, 140);

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
        if (ui->playButton->isChecked()) {
            mPlayPosNow += double(mPlayTimer->interval()) / 1000.0;

            int timeMs = mLogDataTruncated.last().valTime - mLogDataTruncated.first().valTime;
            if (timeMs < 0) { // Handle midnight
                timeMs += 60 * 60 * 24 * 1000;
            }

            if (mLogDataTruncated.size() > 0 &&
                    mPlayPosNow <= double(timeMs) / 1000.0) {
                updateDataAndPlot(mPlayPosNow);
            } else {
                ui->playButton->setChecked(false);
            }
        }
    });

    QFont legendFont = font();
    legendFont.setPointSize(9);

    ui->plot->legend->setVisible(true);
    ui->plot->legend->setFont(legendFont);
    ui->plot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignRight|Qt::AlignBottom);
    ui->plot->legend->setBrush(QBrush(QColor(255,255,255,230)));
    ui->plot->xAxis->setLabel("Seconds (s)");

    auto addDataItem = [this](QString name, bool hasScale = true,
            double scale = 1.0, double scaleStep = 0.1) {
        ui->dataTable->setRowCount(ui->dataTable->rowCount() + 1);
        ui->dataTable->setItem(ui->dataTable->rowCount() - 1, 0, new QTableWidgetItem(name));
        ui->dataTable->setItem(ui->dataTable->rowCount() - 1, 1, new QTableWidgetItem(""));
        if (hasScale) {
            QDoubleSpinBox *sb = new QDoubleSpinBox;
            sb->setSingleStep(scaleStep);
            sb->setValue(scale);
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
    };

    addDataItem("Speed VESC");      // 0
    addDataItem("Speed GNSS");      // 1
    addDataItem("Time of Day", false);     // 2
    addDataItem("Time of trip", false);    // 3
    addDataItem("Trip VESC");       // 4
    addDataItem("Trip ABS VESC");   // 5
    addDataItem("Trip GNSS");       // 6
    addDataItem("Current Motors");  // 7
    addDataItem("Current Battery"); // 8
    addDataItem("Power");           // 9
    addDataItem("ERPM");            // 10
    addDataItem("Duty Cycle");      // 11
    addDataItem("Fault Code");      // 12
    addDataItem("Input Voltage");   // 13
    addDataItem("Battery Level");   // 14
    addDataItem("Temp MOSFET");     // 15
    addDataItem("Temp Motor");      // 16
    addDataItem("Ah Used");         // 17
    addDataItem("Ah Charged");      // 18
    addDataItem("Wh Used");         // 19
    addDataItem("Wh Charged");      // 20
    addDataItem("id");              // 21
    addDataItem("iq");              // 22
    addDataItem("vd");              // 23
    addDataItem("vq");              // 24
    addDataItem("Temp MOSFET 1");   // 25
    addDataItem("Temp MOSFET 2");   // 26
    addDataItem("Temp MOSFET 3");   // 27
    addDataItem("Motor Pos");       // 28
    addDataItem("Altitude GNSS");   // 29
    addDataItem("Roll");            // 30
    addDataItem("Pitch");           // 31
    addDataItem("Yaw");             // 32
    addDataItem("Accel X");         // 33
    addDataItem("Accel Y");         // 34
    addDataItem("Accel Z");         // 35
    addDataItem("Gyro X");          // 36
    addDataItem("Gyro Y");          // 37
    addDataItem("Gyro Z");          // 38
    addDataItem("GNSS Accuracy");   // 39
    addDataItem("V1 Current");      // 40
    addDataItem("V1 Current In");   // 41
    addDataItem("V1 Power");        // 42
    addDataItem("V1 Ah Used");      // 43
    addDataItem("V1 Ah Charged");   // 44
    addDataItem("V1 Wh Used");      // 45
    addDataItem("V1 Wh Charged");   // 46
    addDataItem("Latitude");        // 47
    addDataItem("Longitude");       // 48
    addDataItem("V. Speed GNSS");   // 49
    addDataItem("GNSS V. Acc.");    // 50
    addDataItem("VESC num");        // 51

    mVerticalLine = new QCPCurve(ui->plot->xAxis, ui->plot->yAxis);
    mVerticalLine->removeFromLegend();
    mVerticalLine->setPen(QPen(Qt::black));
    mVerticalLineMsLast = -1;

    auto updateMouse = [this](QMouseEvent *event) {
        if (event->modifiers() == Qt::ShiftModifier) {
            ui->plot->axisRect()->setRangeZoom(Qt::Vertical);
            ui->plot->axisRect()->setRangeDrag(Qt::Vertical);
        } else {
            ui->plot->axisRect()->setRangeZoom(nullptr);
            ui->plot->axisRect()->setRangeDrag(nullptr);
        }

        if (event->buttons() & Qt::LeftButton) {
            double vx = ui->plot->xAxis->pixelToCoord(event->x());
            updateDataAndPlot(vx);
        }
    };

    connect(ui->map, &MapWidget::infoPointClicked, [this](LocPoint info) {
        updateDataAndPlot(double(info.getInfo().toInt() - mLogDataTruncated.first().valTime) / 1000.0);
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
            ui->plot->axisRect()->setRangeZoom(nullptr);
            ui->plot->axisRect()->setRangeDrag(nullptr);

            double upper = ui->plot->xAxis->range().upper;
            double progress = ui->plot->xAxis->pixelToCoord(event->x()) / upper;
            double diff = event->delta();
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

    on_gridBox_toggled(ui->gridBox->isChecked());
}

PageLogAnalysis::~PageLogAnalysis()
{
    delete ui;
}

VescInterface *PageLogAnalysis::vesc() const
{
    return mVesc;
}

void PageLogAnalysis::setVesc(VescInterface *vesc)
{
    mVesc = vesc;
}

void PageLogAnalysis::on_openCsvButton_clicked()
{
    if (mVesc) {
        QString fileName = QFileDialog::getOpenFileName(this,
                                                        tr("Load CSV File"), "",
                                                        tr("CSV files (*.csv)"));

        if (!fileName.isEmpty()) {
            if (mVesc->loadRtLogFile(fileName)) {
                on_openCurrentButton_clicked();
            }
        }
    }
}

void PageLogAnalysis::on_openCurrentButton_clicked()
{
    if (mVesc) {
        mLogData = mVesc->getRtLogData();

        double i_llh[3];
        for (auto d: mLogData) {
            if (d.posTime >= 0) {
                i_llh[0] = d.lat;
                i_llh[1] = d.lon;
                i_llh[2] = d.alt;
                ui->map->setEnuRef(i_llh[0], i_llh[1], i_llh[2]);
                break;
            }
        }

        truncateDataAndPlot();
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
    mLogDataTruncated.clear();

    for (auto d: mLogData) {
        ind++;
        double prop = double(ind) / double(mLogData.size());
        if (prop < start || prop > end) {
            continue;
        }

        mLogDataTruncated.append(d);

        if (d.posTime >= 0 && posTimeLast != d.posTime) {
            double llh[3];
            double xyz[3];

            llh[0] = d.lat;
            llh[1] = d.lon;
            llh[2] = d.alt;
            Utility::llhToEnu(i_llh, llh, xyz);

            LocPoint p;
            p.setXY(xyz[0], xyz[1]);
            p.setRadius(5);
            QString info;
            info.sprintf("%d", d.valTime);
            p.setInfo(info);

            ui->map->addInfoPoint(p, false);
            posTimeLast = d.posTime;
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

    int startTime = -1;
    LOG_DATA prevSampleGnss;
    bool prevSampleGnssSet = false;
    double metersGnss = 0.0;
    LOG_DATA firstData;

    if (mLogDataTruncated.size() > 0) {
        firstData = mLogDataTruncated.first();
    }

    double verticalTime = -1.0;
    LocPoint p, p2;

    for (LOG_DATA d: mLogDataTruncated) {
        if (startTime < 0) {
            startTime = d.valTime;
        }

        int timeMs = d.valTime - startTime;
        if (timeMs < 0) { // Handle midnight
            timeMs += 60 * 60 * 24 * 1000;
        }

        if (mVerticalLineMsLast == d.valTime) {
            verticalTime = double(timeMs) / 1000.0;
        }

        xAxis.append(double(timeMs) / 1000.0);

        if (d.posTime >= 0) {
            if (prevSampleGnssSet) {
                double i_llh[3];
                double llh[3];
                double xyz[3];
                ui->map->getEnuRef(i_llh);

                llh[0] = d.lat;
                llh[1] = d.lon;
                llh[2] = d.alt;
                Utility::llhToEnu(i_llh, llh, xyz);

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

        int rowInd = 0;
        for (int r = 0;r < rows.size();r++) {
            int row = rows.at(r).row();
            double rowScale = 1.0;
            if(QDoubleSpinBox *sb = qobject_cast<QDoubleSpinBox*>
                    (ui->dataTable->cellWidget(row, 2))) {
                rowScale = sb->value();
            }
            if (row == 0) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.setupValues.speed * 3.6 * rowScale);
                names.append(QString("Speed VESC (km/h * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 1) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.gVel * 3.6 * rowScale);
                names.append(QString("Speed GNSS (km/h * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 4) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append((d.setupValues.tachometer -
                                      firstData.setupValues.tachometer) * rowScale);
                names.append(QString("Trip VESC (m * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 5) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append((d.setupValues.tachometer_abs -
                                      firstData.setupValues.tachometer_abs) * rowScale);
                names.append(QString("Trip ABS VESC (m * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 6) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(metersGnss * rowScale);
                names.append(QString("Trip GNSS (m * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 7) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.setupValues.current_motor * rowScale);
                names.append(QString("Current Motor (A * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 8) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.setupValues.current_in * rowScale);
                names.append(QString("Current Battery (A * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 9) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.setupValues.current_in * d.values.v_in * rowScale);
                names.append(QString("Power (W * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 10) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.values.rpm / 1000 * rowScale);
                names.append(QString("ERPM (1/1000 * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 11) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.values.duty_now * 100.0 * rowScale);
                names.append(QString("Duty (% * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 12) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(double(d.values.fault_code) * rowScale);
                names.append(QString("Fault Code (* %1)").arg(rowScale));
                rowInd++;
            } else if (row == 13) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.values.v_in * rowScale);
                names.append(QString("Input Voltage (V * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 14) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.setupValues.battery_level * 100.0 * rowScale);
                names.append(QString("Input Voltage (% * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 15) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.values.temp_mos * rowScale);
                names.append(QString("Temp MOSFET (°C * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 16) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.values.temp_motor * rowScale);
                names.append(QString("Temp Motor (°C * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 17) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.setupValues.amp_hours * rowScale);
                names.append(QString("Ah Used (Ah * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 18) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.setupValues.amp_hours_charged * rowScale);
                names.append(QString("Ah Charged (Ah * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 19) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.setupValues.watt_hours * rowScale);
                names.append(QString("Wh Used (Wh * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 20) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.setupValues.watt_hours_charged * rowScale);
                names.append(QString("Wh Charged (Wh * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 21) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.values.id * rowScale);
                names.append(QString("id (A * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 22) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.values.iq * rowScale);
                names.append(QString("iq (A * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 23) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.values.vd * rowScale);
                names.append(QString("vd (V * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 24) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.values.vq * rowScale);
                names.append(QString("vq (A * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 25) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.values.temp_mos_1 * rowScale);
                names.append(QString("Temp MOSFET 1 (°C * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 26) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.values.temp_mos_2 * rowScale);
                names.append(QString("Temp MOSFET 2 (°C * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 27) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.values.temp_mos_3 * rowScale);
                names.append(QString("Temp MOSFET 3 (°C * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 28) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.values.position * rowScale);
                names.append(QString("Motor Pos (° * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 29) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.alt * rowScale);
                names.append(QString("Altitude GNSS (m * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 30) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.imuValues.roll * 180.0 / M_PI * rowScale);
                names.append(QString("Roll (° * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 31) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.imuValues.pitch * 180.0 / M_PI * rowScale);
                names.append(QString("Pitch (° * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 32) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.imuValues.yaw * 180.0 / M_PI * rowScale);
                names.append(QString("Yaw (° * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 33) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.imuValues.accX * rowScale);
                names.append(QString("Accel X (G * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 34) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.imuValues.accY * rowScale);
                names.append(QString("Accel Y (G * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 35) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.imuValues.accZ * rowScale);
                names.append(QString("Accel Z (G * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 36) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.imuValues.gyroX * rowScale);
                names.append(QString("Gyro X (°/s * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 37) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.imuValues.gyroY * rowScale);
                names.append(QString("Gyro Y (°/s * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 38) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.imuValues.gyroZ * rowScale);
                names.append(QString("Gyro Z (°/s * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 39) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.hAcc * rowScale);
                names.append(QString("GNSS Accuracy (m * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 40) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.values.current_motor * rowScale);
                names.append(QString("V1 Current (A * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 41) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.values.current_in * rowScale);
                names.append(QString("V1 Current In (A * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 42) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.values.current_in * d.values.v_in * rowScale);
                names.append(QString("Power (W * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 43) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.values.amp_hours * rowScale);
                names.append(QString("V1 Ah Used (Ah * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 44) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.values.amp_hours_charged * rowScale);
                names.append(QString("V1 Ah Charged (Ah * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 45) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.values.watt_hours * rowScale);
                names.append(QString("V1 Wh Used (Wh * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 46) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.values.watt_hours_charged * rowScale);
                names.append(QString("V1 Wh Charged (Wh * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 47) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.lat * rowScale);
                names.append(QString("Latitude (° * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 48) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.lon * rowScale);
                names.append(QString("Longitude (° * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 49) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.vVel * 3.6 * rowScale);
                names.append(QString("V. Speed GNSS (km/h * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 50) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(d.vAcc * rowScale);
                names.append(QString("GNSS V. Accuracy (m * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 51) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(double(d.setupValues.num_vescs) * rowScale);
                names.append(QString("VESC num (* %1)").arg(rowScale));
                rowInd++;
            }
        }
    }

    ui->plot->clearGraphs();

    for (int i = 0;i < yAxes.size();i++) {
        QPen pen = QPen(Qt::blue);

        if (i == 1) {
            pen = QPen(Qt::magenta);
        } else if (i == 2) {
            pen = QPen(Qt::green);
        } else if (i == 3) {
            pen = QPen(Qt::darkGreen);
        } else if (i == 4) {
            pen = QPen(Qt::cyan);
        } else if (i == 5) {
            pen = QPen(QColor("#01DFD7"));
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

    ui->plot->replot();
}

void PageLogAnalysis::updateStats()
{
    bool startSampleSet = false;
    LOG_DATA startSample;
    LOG_DATA endSample;

    int samples = 0;
    int timeTotMs = 0;
    double meters = 0.0;
    double metersAbs = 0.0;
    double metersGnss = 0.0;
    double wh = 0.0;
    double whCharge = 0.0;
    double ah = 0.0;
    double ahCharge = 0.0;
    LOG_DATA prevSampleGnss;
    bool prevSampleGnssSet = false;

    for (LOG_DATA d: mLogDataTruncated) {
        if (!startSampleSet) {
            startSample = d;
            startSampleSet = true;
        }

        samples++;

        if (d.posTime >= 0) {
            if (prevSampleGnssSet) {
                double i_llh[3];
                double llh[3];
                double xyz[3];
                ui->map->getEnuRef(i_llh);

                llh[0] = d.lat;
                llh[1] = d.lon;
                llh[2] = d.alt;
                Utility::llhToEnu(i_llh, llh, xyz);

                LocPoint p;
                p.setXY(xyz[0], xyz[1]);
                p.setRadius(10);

                llh[0] = prevSampleGnss.lat;
                llh[1] = prevSampleGnss.lon;
                llh[2] = prevSampleGnss.alt;
                Utility::llhToEnu(i_llh, llh, xyz);

                LocPoint p2;
                p2.setXY(xyz[0], xyz[1]);
                p2.setRadius(10);

                metersGnss += p.getDistanceTo(p2);
            }

            prevSampleGnssSet = true;
            prevSampleGnss = d;
        }

        endSample = d;
    }

    timeTotMs = endSample.valTime - startSample.valTime;
    if (timeTotMs < 0) { // Handle midnight
        timeTotMs += 60 * 60 * 24 * 1000;
    }

    meters = endSample.setupValues.tachometer - startSample.setupValues.tachometer;
    metersAbs = endSample.setupValues.tachometer_abs - startSample.setupValues.tachometer_abs;
    wh = endSample.setupValues.watt_hours - startSample.setupValues.watt_hours;
    whCharge = endSample.setupValues.watt_hours_charged - startSample.setupValues.watt_hours_charged;
    ah = endSample.setupValues.amp_hours - startSample.setupValues.amp_hours;
    ahCharge = endSample.setupValues.amp_hours_charged - startSample.setupValues.amp_hours_charged;

    while (ui->statTable->rowCount() > 0) {
        ui->statTable->removeRow(0);
    }
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
    ui->plot->replot();

    LOG_DATA d = getLogSample(int(time * 1000));
    mVerticalLineMsLast = d.valTime;

    int timeTotMs = d.valTime - getLogSample(0).valTime;
    if (timeTotMs < 0) { // Handle midnight
        timeTotMs += 60 * 60 * 24 * 1000;
    }

    ui->dataTable->item(0, 1)->setText(QString::number(d.setupValues.speed * 3.6, 'f', 2) + " km/h");
    ui->dataTable->item(1, 1)->setText(QString::number(d.gVel * 3.6, 'f', 2) + " km/h");
    QTime t(0, 0, 0, 0);
    t = t.addMSecs(d.valTime);
    ui->dataTable->item(2, 1)->setText(t.toString("hh:mm:ss.zzz"));
    QTime t2(0, 0, 0, 0);
    t2 = t2.addMSecs(timeTotMs);
    ui->dataTable->item(3, 1)->setText(t2.toString("hh:mm:ss.zzz"));
    ui->dataTable->item(4, 1)->setText(QString::number(d.setupValues.tachometer - getLogSample(0).setupValues.tachometer, 'f', 2) + "m");
    ui->dataTable->item(5, 1)->setText(QString::number(d.setupValues.tachometer_abs - getLogSample(0).setupValues.tachometer_abs, 'f', 2) + "m");
    ui->dataTable->item(6, 1)->setText(QString::number(getDistGnssSample(int(time * 1000)), 'f', 2) + "m");
    ui->dataTable->item(7, 1)->setText(QString::number(d.setupValues.current_motor, 'f', 2) + " A");
    ui->dataTable->item(8, 1)->setText(QString::number(d.setupValues.current_in, 'f', 2) + " A");
    ui->dataTable->item(9, 1)->setText(QString::number(d.setupValues.current_in * d.values.v_in, 'f', 2) + " w");
    ui->dataTable->item(10, 1)->setText(QString::number(d.values.rpm / 1000.0, 'f', 2) + " k");
    ui->dataTable->item(11, 1)->setText(QString::number(d.values.duty_now * 100.0, 'f', 2) + " %");
    ui->dataTable->item(12, 1)->setText(Commands::faultToStr(mc_fault_code(d.values.fault_code)).mid(11));
    ui->dataTable->item(13, 1)->setText(QString::number(d.values.v_in, 'f', 2) + " V");
    ui->dataTable->item(14, 1)->setText(QString::number(d.setupValues.battery_level * 100.0, 'f', 2) + " %");
    ui->dataTable->item(15, 1)->setText(QString::number(d.values.temp_mos, 'f', 2) + " °C");
    ui->dataTable->item(16, 1)->setText(QString::number(d.values.temp_motor, 'f', 2) + " °C");
    ui->dataTable->item(17, 1)->setText(QString::number(d.setupValues.amp_hours, 'f', 2) + " Ah");
    ui->dataTable->item(18, 1)->setText(QString::number(d.setupValues.amp_hours_charged, 'f', 2) + " Ah");
    ui->dataTable->item(19, 1)->setText(QString::number(d.setupValues.watt_hours, 'f', 2) + " Wh");
    ui->dataTable->item(20, 1)->setText(QString::number(d.setupValues.watt_hours_charged, 'f', 2) + " Wh");
    ui->dataTable->item(21, 1)->setText(QString::number(d.values.id, 'f', 2) + " A");
    ui->dataTable->item(22, 1)->setText(QString::number(d.values.iq, 'f', 2) + " A");
    ui->dataTable->item(23, 1)->setText(QString::number(d.values.vd, 'f', 2) + " V");
    ui->dataTable->item(24, 1)->setText(QString::number(d.values.vq, 'f', 2) + " V");
    ui->dataTable->item(25, 1)->setText(QString::number(d.values.temp_mos_1, 'f', 2) + " °C");
    ui->dataTable->item(26, 1)->setText(QString::number(d.values.temp_mos_2, 'f', 2) + " °C");
    ui->dataTable->item(27, 1)->setText(QString::number(d.values.temp_mos_3, 'f', 2) + " °C");
    ui->dataTable->item(28, 1)->setText(QString::number(d.values.position, 'f', 2) + " °");
    ui->dataTable->item(29, 1)->setText(QString::number(d.alt, 'f', 2) + " m");
    ui->dataTable->item(30, 1)->setText(QString::number(d.imuValues.roll * 180.0 / M_PI, 'f', 2) + " °");
    ui->dataTable->item(31, 1)->setText(QString::number(d.imuValues.pitch * 180.0 / M_PI, 'f', 2) + " °");
    ui->dataTable->item(32, 1)->setText(QString::number(d.imuValues.yaw * 180.0 / M_PI, 'f', 2) + " °");
    ui->dataTable->item(33, 1)->setText(QString::number(d.imuValues.accX, 'f', 2) + " G");
    ui->dataTable->item(34, 1)->setText(QString::number(d.imuValues.accY, 'f', 2) + " G");
    ui->dataTable->item(35, 1)->setText(QString::number(d.imuValues.accZ, 'f', 2) + " G");
    ui->dataTable->item(36, 1)->setText(QString::number(d.imuValues.gyroX, 'f', 2) + " °/s");
    ui->dataTable->item(37, 1)->setText(QString::number(d.imuValues.gyroY, 'f', 2) + " °/s");
    ui->dataTable->item(38, 1)->setText(QString::number(d.imuValues.gyroZ, 'f', 2) + " °/s");
    ui->dataTable->item(39, 1)->setText(QString::number(d.hAcc, 'f', 2) + " m");
    ui->dataTable->item(40, 1)->setText(QString::number(d.values.current_motor, 'f', 2) + " A");
    ui->dataTable->item(41, 1)->setText(QString::number(d.values.current_in, 'f', 2) + " A");
    ui->dataTable->item(42, 1)->setText(QString::number(d.values.current_in * d.values.v_in, 'f', 2) + " w");
    ui->dataTable->item(43, 1)->setText(QString::number(d.values.amp_hours, 'f', 2) + " Ah");
    ui->dataTable->item(44, 1)->setText(QString::number(d.values.amp_hours_charged, 'f', 2) + " Ah");
    ui->dataTable->item(45, 1)->setText(QString::number(d.values.watt_hours, 'f', 2) + " Wh");
    ui->dataTable->item(46, 1)->setText(QString::number(d.values.watt_hours_charged, 'f', 2) + " Wh");
    ui->dataTable->item(47, 1)->setText(QString::number(d.lat, 'f', 7) + " °");
    ui->dataTable->item(48, 1)->setText(QString::number(d.lon, 'f', 7) + " °");
    ui->dataTable->item(49, 1)->setText(QString::number(d.vVel * 3.6, 'f', 2) + " km/h");
    ui->dataTable->item(50, 1)->setText(QString::number(d.vAcc, 'f', 2) + " m");
    ui->dataTable->item(51, 1)->setText(QString::number(d.setupValues.num_vescs));

    if (d.posTime >= 0) {
        double i_llh[3];
        double llh[3];
        double xyz[3];
        ui->map->getEnuRef(i_llh);
        llh[0] = d.lat;
        llh[1] = d.lon;
        llh[2] = d.alt;
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

    m3dView->setRollPitchYaw(d.imuValues.roll * 180.0 / M_PI, d.imuValues.pitch * 180.0 / M_PI,
                             mUseYawBox->isChecked() ? d.imuValues.yaw * 180.0 / M_PI : 0.0);
}

LOG_DATA PageLogAnalysis::getLogSample(int timeMs)
{
    LOG_DATA d;

    if (mLogDataTruncated.size() > 0) {
        d = mLogDataTruncated.first();
        int startTime = d.valTime;

        for (LOG_DATA dn: mLogDataTruncated) {
            int timeMsNow = dn.valTime - startTime;
            if (timeMsNow < 0) { // Handle midnight
                timeMsNow += 60 * 60 * 24 * 1000;
            }

            if (timeMsNow >= timeMs) {
                d = dn;
                break;
            }
        }
    }

    return d;
}

double PageLogAnalysis::getDistGnssSample(int timeMs)
{
    if (mLogDataTruncated.size() < 2) {
        return 0.0;
    }

    double metersGnss = 0.0;
    LOG_DATA prevSample;
    bool prevSampleSet = false;

    for (LOG_DATA d: mLogDataTruncated) {
        int timeMsNow = d.valTime - mLogDataTruncated.first().valTime;
        if (timeMsNow < 0) { // Handle midnight
            timeMsNow += 60 * 60 * 24 * 1000;
        }

        if (d.posTime < 0) {
            if (timeMsNow >= timeMs) {
                break;
            }
            continue;
        }

        if (!prevSampleSet) {
            prevSampleSet = true;
            prevSample = d;
            if (timeMsNow >= timeMs) {
                break;
            }
            continue;
        }

        double i_llh[3];
        double llh[3];
        double xyz[3];
        ui->map->getEnuRef(i_llh);

        llh[0] = d.lat;
        llh[1] = d.lon;
        llh[2] = d.alt;
        Utility::llhToEnu(i_llh, llh, xyz);

        LocPoint p;
        p.setXY(xyz[0], xyz[1]);
        p.setRadius(10);

        llh[0] = prevSample.lat;
        llh[1] = prevSample.lon;
        llh[2] = prevSample.alt;
        Utility::llhToEnu(i_llh, llh, xyz);

        LocPoint p2;
        p2.setXY(xyz[0], xyz[1]);
        p2.setRadius(10);

        metersGnss += p.getDistanceTo(p2);

        prevSample = d;

        if (timeMsNow >= timeMs) {
            break;
        }
    }

    return metersGnss;
}

void PageLogAnalysis::updateTileServers()
{
    if (ui->tilesOsmButton->isChecked()) {
        ui->map->osmClient()->setTileServerUrl("http://tile.openstreetmap.org");
        ui->map->osmClient()->setCacheDir("osm_tiles/osm");
        ui->map->osmClient()->clearCacheMemory();
    } else if (ui->tilesHiResButton->isChecked()) {
        ui->map->osmClient()->setTileServerUrl("http://c.osm.rrze.fau.de/osmhd");
        ui->map->osmClient()->setCacheDir("osm_tiles/hd");
        ui->map->osmClient()->clearCacheMemory();
    }
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
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("Save PDF"), "",
                                                    tr("PDF Files (*.pdf)"));

    if (!fileName.isEmpty()) {
        if (!fileName.toLower().endsWith(".pdf")) {
            fileName.append(".pdf");
        }

        ui->plot->savePdf(fileName,
                          ui->saveWidthBox->value(),
                          ui->saveHeightBox->value());
    }
}

void PageLogAnalysis::on_savePlotPngButton_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("Save Image"), "",
                                                    tr("PNG Files (*.png)"));

    if (!fileName.isEmpty()) {
        if (!fileName.toLower().endsWith(".png")) {
            fileName.append(".png");
        }

        ui->plot->savePng(fileName,
                          ui->saveWidthBox->value(),
                          ui->saveHeightBox->value());
    }
}

void PageLogAnalysis::on_centerButton_clicked()
{
    ui->map->zoomInOnInfoTrace(-1, 0.1);
}
