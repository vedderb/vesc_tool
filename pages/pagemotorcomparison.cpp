/*
    Copyright 2021 Benjamin Vedder	benjamin@vedder.se

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

#include "pagemotorcomparison.h"
#include "ui_pagemotorcomparison.h"
#include "utility.h"

PageMotorComparison::PageMotorComparison(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PageMotorComparison)
{
    ui->setupUi(this);
    layout()->setContentsMargins(0, 0, 0, 0);

    mVesc = nullptr;
    mM1ConfigLoaded = false;
    mM2ConfigLoaded = false;
    mRunDone = false;

    Utility::setPlotColors(ui->plot);
    ui->plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

    mVerticalLine = new QCPCurve(ui->plot->xAxis, ui->plot->yAxis);
    mVerticalLine->removeFromLegend();
    mVerticalLine->setPen(QPen(Utility::getAppQColor("normalText")));
    mVerticalLinePosLast = -1.0;

    ui->m1PlotTable->setColumnWidth(0, 140);
    ui->m1PlotTable->setColumnWidth(1, 120);
    ui->m2PlotTable->setColumnWidth(0, 140);
    ui->m2PlotTable->setColumnWidth(1, 120);

    QFont legendFont = font();
    legendFont.setPointSize(9);

    Utility::setPlotColors(ui->plot);
    ui->plot->legend->setVisible(true);
    ui->plot->legend->setFont(legendFont);
    ui->plot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignBottom | Qt::AlignRight);
    ui->plot->xAxis->setLabel("Torque (Nm)");

    auto updateZoom = [&]() {
        Qt::Orientations plotOrientations = (Qt::Orientations)
                ((ui->zoomHButton->isChecked() ? Qt::Horizontal : 0) |
                 (ui->zoomVButton->isChecked() ? Qt::Vertical : 0));
        ui->plot->axisRect()->setRangeZoom(plotOrientations);
    };

    connect(ui->zoomHButton, &QPushButton::clicked, [updateZoom]() {
        updateZoom();
    });

    connect(ui->zoomVButton, &QPushButton::clicked, [updateZoom]() {
        updateZoom();
    });

    connect(ui->rescaleButton, &QPushButton::clicked, [this]() {
        ui->plot->rescaleAxes();
        ui->plot->replot();
    });

    connect(ui->m1ConfLocalButton, &QRadioButton::toggled, [this]() {
        mM1ConfigLoaded = false;
        settingChanged();
    });

    connect(ui->m2ConfLocalButton, &QRadioButton::toggled, [this]() {
        mM2ConfigLoaded = false;
        settingChanged();
    });

    QSettings set;
    ui->m1ConfFileEdit->setText(set.value("pagemotorcomparison/m1confpath", "").toString());
    ui->m2ConfFileEdit->setText(set.value("pagemotorcomparison/m2confpath", "").toString());

    connect(ui->m1ConfChooseButton, &QPushButton::clicked, [this]() {
        QString fileName = QFileDialog::getOpenFileName(this,
                                                        tr("McConf XML File"), "",
                                                        tr("XML files (*.xml)"));

        if (!fileName.isEmpty()) {
            ui->m1ConfFileEdit->setText(fileName);
            mM1ConfigLoaded = false;
            settingChanged();
        }
    });

    connect(ui->m2ConfChooseButton, &QPushButton::clicked, [this]() {
        QString fileName = QFileDialog::getOpenFileName(this,
                                                        tr("McConf XML File"), "",
                                                        tr("XML files (*.xml)"));

        if (!fileName.isEmpty()) {
            ui->m2ConfFileEdit->setText(fileName);
            mM2ConfigLoaded = false;
            settingChanged();
        }
    });

    connect(ui->testSingleBox, &QCheckBox::toggled, [this](bool checked) {
        ui->m2Box->setEnabled(!checked);
    });

    connect(ui->savePlotPdfButton, &QPushButton::clicked, [this]() {
        Utility::plotSavePdf(ui->plot, ui->saveWidthBox->value(),
                             ui->saveHeightBox->value(), ui->titleEdit->text());
    });

    connect(ui->savePlotPngButton, &QPushButton::clicked, [this]() {
        Utility::plotSavePng(ui->plot, ui->saveWidthBox->value(),
                             ui->saveHeightBox->value(), ui->titleEdit->text());
    });

    connect(ui->testModeTorqueButton, &QRadioButton::toggled,
            [this]() { settingChanged(); });

    connect(ui->testLiveUpdateBox, &QCheckBox::toggled,
            [this](bool checked) { (void)checked; settingChanged(); });

    connect(ui->testRpmBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double value) { (void)value; settingChanged(); });
    connect(ui->testTorqueBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double value) { (void)value; settingChanged(); });

    connect(ui->m1GearingBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double value) { (void)value; settingChanged(); });
    connect(ui->m2GearingBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double value) { (void)value; settingChanged(); });

    connect(ui->m1MotorNumBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double value) { (void)value; settingChanged(); });
    connect(ui->m2MotorNumBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double value) { (void)value; settingChanged();});

    connect(ui->m1TempIncBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double value) { (void)value; settingChanged(); });
    connect(ui->m2TempIncBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double value) { (void)value; settingChanged(); });

    connect(ui->m1PlotTable, &QTableWidget::itemSelectionChanged,
            [this]() {settingChanged(); });
    connect(ui->m2PlotTable, &QTableWidget::itemSelectionChanged,
            [this](){ settingChanged(); });

    connect(ui->compAEdit, &QLineEdit::textChanged,
            [=]() {settingChanged();});
    connect(ui->compBEdit, &QLineEdit::textChanged,
            [=]() {settingChanged();});

    auto updateMouse = [this](QMouseEvent *event) {
        if (event->buttons() & Qt::RightButton) {
            double vx = ui->plot->xAxis->pixelToCoord(event->x());
            updateDataAndPlot(vx, ui->plot->yAxis->range().lower, ui->plot->yAxis->range().upper);
        }
    };

    connect(ui->plot, &QCustomPlot::mousePress, [updateMouse](QMouseEvent *event) {
        updateMouse(event);
    });

    connect(ui->plot, &QCustomPlot::mouseMove, [updateMouse](QMouseEvent *event) {
        updateMouse(event);
    });

    auto addDataItem = [this](QString name, QTableWidget *table, bool hasScale = true,
            double scale = 1.0, double scaleStep = 0.1) {
        table->setRowCount(table->rowCount() + 1);
        table->setItem(table->rowCount() - 1, 0, new QTableWidgetItem(name));
        table->setItem(table->rowCount() - 1, 1, new QTableWidgetItem(""));
        if (hasScale) {
            QDoubleSpinBox *sb = new QDoubleSpinBox;
            sb->setSingleStep(scaleStep);
            sb->setValue(scale);
            // Prevent mouse wheel focus to avoid changing the selection
            sb->setFocusPolicy(Qt::StrongFocus);
            table->setCellWidget(table->rowCount() - 1, 2, sb);
            connect(sb, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                    [this](double value) {
                (void)value;
                settingChanged();
            });
        } else {
            table->setItem(table->rowCount() - 1, 2, new QTableWidgetItem("Not Plottable"));
        }
    };

    auto addDataItemBoth = [this, addDataItem](QString name, bool hasScale = true) {
        addDataItem(name, ui->m1PlotTable, hasScale);
        addDataItem(name, ui->m2PlotTable, hasScale);
    };

    addDataItemBoth("Efficiency");
    addDataItemBoth("Losses Total");
    addDataItemBoth("Losses Resistive");
    addDataItemBoth("Losses Other");
    addDataItemBoth("id (per motor)");
    addDataItemBoth("Power In");
    addDataItemBoth("Power Out");
    addDataItemBoth("Vq");
    addDataItemBoth("Vd");
    addDataItemBoth("VBus Min");
    addDataItemBoth("Torque Out", false);
    addDataItemBoth("Torque Shaft", false);
    addDataItemBoth("RPM Out", false);
    addDataItemBoth("RPM Shaft", false);
    addDataItemBoth("ERPM", false);
}

PageMotorComparison::~PageMotorComparison()
{
    QSettings set;
    set.setValue("pagemotorcomparison/m1confpath", ui->m1ConfFileEdit->text());
    set.setValue("pagemotorcomparison/m2confpath", ui->m2ConfFileEdit->text());
    delete ui;
}

VescInterface *PageMotorComparison::vesc() const
{
    return mVesc;
}

void PageMotorComparison::setVesc(VescInterface *vesc)
{
    mVesc = vesc;

    connect(mVesc->mcConfig(), &ConfigParams::paramChangedDouble,
            [this](QObject *src, QString name, double newParam) {
        (void)src; (void)name; (void)newParam;

        if (ui->m1ConfLocalButton->isChecked()) {
            mM1ConfigLoaded = false;
        }

        if (ui->m2ConfLocalButton->isChecked()) {
            mM2ConfigLoaded = false;
        }
    });
}

void PageMotorComparison::settingChanged()
{
    if (ui->testLiveUpdateBox->isChecked()) {
        on_testRunButton_clicked();
    }
}

bool PageMotorComparison::reloadConfigs()
{
    if (!mM1ConfigLoaded) {
        mM1Config = *mVesc->mcConfig();

        if (!ui->m1ConfLocalButton->isChecked()) {
            if (!mM1Config.loadXml(ui->m1ConfFileEdit->text(), "MCConfiguration")) {
                mVesc->emitMessageDialog("Load M1 Config",
                                         "Could not load motor 1 configuration. Make sure "
                                         "that the file path is valid.", false);
                return false;
            }
        }

        mM1ConfigLoaded = true;
    }

    if (!mM2ConfigLoaded) {
        mM2Config = *mVesc->mcConfig();

        if (!ui->m2ConfLocalButton->isChecked()) {
            if (!mM2Config.loadXml(ui->m2ConfFileEdit->text(), "MCConfiguration")) {
                mVesc->emitMessageDialog("Load M2 Config",
                                         "Could not load motor 2 configuration. Make sure "
                                         "that the file path is valid.", false);
                return false;
            }
        }

        mM2ConfigLoaded = true;
    }

    return true;
}

void PageMotorComparison::updateDataAndPlot(double posx, double yMin, double yMax)
{
    if (posx < 0.0) {
        posx = 0.0;
    }

    QVector<double> x(2) , y(2);
    x[0] = posx; y[0] = yMin;
    x[1] = posx; y[1] = yMax;
    mVerticalLine->setData(x, y);
    mVerticalLine->setVisible(true);
    ui->plot->replot();
    mVerticalLinePosLast = posx;

    auto updateTable = [](MotorData &md, QTableWidget *table) {
        table->item(0, 1)->setText(QString::number(md.efficiency * 100.0, 'f', 1) + " %");
        table->item(1, 1)->setText(QString::number(md.loss_tot, 'f', 1) + " W");
        table->item(2, 1)->setText(QString::number(md.loss_res, 'f', 1) + " W");
        table->item(3, 1)->setText(QString::number(md.loss_other, 'f', 1) + " W");
        table->item(4, 1)->setText(QString::number(md.iq, 'f', 1) + " A");
        table->item(5, 1)->setText(QString::number(md.p_in, 'f', 1) + " W");
        table->item(6, 1)->setText(QString::number(md.p_out, 'f', 1) + " W");
        table->item(7, 1)->setText(QString::number(md.vq, 'f', 1) + " V");
        table->item(8, 1)->setText(QString::number(md.vd, 'f', 1) + " V");
        table->item(9, 1)->setText(QString::number(md.vbus_min, 'f', 1) + " V");
        table->item(10, 1)->setText(QString::number(md.torque_out, 'f', 1) + " Nm");
        table->item(11, 1)->setText(QString::number(md.torque_motor_shaft, 'f', 1) + " Nm");
        table->item(12, 1)->setText(QString::number(md.rpm_out, 'f', 1));
        table->item(13, 1)->setText(QString::number(md.rpm_motor_shaft, 'f', 1));
        table->item(14, 1)->setText(QString::number(md.erpm, 'f', 1));
    };

    if (!mRunDone || !reloadConfigs()) {
        return;
    }

    if (ui->testModeTorqueButton->isChecked()) {
        MotorData md;
        md.update(mM1Config, ui->testRpmBox->value(), posx, ui->m1GearingBox->value(),
                  ui->m1MotorNumBox->value(), ui->m1TempIncBox->value());
        updateTable(md, ui->m1PlotTable);
        md.update(mM2Config, ui->testRpmBox->value(), posx, ui->m2GearingBox->value(),
                  ui->m2MotorNumBox->value(), ui->m2TempIncBox->value());
        updateTable(md, ui->m2PlotTable);
    } else {
        MotorData md;
        md.update(mM1Config, posx, ui->testTorqueBox->value(), ui->m1GearingBox->value(),
                  ui->m1MotorNumBox->value(), ui->m1TempIncBox->value());
        updateTable(md, ui->m1PlotTable);
        md.update(mM2Config, posx, ui->testTorqueBox->value(), ui->m2GearingBox->value(),
                  ui->m2MotorNumBox->value(), ui->m2TempIncBox->value());
        updateTable(md, ui->m2PlotTable);
    }
}

void PageMotorComparison::on_testRunButton_clicked()
{
    if (!reloadConfigs()) {
        return;
    }

    auto updateData = [this](MotorData &md,
            QTableWidget *table,
            QVector<QVector<double> > &yAxes,
            QVector<QString> &names) {

        auto rows = table->selectionModel()->selectedRows();
        int rowInd = 0;

        for (int r = 0;r < rows.size();r++) {
            int row = rows.at(r).row();
            double rowScale = 1.0;
            if (QDoubleSpinBox *sb = qobject_cast<QDoubleSpinBox*>(table->cellWidget(row, 2))) {
                rowScale = sb->value();
            }

            QString namePrefix = "";
            if (table == ui->m1PlotTable) {
                namePrefix = ui->compAEdit->text() + " ";
            } else {
                namePrefix = ui->compBEdit->text() + " ";
            }

            if (row == 0) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(md.efficiency * 100.0 * rowScale);
                names.append(namePrefix + QString("Efficiency (% * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 1) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(md.loss_tot * rowScale);
                names.append(namePrefix + QString("Losses Total (W * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 2) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(md.loss_res * rowScale);
                names.append(namePrefix + QString("Losses Resistive (W * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 3) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(md.loss_other * rowScale);
                names.append(namePrefix + QString("Losses Other (W * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 4) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(md.iq * rowScale);
                names.append(namePrefix + QString("Current Motor (A * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 5) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(md.p_in * rowScale);
                names.append(namePrefix + QString("Power In (W * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 6) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(md.p_out * rowScale);
                names.append(namePrefix + QString("Power Out (W * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 7) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(md.vq * rowScale);
                names.append(namePrefix + QString("Vq (V * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 8) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(md.vd * rowScale);
                names.append(namePrefix + QString("Vd (V * %1)").arg(rowScale));
                rowInd++;
            } else if (row == 9) {
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(md.vbus_min * rowScale);
                names.append(namePrefix + QString("VBus Min (V * %1)").arg(rowScale));
                rowInd++;
            }
        }
    };

    auto updateGraphs = [this](
            QVector<double> &xAxis,
            QVector<QVector<double> > &yAxes,
            QVector<QString> &names) {
        int graphsStart = ui->plot->graphCount();

        if (yAxes.isEmpty()) {
            return;
        }

        double min = yAxes.first().first();
        double max = min;

        for (auto a: yAxes) {
            for (auto b: a) {
                if (b < min) {
                    min = b;
                }

                if (b > max) {
                    max = b;
                }
            }
        }

        if (mVerticalLinePosLast < xAxis.first()) {
            updateDataAndPlot(xAxis.first(), min, max);
        } else if (mVerticalLinePosLast > xAxis.last()) {
            updateDataAndPlot(xAxis.last(), min, max);
        } else {
            updateDataAndPlot(mVerticalLinePosLast, min, max);
        }

        for (int i = 0;i < yAxes.size();i++) {
            QPen pen = QPen(Utility::getAppQColor("plot_graph1"));

            int graphNow = i + graphsStart;

            if (graphNow == 1) {
                pen = QPen(Qt::magenta);
            } else if (graphNow == 2) {
                pen = QPen(Utility::getAppQColor("plot_graph2"));
            } else if (graphNow == 3) {
                pen = QPen(Utility::getAppQColor("plot_graph3"));
            } else if (graphNow == 4) {
                pen = QPen(Qt::cyan);
            } else if (graphNow == 5) {
                pen = QPen(Utility::getAppQColor("plot_graph4"));
            }

            ui->plot->addGraph();
            ui->plot->graph(graphNow)->setPen(pen);
            ui->plot->graph(graphNow)->setName(names.at(i));
            ui->plot->graph(graphNow)->setData(xAxis, yAxes.at(i));
        }

        ui->plot->rescaleAxes();
        ui->plot->replot();
    };

    auto plotTorqueSweep = [this, updateData, updateGraphs](QTableWidget *table,
            ConfigParams &config, double gearing, double motors, double temp_inc) {
        double torque = ui->testTorqueBox->value();
        double rpm = ui->testRpmBox->value();

        QVector<double> xAxis;
        QVector<QVector<double> > yAxes;
        QVector<QString> names;

        for (double t = torque / 1000.0;t < torque;t += (torque / 1000.0)) {
            MotorData md;
            md.update(config, rpm, t, gearing, motors, temp_inc);
            xAxis.append(t);
            updateData(md, table, yAxes, names);
        }

        ui->plot->xAxis->setLabel("Torque (Nm)");
        updateGraphs(xAxis, yAxes, names);
    };

    auto plotRpmSweep = [this, updateData, updateGraphs](QTableWidget *table,
            ConfigParams &config, double gearing, double motors, double temp_inc) {
        double torque = ui->testTorqueBox->value();
        double rpm = ui->testRpmBox->value();

        QVector<double> xAxis;
        QVector<QVector<double> > yAxes;
        QVector<QString> names;

        for (double r = rpm / 1000.0;r < rpm;r += (rpm / 1000.0)) {
            MotorData md;
            md.update(config, r, torque, gearing, motors,temp_inc);
            xAxis.append(r);
            updateData(md, table, yAxes, names);
        }

        ui->plot->xAxis->setLabel("RPM");
        updateGraphs(xAxis, yAxes, names);
    };

    if (ui->testModeTorqueButton->isChecked()) {
        ui->plot->clearGraphs();
        plotTorqueSweep(ui->m1PlotTable, mM1Config, ui->m1GearingBox->value(),
                        ui->m1MotorNumBox->value(), ui->m1TempIncBox->value());
        if (!ui->testSingleBox->isChecked()) {
            plotTorqueSweep(ui->m2PlotTable, mM2Config, ui->m2GearingBox->value(),
                            ui->m2MotorNumBox->value(), ui->m2TempIncBox->value());
        }
    } else {
        ui->plot->clearGraphs();
        plotRpmSweep(ui->m1PlotTable, mM1Config, ui->m1GearingBox->value(),
                     ui->m1MotorNumBox->value(), ui->m1TempIncBox->value());
        if (!ui->testSingleBox->isChecked()) {
            plotRpmSweep(ui->m2PlotTable, mM2Config, ui->m2GearingBox->value(),
                         ui->m2MotorNumBox->value(), ui->m2TempIncBox->value());
        }
    }

    mRunDone = true;
}
