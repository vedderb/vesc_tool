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

#include "detectimu.h"
#include "ui_detectimu.h"
#include "helpdialog.h"
#include <QMessageBox>

DetectIMU::DetectIMU(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DetectIMU)
{
    ui->setupUi(this);
    layout()->setContentsMargins(0, 0, 0, 0);
    mVesc = 0;
}

DetectIMU::~DetectIMU()
{
    delete ui;
}

void DetectIMU::on_helpButton_clicked()
{
    if (mVesc) {
        HelpDialog::showHelp(this, mVesc->infoConfig(), "help_foc_hall_detect");
    }
}

void DetectIMU::on_startButton_clicked()
{
    if (mVesc) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::warning(this,
                                     tr("Detect IMU calibration"),
                                     tr("Make sure to your vehicle is level and steady."),
                                     QMessageBox::Ok | QMessageBox::Cancel);

        if (reply == QMessageBox::Ok) {
            mVesc->commands()->getImuCalibration(ui->yawBox->value());
        }
    }
}

void DetectIMU::on_applyButton_clicked()
{
    if (mVesc) {
        mVesc->appConfig()->updateParamDouble("imu_conf.rot_roll", ui->calRBox->value());
        mVesc->appConfig()->updateParamDouble("imu_conf.rot_pitch", ui->calPBox->value());
        mVesc->appConfig()->updateParamDouble("imu_conf.rot_yaw", ui->calYBox->value());
        mVesc->appConfig()->updateParamDouble("imu_conf.accel_offsets__0", ui->calAXBox->value());
        mVesc->appConfig()->updateParamDouble("imu_conf.accel_offsets__1", ui->calAYBox->value());
        mVesc->appConfig()->updateParamDouble("imu_conf.accel_offsets__2", ui->calAZBox->value());
        mVesc->appConfig()->updateParamDouble("imu_conf.gyro_offsets__0", ui->calGXBox->value());
        mVesc->appConfig()->updateParamDouble("imu_conf.gyro_offsets__1", ui->calGYBox->value());
        mVesc->appConfig()->updateParamDouble("imu_conf.gyro_offsets__2", ui->calGZBox->value());
        mVesc->emitStatusMessage(tr("IMU Parameters Applied"), true);
    }
}

VescInterface *DetectIMU::vesc() const
{
    return mVesc;
}

void DetectIMU::setVesc(VescInterface *vesc)
{
    mVesc = vesc;

    if (mVesc) {
//        connect(mVesc->commands(), SIGNAL(focHallTableReceived(QVector<int>,int)),
//                this, SLOT(focHallTableReceived(QVector<int>,int)));
    }
}

void DetectIMU::imuCalibrationReceived(QVector<float> hall_table, int res)
{
    if (res != 0) {
        mVesc->emitStatusMessage(tr("Bad FOC Hall Detection Result Received"), false);
    } else {
        mVesc->emitStatusMessage(tr("FOC Hall Result Received"), true);
//        ui->hall0Box->setValue(hall_table.at(0));
//        ui->hall1Box->setValue(hall_table.at(1));
//        ui->hall2Box->setValue(hall_table.at(2));
//        ui->hall3Box->setValue(hall_table.at(3));
//        ui->hall4Box->setValue(hall_table.at(4));
//        ui->hall5Box->setValue(hall_table.at(5));
//        ui->hall6Box->setValue(hall_table.at(6));
//        ui->hall7Box->setValue(hall_table.at(7));
    }
}
