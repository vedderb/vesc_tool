/*
    Copyright 2021 - 2022 Benjamin Vedder	benjamin@vedder.se

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
#include <QQmlEngine>
#include <QQmlContext>
#include <QQuickItem>

PageMotorComparison::PageMotorComparison(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PageMotorComparison)
{
    ui->setupUi(this);
    layout()->setContentsMargins(0, 0, 0, 0);

    ui->qmlWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    ui->qmlWidget->setClearColor(Utility::getAppQColor("normalBackground"));

    ui->testRunButton->setIcon(Utility::getIcon("icons/Process-96.png"));
    ui->rescaleButton->setIcon(Utility::getIcon("icons/expand_off.png"));
    ui->qmlChooseButton->setIcon(Utility::getIcon("icons/Open Folder-96.png"));
    ui->qmlRunButton->setIcon(Utility::getIcon("icons/Circled Play-96.png"));
    ui->savePlotPdfButton->setIcon(Utility::getIcon("icons/Line Chart-96.png"));
    ui->savePlotPngButton->setIcon(Utility::getIcon("icons/Line Chart-96.png"));

    ui->m1SetupGearingButton->setIcon(Utility::getIcon("icons/motor_up.png"));
    ui->m1LoadConfButton->setIcon(Utility::getIcon("icons/motor_up.png"));
    ui->m1ConfChooseButton->setIcon(Utility::getIcon("icons/Open Folder-96.png"));

    ui->m2SetupGearingButton->setIcon(Utility::getIcon("icons/motor_up.png"));
    ui->m2LoadConfButton->setIcon(Utility::getIcon("icons/motor_up.png"));
    ui->m2ConfChooseButton->setIcon(Utility::getIcon("icons/Open Folder-96.png"));

    QIcon mycon = QIcon(Utility::getIcon("icons/expand_off.png"));
    mycon.addPixmap(Utility::getIcon("icons/expand_on.png"), QIcon::Normal, QIcon::On);
    mycon.addPixmap(Utility::getIcon("icons/expand_off.png"), QIcon::Normal, QIcon::Off);
    ui->zoomHButton->setIcon(mycon);

    mycon = QIcon(Utility::getIcon("icons/expand_v_off.png"));
    mycon.addPixmap(Utility::getIcon("icons/expand_v_on.png"), QIcon::Normal, QIcon::On);
    mycon.addPixmap(Utility::getIcon("icons/expand_v_off.png"), QIcon::Normal, QIcon::Off);
    ui->zoomVButton->setIcon(mycon);

    mycon = QIcon(Utility::getIcon("icons/size_off.png"));
    mycon.addPixmap(Utility::getIcon("icons/size_on.png"), QIcon::Normal, QIcon::On);
    mycon.addPixmap(Utility::getIcon("icons/size_off.png"), QIcon::Normal, QIcon::Off);
    ui->autoscaleButton->setIcon(mycon);


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
        ui->plot->replotWhenVisible();
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
    ui->qmlFileEdit->setText(set.value("pagemotorcomparison/qmlpath", "").toString());

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
    connect(ui->testModeExpButton, &QRadioButton::toggled,
            [this]() { settingChanged(); });
    connect(ui->testModeVbusButton, &QRadioButton::toggled,
            [this]() { settingChanged(); });
    connect(ui->testModeVBFWButton, &QRadioButton::toggled,
            [this]() { settingChanged(); });
    connect(ui->testModeVBRPMButton, &QRadioButton::toggled,
            [this]() { settingChanged(); });

    connect(ui->testLiveUpdateBox, &QCheckBox::toggled,
            [this](bool checked) { (void)checked; settingChanged(); });
    connect(ui->testNegativeBox, &QCheckBox::toggled,
            [this](bool checked) { (void)checked; settingChanged(); });
    connect(ui->scatterPlotBox, &QCheckBox::toggled,
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
    connect(ui->testVbusBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double value) { (void)value; settingChanged(); });
    connect(ui->pointsBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
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
    connect(ui->m1MtpaBox, &QCheckBox::toggled,
            [this](bool checked) { (void)checked; settingChanged(); });
    connect(ui->m2MtpaBox, &QCheckBox::toggled,
            [this](bool checked) { (void)checked; settingChanged(); });

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

    mSettingUpdateRequired = false;
    mSettingUpdateTimer = new QTimer(this);
    mSettingUpdateTimer->start(20);

    connect(mSettingUpdateTimer, &QTimer::timeout, [this]() {
        if (mSettingUpdateRequired) {
            ui->testTorqueBox->setEnabled(ui->testModeTorqueButton->isChecked() ||
                                          ui->testModeRpmButton->isChecked() ||
                                          ui->testModeVbusButton->isChecked() ||
                                          ui->testModeVBFWButton->isChecked() ||
                                          ui->testModeVBRPMButton->isChecked());
            ui->testPowerBox->setEnabled(ui->testModeRpmPowerButton->isChecked() ||
                                         ui->testModeExpButton->isChecked());
            ui->testRpmStartBox->setEnabled(ui->testModeRpmPowerButton->isChecked() ||
                                            ui->testModeExpButton->isChecked());
            ui->testRpmBox->setEnabled(ui->testModeRpmPowerButton->isChecked() ||
                                       ui->testModeExpButton->isChecked() ||
                                       ui->testModeRpmButton->isChecked() ||
                                       ui->testModeTorqueButton->isChecked() ||
                                       ui->testModeVBFWButton->isChecked() ||
                                       ui->testModeVBRPMButton->isChecked());
            ui->testExpBox->setEnabled(ui->testModeExpButton->isChecked());
            ui->testExpBaseTorqueBox->setEnabled(ui->testModeExpButton->isChecked());
            ui->testVbusBox->setEnabled(ui->testModeVbusButton->isChecked() ||
                                        ui->testModeVBFWButton->isChecked() ||
                                        ui->testModeVBRPMButton->isChecked());

            if (ui->tabWidget->currentIndex() == 1) {
                setQmlMotorParams();
            }

            if (ui->testLiveUpdateBox->isChecked()) {
                on_testRunButton_clicked();
            }

            mSettingUpdateRequired = false;
        }
    });

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
    addDataItemBoth("Torque Out");
    addDataItemBoth("Torque Shaft");
    addDataItemBoth("RPM Out");
    addDataItemBoth("RPM Shaft");
    addDataItemBoth("ExtraVal");
    addDataItemBoth("ExtraVal2");
    addDataItemBoth("ExtraVal3");
    addDataItemBoth("ExtraVal4");
    addDataItemBoth("ERPM");
    addDataItemBoth("km/h");
    addDataItemBoth("mph", false);
    addDataItemBoth("wh/km", false);
    addDataItemBoth("wh/mi", false);
    addDataItemBoth("KV (BLDC)", false);
    addDataItemBoth("KV Noload (BLDC)", false);

    ui->splitter->setSizes(QList<int>({100, 6000}));
}

PageMotorComparison::~PageMotorComparison()
{    
    QSettings set;
    set.setValue("pagemotorcomparison/m1confpath", ui->m1ConfFileEdit->text());
    set.setValue("pagemotorcomparison/m2confpath", ui->m2ConfFileEdit->text());
    set.setValue("pagemotorcomparison/qmlpath", ui->qmlFileEdit->text());
    delete ui;
}

VescInterface *PageMotorComparison::vesc() const
{
    return mVesc;
}

void PageMotorComparison::setVesc(VescInterface *vesc)
{
    mVesc = vesc;

    ui->qmlWidget->engine()->rootContext()->setContextProperty("VescIf", mVesc);
    ui->qmlWidget->engine()->rootContext()->setContextProperty("QmlUi", this);
    ui->qmlWidget->engine()->rootContext()->setContextProperty("Utility", &mUtil);

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
    mSettingUpdateRequired = true;
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
    ui->plot->replotWhenVisible();
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
        table->item(ind++, 1)->setText(QString::number(md.torque_out, 'f', 3) + " Nm");
        table->item(ind++, 1)->setText(QString::number(md.torque_motor_shaft, 'f', 3) + " Nm");
        table->item(ind++, 1)->setText(QString::number(md.rpm_out, 'f', 1));
        table->item(ind++, 1)->setText(QString::number(md.rpm_motor_shaft, 'f', 1));
        table->item(ind++, 1)->setText(QString::number(md.extraVal, 'f', 1));
        table->item(ind++, 1)->setText(QString::number(md.extraVal2, 'f', 1));
        table->item(ind++, 1)->setText(QString::number(md.extraVal3, 'f', 1));
        table->item(ind++, 1)->setText(QString::number(md.extraVal4, 'f', 1));
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

    if (ui->tabWidget->currentIndex() == 1) {
        auto param = getQmlParam(posx);
        MotorData md;
        md.configure(&mM1Config, getParamsUi(1));
        md.update(param.rpmM1, param.torqueM1);
        md.extraVal = param.extraM1;
        md.extraVal2 = param.extraM1_2;
        md.extraVal3 = param.extraM1_3;
        md.extraVal4 = param.extraM1_4;
        updateTable(md, ui->m1PlotTable);
        md.configure(&mM2Config, getParamsUi(2));
        md.update(param.rpmM2, param.torqueM2);
        md.extraVal = param.extraM2;
        md.extraVal2 = param.extraM2_2;
        md.extraVal3 = param.extraM2_3;
        md.extraVal4 = param.extraM2_4;
        updateTable(md, ui->m2PlotTable);
        setQmlProgressSelected(posx);
    } else  {
        if (ui->testModeTorqueButton->isChecked()) {
            MotorData md;
            md.configure(&mM1Config, getParamsUi(1));
            md.update(ui->testRpmBox->value(), posx);
            updateTable(md, ui->m1PlotTable);
            md.configure(&mM2Config, getParamsUi(2));
            md.update(ui->testRpmBox->value(), posx);
            updateTable(md, ui->m2PlotTable);
        } else if (ui->testModeRpmButton->isChecked()) {
            MotorData md;
            md.configure(&mM1Config, getParamsUi(1));
            md.update(posx, ui->testTorqueBox->value());
            updateTable(md, ui->m1PlotTable);
            md.configure(&mM2Config, getParamsUi(2));
            md.update(posx, ui->testTorqueBox->value());
            updateTable(md, ui->m2PlotTable);
        } else if (ui->testModeRpmPowerButton->isChecked()) {
            double rps = posx * 2.0 * M_PI / 60.0;
            double torque = ui->testPowerBox->value() / rps;

            MotorData md;
            md.configure(&mM1Config, getParamsUi(1));
            md.update(posx, torque);
            updateTable(md, ui->m1PlotTable);
            md.configure(&mM2Config, getParamsUi(2));
            md.update(posx, torque);
            updateTable(md, ui->m2PlotTable);
        } else if (ui->testModeExpButton->isChecked()) {
            double rpm_start = ui->testRpmStartBox->value();
            double rps = posx * 2.0 * M_PI / 60.0;
            double prop_exp = ui->testExpBox->value();
            double baseTorque = ui->testExpBaseTorqueBox->value();
            double topRpm = ui->testRpmBox->value();
            double power = ui->testPowerBox->value();
            double p_max_const = power / pow(topRpm - rpm_start, prop_exp);
            double torque = (p_max_const * pow(posx > rpm_start ? (posx - rpm_start) : 0.0, prop_exp)) / rps;
            torque += baseTorque;

            MotorData md;
            md.configure(&mM1Config, getParamsUi(1));
            md.update(posx, torque);
            updateTable(md, ui->m1PlotTable);
            md.configure(&mM2Config, getParamsUi(2));
            md.update(posx, torque);
            updateTable(md, ui->m2PlotTable);
        } else if (ui->testModeVbusButton->isChecked()) {
            MotorData md;
            md.configure(&mM1Config, getParamsUi(1));
            md.updateTorqueVBus(posx, ui->testVbusBox->value());
            updateTable(md, ui->m1PlotTable);
            md.configure(&mM2Config, getParamsUi(2));
            md.updateTorqueVBus(posx, ui->testVbusBox->value());
            updateTable(md, ui->m2PlotTable);
        } else if (ui->testModeVBFWButton->isChecked()) {
            MotorData md;
            md.configure(&mM1Config, getParamsUi(1));
            md.updateTorqueVBusFW(posx, ui->testRpmBox->value(), ui->testVbusBox->value());
            updateTable(md, ui->m1PlotTable);
            md.configure(&mM2Config, getParamsUi(2));
            md.updateTorqueVBusFW(posx, ui->testRpmBox->value(), ui->testVbusBox->value());
            updateTable(md, ui->m2PlotTable);
        } else if (ui->testModeVBRPMButton->isChecked()) {
            MotorData md;
            md.configure(&mM1Config, getParamsUi(1));
            md.updateRpmVBusFW(ui->testTorqueBox->value(), posx, ui->testVbusBox->value());
            updateTable(md, ui->m1PlotTable);
            md.configure(&mM2Config, getParamsUi(2));
            md.updateRpmVBusFW(ui->testTorqueBox->value(), posx, ui->testVbusBox->value());
            updateTable(md, ui->m2PlotTable);
        }
    }
}

MotorDataParams PageMotorComparison::getParamsUi(int motor)
{
    MotorDataParams sel;

    if (motor == 1) {
        sel.gearing = ui->m1GearingBox->value();
        sel.gearingEfficiency = ui->m1GearEfficiencyBox->value() / 100.0;
        sel.motorNum = ui->m1MotorNumBox->value();
        sel.tempInc = ui->m1TempIncBox->value();
        sel.fwCurrent = ui->m1FwBox->value();
        sel.maxRpm = ui->m1MaxRpmBox->value();
        sel.mtpa = ui->m1MtpaBox->isChecked();
    } else if (motor == 2) {
        sel.gearing = ui->m2GearingBox->value();
        sel.gearingEfficiency = ui->m2GearEfficiencyBox->value() / 100.0;
        sel.motorNum = ui->m2MotorNumBox->value();
        sel.tempInc = ui->m2TempIncBox->value();
        sel.fwCurrent = ui->m2FwBox->value();
        sel.maxRpm = ui->m2MaxRpmBox->value();
        sel.mtpa = ui->m2MtpaBox->isChecked();
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
            case 14:
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(md.torque_out * rowScale);
                names.append(namePrefix + QString("(Nm * %1)").arg(rowScale));
                rowInd++; break;
            case 15:
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(md.torque_motor_shaft * rowScale);
                names.append(namePrefix + QString("(Nm * %1)").arg(rowScale));
                rowInd++; break;
            case 16:
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(md.rpm_out * rowScale);
                names.append(namePrefix + QString("(RPM * %1)").arg(rowScale));
                rowInd++; break;
            case 17:
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(md.rpm_motor_shaft * rowScale);
                names.append(namePrefix + QString("(RPM * %1)").arg(rowScale));
                rowInd++; break;
            case 18:
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(md.extraVal * rowScale);
                names.append(namePrefix + QString("(Unit * %1)").arg(rowScale));
                rowInd++; break;
            case 19:
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(md.extraVal2 * rowScale);
                names.append(namePrefix + QString("(Unit * %1)").arg(rowScale));
                rowInd++; break;
            case 20:
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(md.extraVal3 * rowScale);
                names.append(namePrefix + QString("(Unit * %1)").arg(rowScale));
                rowInd++; break;
            case 21:
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(md.extraVal4 * rowScale);
                names.append(namePrefix + QString("(Unit * %1)").arg(rowScale));
                rowInd++; break;
            case 22:
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(md.erpm * rowScale);
                names.append(namePrefix + QString("(Unit * %1)").arg(rowScale));
                rowInd++; break;
            case 23:
                if (yAxes.size() <= rowInd) yAxes.append(QVector<double>());
                yAxes[rowInd].append(md.km_h * rowScale);
                names.append(namePrefix + QString("(Unit * %1)").arg(rowScale));
                rowInd++; break;
            default:
                break;
            }
        }
    };

    double plotPoints = ui->pointsBox->value();

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
            if (ui->scatterPlotBox->isChecked()) {
                ui->plot->graph(graphNow)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 5));
            }
            ui->plot->graph(graphNow)->setName(names.at(i));
            ui->plot->graph(graphNow)->setData(xAxis, yAxes.at(i));
        }

        if (ui->autoscaleButton->isChecked()) {
            ui->plot->rescaleAxes();
        }

        ui->plot->replotWhenVisible();
    };

    auto plotTorqueSweep = [this, updateData, updateGraphs, plotPoints](QTableWidget *table,
            ConfigParams &config, MotorDataParams param) {
        double torque = fabs(ui->testTorqueBox->value());
        double rpm = ui->testRpmBox->value();

        QVector<double> xAxis;
        QVector<QVector<double> > yAxes;
        QVector<QString> names;

        double torque_start = -torque;
        if (!ui->testNegativeBox->isChecked()) {
            torque_start = torque / plotPoints;
        }

        for (double t = torque_start;t < torque;t += (torque / plotPoints)) {
            MotorData md;
            md.configure(&config, param);
            md.update(rpm, t);
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

    auto plotRpmSweep = [this, updateData, updateGraphs, plotPoints](QTableWidget *table,
            ConfigParams &config, MotorDataParams param) {
        double torque = ui->testTorqueBox->value();
        double rpm = ui->testRpmBox->value();

        QVector<double> xAxis;
        QVector<QVector<double> > yAxes;
        QVector<QString> names;

        double rpm_start = -rpm;
        if (!ui->testNegativeBox->isChecked()) {
            rpm_start = rpm / plotPoints;
        }

        for (double r = rpm_start;r < rpm;r += (rpm / plotPoints)) {
            MotorData md;
            md.configure(&config, param);
            md.update(r, torque);
            xAxis.append(r);
            updateData(md, table, yAxes, names);

            if (md.rpm_motor_shaft >= param.maxRpm) {
                break;
            }
        }

        ui->plot->xAxis->setLabel("RPM");
        updateGraphs(xAxis, yAxes, names);
    };

    auto plotPowerSweep = [this, updateData, updateGraphs, plotPoints](QTableWidget *table,
            ConfigParams &config, MotorDataParams param) {
        double rpm = ui->testRpmBox->value();
        double rpm_start = ui->testRpmStartBox->value();
        double power = ui->testPowerBox->value();

        QVector<double> xAxis;
        QVector<QVector<double> > yAxes;
        QVector<QString> names;

        for (double r = rpm_start;r < rpm;r += (rpm / plotPoints)) {
            double rps = r * 2.0 * M_PI / 60.0;
            double torque = power / rps;

            MotorData md;
            md.configure(&config, param);
            md.update(r, torque);
            xAxis.append(r);
            updateData(md, table, yAxes, names);

            if (md.rpm_motor_shaft >= param.maxRpm) {
                break;
            }
        }

        ui->plot->xAxis->setLabel("RPM");
        updateGraphs(xAxis, yAxes, names);
    };

    auto plotPropSweep = [this, updateData, updateGraphs, plotPoints](QTableWidget *table,
            ConfigParams &config, MotorDataParams param) {
        double rpm = ui->testRpmBox->value();
        double power = ui->testPowerBox->value();
        double prop_exp = ui->testExpBox->value();
        double baseTorque = ui->testExpBaseTorqueBox->value();
        double rpm_start = ui->testRpmStartBox->value();
        double p_max_const = power / pow(rpm - rpm_start, prop_exp);

        QVector<double> xAxis;
        QVector<QVector<double> > yAxes;
        QVector<QString> names;

        for (double r = rpm / plotPoints;r < rpm;r += (rpm / plotPoints)) {
            double rps = r * 2.0 * M_PI / 60.0;
            double power = p_max_const * pow(r > rpm_start ? (r - rpm_start) : 0.0, prop_exp);
            double torque = power / rps;
            torque += baseTorque;

            MotorData md;
            md.configure(&config, param);
            md.update(r, torque);
            xAxis.append(r);
            updateData(md, table, yAxes, names);

            if (md.rpm_motor_shaft >= param.maxRpm) {
                break;
            }
        }

        ui->plot->xAxis->setLabel("RPM");
        updateGraphs(xAxis, yAxes, names);
    };

    auto plotVbusSweep = [this, updateData, updateGraphs, plotPoints](QTableWidget *table,
            ConfigParams &config, MotorDataParams param) {
        double torque = fabs(ui->testTorqueBox->value());
        double vbus = ui->testVbusBox->value();

        QVector<double> xAxis;
        QVector<QVector<double> > yAxes;
        QVector<QString> names;

        double torque_start = -torque;
        if (!ui->testNegativeBox->isChecked()) {
            torque_start = torque / plotPoints;
        }

        for (double t = torque_start;t < torque;t += (torque / plotPoints)) {
            MotorData md;
            md.configure(&config, param);
            md.updateTorqueVBus(t, vbus);
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

    auto plotVBFWSweep = [this, updateData, updateGraphs, plotPoints](QTableWidget *table,
            ConfigParams &config, MotorDataParams param) {
        double torque = fabs(ui->testTorqueBox->value());
        double vbus = ui->testVbusBox->value();
        double rpm = ui->testRpmBox->value();

        QVector<double> xAxis;
        QVector<QVector<double> > yAxes;
        QVector<QString> names;

        double torque_start = -torque;
        if (!ui->testNegativeBox->isChecked()) {
            torque_start = torque / plotPoints;
        }

        for (double t = torque_start;t < torque;t += (torque / plotPoints)) {
            MotorData md;
            md.configure(&config, param);
            md.updateTorqueVBusFW(t, rpm, vbus);
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

    auto plotVBRPMSweep = [this, updateData, updateGraphs, plotPoints](QTableWidget *table,
            ConfigParams &config, MotorDataParams param) {
        double torque = fabs(ui->testTorqueBox->value());
        double vbus = ui->testVbusBox->value();
        double rpm = ui->testRpmBox->value();

        QVector<double> xAxis;
        QVector<QVector<double> > yAxes;
        QVector<QString> names;

        double rpm_start = -rpm;
        if (!ui->testNegativeBox->isChecked()) {
            rpm_start = rpm / plotPoints;
        }

        for (double r = rpm_start;r < rpm;r += (rpm / plotPoints)) {
            MotorData md;
            md.configure(&config, param);
            md.updateRpmVBusFW(torque, r, vbus);
            xAxis.append(r);
            updateData(md, table, yAxes, names);

            if (md.rpm_motor_shaft >= param.maxRpm) {
                mVesc->emitMessageDialog("Max RPM", "Maximum motor shaft RPM exceeded", false);
                break;
            }
        }

        ui->plot->xAxis->setLabel("RPM");
        updateGraphs(xAxis, yAxes, names);
    };

    auto plotQmlSweep = [this, updateData, updateGraphs, plotPoints](QTableWidget *table,
            ConfigParams &config, MotorDataParams param, int motor) {

        QVector<double> xAxis;
        QVector<QVector<double> > yAxes;
        QVector<QString> names;
        double min = getQmlXMin();
        double max = getQmlXMax();

        for (double p = min; p < max; p += (max - min) / plotPoints) {
            auto rpmTorque = getQmlParam(p);

            MotorData md;
            md.configure(&config, param);

            if (motor == 1) {
                md.update(rpmTorque.rpmM1, rpmTorque.torqueM1);
                md.extraVal = rpmTorque.extraM1;
                md.extraVal2 = rpmTorque.extraM1_2;
                md.extraVal3 = rpmTorque.extraM1_3;
                md.extraVal4 = rpmTorque.extraM1_4;
            } else {
                md.update(rpmTorque.rpmM2, rpmTorque.torqueM2);
                md.extraVal = rpmTorque.extraM2;
                md.extraVal2 = rpmTorque.extraM2_2;
                md.extraVal3 = rpmTorque.extraM2_3;
                md.extraVal4 = rpmTorque.extraM2_4;
            }

            xAxis.append(p);
            updateData(md, table, yAxes, names);

            if (md.rpm_motor_shaft >= param.maxRpm) {
                break;
            }
        }

        ui->plot->xAxis->setLabel(getQmlXName());
        updateGraphs(xAxis, yAxes, names);
    };

    if (ui->tabWidget->currentIndex() == 1) {
        ui->plot->clearGraphs();
        plotQmlSweep(ui->m1PlotTable, mM1Config, getParamsUi(1), 1);
        plotQmlSweep(ui->m2PlotTable, mM2Config, getParamsUi(2), 2);
    } else {
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
        } else if (ui->testModeExpButton->isChecked()) {
            ui->plot->clearGraphs();
            plotPropSweep(ui->m1PlotTable, mM1Config, getParamsUi(1));
            plotPropSweep(ui->m2PlotTable, mM2Config, getParamsUi(2));
        } else if (ui->testModeVbusButton->isChecked()) {
            ui->plot->clearGraphs();
            plotVbusSweep(ui->m1PlotTable, mM1Config, getParamsUi(1));
            plotVbusSweep(ui->m2PlotTable, mM2Config, getParamsUi(2));
        } else if (ui->testModeVBFWButton->isChecked()) {
            ui->plot->clearGraphs();
            plotVBFWSweep(ui->m1PlotTable, mM1Config, getParamsUi(1));
            plotVBFWSweep(ui->m2PlotTable, mM2Config, getParamsUi(2));
        } else if (ui->testModeVBRPMButton->isChecked()) {
            ui->plot->clearGraphs();
            plotVBRPMSweep(ui->m1PlotTable, mM1Config, getParamsUi(1));
            plotVBRPMSweep(ui->m2PlotTable, mM2Config, getParamsUi(2));
        }
    }

    mRunDone = true;
}

void PageMotorComparison::on_qmlChooseButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("QML File"),
                                                    QFileInfo(ui->qmlFileEdit->text()).canonicalFilePath(),
                                                    tr("QML files (*.qml)"));

    if (!fileName.isEmpty()) {
        ui->qmlFileEdit->setText(fileName);
    }
}

void PageMotorComparison::on_qmlRunButton_clicked()
{
    QFile file(ui->qmlFileEdit->text());
    QFileInfo fi(ui->qmlFileEdit->text());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        mVesc->emitMessageDialog(tr("Open QML"), tr("Could not open file."), false, false);
        return;
    }

    QString code = file.readAll();
    file.close();

    ui->qmlWidget->setSource(QUrl(QLatin1String("qrc:/res/qml/DynamicLoader.qml")));
    ui->qmlWidget->engine()->clearComponentCache();

    code.prepend("import \"qrc:/mobile\";");
    code.prepend("import Vedder.vesc.vescinterface 1.0;");

    QFileInfo f(fi.path());
    if (f.exists() && f.isDir()) {
        code.prepend("import \"file:/" + fi.path() + "\";");
    }

    QTimer::singleShot(500, [this]() {
        connect(ui->qmlWidget->rootObject()->findChild<QObject*>("idComp"),
                SIGNAL(testChanged()), this, SLOT(qmlTestChanged()));
        connect(ui->qmlWidget->rootObject()->findChild<QObject*>("idComp"),
                SIGNAL(namesUpdated()), this, SLOT(qmlNamesUpdated()));
        setQmlMotorParams();
        on_testRunButton_clicked();
    });

    emit reloadQml(code);

    mQmlXNameOk = true;
    mQmlXMinOk = true;
    mQmlXMaxOk = true;
    mQmlProgressOk = true;
    mQmlMotorParamsOk = true;
    mQmlReadNamesDone = false;
}

void PageMotorComparison::on_qmlStopButton_clicked()
{
    ui->qmlWidget->setSource(QUrl(QLatin1String("")));
}

void PageMotorComparison::qmlTestChanged()
{
    QTimer::singleShot(0, [this]() {
        on_testRunButton_clicked();
    });
}

void PageMotorComparison::qmlNamesUpdated()
{
    QTimer::singleShot(0, [this]() {
        qmlUpdateNames();
    });
}

PageMotorComparison::QmlParams PageMotorComparison::getQmlParam(double progress)
{
    QVariant returnedValue;
    bool ok = QMetaObject::invokeMethod(ui->qmlWidget->rootObject()->findChild<QObject*>("idComp"),
                                        "progressToParams",
                                        Q_RETURN_ARG(QVariant, returnedValue), Q_ARG(QVariant, QVariant(progress)));

    QmlParams res;

    if (ok) {
        ok = returnedValue.canConvert(QMetaType::QVariantList);
    }

    if (!ok) {
        return res;
    }

    auto list = returnedValue.toList();

    if (list.size() >= 2) {
        res.rpmM1 = list.at(0).toDouble();
        res.torqueM1 = list.at(1).toDouble();
    }

    if (list.size() >= 3) {
        res.extraM1 = list.at(2).toDouble();
    }

    if (list.size() >= 4) {
        res.extraM1_2 = list.at(3).toDouble();
    }

    if (list.size() >= 5) {
        res.extraM1_3 = list.at(4).toDouble();
    }

    if (list.size() >= 6) {
        res.extraM1_4 = list.at(5).toDouble();
    }

    if (list.size() >= 12) {
        res.rpmM2 = list.at(6).toDouble();
        res.torqueM2 = list.at(7).toDouble();
        res.extraM2 = list.at(8).toDouble();
        res.extraM2_2 = list.at(9).toDouble();
        res.extraM2_3 = list.at(10).toDouble();
        res.extraM2_4 = list.at(11).toDouble();
    } else {
        res.rpmM2 = res.rpmM1;
        res.torqueM2 = res.torqueM1;
        res.extraM2 = res.extraM1;
        res.extraM2_2 = res.extraM1_2;
        res.extraM2_3 = res.extraM1_3;
        res.extraM2_4 = res.extraM1_4;
    }

    if (!mQmlReadNamesDone) {
        mQmlReadNamesDone = true;
        qmlUpdateNames();
    }

    return res;
}

bool PageMotorComparison::qmlUpdateNames()
{
    QVariant returnedValue;

    bool ok = QMetaObject::invokeMethod(ui->qmlWidget->rootObject()->findChild<QObject*>("idComp"),
                                   "extraNames",
                                   Q_RETURN_ARG(QVariant, returnedValue));

    if (ok) {
        ok = returnedValue.canConvert(QMetaType::QVariantList);
    }

    if (ok) {
        auto list = returnedValue.toList();

        if (list.size() >= 1 && list.at(0).canConvert(QMetaType::QString)) {
            ui->m1PlotTable->item(18, 0)->setText(list.at(0).toString());
        }

        if (list.size() >= 2 && list.at(1).canConvert(QMetaType::QString)) {
            ui->m1PlotTable->item(19, 0)->setText(list.at(1).toString());
        }

        if (list.size() >= 3 && list.at(2).canConvert(QMetaType::QString)) {
            ui->m1PlotTable->item(20, 0)->setText(list.at(2).toString());
        }

        if (list.size() >= 4 && list.at(3).canConvert(QMetaType::QString)) {
            ui->m1PlotTable->item(21, 0)->setText(list.at(3).toString());
        }

        if (list.size() >= 5 && list.at(4).canConvert(QMetaType::QString)) {
            ui->m2PlotTable->item(18, 0)->setText(list.at(4).toString());
        }

        if (list.size() >= 6 && list.at(5).canConvert(QMetaType::QString)) {
            ui->m2PlotTable->item(19, 0)->setText(list.at(5).toString());
        }

        if (list.size() >= 7 && list.at(6).canConvert(QMetaType::QString)) {
            ui->m2PlotTable->item(20, 0)->setText(list.at(6).toString());
        }

        if (list.size() >= 8 && list.at(7).canConvert(QMetaType::QString)) {
            ui->m2PlotTable->item(21, 0)->setText(list.at(7).toString());
        }
    }

    return ok;
}

QString PageMotorComparison::getQmlXName()
{
    if (!mQmlXNameOk) {
        return "Progress";
    }

    QVariant returnedValue;
    bool ok = QMetaObject::invokeMethod(ui->qmlWidget->rootObject()->findChild<QObject*>("idComp"),
                                        "xAxisName",
                                        Q_RETURN_ARG(QVariant, returnedValue));

    if (ok) {
        ok = returnedValue.canConvert(QMetaType::QString);
    }

    mQmlXNameOk = ok;

    if (ok) {
        return returnedValue.toString();
    } else {
        return "Progress";
    }
}

double PageMotorComparison::getQmlXMin()
{
    if (!mQmlXMinOk) {
        return 0.0;
    }

    QVariant returnedValue;
    bool ok = QMetaObject::invokeMethod(ui->qmlWidget->rootObject()->findChild<QObject*>("idComp"),
                                        "xAxisMin",
                                        Q_RETURN_ARG(QVariant, returnedValue));

    if (ok) {
        ok = returnedValue.canConvert(QMetaType::Double);
    }

    mQmlXMinOk = ok;

    if (ok) {
        return returnedValue.toDouble();
    } else {
        return 0.0;
    }
}

double PageMotorComparison::getQmlXMax()
{
    if (!mQmlXMaxOk) {
        return 1.0;
    }

    QVariant returnedValue;
    bool ok = QMetaObject::invokeMethod(ui->qmlWidget->rootObject()->findChild<QObject*>("idComp"),
                                        "xAxisMax",
                                        Q_RETURN_ARG(QVariant, returnedValue));

    if (ok) {
        ok = returnedValue.canConvert(QMetaType::Double);
    }

    mQmlXMaxOk = ok;

    if (ok) {
        return returnedValue.toDouble();
    } else {
        return 1.0;
    }
}

void PageMotorComparison::setQmlProgressSelected(double progress)
{
    if (!mQmlProgressOk) {
        return;
    }

    mQmlProgressOk = QMetaObject::invokeMethod(ui->qmlWidget->rootObject()->findChild<QObject*>("idComp"),
                                               "progressSelected",
                                               Q_ARG(QVariant, progress));
}

void PageMotorComparison::setQmlMotorParams()
{
    if (!mQmlMotorParamsOk) {
        return;
    }

    mQmlMotorParamsOk = QMetaObject::invokeMethod(ui->qmlWidget->rootObject()->findChild<QObject*>("idComp"),
                                                  "motorDataUpdated",
                                                  Q_ARG(QVariant, QVariant::fromValue(MotorData(&mM1Config, getParamsUi(1)))),
                                                  Q_ARG(QVariant, QVariant::fromValue(MotorData(&mM2Config, getParamsUi(2)))));
}
