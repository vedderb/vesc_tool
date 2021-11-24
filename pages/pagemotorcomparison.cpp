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
#include <QFileDialog>
#include <QFileInfo>

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
        on_testRunButton_clicked();
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

    connect(ui->m1ConfFileEdit, &QLineEdit::textChanged, [=]() {
        mM1ConfigLoaded = false;
        settingChanged();
    });
    connect(ui->m2ConfFileEdit, &QLineEdit::textChanged, [=]() {
        mM2ConfigLoaded = false;
        settingChanged();
    });

    connect(ui->m1ConfChooseButton, &QPushButton::clicked, [this]() {
        QString fileName = QFileDialog::getOpenFileName(this,
                                                        tr("McConf XML File"),
                                                        QFileInfo(ui->m1ConfFileEdit->text()).canonicalFilePath(),
                                                        tr("XML files (*.xml)"));

        if (!fileName.isEmpty()) {
            ui->m1ConfFileEdit->setText(fileName);
            mM1ConfigLoaded = false;
            settingChanged();
        }
    });

    connect(ui->m2ConfChooseButton, &QPushButton::clicked, [this]() {
        QString fileName = QFileDialog::getOpenFileName(this,
                                                        tr("McConf XML File"),
                                                        QFileInfo(ui->m2ConfFileEdit->text()).canonicalFilePath(),
                                                        tr("XML files (*.xml)"));

        if (!fileName.isEmpty()) {
            ui->m2ConfFileEdit->setText(fileName);
            mM2ConfigLoaded = false;
            settingChanged();
        }
    });

    connect(ui->m1LoadConfButton, &QPushButton::clicked, [this]() {
        bool res = mVesc->mcConfig()->loadXml(ui->m1ConfFileEdit->text(), "MCConfiguration");
        if (res) {
            mVesc->emitStatusMessage("Loaded motor configuration", true);
        } else {
            mVesc->emitMessageDialog(tr("Load motor configuration"),
                              tr("Could not load motor configuration:<BR>"
                                 "%1").arg(mVesc->mcConfig()->xmlStatus()), false, false);
        }
    });
    connect(ui->m2LoadConfButton, &QPushButton::clicked, [this]() {
        bool res = mVesc->mcConfig()->loadXml(ui->m2ConfFileEdit->text(), "MCConfiguration");
        if (res) {
            mVesc->emitStatusMessage("Loaded motor configuration", true);
        } else {
            mVesc->emitMessageDialog(tr("Load motor configuration"),
                              tr("Could not load motor configuration:<BR>"
                                 "%1").arg(mVesc->mcConfig()->xmlStatus()), false, false);
        }
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
    connect(ui->testModeRpmButton, &QRadioButton::toggled,
            [this]() { settingChanged(); });
    connect(ui->testModeRpmPowerButton, &QRadioButton::toggled,
            [this]() { settingChanged(); });

    connect(ui->testLiveUpdateBox, &QCheckBox::toggled,
            [this](bool checked) { (void)checked; settingChanged(); });
    connect(ui->testNegativeBox, &QCheckBox::toggled,
            [this](bool checked) { (void)checked; settingChanged(); });

    connect(ui->testRpmBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double value) { (void)value; settingChanged(); });
    connect(ui->testTorqueBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double value) { (void)value; settingChanged(); });
    connect(ui->testPowerBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double value) { (void)value; settingChanged(); });
    connect(ui->testRpmStartBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double value) { (void)value; settingChanged(); });
    connect(ui->testExpBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double value) { (void)value; settingChanged(); });
    connect(ui->testExpBaseTorqueBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double value) { (void)value; settingChanged(); });

    connect(ui->m1GearingBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double value) { (void)value; settingChanged(); });
    connect(ui->m2GearingBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double value) { (void)value; settingChanged(); });

    connect(ui->m1GearEfficiencyBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double value) { (void)value; settingChanged(); });
    connect(ui->m2GearEfficiencyBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double value) { (void)value; settingChanged(); });

    connect(ui->m1MotorNumBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double value) { (void)value; settingChanged(); });
    connect(ui->m2MotorNumBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double value) { (void)value; settingChanged();});

    connect(ui->m1FwBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double value) { (void)value; settingChanged(); });
    connect(ui->m2FwBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double value) { (void)value; settingChanged();});

    connect(ui->m1TempIncBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double value) { (void)value; settingChanged(); });
    connect(ui->m2TempIncBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double value) { (void)value; settingChanged(); });

    connect(ui->m1MaxRpmBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double value) { (void)value; settingChanged(); });
    connect(ui->m2MaxRpmBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double value) { (void)value; settingChanged(); });


    connect(ui->m1PlotTable, &QTableWidget::itemSelectionChanged,
            [this]() {settingChanged(); });
    connect(ui->m2PlotTable, &QTableWidget::itemSelectionChanged,
            [this](){ settingChanged(); });

    connect(ui->compAEdit, &QLineEdit::textChanged,
            [=]() {settingChanged();});
    connect(ui->compBEdit, &QLineEdit::textChanged,
            [=]() {settingChanged();});

    connect(ui->m1SetupGearingButton, &QPushButton::clicked, [this]() {
        if (reloadConfigs()) {
            ui->m1GearingBox->setValue(mM1Config.getParamDouble("si_gear_ratio"));
        }
    });
    connect(ui->m2SetupGearingButton, &QPushButton::clicked, [this]() {
        if (reloadConfigs()) {
            ui->m2GearingBox->setValue(mM2Config.getParamDouble("si_gear_ratio"));
        }
    });

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
    addDataItemBoth("Mot Loss Tot");
    addDataItemBoth("Mot Loss Res");
    addDataItemBoth("Mot Loss Other");
    addDataItemBoth("Gearing Loss");
    addDataItemBoth("Total Losses");
    addDataItemBoth("iq (per motor)");
    addDataItemBoth("id (per motor)");
    addDataItemBoth("i_abs (per motor)");
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
    addDataItemBoth("km/h", false);
    addDataItemBoth("mph", false);
    addDataItemBoth("wh/km", false);
    addDataItemBoth("wh/mi", false);
    addDataItemBoth("KV (BLDC)", false);
    addDataItemBoth("KV Noload (BLDC)", false);


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

        if (mRunDone) {
            settingChanged();
        }
    });
}

void PageMotorComparison::settingChanged()
{
    ui->testTorqueBox->setEnabled(ui->testModeTorqueButton->isChecked() || ui->testModeRpmButton->isChecked());
    ui->testPowerBox->setEnabled(ui->testModeRpmPowerButton->isChecked() || ui->testModeExpButton->isChecked());
    ui->testRpmStartBox->setEnabled(ui->testModeRpmPowerButton->isChecked());
    ui->testExpBox->setEnabled(ui->testModeExpButton->isChecked());
    ui->testExpBaseTorqueBox->setEnabled(ui->testModeExpButton->isChecked());

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
    if (posx < 0.0 && !ui->testNegativeBox->isChecked()) {
        posx = 0.0;
    }

    QVector<double> x(2) , y(2);
    x[0] = posx; y[0] = yMin;
    x[1] = posx; y[1] = yMax;
    mVerticalLine->setData(x, y);
    mVerticalLine->setVisible(true);
    ui->plot->replot();
    mVerticalLinePosLast = posx;
    mVerticalLineYLast.first = yMin;
    mVerticalLineYLast.second = yMax;

    auto updateTable = [](MotorData &md, QTableWidget *table) {
        int ind = 0;
        table->item(ind++, 1)->setText(QString::number(md.efficiency * 100.0, 'f', 1) + " %");
        table->item(ind++, 1)->setText(QString::number(md.loss_motor_tot, 'f', 1) + " W");
        table->item(ind++, 1)->setText(QString::number(md.loss_motor_res, 'f', 1) + " W");
        table->item(ind++, 1)->setText(QString::number(md.loss_motor_other, 'f', 1) + " W");
        table->item(ind++, 1)->setText(QString::number(md.loss_gearing, 'f', 1) + " W");
        table->item(ind++, 1)->setText(QString::number(md.loss_tot, 'f', 1) + " W");
        table->item(ind++, 1)->setText(QString::number(md.iq, 'f', 1) + " A");
        table->item(ind++, 1)->setText(QString::number(md.id, 'f', 1) + " A");
        table->item(ind++, 1)->setText(QString::number(md.i_mag, 'f', 1) + " A");
        table->item(ind++, 1)->setText(QString::number(md.p_in, 'f', 1) + " W");
        table->item(ind++, 1)->setText(QString::number(md.p_out, 'f', 1) + " W");
        table->item(ind++, 1)->setText(QString::number(md.vq, 'f', 1) + " V");
        table->item(ind++, 1)->setText(QString::number(md.vd, 'f', 1) + " V");
        table->item(ind++, 1)->setText(QString::number(md.vbus_min, 'f', 1) + " V");
        table->item(ind++, 1)->setText(QString::number(md.torque_out, 'f', 1) + " Nm");
        table->item(ind++, 1)->setText(QString::number(md.torque_motor_shaft, 'f', 1) + " Nm");
        table->item(ind++, 1)->setText(QString::number(md.rpm_out, 'f', 1));
        table->item(ind++, 1)->setText(QString::number(md.rpm_motor_shaft, 'f', 1));
        table->item(ind++, 1)->setText(QString::number(md.erpm, 'f', 1));
        table->item(ind++, 1)->setText(QString::number(md.km_h, 'f', 1) + " km/h");
        table->item(ind++, 1)->setText(QString::number(md.mph, 'f', 1) + " mph");
        table->item(ind++, 1)->setText(QString::number(md.wh_km, 'f', 1) + " wh/km");
        table->item(ind++, 1)->setText(QString::number(md.wh_mi, 'f', 1) + " wh/mi");
        table->item(ind++, 1)->setText(QString::number(md.kv_bldc, 'f', 1) + " RPM/V");
        table->item(ind++, 1)->setText(QString::number(md.kv_bldc_noload, 'f', 1) + " RPM/V");
    };

    if (!mRunDone || !reloadConfigs()) {
        return;
    }

    if (ui->testModeTorqueButton->isChecked()) {
        MotorData md;
        md.update(mM1Config, ui->testRpmBox->value(), posx, getParamsUi(1));
        updateTable(md, ui->m1PlotTable);
        md.update(mM2Config, ui->testRpmBox->value(), posx, getParamsUi(2));
        updateTable(md, ui->m2PlotTable);
    } else if (ui->testModeRpmButton->isChecked()) {
        MotorData md;
        md.update(mM1Config, posx, ui->testTorqueBox->value(), getParamsUi(1));
        updateTable(md, ui->m1PlotTable);
        md.update(mM2Config, posx, ui->testTorqueBox->value(), getParamsUi(2));
        updateTable(md, ui->m2PlotTable);
    } else if (ui->testModeRpmPowerButton->isChecked()) {
        double rps = posx * 2.0 * M_PI / 60.0;
        double torque = ui->testPowerBox->value() / rps;

        MotorData md;
        md.update(mM1Config, posx, torque, getParamsUi(1));
        updateTable(md, ui->m1PlotTable);
        md.update(mM2Config, posx, torque, getParamsUi(2));
        updateTable(md, ui->m2PlotTable);
    } else {
        double rps = posx * 2.0 * M_PI / 60.0;
        double prop_exp = ui->testExpBox->value();
        double baseTorque = ui->testExpBaseTorqueBox->value();
        double topRpm = ui->testRpmBox->value();
        double power = ui->testPowerBox->value();
        double p_max_const = power / pow(topRpm, prop_exp);
        double torque = (p_max_const * pow(posx, prop_exp)) / rps;
        torque += baseTorque;

        MotorData md;
        md.update(mM1Config, posx, torque, getParamsUi(1));
        updateTable(md, ui->m1PlotTable);
        md.update(mM2Config, posx, torque, getParamsUi(2));
        updateTable(md, ui->m2PlotTable);
    }
}

PageMotorComparison::TestParams PageMotorComparison::getParamsUi(int motor)
{
    TestParams sel;

    if (motor == 1) {
        sel.gearing = ui->m1GearingBox->value();
        sel.gearingEfficiency = ui->m1GearEfficiencyBox->value() / 100.0;
        sel.motorNum = ui->m1MotorNumBox->value();
        sel.tempInc = ui->m1TempIncBox->value();
        sel.fwCurrent = ui->m1FwBox->value();
        sel.maxRpm = ui->m1MaxRpmBox->value();
    } else if (motor == 2) {
        sel.gearing = ui->m2GearingBox->value();
        sel.gearingEfficiency = ui->m2GearEfficiencyBox->value() / 100.0;
        sel.motorNum = ui->m2MotorNumBox->value();
        sel.tempInc = ui->m2TempIncBox->value();
        sel.fwCurrent = ui->m2FwBox->value();
        sel.maxRpm = ui->m2MaxRpmBox->value();
    }

    return sel;
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

            namePrefix += table->item(row, 0)->text() + " ";

            switch (row) {
            case 0:
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(md.efficiency * 100.0 * rowScale);
                names.append(namePrefix + QString("(% * %1)").arg(rowScale));
                rowInd++; break;
            case 1:
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(md.loss_motor_tot * rowScale);
                names.append(namePrefix + QString("(W * %1)").arg(rowScale));
                rowInd++; break;
            case 2:
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(md.loss_motor_res * rowScale);
                names.append(namePrefix + QString("(W * %1)").arg(rowScale));
                rowInd++; break;
            case 3:
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(md.loss_motor_other * rowScale);
                names.append(namePrefix + QString("(W * %1)").arg(rowScale));
                rowInd++; break;
            case 4:
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(md.loss_gearing * rowScale);
                names.append(namePrefix + QString("(W * %1)").arg(rowScale));
                rowInd++; break;
            case 5:
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(md.loss_tot * rowScale);
                names.append(namePrefix + QString("(W * %1)").arg(rowScale));
                rowInd++; break;
            case 6:
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(md.iq * rowScale);
                names.append(namePrefix + QString("(A * %1)").arg(rowScale));
                rowInd++; break;
            case 7:
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(md.id * rowScale);
                names.append(namePrefix + QString("(A * %1)").arg(rowScale));
                rowInd++; break;
            case 8:
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(md.i_mag * rowScale);
                names.append(namePrefix + QString("(A * %1)").arg(rowScale));
                rowInd++; break;
            case 9:
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(md.p_in * rowScale);
                names.append(namePrefix + QString("(W * %1)").arg(rowScale));
                rowInd++; break;
            case 10:
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(md.p_out * rowScale);
                names.append(namePrefix + QString("(W * %1)").arg(rowScale));
                rowInd++; break;
            case 11:
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(md.vq * rowScale);
                names.append(namePrefix + QString("(V * %1)").arg(rowScale));
                rowInd++; break;
            case 12:
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(md.vd * rowScale);
                names.append(namePrefix + QString("(V * %1)").arg(rowScale));
                rowInd++; break;
            case 13:
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(md.vbus_min * rowScale);
                names.append(namePrefix + QString("(V * %1)").arg(rowScale));
                rowInd++; break;
            default:
                break;
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

        if (graphsStart != 0) {
            min = mVerticalLineYLast.first;
            max = mVerticalLineYLast.second;
        }

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

        if (ui->autoscaleButton->isChecked()) {
            ui->plot->rescaleAxes();
        }

        ui->plot->replot();
    };

    auto plotTorqueSweep = [this, updateData, updateGraphs](QTableWidget *table,
            ConfigParams &config, TestParams param) {
        double torque = fabs(ui->testTorqueBox->value());
        double rpm = ui->testRpmBox->value();

        QVector<double> xAxis;
        QVector<QVector<double> > yAxes;
        QVector<QString> names;

        double torque_start = -torque;
        if (!ui->testNegativeBox->isChecked()) {
            torque_start = torque / 1000.0;
        }

        for (double t = torque_start;t < torque;t += (torque / 1000.0)) {
            MotorData md;
            md.update(config, rpm, t, param);
            xAxis.append(t);
            updateData(md, table, yAxes, names);

            if (md.rpm_motor_shaft >= param.maxRpm) {
                mVesc->emitMessageDialog("Max RPM", "Maximum motor shaft RPM exceeded", false);
                break;
            }
        }

        ui->plot->xAxis->setLabel("Torque (Nm)");
        updateGraphs(xAxis, yAxes, names);
    };

    auto plotRpmSweep = [this, updateData, updateGraphs](QTableWidget *table,
            ConfigParams &config, TestParams param) {
        double torque = ui->testTorqueBox->value();
        double rpm = ui->testRpmBox->value();

        QVector<double> xAxis;
        QVector<QVector<double> > yAxes;
        QVector<QString> names;

        double rpm_start = -rpm;
        if (!ui->testNegativeBox->isChecked()) {
            rpm_start = rpm / 1000.0;
        }

        for (double r = rpm_start;r < rpm;r += (rpm / 1000.0)) {
            MotorData md;
            md.update(config, r, torque, param);
            xAxis.append(r);
            updateData(md, table, yAxes, names);

            if (md.rpm_motor_shaft >= param.maxRpm) {
                break;
            }
        }

        ui->plot->xAxis->setLabel("RPM");
        updateGraphs(xAxis, yAxes, names);
    };

    auto plotPowerSweep = [this, updateData, updateGraphs](QTableWidget *table,
            ConfigParams &config, TestParams param) {
        double rpm = ui->testRpmBox->value();
        double rpm_start = ui->testRpmStartBox->value();
        double power = ui->testPowerBox->value();

        QVector<double> xAxis;
        QVector<QVector<double> > yAxes;
        QVector<QString> names;

        for (double r = rpm_start;r < rpm;r += (rpm / 1000.0)) {
            double rps = r * 2.0 * M_PI / 60.0;
            double torque = power / rps;

            MotorData md;
            md.update(config, r, torque, param);
            xAxis.append(r);
            updateData(md, table, yAxes, names);

            if (md.rpm_motor_shaft >= param.maxRpm) {
                break;
            }
        }

        ui->plot->xAxis->setLabel("RPM");
        updateGraphs(xAxis, yAxes, names);
    };

    auto plotPropSweep = [this, updateData, updateGraphs](QTableWidget *table,
            ConfigParams &config, TestParams param) {
        double rpm = ui->testRpmBox->value();
        double power = ui->testPowerBox->value();
        double prop_exp = ui->testExpBox->value();
        double baseTorque = ui->testExpBaseTorqueBox->value();
        double p_max_const = power / pow(rpm, prop_exp);

        QVector<double> xAxis;
        QVector<QVector<double> > yAxes;
        QVector<QString> names;

        for (double r = rpm / 1000.0;r < rpm;r += (rpm / 1000.0)) {
            double rps = r * 2.0 * M_PI / 60.0;
            double power = p_max_const * pow(r, prop_exp);
            double torque = power / rps;
            torque += baseTorque;

            MotorData md;
            md.update(config, r, torque, param);
            xAxis.append(r);
            updateData(md, table, yAxes, names);

            if (md.rpm_motor_shaft >= param.maxRpm) {
                break;
            }
        }

        ui->plot->xAxis->setLabel("RPM");
        updateGraphs(xAxis, yAxes, names);
    };

    if (ui->testModeTorqueButton->isChecked()) {
        ui->plot->clearGraphs();
        plotTorqueSweep(ui->m1PlotTable, mM1Config, getParamsUi(1));
        plotTorqueSweep(ui->m2PlotTable, mM2Config, getParamsUi(2));
    } else if (ui->testModeRpmButton->isChecked()) {
        ui->plot->clearGraphs();
        plotRpmSweep(ui->m1PlotTable, mM1Config, getParamsUi(1));
        plotRpmSweep(ui->m2PlotTable, mM2Config, getParamsUi(2));
    } else if (ui->testModeRpmPowerButton->isChecked()) {
        ui->plot->clearGraphs();
        plotPowerSweep(ui->m1PlotTable, mM1Config, getParamsUi(1));
        plotPowerSweep(ui->m2PlotTable, mM2Config, getParamsUi(2));
    } else {
        ui->plot->clearGraphs();
        plotPropSweep(ui->m1PlotTable, mM1Config, getParamsUi(1));
        plotPropSweep(ui->m2PlotTable, mM2Config, getParamsUi(2));
    }

    mRunDone = true;
}
