#include "boardsetupwindow.h"
#include "ui_boardsetupwindow.h"
#include <QMessageBox>
#include <QDebug>
#include <QThread>
#include <QFileDialog>
#include <QListWidgetItem>
#include <QScrollBar>
#include <cmath>
#include <QEventLoop>
#include <QDirIterator>
#include <QDesktopServices>
#include <QProgressDialog>
#include "parametereditor.h"
#include "startupwizard.h"
#include "widgets/helpdialog.h"
#include "utility.h"
#include "widgets/paramdialog.h"
#include "widgets/detectallfocdialog.h"

BoardSetupWindow::BoardSetupWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::BoardSetupWindow)
{
    ui->setupUi(this);


    mVersion = QString::number(VT_VERSION, 'f', 2);
    mVesc = new VescInterface(this);



    mStatusInfoTime = 0;
    mStatusLabel = new QLabel(this);
    ui->statusBar->addPermanentWidget(mStatusLabel);
    mTimer = new QTimer(this);
    mKeyLeft = false;
    mKeyRight = false;


    connect(mTimer, SIGNAL(timeout()),
            this, SLOT(timerSlot()));
    connect(mVesc, SIGNAL(statusMessage(QString,bool)),
            this, SLOT(showStatusInfo(QString,bool)));
    connect(mVesc, SIGNAL(messageDialog(QString,QString,bool,bool)),
            this, SLOT(showMessageDialog(QString,QString,bool,bool)));
    connect(mVesc, SIGNAL(serialPortNotWritable(QString)),
            this, SLOT(serialPortNotWritable(QString)));
    connect(mVesc->mcConfig(), SIGNAL(paramChangedDouble(QObject*,QString,double)),
            this, SLOT(paramChangedDouble(QObject*,QString,double)));
    connect(mVesc->commands(), SIGNAL(pingCanRx(QVector<int>,bool)),
            this, SLOT(pingCanRx(QVector<int>,bool)));
    connect(mVesc, SIGNAL(fwUploadStatus(QString,double,bool)),
            this, SLOT(fwUploadStatus(QString,double,bool)));

    mTimer->start(20);

    // Restore size and position
    if (mSettings.contains("boardsetupwindow/size")) {
        resize(mSettings.value("boardsetupwindow/size").toSize());
    }

    if (mSettings.contains("boardsetupwindow/position")) {
        move(mSettings.value("boardsetupwindow/position").toPoint());
    }

    if (mSettings.contains("boardsetupwindow/maximized")) {
        bool maximized = mSettings.value("boardsetupwindow/maximized").toBool();
        if (maximized) {
            showMaximized();
        }
    }

    mVesc->fwConfig()->loadParamsXml("://res/config/fw.xml");
    Utility::configLoadLatest(mVesc);

    mMcConfig_Target = new ConfigParams(this);
    mAppConfig_Target = new ConfigParams(this);
    QPair<int, int> latestSupported = Utility::configLatestSupported();
    QString FW_Ver =  QString::number(latestSupported.first) + "." + QStringLiteral("%1").arg(latestSupported.second, 2, 10, QLatin1Char('0'));
    mMcConfig_Target->loadParamsXml("://res/config/" + FW_Ver + "/parameters_mcconf.xml");
    mAppConfig_Target->loadParamsXml("://res/config/" + FW_Ver + "/parameters_appconf.xml");


    QDirIterator dir(QDir::currentPath(),QStringList() << "app_settings*.xml", QDir::NoFilter ,QDirIterator::Subdirectories);
    if(dir.hasNext()){
        loadAppConfXML(dir.next());
    }
    QDirIterator dir2(QDir::currentPath(),QStringList() << "motor_settings*.xml", QDir::NoFilter ,QDirIterator::Subdirectories);
    if(dir2.hasNext()){
        loadMotorConfXML(dir2.next());
    }
    on_serialRefreshButton_clicked();
}

BoardSetupWindow::~BoardSetupWindow()
{

    // Save settings
    mSettings.setValue("version", mVersion);
    mSettings.setValue("introVersion", VT_INTRO_VERSION);
    mSettings.setValue("boardsetupwindow/position", pos());
    mSettings.setValue("boardsetupwindow/maximized", isMaximized());

    if (!isMaximized()) {
        mSettings.setValue("boardsetupwindow/size", size());
    }

    delete ui;
}

void BoardSetupWindow::timerSlot()
{
    // Update status label
    if (mStatusInfoTime) {
        mStatusInfoTime--;
        if (!mStatusInfoTime) {
            mStatusLabel->setStyleSheet(qApp->styleSheet());
        }
    } else {
        QString str = mVesc->getConnectedPortName();
        if (str != mStatusLabel->text()) {
            mStatusLabel->setText(mVesc->getConnectedPortName());
            static QString statusLast = "";
            if (str != statusLast) {
                // mPageDebugPrint->printConsole("Status: " + str + "<br>");
                statusLast = str;
            }
        }
    }


    // RT data
    bool getRtData = false;
    bool sendAlive = false;
    if ( getRtData) {
        mVesc->commands()->getValues();
        if (mVesc->isRtLogOpen()) {
            mVesc->commands()->getValuesSetup();
        }
    }

    // APP RT data
    if (getRtData) {
        mVesc->commands()->getDecodedAdc();
        mVesc->commands()->getDecodedChuk();
        mVesc->commands()->getDecodedPpm();
        mVesc->commands()->getDecodedBalance();
    }

    // IMU Data
    if (getRtData) {
        mVesc->commands()->getImuData(0xFFFF);
    }

    // Send alive command once every 10 iterations
    if (sendAlive) {
        static int alive_cnt = 0;
        alive_cnt++;
        if (alive_cnt >= 10) {
            alive_cnt = 0;
            mVesc->commands()->sendAlive();
        }
    }

    // Read configurations if they haven't been read since starting VESC Tool
    if (mVesc->isPortConnected()) {
        static int conf_cnt = 0;
        // disconected_cnt = 0;
        conf_cnt++;
        if (conf_cnt >= 20) {
            conf_cnt = 0;

            if (!mVesc->deserializeFailedSinceConnected() && mVesc->fwRx()) {
                if (!mMcConfRead) {
                    mVesc->commands()->getMcconf();
                }

                if (!mAppConfRead) {
                    mVesc->commands()->getAppConf();
                }
            }
        }
    }

    // Disable all data streaming when uploading firmware
    if (mVesc->getFwUploadProgress() > 0.1) {
        //       ui->actionSendAlive->setChecked(false);
        //        ui->actionRtData->setChecked(false);
        //        ui->actionRtDataApp->setChecked(false);
        //        ui->actionIMU->setChecked(false);
        //       ui->actionKeyboardControl->setChecked(false);
    }


    // Run startup checks
    static bool has_run_start_checks = false;
    if (!has_run_start_checks) {

        // put code when opening tool here

        has_run_start_checks = true;
        Utility::checkVersion(mVesc);
    }
}

void BoardSetupWindow::on_startButton_clicked()
{
    ui->startButton->setEnabled(false);
    bool res;
    res = trySerialConnect();
    if(!res){
        resetRoutine();
        return;
    }
    res = tryCANScan();
    if(!res){
        return;
    }
    if(ui->bootloaderCheckBox->isChecked()){
        res = tryBootloaderUpload();
        if(!res){
            resetRoutine();
            return;
        }
    }
    if(ui->firmwareCheckBox->isChecked()){
        res = tryFirmwareUpload();
        if(!res){
            resetRoutine();
            return;
        }
    }
    res = tryFOCCalibration();
    if(!res){ showMessageDialog(tr("FOC Calibration"),
                                tr("FOC Routine failed. Check Motor Connections."),
                                false, false);
        resetRoutine();
        return;
    }
    res = tryTestMotorParameters();
    if(!res){ showMessageDialog(tr("Motor Parameters"),
                                tr("Your motor parameters are outside of expected bounds. Ensure you have selected the correct configuration file for your board."),
                                false, false);
        resetRoutine();
        return;
    }
    res = tryMotorDirection();
    if(!res){ showMessageDialog(tr("Motor Direction"),
                                tr("Please select the motor directions within the time window"),
                                false, false);
        resetRoutine();
        return;
    }
    res = tryApplySlaveAppSettings();
    if(!res){ showMessageDialog(tr("Slave App Settings"),
                                tr("Please ensure that all CAN Bus connections are solid. The app settings write to slave failed."),
                                false, false);
        resetRoutine();
        return;
    }
    res = tryApplyMasterAppSettings();
    if(!res){
        resetRoutine();
        return;
    }
    res = tryRemoteTest();
    if(!res){ showMessageDialog(tr("Master App Settings"),
                                tr("The Master App settings were unable to be applied. There may be a problem with the firmware or connections."),
                                false, false);
        resetRoutine();
        return;
    }
    res = tryFinalDiagnostics();
    if(!res){ showMessageDialog(tr("Failed Final Diagnostics"),
                                tr("Applied load caused issues with your setup, further investigation needed."),
                                false, false);
        resetRoutine();
        return;
    }
    testResultMsg = "Setup completed succesfully. Your board should now be ready to ride.";
    testResult = true;
    resetRoutine();
}

void BoardSetupWindow::resetRoutine(){
    showMessageDialog(tr("Test Results"),
                      tr(testResultMsg.toUtf8()),
                      testResult, false);
    ui->startButton->setEnabled(true);

}

bool BoardSetupWindow::trySerialConnect(){
    bool res;
    res = mVesc->connectSerial(ui->serialPortBox->currentData().toString(), 115200);
    if(res){
        Utility::waitSignal(mVesc, SIGNAL(fwRxChanged(bool, bool)), 5000);
    }
    if(mVesc->isPortConnected()){
        ui->usbConnectLabel->setStyleSheet("QLabel { background-color : lightGreen; color : black; }");
    }else{
        ui->usbConnectLabel->setStyleSheet("QLabel { background-color : red; color : black; }");
        testResultMsg = "Could not connect USB. Make sure the unit is powered on, connected to the computer, and the correct Serial port is selected.";
        testResult = false;
    }
    return mVesc->isPortConnected();
}

bool BoardSetupWindow::tryCANScan(){
    bool res = false;
    if(mVesc->isPortConnected()){
        mVesc->commands()->pingCan();
        Utility::waitSignal(mVesc->commands(), SIGNAL(pingCanRx(QVector<int>, bool)), 10000);
    }
    if(CAN_Timeout){
        res = false;
        ui->CANScanLabel->setStyleSheet("QLabel { background-color : lightGreen; color : black; }");
        testResultMsg = "Could not connect USB. Make sure the unit is powered on, connected to the computer, and the correct Serial port is selected.";
        testResult = false;
    }else{
        res = true;
        int this_ID = mVesc->appConfig()->getParamInt("controller_id");
        QString canTxt("CAN IDs: " + QString::number(this_ID));
        for(int i = 0; i < CAN_IDs.size(); i++){
            canTxt = canTxt + ", " + QString::number(CAN_IDs.at(i));
        }
        canTxt = canTxt + " (" + QString::number(CAN_IDs.size() + 1) + " motors total)";
        ui->CANScanLabel->setText(canTxt);
        ui->CANScanLabel->setStyleSheet("QLabel { background-color : lightGreen; color : black; }");
    }

    return res;
}

bool BoardSetupWindow::tryBootloaderUpload(){
    QFile file;
    file.setFileName("://res/bootloaders/generic.bin");
    is_Bootloader = true;
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this,
                              tr("Upload Error"),
                              tr("Could not open file. Make sure that the path is valid."));
        testResultMsg = "Something is wrong with the included bootloader file in this software.";
        testResult = false;
        return false;
    }
    if (file.size() > 400000) {
        QMessageBox::critical(this,
                              tr("Upload Error"),
                              tr("The selected file is too large to be a firmware."));
        testResultMsg = "The included bootloader file is too large.";
        testResult = false;
        return false;
    }
    QByteArray data = file.readAll();

    bool fwRes = mVesc->fwUpload(data, is_Bootloader, true);
    if(fwRes){
        ui->bootloaderLabel->setStyleSheet("QLabel { background-color : lightGreen; color : black; }");
    }else{
        ui->bootloaderLabel->setStyleSheet("QLabel { background-color : red; color : black; }");
        testResultMsg = "The bootloader upload timed out. Please try the routine again, you may need to update the firmware first seperatley from the VESC tool.";
        testResult = false;
    }
    return fwRes;
}

bool BoardSetupWindow::tryFirmwareUpload(){
    QString HW_Name = mVesc->getFirmwareNow().split("Hw: ").last();
    QString FW_Path;
    HW_Name = HW_Name.split("\n").first();
    int num_VESCs;
    if(HW_Name.contains("STORMCORE")|| HW_Name.contains("UNITY")){
        num_VESCs = (CAN_IDs.size() + 1)/2;
    }else{
        num_VESCs = CAN_IDs.size() + 1;
    }

    QDirIterator it("://res/firmwares");

    while (it.hasNext()) {
        QFileInfo fi(it.next());
        QStringList names = fi.fileName().split("_o_");
        if (fi.isDir() && (HW_Name.isEmpty() || names.contains(HW_Name, Qt::CaseInsensitive))) {
            FW_Path = fi.absoluteFilePath() +  "/VESC_default.bin";
        }
    }
    bool fwRes = false;
    is_Bootloader = false;
    QFile file;
    if(!FW_Path.isEmpty()){
        file.setFileName(FW_Path);
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::critical(this,
                                  tr("Upload Error"),
                                  tr("Could not open file. Make sure that the path is valid."));
            testResultMsg = "Something is wrong with the included firmware file for your hardware in this software.";
            testResult = false;
            return false;
        }
        if (file.size() > 400000) {
            QMessageBox::critical(this,
                                  tr("Upload Error"),
                                  tr("The selected file is too large to be a firmware."));
            testResultMsg = "The included firmware file is too large.";
            testResult = false;
            return false;
        }
        QByteArray data = file.readAll();

        fwRes = mVesc->fwUpload(data, is_Bootloader, num_VESCs > 1);
        if(!fwRes){
            ui->firmwareLabel->setStyleSheet("QLabel { background-color : red; color : black; }");
            ui->bootloaderLabel->setStyleSheet("QLabel { background-color : red; color : black; }");
            testResultMsg = "The firmware upload timed out. Please try the routine again, you may need to update the firmware first seperatley from the VESC tool.";
            return false;
        }
    }else{
        // Didn't find any supported default firmwares
        ui->bootloaderLabel->setStyleSheet("QLabel { background-color : red; color : black; }");
        testResultMsg = "The firmware file path is non-existent.";
        return false;
    }
    for(int j = 12; j>-1; j--){
        ui->firmwareLabel->setText("Waiting for Reboot: " + QString::number(j)  + " s");
        Utility::sleepWithEventLoop(1000);
    }
    bool reconnected = trySerialConnect();
    if(reconnected){
        ui->firmwareLabel->setText("Firmware Uploaded Succesfully and Reconnected");
        ui->firmwareLabel->setStyleSheet("QLabel { background-color : lightGreen; color : black; }");
    }else{
        ui->firmwareLabel->setText("Unit did not reconnect after firmware upload.");
        ui->firmwareLabel->setStyleSheet("QLabel { background-color : red; color : black; }");
        testResult = false;
    }
    return reconnected;
}


bool BoardSetupWindow::tryFOCCalibration(){
    double max_loss = pow(mMcConfig_Target->getParamDouble("l_current_max"),2.0) * mMcConfig_Target->getParamDouble("foc_motor_r");
    mVesc->mcConfig();
    QString res = Utility::detectAllFoc(mVesc, true,
                                        max_loss,
                                        mMcConfig_Target->getParamDouble("l_in_current_min"),
                                        mMcConfig_Target->getParamDouble("l_in_current_max"),
                                        mMcConfig_Target->getParamDouble("foc_openloop_rpm"),
                                        mMcConfig_Target->getParamDouble("foc_sl_erpm"));
    return true;
}

bool BoardSetupWindow::tryTestMotorParameters(){
    return true;
}

bool BoardSetupWindow::tryMotorDirection(){
    return true;
}

bool BoardSetupWindow::tryApplySlaveAppSettings(){
    return true;
}

bool BoardSetupWindow::tryApplyMasterAppSettings(){

    //"The Master App settings were unable to be applied. There may be a problem with the firmware or connections."
    return true;
}

bool BoardSetupWindow::tryRemoteTest(){
    return true;
}

bool BoardSetupWindow::tryFinalDiagnostics(){
    return true;
}






void BoardSetupWindow::mcConfigCheckResult(QStringList paramsNotSet)
{
    if (!paramsNotSet.isEmpty()) {
        ParamDialog::showParams(tr("Parameters truncated"),
                                tr("The following parameters were truncated because they were set outside "
                                   "of their allowed limits."),
                                mVesc->mcConfig(),
                                paramsNotSet,
                                this);
    }
}



void BoardSetupWindow::on_motorConfigButton_clicked()
{
    QString path;
    path = QFileDialog::getOpenFileName(this,
                                        tr("Choose motor configuration file to load"),
                                        ".",
                                        tr("Xml files (*.xml)"));

    loadMotorConfXML(path);


}

void BoardSetupWindow::on_appConfigButton_clicked()
{
    QString path;
    path = QFileDialog::getOpenFileName(this,
                                        tr("Choose app configuration file to load"),
                                        ".",
                                        tr("Xml files (*.xml)"));
    loadAppConfXML(path);
}

void BoardSetupWindow::on_bleFirmwareButton_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Choose Firmware File"), ".",
                                                    tr("Binary files (*.bin)"));

    if (filename.isNull()) {
        return;
    }

    ui->bleFirmwareEdit->setText(filename);

}

void BoardSetupWindow::on_serialRefreshButton_clicked()
{
    if (mVesc) {
        ui->serialPortBox->clear();
        QList<VSerialInfo_t> ports = mVesc->listSerialPorts();
        foreach(const VSerialInfo_t &port, ports) {
            ui->serialPortBox->addItem(port.name, port.systemPath);
        }
        ui->serialPortBox->setCurrentIndex(0);
    }

}
void BoardSetupWindow::on_firmwareCheckBox_stateChanged(){
    ui->firmwareLabel->setEnabled(ui->firmwareCheckBox->isChecked());
}


void BoardSetupWindow::showStatusInfo(QString info, bool isGood)
{
    if (isGood) {
        mStatusLabel->setStyleSheet("QLabel { background-color : lightgreen; color : black; }");
        // mPageDebugPrint->printConsole("Status: " + info + "<br>");
    } else {
        mStatusLabel->setStyleSheet("QLabel { background-color : red; color : black; }");
        // mPageDebugPrint->printConsole("<font color=\"red\">Status: " + info + "</font><br>");
    }

    mStatusInfoTime = 80;
    mStatusLabel->setText(info);
}


void BoardSetupWindow::showMessageDialog(const QString &title, const QString &msg, bool isGood, bool richText)
{
    (void)richText;

    if (isGood) {
        QMessageBox::information(this, title, msg);
    } else {
        QMessageBox::warning(this, title, msg);
    }
}

void BoardSetupWindow::loadAppConfXML(QString path){
    if (path.isNull()) {
        return;
    }

    bool res = mAppConfig_Target->loadXml(path, "APPConfiguration");

    if (res) {
        ui->appConfigEdit->setText(path);
        showStatusInfo("Loaded app configuration", true);
        QString str;
        ui->appTab->clearParams();
        ui->appTab->addParamRow(mAppConfig_Target, "app_to_use");
        //ui->appTab->setEnabled(false);



        switch(mAppConfig_Target->getParamEnum("app_to_use")){
        case 4: //ppm and uart
            ui->appTab->addParamRow(mAppConfig_Target, "app_uart_baudrate");
            [[clang::fallthrough]];
        case 1: //ppm
            ui->appTab->addParamRow(mAppConfig_Target, "app_ppm_conf.ctrl_type");
            ui->appTab->addParamRow(mAppConfig_Target, "app_ppm_conf.pulse_start");
            ui->appTab->addParamRow(mAppConfig_Target, "app_ppm_conf.pulse_center");
            ui->appTab->addParamRow(mAppConfig_Target, "app_ppm_conf.hyst");
            ui->appTab->addParamRow(mAppConfig_Target, "app_ppm_conf.ramp_time_pos");
            ui->appTab->addParamRow(mAppConfig_Target, "app_ppm_conf.ramp_time_neg");
            if(mAppConfig_Target->getParamBool("app_ppm_conf.tc")){
                ui->appTab->addParamRow(mAppConfig_Target, "app_ppm_conf.tc");
                ui->appTab->addParamRow(mAppConfig_Target, "app_ppm_conf.tc_max_diff");
            }
            if(mAppConfig_Target->getParamEnum("app_ppm_conf.ctrl_type") == 9){
                ui->appTab->addParamRow(mAppConfig_Target, "app_ppm_conf.smart_rev_max_duty");
                ui->appTab->addParamRow(mAppConfig_Target, "app_ppm_conf.smart_rev_ramp_time");
            }
            break;
        case 2: //adc
            break;
        case 3: //uart
            ui->appTab->addParamRow(mAppConfig_Target, "app_uart_baudrate");
            ui->appTab->addParamRow(mAppConfig_Target, "app_chuk_conf.ctrl_type");
            ui->appTab->addParamRow(mAppConfig_Target, "app_chuk_conf.hyst");
            ui->appTab->addParamRow(mAppConfig_Target, "app_chuk_conf.ramp_time_pos");
            ui->appTab->addParamRow(mAppConfig_Target, "app_chuk_conf.ramp_time_neg");
            if(mAppConfig_Target->getParamBool("app_chuk_conf.tc")){
                ui->appTab->addParamRow(mAppConfig_Target, "app_chuk_conf.tc");
                ui->appTab->addParamRow(mAppConfig_Target, "app_chuk_conf.tc_max_diff");
            }
            if(mAppConfig_Target->getParamBool("app_chuk_conf.use_smart_rev")){
                ui->appTab->addParamRow(mAppConfig_Target, "app_chuk_conf.smart_rev_max_duty");
                ui->appTab->addParamRow(mAppConfig_Target, "app_chuk_conf.smart_rev_ramp_time");
            }
            break;
        default:
            str.sprintf("App Config Values:\n"
                        "Unsupported app selected");
            break;
        }
        foreach(QObject *p, ui->appTab->children().at(0)->children()){
            p->setProperty("enabled",false);
        }
        //ui->appTab->children().at(0)->children();
        //  setEnabled(true);
        //ui->appTab->verticalScrollBar()->setEnabled(true);




    } else {
        showMessageDialog(tr("Load app configuration"),
                          tr("Could not load app configuration:<BR>"
                             "%1").arg(mAppConfig_Target->xmlStatus()),
                          false, false);
    }
}
void BoardSetupWindow::loadMotorConfXML(QString path){

    if (path.isNull()) {
        return;
    }



    bool res = mMcConfig_Target->loadXml(path, "MCConfiguration");

    if (res) {


        showStatusInfo("Loaded motor configuration", true);
        QString str;
        ui->motorTab->clearParams();
        ui->motorTab->addParamRow(mMcConfig_Target, "foc_motor_r");
        ui->motorTab->addParamRow(mMcConfig_Target, "foc_motor_l");
        ui->motorTab->addParamRow(mMcConfig_Target, "foc_motor_flux_linkage");
        ui->motorTab->addParamRow(mMcConfig_Target, "foc_sensor_mode");
        ui->motorTab->addParamRow(mMcConfig_Target, "l_current_max");
        ui->motorTab->addParamRow(mMcConfig_Target, "l_current_min");
        ui->motorTab->addParamRow(mMcConfig_Target, "l_in_current_max");
        ui->motorTab->addParamRow(mMcConfig_Target, "l_in_current_min");
        ui->motorTab->addParamRow(mMcConfig_Target, "l_battery_cut_start");
        ui->motorTab->addParamRow(mMcConfig_Target, "l_battery_cut_end");
        ui->motorConfigEdit->setText(path);

        foreach(QObject *p, ui->motorTab->children().at(0)->children()){
            p->setProperty("enabled",false);
        }

    } else {
        showMessageDialog(tr("Load motor configuration"),
                          tr("Could not load motor configuration:<BR>"
                             "%1").arg(mVesc->mcConfig()->xmlStatus()),
                          false, false);
    }

}

void BoardSetupWindow::pingCanRx(QVector<int> devs, bool isTimeout)
{
    CAN_Timeout = isTimeout;
    if(!isTimeout){
        CAN_IDs = devs;
    }
}

void BoardSetupWindow::fwUploadStatus(const QString &status, double progress, bool isOngoing)
{
    if(is_Bootloader){
        if (isOngoing) {
            ui->bootloaderLabel->setText(tr("%1 (%2 %)").
                                         arg(status).
                                         arg(progress * 100, 0, 'f', 1));
        } else {
            ui->bootloaderLabel->setText("Bootloader " + status);
        }
    }else{
        if (isOngoing) {
            ui->firmwareLabel->setText(tr("%1 (%2 %)").
                                       arg(status).
                                       arg(progress * 100, 0, 'f', 1));
        } else {
            ui->firmwareLabel->setText("Firmware " + status);
        }
    }
}

