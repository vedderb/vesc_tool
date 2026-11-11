/*
    Copyright 2016 - 2019 Benjamin Vedder	benjamin@vedder.se

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

#include "preferences.h"
#include "ui_preferences.h"
#include <QDebug>
#include <cmath>
#include <QFileDialog>
#include "utility.h"

Preferences::Preferences(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Preferences)
{
    mVesc = nullptr;

    ui->setupUi(this);

    ui->pollRestoreButton->setIcon(Utility::getIcon("icons/Restart-96.png"));
    ui->pathScriptInputChooseButton->setIcon(Utility::getIcon("icons/Open Folder-96.png"));
    ui->pathRtLogChooseButton->setIcon(Utility::getIcon("icons/Open Folder-96.png"));
    ui->pathScriptOutputChooseButton->setIcon(Utility::getIcon("icons/Open Folder-96.png"));
    ui->pathLocalLogChooseButton->setIcon(Utility::getIcon("icons/Open Folder-96.png"));

    ui->uiScaleBox->setValue(mSettings.value("app_scale_factor", 1.0).toDouble());
    ui->uiPlotWidthBox->setValue(mSettings.value("plot_line_width",4.0).toDouble());
    ui->pathRtLogEdit->setText(mSettings.value("path_rt_log", "./log").toString());
    ui->pathScriptInputEdit->setText(mSettings.value("path_script_input", "./log").toString());
    ui->pathScriptOutputEdit->setText(mSettings.value("path_script_output", "./log").toString());
    ui->pathLocalLogEdit->setText(mSettings.value("path_local_log", "./log_local").toString());
    ui->pollRtDataBox->setValue(mSettings.value("poll_rate_rt_data", 50.0).toDouble());
    ui->pollAppDataBox->setValue(mSettings.value("poll_rate_app_data", 20.0).toDouble());
    ui->pollImuDataBox->setValue(mSettings.value("poll_rate_imu_data", 50.0).toDouble());
    ui->pollBmsDataBox->setValue(mSettings.value("poll_rate_bms_data", 10.0).toDouble());
    ui->darkModeBox->setChecked(Utility::isDarkMode());
    ui->estopTimeBox->setValue(mSettings.value("estop_ms", 5000).toInt());

    ui->uploadContentEditorButton->setChecked(mSettings.value("scripting/uploadContentEditor", true).toBool());
    ui->uploadContentFileButton->setChecked(!mSettings.value("scripting/uploadContentEditor", true).toBool());
    ui->reconnectCanBox->setChecked(mSettings.value("reconnectLastCan", true).toBool());
    ui->reduceLispBox->setChecked(mSettings.value("reduceLisp", false).toBool());
    ui->replMaxLineBox->setValue(mSettings.value("scripting/replMaxLines", 5000).toInt());

    saveSettingsChanged();
}

Preferences::~Preferences()
{
    saveSettingsChanged();
    delete ui;
}

VescInterface *Preferences::vesc() const
{
    return mVesc;
}

void Preferences::setVesc(VescInterface *vesc)
{
    mVesc = vesc;

    if (mVesc) {
        ui->useImperialBox->setChecked(vesc->useImperialUnits());

        connect(mVesc, &VescInterface::useImperialUnitsChanged,
                [this](bool useImperialUnits) {
            ui->useImperialBox->setChecked(useImperialUnits);
        });
    }
}

void Preferences::closeEvent(QCloseEvent *event)
{
    if (Utility::isDarkMode() != mLastIsDark) {
        mVesc->emitMessageDialog("Theme Changed",
                                 "Please restart VESC Tool for the theme changes to take effect.",
                                 false, false);
    }

    if (!Utility::almostEqual(mLastScaling,
                              mSettings.value("app_scale_factor", 1.0).toDouble(), 0.001)) {
        mVesc->emitMessageDialog("Scaling Changed",
                                 "Please restart VESC Tool for the scaling change to take effect.",
                                 false, false);
    }

    saveSettingsChanged();
    event->accept();
}

void Preferences::showEvent(QShowEvent *event)
{
    if (mVesc) {
        ui->loadQmlUiConnectBox->setChecked(mVesc->getLoadQmlUiOnConnect());
        ui->qmlUiAskBox->setChecked(mVesc->askQmlLoad());
        ui->showFwUpdateBox->setChecked(mVesc->showFwUpdateAvailable());
    }
    event->accept();
}

void Preferences::on_uiScaleBox_valueChanged(double arg1)
{
    mSettings.setValue("app_scale_factor", arg1);
}

void Preferences::on_uiPlotWidthBox_valueChanged(double arg1)
{
    mSettings.setValue("plot_line_width", arg1);
}

void Preferences::on_loadQmlUiConnectBox_toggled(bool checked)
{
    if (mVesc) {
        mVesc->setLoadQmlUiOnConnect(checked);
    }
}

void Preferences::on_qmlUiAskBox_toggled(bool checked)
{
    if (mVesc) {
        mVesc->setAskQmlLoad(checked);
    }
}

void Preferences::on_pathRtLogChooseButton_clicked()
{
    ui->pathRtLogEdit->setText(
                QFileDialog::getExistingDirectory(this, "Choose RT log output directory"));
}

void Preferences::on_pathScriptInputChooseButton_clicked()
{
    ui->pathScriptInputEdit->setText(
                QFileDialog::getExistingDirectory(this, "Choose script input file directory"));
}

void Preferences::on_pathRtLogEdit_textChanged(const QString &arg1)
{
    mSettings.setValue("path_rt_log", arg1);
    mSettings.sync();
}

void Preferences::on_pathScriptInputEdit_textChanged(const QString &arg1)
{
    mSettings.setValue("path_script_input", arg1);
    mSettings.sync();
}

void Preferences::on_pathScriptOutputChooseButton_clicked()
{
    ui->pathScriptOutputEdit->setText(
                QFileDialog::getExistingDirectory(this, "Choose script output file directory"));
}

void Preferences::on_pathScriptOutputEdit_textChanged(const QString &arg1)
{
    mSettings.setValue("path_script_output", arg1);
    mSettings.sync();
}

void Preferences::on_pollRtDataBox_valueChanged(double arg1)
{
    mSettings.setValue("poll_rate_rt_data", arg1);
    mSettings.sync();
}

void Preferences::on_pollAppDataBox_valueChanged(double arg1)
{
    mSettings.setValue("poll_rate_app_data", arg1);
    mSettings.sync();
}

void Preferences::on_pollImuDataBox_valueChanged(double arg1)
{
    mSettings.setValue("poll_rate_imu_data", arg1);
    mSettings.sync();
}

void Preferences::on_pollBmsDataBox_valueChanged(double arg1)
{
    mSettings.setValue("poll_rate_bms_data", arg1);
    mSettings.sync();
}

void Preferences::on_pollRestoreButton_clicked()
{
    ui->pollRtDataBox->setValue(50.0);
    ui->pollAppDataBox->setValue(20.0);
    ui->pollImuDataBox->setValue(50.0);
    ui->pollBmsDataBox->setValue(10.0);
}

void Preferences::on_darkModeBox_toggled(bool checked)
{
    Utility::setDarkMode(checked);
}

void Preferences::on_okButton_clicked(){
    close();
}

void Preferences::saveSettingsChanged()
{
    mLastScaling = mSettings.value("app_scale_factor", 1.0).toDouble();
    mLastIsDark = Utility::isDarkMode();
    mSettings.setValue("scripting/uploadContentEditor", ui->uploadContentEditorButton->isChecked());
    mSettings.setValue("reconnectLastCan", ui->reconnectCanBox->isChecked());
    mSettings.setValue("reduceLisp", ui->reduceLispBox->isChecked());
    mSettings.setValue("scripting/replMaxLines", ui->replMaxLineBox->value());

    mSettings.sync();
}

void Preferences::on_useImperialBox_toggled(bool checked)
{
    if (mVesc) {
        mVesc->setUseImperialUnits(checked);
        mVesc->commands()->emitEmptySetupValues();
    }
}

void Preferences::on_showFwUpdateBox_toggled(bool checked)
{
    if (mVesc) {
        mVesc->setShowFwUpdateAvailable(checked);
    }
}

void Preferences::on_estopTimeBox_valueChanged(int arg1)
{
    mSettings.setValue("estop_ms", arg1);
    mSettings.sync();
}

void Preferences::on_pathLocalLogChooseButton_clicked()
{
    ui->pathLocalLogEdit->setText(
                QFileDialog::getExistingDirectory(this, "Choose local log output file directory"));
}

void Preferences::on_pathLocalLogEdit_textChanged(const QString &arg1)
{
    mSettings.setValue("path_local_log", arg1);
    mSettings.sync();
}
