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

#ifdef HAS_GAMEPAD
    mGamepad = nullptr;
    mUseGamepadControl = false;
#endif

    mTimer = new QTimer(this);
    mTimer->start(100);
    connect(mTimer, SIGNAL(timeout()),
            this, SLOT(timerSlot()));

    ui->setupUi(this);

    ui->pollRestoreButton->setIcon(Utility::getIcon("icons/Restart-96.png"));
    ui->pathScriptInputChooseButton->setIcon(Utility::getIcon("icons/Open Folder-96.png"));
    ui->pathRtLogChooseButton->setIcon(Utility::getIcon("icons/Open Folder-96.png"));
    ui->pathScriptOutputChooseButton->setIcon(Utility::getIcon("icons/Open Folder-96.png"));
    ui->jsConf1Button->setIcon(Utility::getIcon("icons/Horizontal Settings Mixer-96.png"));
    ui->jsConf2Button->setIcon(Utility::getIcon("icons/Horizontal Settings Mixer-96.png"));
    ui->jsConf3Button->setIcon(Utility::getIcon("icons/Horizontal Settings Mixer-96.png"));
    ui->jsConf4Button->setIcon(Utility::getIcon("icons/Horizontal Settings Mixer-96.png"));
    ui->jsConnectButton->setIcon(Utility::getIcon("icons/Connected-96.png"));
    ui->jsScanButton->setIcon(Utility::getIcon("icons/Connected-96.png"));
    ui->jsResetConfigButton->setIcon(Utility::getIcon("icons/Restart-96.png"));

    ui->uiScaleBox->setValue(mSettings.value("app_scale_factor", 1.0).toDouble());
    ui->uiPlotWidthBox->setValue(mSettings.value("plot_line_width",4.0).toDouble());
    ui->pathRtLogEdit->setText(mSettings.value("path_rt_log", "./log").toString());
    ui->pathScriptInputEdit->setText(mSettings.value("path_script_input", "./log").toString());
    ui->pathScriptOutputEdit->setText(mSettings.value("path_script_output", "./log").toString());
    ui->pollRtDataBox->setValue(mSettings.value("poll_rate_rt_data", 50.0).toDouble());
    ui->pollAppDataBox->setValue(mSettings.value("poll_rate_app_data", 20.0).toDouble());
    ui->pollImuDataBox->setValue(mSettings.value("poll_rate_imu_data", 50.0).toDouble());
    ui->pollBmsDataBox->setValue(mSettings.value("poll_rate_bms_data", 10.0).toDouble());
    ui->darkModeBox->setChecked(Utility::isDarkMode());

#ifdef HAS_GAMEPAD
    auto confAxis = [](QGamepad *gp, QGamepadManager::GamepadAxis axis) {
        if (gp) {
            QGamepadManager::instance()->configureAxis(gp->deviceId(), axis);
        }
    };

    connect(ui->jsConf1Button, &QPushButton::clicked,
            [=]() {confAxis(mGamepad, QGamepadManager::AxisLeftX);});
    connect(ui->jsConf2Button, &QPushButton::clicked,
            [=]() {confAxis(mGamepad, QGamepadManager::AxisLeftY);});
    connect(ui->jsConf3Button, &QPushButton::clicked,
            [=]() {confAxis(mGamepad, QGamepadManager::AxisRightX);});
    connect(ui->jsConf4Button, &QPushButton::clicked,
            [=]() {confAxis(mGamepad, QGamepadManager::AxisRightY);});

    if (mSettings.contains("js_is_configured")) {
        ui->jsConfigOkBox->setChecked(mSettings.value("js_is_configured").toBool());
    }
    if (mSettings.contains("js_is_inverted")) {
        ui->jsInvertedBox->setChecked(mSettings.value("js_is_inverted").toBool());
    }
    if (mSettings.contains("js_is_bidirectional")) {
        ui->jsBidirectionalBox->setChecked(mSettings.value("js_is_bidirectional").toBool());
    }
    if (mSettings.contains("js_axis")) {
        ui->jseAxisBox->setCurrentIndex(mSettings.value("js_axis").toInt());
    }
    if (mSettings.contains("js_control_type")) {
        ui->jsControlTypeBox->setCurrentIndex(mSettings.value("js_control_type").toInt());
    }
    if (mSettings.contains("js_current_min")) {
        ui->jsCurrentMinBox->setValue(mSettings.value("js_current_min").toDouble());
    }
    if (mSettings.contains("js_current_max")) {
        ui->jsCurrentMaxBox->setValue(mSettings.value("js_current_max").toDouble());
    }
    if (mSettings.contains("js_erpm_min")) {
        ui->jsErpmMinBox->setValue(mSettings.value("js_erpm_min").toDouble());
    }
    if (mSettings.contains("js_erpm_max")) {
        ui->jsErpmMaxBox->setValue(mSettings.value("js_erpm_max").toDouble());
    }
    if (mSettings.contains("js_range_min")) {
        ui->jsMinBox->setValue(mSettings.value("js_range_min").toDouble());
    }
    if (mSettings.contains("js_range_max")) {
        ui->jsMaxBox->setValue(mSettings.value("js_range_max").toDouble());
    }

    if (mSettings.contains("js_name")) {
        ui->jsListBox->clear();
        auto gamepads = QGamepadManager::instance()->connectedGamepads();
        for (auto g: gamepads) {
            auto name = QGamepadManager::instance()->gamepadName(g);
            ui->jsListBox->addItem(name, g);
            if (name == mSettings.value("js_name").toString()) {
                if (mGamepad) {
                    mGamepad->deleteLater();
                }
                mGamepad = new QGamepad(g, this);
            }
        }
    }
#endif

    ui->uploadContentEditorButton->setChecked(mSettings.value("scripting/uploadContentEditor", true).toBool());
    ui->uploadContentFileButton->setChecked(!mSettings.value("scripting/uploadContentEditor", true).toBool());
    ui->reconnectCanBox->setChecked(mSettings.value("reconnectLastCan", true).toBool());

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

void Preferences::setUseGamepadControl(bool useControl)
{
#ifdef HAS_GAMEPAD
    if (ui->jsConfigOkBox->isChecked()) {
        if (mGamepad) {
            mUseGamepadControl = useControl;
        } else {
            mVesc->emitMessageDialog("Gamepad Control",
                                     "No recognized gamepad is connected.",
                                     false, false);
        }
    } else if (mVesc) {
        mVesc->emitMessageDialog("Gamepad Control",
                                 "Gamepad control is not configured. Go to Settings->Gamepad to configure it.",
                                 false, false);
    }
#else
    (void)useControl;
#endif
}

bool Preferences::isUsingGamepadControl()
{
#ifdef HAS_GAMEPAD
    return mUseGamepadControl;
#else
    return false;
#endif
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

void Preferences::timerSlot()
{
#ifdef HAS_GAMEPAD
    if (mGamepad) {
        ui->jsAxis1Bar->setValue(mGamepad->axisLeftX() * 1000.0);
        ui->jsAxis2Bar->setValue(mGamepad->axisLeftY() * 1000.0);
        ui->jsAxis3Bar->setValue(mGamepad->axisRightX() * 1000.0);
        ui->jsAxis4Bar->setValue(mGamepad->axisRightY() * 1000.0);

        double ax = 0.0;
        if (ui->jseAxisBox->currentIndex() == 0) {
            ax = mGamepad->axisLeftX() * 1000.0;
        } else if (ui->jseAxisBox->currentIndex() == 1) {
            ax = mGamepad->axisLeftY() * 1000.0;
        } else if (ui->jseAxisBox->currentIndex() == 2) {
            ax = mGamepad->axisRightX() * 1000.0;
        } else if (ui->jseAxisBox->currentIndex() == 3) {
            ax = mGamepad->axisRightY() * 1000.0;
        }

        if (ui->jsInvertedBox->isChecked()) {
            ax = -ax;
        }

        double input = Utility::map(ax,
                                    ui->jsMinBox->value(), ui->jsMaxBox->value(),
                                    ui->jsBidirectionalBox->isChecked() ? -1.0 : 0.0, 1.0);
        double range = 0.0;
        int decimals = 2;
        QString name = "Undefined";
        QString unit = "";
        int ctrlt = ui->jsControlTypeBox->currentIndex();
        if (ctrlt == 0 || ctrlt == 1) {
            range = input >= 0 ? fabs(ui->jsCurrentMaxBox->value()) : fabs(ui->jsCurrentMinBox->value());
            input *= range;
            name = "Current";
            unit = " A";

            if (mVesc && mUseGamepadControl) {
                if (ctrlt == 0 || input > 0) {
                    mVesc->commands()->setCurrent(input);
                } else {
                    mVesc->commands()->setCurrentBrake(input);
                }
            }
        } else if (ctrlt == 2) {
            range = 1.0;
            input *= range;
            name = "Duty";
            unit = "";

            if (mVesc && mUseGamepadControl) {
                mVesc->commands()->setDutyCycle(input);
            }
        } else if (ctrlt == 3) {
            range = input >= 0 ? fabs(ui->jsErpmMaxBox->value()) : fabs(ui->jsErpmMinBox->value());
            input *= range;
            name = "Speed";
            unit = " ERPM";
            decimals = 0;

            if (mVesc && mUseGamepadControl) {
                mVesc->commands()->setRpm(input);
            }
        } else if (ctrlt == 4) {
            range = 360.0;
            input *= range;
            name = "Position";
            unit = " Degrees";
            decimals = 1;

            if (mVesc && mUseGamepadControl) {
                mVesc->commands()->setPos(input);
            }
        }

        ui->jsDisp->setRange(range);
        ui->jsDisp->setUnit(unit);
        ui->jsDisp->setName(name);
        ui->jsDisp->setVal(input);
        ui->jsDisp->setDecimals(decimals);

        if (!mGamepad->isConnected()) {
            mGamepad->deleteLater();
            mGamepad = nullptr;
        }
    }
#endif
}

void Preferences::on_uiScaleBox_valueChanged(double arg1)
{
    mSettings.setValue("app_scale_factor", arg1);
}

void Preferences::on_uiPlotWidthBox_valueChanged(double arg1)
{
    mSettings.setValue("plot_line_width", arg1);
}

void Preferences::on_jsScanButton_clicked()
{
#ifdef HAS_GAMEPAD
    ui->jsListBox->clear();
    auto gamepads = QGamepadManager::instance()->connectedGamepads();
    for (auto g: gamepads) {
        ui->jsListBox->addItem(QGamepadManager::instance()->gamepadName(g), g);
    }
#endif
}

void Preferences::on_jsConnectButton_clicked()
{
#ifdef HAS_GAMEPAD
    QVariant item = ui->jsListBox->currentData();
    if (item.isValid()) {
        if (mGamepad) {
            mGamepad->deleteLater();
        }
        mGamepad = new QGamepad(item.toInt(), this);
    }
#endif
}

void Preferences::on_jsResetConfigButton_clicked()
{
#ifdef HAS_GAMEPAD
    if (mGamepad) {
        QGamepadManager::instance()->resetConfiguration(mGamepad->deviceId());
    }
#endif
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
#ifdef HAS_GAMEPAD
    mSettings.setValue("js_is_configured", ui->jsConfigOkBox->isChecked());
    mSettings.setValue("js_is_inverted", ui->jsInvertedBox->isChecked());
    mSettings.setValue("js_is_bidirectional", ui->jsBidirectionalBox->isChecked());
    mSettings.setValue("js_axis", ui->jseAxisBox->currentIndex());
    mSettings.setValue("js_control_type", ui->jsControlTypeBox->currentIndex());
    mSettings.setValue("js_current_min", ui->jsCurrentMinBox->value());
    mSettings.setValue("js_current_max", ui->jsCurrentMaxBox->value());
    mSettings.setValue("js_erpm_min", ui->jsErpmMinBox->value());
    mSettings.setValue("js_erpm_max", ui->jsErpmMaxBox->value());
    mSettings.setValue("js_range_min", ui->jsMinBox->value());
    mSettings.setValue("js_range_max", ui->jsMaxBox->value());
    if (mGamepad) {
        mSettings.setValue("js_name", mGamepad->name());
    }
#endif

    mLastScaling = mSettings.value("app_scale_factor", 1.0).toDouble();
    mLastIsDark = Utility::isDarkMode();
    mSettings.setValue("scripting/uploadContentEditor", ui->uploadContentEditorButton->isChecked());
    mSettings.setValue("reconnectLastCan", ui->reconnectCanBox->isChecked());

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

