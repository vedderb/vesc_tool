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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QTimer>
#include <QProcess>
#include <QSettings>
#include "vescinterface.h"
#include "widgets/pagelistitem.h"

#include "pages/pagewelcome.h"
#include "pages/pageconnection.h"
#include "pages/pagedataanalysis.h"
#include "pages/pagertdata.h"
#include "pages/pagesampleddata.h"
#include "pages/pageterminal.h"
#include "pages/pagefirmware.h"
#include "pages/pagedebugprint.h"
#include "pages/pagemotorsettings.h"
#include "pages/pagemotor.h"
#include "pages/pagebldc.h"
#include "pages/pagedc.h"
#include "pages/pagefoc.h"
#include "pages/pagecontrollers.h"
#include "pages/pagemotorinfo.h"
#include "pages/pageappsettings.h"
#include "pages/pageappgeneral.h"
#include "pages/pageappppm.h"
#include "pages/pageappadc.h"
#include "pages/pageappuart.h"
#include "pages/pageappnunchuk.h"
#include "pages/pageappnrf.h"
#include "pages/pageappbalance.h"
#include "pages/pagesettings.h"
#include "pages/pagegpd.h"
#include "pages/pageexperiments.h"
#include "pages/pageimu.h"
#include "pages/pageswdprog.h"
#include "pages/pageappimu.h"
#include "pages/pageloganalysis.h"
#include "pages/pagecananalyzer.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    bool eventFilter(QObject *object, QEvent *e);

private slots:
    void timerSlot();
    void showStatusInfo(QString info, bool isGood);
    void showMessageDialog(const QString &title, const QString &msg, bool isGood, bool richText);
    void serialPortNotWritable(const QString &port);
    void valuesReceived(MC_VALUES values, unsigned int mask);
    void paramChangedDouble(QObject *src, QString name, double newParam);
    void mcConfigCheckResult(QStringList paramsNotSet);

    void on_actionReconnect_triggered();
    void on_actionDisconnect_triggered();
    void on_actionReboot_triggered();
    void on_stopButton_clicked();
    void on_fullBrakeButton_clicked();
    void on_actionReadMcconf_triggered();
    void on_actionReadMcconfDefault_triggered();
    void on_actionWriteMcconf_triggered();
    void on_actionReadAppconf_triggered();
    void on_actionReadAppconfDefault_triggered();
    void on_actionWriteAppconf_triggered();
    void on_actionSaveMotorConfXml_triggered();
    void on_actionLoadMotorConfXml_triggered();
    void on_actionSaveAppconfXml_triggered();
    void on_actionLoadAppconfXml_triggered();
    void on_actionExit_triggered();
    void on_actionAbout_triggered();
    void on_actionLibrariesUsed_triggered();
    void on_dutyButton_clicked();
    void on_currentButton_clicked();
    void on_speedButton_clicked();
    void on_posButton_clicked();
    void on_brakeCurrentButton_clicked();
    void on_handbrakeButton_clicked();
    void on_pageList_currentRowChanged(int currentRow);
    void on_actionParameterEditorMcconf_triggered();
    void on_actionParameterEditorAppconf_triggered();
    void on_actionParameterEditorInfo_triggered();
    void on_actionSaveMotorConfigurationHeader_triggered();
    void on_actionSaveAppConfigurationHeader_triggered();
    void on_actionSaveMotorConfigurationHeaderWrap_triggered();
    void on_actionSaveAppConfigurationHeaderWrap_triggered();
    void on_actionTerminalPrintFaults_triggered();
    void on_actionTerminalShowHelp_triggered();
    void on_actionTerminalClear_triggered();
    void on_actionTerminalPrintThreads_triggered();
    void on_actionTerminalDRV8301ResetLatchedFaults_triggered();
    void on_actionCanFwd_toggled(bool arg1);
    void on_actionSafetyInformation_triggered();
    void on_actionWarrantyStatement_triggered();
    void on_actionVESCToolChangelog_triggered();
    void on_actionFirmwareChangelog_triggered();
    void on_actionVESCProjectForums_triggered();
    void on_actionLicense_triggered();
    void on_posBox_editingFinished();
    void on_posBox_valueChanged(double arg1);
    void on_actionExportConfigurationParser_triggered();
    void on_actionBackupConfiguration_triggered();
    void on_actionRestoreConfiguration_triggered();
    void on_actionClearConfigurationBackups_triggered();
    void on_actionParameterEditorFW_triggered();
    void on_actionBackupConfigurationsCAN_triggered();
    void on_actionRestoreConfigurationsCAN_triggered();

private:
    Ui::MainWindow *ui;

    QSettings mSettings;
    QString mVersion;
    VescInterface *mVesc;
    QTimer *mTimer;
    QLabel *mStatusLabel;
    int mStatusInfoTime;
    bool mKeyLeft;
    bool mKeyRight;
    bool mMcConfRead;
    bool mAppConfRead;

    PageWelcome *mPageWelcome;
    PageConnection *mPageConnection;
    PageDataAnalysis *mPageDataAnalysis;
    PageRtData *mPageRtData;
    PageSampledData *mPageSampledData;
    PageImu *mPageImu;
    PageTerminal *mPageTerminal;
    PageFirmware *mPageFirmware;
    PageDebugPrint *mPageDebugPrint;
    PageMotorSettings *mPageMotorSettings;
    PageMotor *mPageMotor;
    PageBldc *mPageBldc;
    PageDc *mPageDc;
    PageFoc *mPageFoc;
    PageGPD *mPageGpd;
    PageControllers *mPageControllers;
    PageMotorInfo *mPageMotorInfo;
    PageExperiments *mPageExperiments;
    PageAppSettings *mPageAppSettings;
    PageAppGeneral *mPageAppGeneral;
    PageAppPpm *mPageAppPpm;
    PageAppAdc *mPageAppAdc;
    PageAppUart *mPageAppUart;
    PageAppNunchuk *mPageAppNunchuk;
    PageAppNrf *mPageAppNrf;
    PageAppBalance *mPageAppBalance;
    PageSettings *mPageSettings;
    PageSwdProg *mPageSwdProg;
    PageAppImu *mPageAppImu;
    PageLogAnalysis *mPageLogAnalysis;
    PageCanAnalyzer *mPageCanAnalyzer;

    void addPageItem(QString name,
                     QString icon = "",
                     QString groupIcon = "",
                     bool bold = false,
                     bool indented = false);
    void saveParamFileDialog(QString conf, bool wrapIfdef);
    void showPage(const QString &name);
    void reloadPages();
    void checkUdev();
    bool waitProcess(QProcess &process, bool block = true, int timeoutMs = 300000);
    QString runCmd(QString cmd, QStringList args);

};

#endif // MAINWINDOW_H
