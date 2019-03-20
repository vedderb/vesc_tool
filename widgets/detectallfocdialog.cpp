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

#include "detectallfocdialog.h"
#include "ui_detectallfocdialog.h"
#include "utility.h"

#include <QMessageBox>

DetectAllFocDialog::DetectAllFocDialog(VescInterface *vesc, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DetectAllFocDialog)
{
    ui->setupUi(this);

    mVesc = vesc;
    mRejectOk = true;
    mPulleyMotorOld = 1;
    mPulleyWheelOld = 1;

    ui->dirSetup->setVesc(mVesc);

    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);

    // Simple tab
    QListWidgetItem *item = new QListWidgetItem;
    item->setText(tr("Mini Outrunner (~75 g)"));
    item->setIcon(QIcon("://res/images/motors/outrunner_mini.jpg"));
    item->setData(Qt::UserRole, QVariant::fromValue(MotorData(10, 1400, 4000, 14)));
    ui->motorList->addItem(item);

    item = new QListWidgetItem;
    item->setText(tr("Small Outrunner (~200 g)"));
    item->setIcon(QIcon("://res/images/motors/outrunner_small.jpg"));
    item->setData(Qt::UserRole, QVariant::fromValue(MotorData(25, 1400, 4000, 14)));
    ui->motorList->addItem(item);

    item = new QListWidgetItem;
    item->setText(tr("Medium Outrunner (~750 g)"));
    item->setIcon(QIcon("://res/images/motors/6374.jpg"));
    item->setData(Qt::UserRole, QVariant::fromValue(MotorData(60, 700, 4000, 14)));
    ui->motorList->addItem(item);

    item = new QListWidgetItem;
    item->setText(tr("Large Outrunner (~2000 g)"));
    item->setIcon(QIcon("://res/icons/motor.png"));
    item->setData(Qt::UserRole, QVariant::fromValue(MotorData(200, 700, 4000, 14)));
    ui->motorList->addItem(item);

    item = new QListWidgetItem;
    item->setText(tr("Small Inrunner (~200 g)"));
    item->setIcon(QIcon("://res/images/motors/inrunner_small.jpg"));
    item->setData(Qt::UserRole, QVariant::fromValue(MotorData(25, 1400, 4000, 2)));
    ui->motorList->addItem(item);

    item = new QListWidgetItem;
    item->setText(tr("Medium Inrunner (~750 g)"));
    item->setIcon(QIcon("://res/images/motors/inrunner_medium.jpg"));
    item->setData(Qt::UserRole, QVariant::fromValue(MotorData(70, 1400, 4000, 4)));
    ui->motorList->addItem(item);

    item = new QListWidgetItem;
    item->setText(tr("Large Inrunner (~2000 g)"));
    item->setIcon(QIcon("://res/icons/motor.png"));
    item->setData(Qt::UserRole, QVariant::fromValue(MotorData(200, 1000, 4000, 4)));
    ui->motorList->addItem(item);

    item = new QListWidgetItem;
    item->setText(tr("E-Bike DD hub motor (~6 kg)"));
    item->setIcon(QIcon("://res/images/motors/ebike_dd_1kw.jpg"));
    item->setData(Qt::UserRole, QVariant::fromValue(MotorData(75, 300, 2000, 46)));
    ui->motorList->addItem(item);

    item = new QListWidgetItem;
    item->setText(tr("EDF Inrunner Small (~200 g)"));
    item->setIcon(QIcon("://res/images/motors/edf_small.jpg"));
    item->setData(Qt::UserRole, QVariant::fromValue(MotorData(55, 1400, 4000, 6)));
    ui->motorList->addItem(item);

    ui->motorList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->motorList->setIconSize(QSize(120, 120));
    ui->motorList->setWordWrap(true);
    ui->motorList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->motorList->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->motorList->setCurrentRow(0);

    ui->simpleBatteryTab->addParamRow(mVesc->mcConfig(), "si_battery_type");
    ui->simpleBatteryTab->addParamRow(mVesc->mcConfig(), "si_battery_cells");
    ui->simpleBatteryTab->addParamRow(mVesc->mcConfig(), "si_battery_ah");

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(mVesc->mcConfig()->getEditor("si_wheel_diameter"));
    ui->wheelDiameterBox->setLayout(layout);

    // Advanced tab
    ui->paramTab->addParamRow(mVesc->mcConfig(), "si_motor_poles");
    ui->paramTab->addParamRow(mVesc->mcConfig(), "si_gear_ratio");
    ui->paramTab->addParamRow(mVesc->mcConfig(), "si_wheel_diameter");
    ui->paramTab->addParamRow(mVesc->mcConfig(), "si_battery_type");
    ui->paramTab->addParamRow(mVesc->mcConfig(), "si_battery_cells");
    ui->paramTab->addParamRow(mVesc->mcConfig(), "si_battery_ah");

    connect(ui->pulleyMotorBox, SIGNAL(valueChanged(int)),
            this, SLOT(updateGearRatio()));
    connect(ui->pulleyWheelBox, SIGNAL(valueChanged(int)),
            this, SLOT(updateGearRatio()));

    updateGearRatio();
}

DetectAllFocDialog::~DetectAllFocDialog()
{
    delete ui;
}

void DetectAllFocDialog::showDialog(VescInterface *vesc, QWidget *parent)
{
    DetectAllFocDialog *p = new DetectAllFocDialog(vesc, parent);
    vesc->mcConfig()->updateParamInt("si_battery_cells", 3);
    p->exec();
}

void DetectAllFocDialog::reject()
{
    if (mRejectOk) {
        QDialog::reject();
    }
}

void DetectAllFocDialog::updateGearRatio()
{
    mVesc->mcConfig()->updateParamDouble("si_gear_ratio",
                                         (double)ui->pulleyWheelBox->value() /
                                         (double)ui->pulleyMotorBox->value());
}

void DetectAllFocDialog::on_runButton_clicked()
{
    if (!mVesc->isPortConnected()) {
        mVesc->emitMessageDialog("Error",
                                 "The VESC is not connected. Please connect and try again.",
                                 false, false);
        return;
    }

    QMessageBox::StandardButton reply;
    reply = QMessageBox::warning(this,
                                 tr("Warning"),
                                 tr("<font color=\"red\">Warning: </font>"
                                    "This is going to spin up all motors connected on "
                                    "the CAN-bus. Make sure that nothing is in the way."),
                                 QMessageBox::Ok | QMessageBox::Cancel);

    if (reply != QMessageBox::Ok) {
        return;
    }

    mRejectOk = false;
    ui->tabWidget->setEnabled(false);
    ui->runButton->setEnabled(false);
    ui->closeButton->setEnabled(false);

    ui->progressBar->setRange(0, 0);

    mVesc->commands()->setMcconf(false);
    Utility::waitSignal(mVesc->commands(), SIGNAL(ackReceived(QString)), 2000);
    auto canDevs = mVesc->scanCan();

    if (!Utility::setBatteryCutCan(mVesc, canDevs, 6.0, 6.0)) {
        ui->tabWidget->setEnabled(true);
        ui->runButton->setEnabled(true);
        ui->closeButton->setEnabled(true);
        mRejectOk = true;
        ui->progressBar->setRange(0, 100);
        ui->progressBar->setValue(0);
        return;
    }

    QString res = Utility::detectAllFoc(mVesc, true,
                                        ui->maxPowerLossBox->value(),
                                        ui->currentInMinBox->value(),
                                        ui->currentInMaxBox->value(),
                                        ui->openloopErpmBox->value(),
                                        ui->sensorlessErpmBox->value());

    if (res.startsWith("Success!")) {
        Utility::setBatteryCutCanFromCurrentConfig(mVesc, canDevs);
    }

    ui->progressBar->setRange(0, 100);
    ui->progressBar->setValue(100);

    ui->tabWidget->setEnabled(true);
    ui->runButton->setEnabled(true);
    ui->closeButton->setEnabled(true);
    mRejectOk = true;

    QMessageBox *msg = new QMessageBox(QMessageBox::Information,
                                       "FOC Detection Result", res,
                                       QMessageBox::Ok, this);
    QFont font = QFont("DejaVu Sans Mono");
    msg->setFont(font);
    msg->exec();

    if (res.startsWith("Success!")) {
        ui->simpleSetupBox->setCurrentIndex(3);
        ui->dirSetup->scanVescs();
    }
}

void DetectAllFocDialog::on_closeButton_clicked()
{
    close();
}

void DetectAllFocDialog::on_nextMotorButton_clicked()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::warning(this,
                                 tr("Motor Selection"),
                                 tr("Warning: Choosing a significantly too large motor for detection is likely to destroy the motor "
                                    "during detection. It is important to choose a motor size similar to the motor you are using. "
                                    "Are you sure that your motor selection is within range?"),
                                 QMessageBox::Yes | QMessageBox::Cancel);
    if (reply == QMessageBox::Yes) {
        ui->simpleSetupBox->setCurrentIndex(1);
    }
}

void DetectAllFocDialog::on_prevBattButton_clicked()
{
    ui->simpleSetupBox->setCurrentIndex(0);
}

void DetectAllFocDialog::on_nextBattButton_clicked()
{
    ui->simpleSetupBox->setCurrentIndex(2);
}

void DetectAllFocDialog::on_prevSetupButton_clicked()
{
    ui->simpleSetupBox->setCurrentIndex(1);
}

void DetectAllFocDialog::on_directDriveBox_toggled(bool checked)
{
    ui->pulleyMotorBox->setEnabled(!checked);
    ui->pulleyWheelBox->setEnabled(!checked);

    if (checked) {
        mPulleyMotorOld = ui->pulleyMotorBox->value();
        mPulleyWheelOld = ui->pulleyWheelBox->value();
        ui->pulleyMotorBox->setValue(1);
        ui->pulleyWheelBox->setValue(1);
    } else {
        ui->pulleyMotorBox->setValue(mPulleyMotorOld);
        ui->pulleyWheelBox->setValue(mPulleyWheelOld);
    }
}

void DetectAllFocDialog::on_motorList_currentRowChanged(int currentRow)
{
    (void)currentRow;
    MotorData md = ui->motorList->currentItem()->data(Qt::UserRole).value<MotorData>();

    ui->maxPowerLossBox->setValue(md.maxLosses);
    ui->openloopErpmBox->setValue(md.openloopErpm);
    ui->sensorlessErpmBox->setValue(md.sensorlessErpm);
    mVesc->mcConfig()->updateParamInt("si_motor_poles", md.poles);
}

void DetectAllFocDialog::on_prevDirButton_clicked()
{
    ui->simpleSetupBox->setCurrentIndex(2);
}
