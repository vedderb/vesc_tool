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

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QDialog>
#include <QSettings>
#include <QLabel>
#include <QListWidget>
#include <QTimer>
#include <QPushButton>
#include <QFileSelector>
#include <QMainWindow>

#include "vescinterface.h"
#include "widgets/paramtable.h"
#include "widgets/nrfpair.h"
#include "widgets/displaypercentage.h"
#include "pages/pageconnection.h"
#include "widgets/ppmmap.h"
#include "widgets/adcmap.h"
#include "widgets/aspectimglabel.h"

#ifdef HAS_GAMEPAD
#include <QtGamepad/QGamepad>
#endif

#include "vescinterface.h"

namespace Ui {
class Preferences;
}

class Preferences : public QDialog
{
    Q_OBJECT

public:
    explicit Preferences(QWidget *parent = nullptr);
    ~Preferences();

    VescInterface *vesc() const;
    void setVesc(VescInterface *vesc);
    void setUseGamepadControl(bool useControl);
    bool isUsingGamepadControl();

protected:
    void closeEvent(QCloseEvent *event);
    void showEvent(QShowEvent *event);

private slots:
    void timerSlot();
    void on_uiScaleBox_valueChanged(double arg1);
    void on_uiPlotWidthBox_valueChanged(double arg1);
    void on_jsScanButton_clicked();
    void on_jsConnectButton_clicked();
    void on_jsResetConfigButton_clicked();
    void on_loadQmlUiConnectBox_toggled(bool checked);
    void on_qmlUiAskBox_toggled(bool checked);
    void on_pathRtLogChooseButton_clicked();
    void on_pathScriptInputChooseButton_clicked();
    void on_pathRtLogEdit_textChanged(const QString &arg1);
    void on_pathScriptInputEdit_textChanged(const QString &arg1);
    void on_pathScriptOutputChooseButton_clicked();
    void on_pathScriptOutputEdit_textChanged(const QString &arg1);
    void on_pollRtDataBox_valueChanged(double arg1);
    void on_pollAppDataBox_valueChanged(double arg1);
    void on_pollImuDataBox_valueChanged(double arg1);
    void on_pollBmsDataBox_valueChanged(double arg1);
    void on_pollRestoreButton_clicked();
    void on_darkModeBox_toggled(bool checked);
    void on_okButton_clicked();
    void on_useImperialBox_toggled(bool checked);
    void on_showFwUpdateBox_toggled(bool checked);

private:
    Ui::Preferences *ui;

    VescInterface *mVesc;
    QSettings mSettings;
    QTimer *mTimer;

#ifdef HAS_GAMEPAD
    QGamepad *mGamepad;
    bool mUseGamepadControl;
#endif

    double mLastScaling;
    bool mLastIsDark;

    void saveSettingsChanged();

};

#endif // SETUPWIZARDAPP_H
