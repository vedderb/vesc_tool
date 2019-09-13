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

#include "setupwizardapp.h"
#include <QVBoxLayout>
#include <QMessageBox>
#include <QProgressDialog>
#include "utility.h"

SetupWizardApp::SetupWizardApp(VescInterface *vesc, QWidget *parent)
    : QWizard(parent)
{
    mVesc = vesc;
    mCanLastFwd = mVesc->commands()->getSendCan();
    mCanLastId = mVesc->commands()->getCanSendId();

    setPage(Page_Intro, new AppIntroPage(vesc));
    setPage(Page_Connection, new AppConnectionPage(vesc));
    setPage(Page_Firmware, new AppFirmwarePage(vesc));
    setPage(Page_Multi, new AppMultiPage(vesc));
    setPage(Page_General, new AppGeneralPage(vesc));
    setPage(Page_Nunchuk, new AppNunchukPage(vesc));
    setPage(Page_Ppm_Map, new AppPpmMapPage(vesc));
    setPage(Page_Ppm, new AppPpmPage(vesc));
    setPage(Page_Adc_Map, new AppAdcMapPage(vesc));
    setPage(Page_Adc, new AppAdcPage(vesc));
    setPage(Page_Conclusion, new AppConclusionPage(vesc));

    setStartId(Page_Intro);
    setWizardStyle(ModernStyle);
    setPixmap(QWizard::LogoPixmap, QPixmap("://res/icon.png").
              scaled(40, 40,
                     Qt::KeepAspectRatio,
                     Qt::SmoothTransformation));
    resize(800, 450);

    setWindowTitle(tr("App Setup Wizard"));

    mSideLabel = new AspectImgLabel(Qt::Vertical);
    mSideLabel->setPixmap(QPixmap("://res/logo_wizard.png"));
    mSideLabel->setScaledContents(true);
    setSideWidget(mSideLabel);

    connect(this, SIGNAL(currentIdChanged(int)),
            this, SLOT(idChanged(int)));
    connect(this, SIGNAL(rejected()), this, SLOT(ended()));
    connect(this, SIGNAL(accepted()), this, SLOT(ended()));
}

void SetupWizardApp::idChanged(int id)
{
    if (id == Page_Intro || id == Page_Conclusion) {
        setSideWidget(mSideLabel);
        mSideLabel->setVisible(true);
    } else {
        setSideWidget(nullptr);
    }
}

void SetupWizardApp::ended()
{
    mVesc->commands()->setSendCan(mCanLastFwd, mCanLastId);
    mVesc->commands()->getAppConf();
}

AppIntroPage::AppIntroPage(VescInterface *vesc, QWidget *parent)
    : QWizardPage(parent)
{
    mVesc = vesc;
    setTitle(tr("VESC® Input Setup Wizard"));

    mLabel = new QLabel(tr("This wizard will help you choose what type of input to use "
                           "for your VESC®, and set up the apps according to your input."
                           "<br><br>"
                           "To get more information about the parameters and tools in the "
                           "wizard, click on the questionmark next to them."));

    mLabel->setWordWrap(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(mLabel);
    setLayout(layout);
}

int AppIntroPage::nextId() const
{
    if (mVesc->isPortConnected()) {
        if (mVesc->commands()->isLimitedMode() || !mResetInputOk) {
            return SetupWizardApp::Page_Firmware;
        } else {
            if (mVesc->getCanDevsLast().size() == 0) {
                return SetupWizardApp::Page_General;
            } else {
                return SetupWizardApp::Page_Multi;
            }
        }
    } else {
        return SetupWizardApp::Page_Connection;
    }
}

bool AppIntroPage::validatePage()
{
    bool res = false;

    if (!mVesc->isPortConnected()) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::information(this,
                                         tr("Connection"),
                                         tr("You are not connected to the VESC. Would you like to try to "
                                            "automatically connect?<br><br>"
                                            ""
                                            "<i>Notice that the USB cable must be plugged in and that the VESC "
                                            "must be powered for the connection to work.</i>"),
                                         QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

        if (reply == QMessageBox::Yes) {
            Utility::autoconnectBlockingWithProgress(mVesc, this);
            res = true;
        }
    } else {
        mVesc->commands()->getAppConf();

        QProgressDialog dialog("Scanning CAN-Bus...", QString(), 0, 0, this);
        dialog.setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
        dialog.setWindowModality(Qt::WindowModal);
        dialog.setAttribute(Qt::WA_DeleteOnClose);
        dialog.show();

        setEnabled(false);
        auto devs = mVesc->scanCan();
        mResetInputOk = Utility::resetInputCan(mVesc, devs);
        setEnabled(true);
    }

    return !res;
}

AppConnectionPage::AppConnectionPage(VescInterface *vesc, QWidget *parent)
    : QWizardPage(parent)
{
    mVesc = vesc;

    setTitle(tr("Connect VESC"));
    setSubTitle(tr("The VESC has to be connected in order to use this "
                   "wizard. Please connect using one of the available "
                   "interfaces."));

    mPageConnection = new PageConnection;
    mPageConnection->setVesc(mVesc);

    connect(mVesc, SIGNAL(fwRxChanged(bool,bool)),
            this, SIGNAL(completeChanged()));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(mPageConnection);
    setLayout(layout);
}

int AppConnectionPage::nextId() const
{
    if (mVesc->commands()->isLimitedMode()) {
        return SetupWizardApp::Page_Firmware;
    } else {
        return SetupWizardApp::Page_Multi;
    }
}

bool AppConnectionPage::isComplete() const
{
    return mVesc->fwRx();
}

AppFirmwarePage::AppFirmwarePage(VescInterface *vesc, QWidget *parent)
    : QWizardPage(parent)
{
    mVesc = vesc;

    setTitle(tr("Update Firmware"));
    setSubTitle(tr("You need to update the firmware on the VESC in order "
                   "to use it with this version of VESC Tool."));

    mLabel = new QLabel(tr("Your VESC (or one of the VESCs on the CAN-bus) has old firmware, "
                           "and needs to be updated. After that, "
                           "the motor configuration has to be done again."));

    mLabel->setWordWrap(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(mLabel);
    setLayout(layout);
}

int AppFirmwarePage::nextId() const
{
    return -1;
}

AppMultiPage::AppMultiPage(VescInterface *vesc, QWidget *parent)
    : QWizardPage(parent)
{
    mVesc = vesc;

    setTitle(tr("Multiple VESCs"));
    setSubTitle(tr("Found multiple VESCs on the CAN-bus, choose "
                   "which one the input is connected to."));

    mCanFwdList = new QListWidget;
    mCanFwdList->setIconSize(QSize(50, 50));
    mCanFwdList->setWordWrap(true);
    mCanFwdList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    registerField("CanFwd", this, "canFwd", SIGNAL(canFwdChanged));
    connect(mCanFwdList, SIGNAL(currentRowChanged(int)),
            this, SIGNAL(canFwdChanged()));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(mCanFwdList);
    setLayout(layout);
}

void AppMultiPage::initializePage()
{
    mCanFwdList->clear();

    QListWidgetItem *item = new QListWidgetItem;
    item->setText(tr("This VESC (ID: %1)").
                  arg(mVesc->appConfig()->getParamInt("controller_id")));
    item->setIcon(QIcon("://res/icons/Connected-96.png"));
    item->setData(Qt::UserRole, -1);
    mCanFwdList->addItem(item);

    for (int dev: mVesc->getCanDevsLast()) {
        item = new QListWidgetItem;
        item->setText(tr("VESC with ID: %1").arg(dev));
        item->setIcon(QIcon("://res/icons/can_off.png"));
        item->setData(Qt::UserRole, dev);
        mCanFwdList->addItem(item);
    }

    mCanFwdList->setCurrentItem(nullptr);
}

int AppMultiPage::nextId() const
{
    return SetupWizardApp::Page_General;
}

bool AppMultiPage::validatePage()
{
    if (field("CanFwd").toInt() >= 0) {
        mVesc->commands()->setSendCan(true, field("CanFwd").toInt());
    } else {
        mVesc->commands()->setSendCan(false);
    }

    mVesc->commands()->getAppConf();
    Utility::waitSignal(mVesc->appConfig(), SIGNAL(updated()), 2000);

    return true;
}

void AppMultiPage::cleanupPage()
{
    // Do nothing here, but override so that fields are not reset.
}

int AppMultiPage::getCanFwd()
{
    QListWidgetItem *item = mCanFwdList->currentItem();
    int canFwd = 0;
    if (item) {
        canFwd = item->data(Qt::UserRole).toInt();
    }
    return canFwd;
}

AppGeneralPage::AppGeneralPage(VescInterface *vesc, QWidget *parent)
    : QWizardPage(parent)
{
    mVesc = vesc;

    setTitle(tr("Choose App"));
    setSubTitle(tr("Choose what type of input you want to control this VESC with."));

    mInputList = new QListWidget;
    QListWidgetItem *item = new QListWidgetItem;
    item->setText(tr("PPM input, such as conventional RC receivers."));
    item->setIcon(QIcon("://res/images/rc_rx.jpg"));
    item->setData(Qt::UserRole, SetupWizardApp::Input_Ppm);
    mInputList->addItem(item);

    item = new QListWidgetItem;
    item->setText(tr("NRF controller."));
    item->setIcon(QIcon("://res/images/vedder_nunchuk.jpg"));
    item->setData(Qt::UserRole, SetupWizardApp::Input_NunchukNrf);
    mInputList->addItem(item);

    item = new QListWidgetItem;
    item->setText(tr("ADC input, such as conventional ebike throttles."));
    item->setIcon(QIcon("://res/images/ebike_throttle.jpg"));
    item->setData(Qt::UserRole, SetupWizardApp::Input_Adc);
    mInputList->addItem(item);

    item = new QListWidgetItem;
    item->setText(tr("Wireless nyko kama nunchuk."));
    item->setIcon(QIcon("://res/images/nunchuk.jpg"));
    item->setData(Qt::UserRole, SetupWizardApp::Input_Nunchuk);
    mInputList->addItem(item);

    mInputList->setIconSize(QSize(60, 60));
    mInputList->setCurrentItem(nullptr);

    registerField("Input", this, "inputType", SIGNAL(inputTypeChanged));
    connect(mInputList, SIGNAL(currentRowChanged(int)),
            this, SIGNAL(inputTypeChanged()));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(mInputList);
    setLayout(layout);
}

int AppGeneralPage::nextId() const
{
    switch (field("Input").toInt()) {
    case SetupWizardApp::Input_Nunchuk:
    case SetupWizardApp::Input_NunchukNrf:
        return SetupWizardApp::Page_Nunchuk;

    case SetupWizardApp::Input_Ppm:
        return SetupWizardApp::Page_Ppm_Map;

    case SetupWizardApp::Input_Adc:
        return SetupWizardApp::Page_Adc_Map;

    default:
        break;
    }

    return SetupWizardApp::Page_Conclusion;
}

bool AppGeneralPage::validatePage()
{
    return true;
}

void AppGeneralPage::cleanupPage()
{
    // Do nothing here, but override so that fields are not reset.
}

int AppGeneralPage::getInputType()
{
    QListWidgetItem *item = mInputList->currentItem();
    int input = 0;
    if (item) {
        input = item->data(Qt::UserRole).toInt();
    }
    return input;
}

AppNunchukPage::AppNunchukPage(VescInterface *vesc, QWidget *parent)
    : QWizardPage(parent)
{
    mVesc = vesc;

    setTitle(tr("Nunchuk Configuration"));

    mParamTab = new ParamTable;
    mNrfPair = new NrfPair;
    mTimer = new QTimer(this);
    mWriteButton = new QPushButton(tr(" | Write Configuration To Vesc"));
    mWriteButton->setIcon(QIcon("://res/icons/app_down.png"));
    mWriteButton->setIconSize(QSize(24, 24));

    mNrfPair->setVesc(mVesc);

    mDisplay = new DisplayPercentage;
    mDisplay->setMinimumHeight(30);
    mDisplay->setDual(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(mParamTab);
    layout->addWidget(mWriteButton);
    layout->addWidget(mDisplay);
    layout->addWidget(mNrfPair);
    setLayout(layout);

    connect(mVesc->commands(), SIGNAL(decodedChukReceived(double)),
            this, SLOT(decodedChukReceived(double)));
    connect(mTimer, SIGNAL(timeout()), this, SLOT(timerSlot()));
    connect(mWriteButton, SIGNAL(clicked(bool)),
            mVesc->commands(), SLOT(setAppConf()));
}

int AppNunchukPage::nextId() const
{
    return SetupWizardApp::Page_Conclusion;
}

bool AppNunchukPage::validatePage()
{
    mVesc->commands()->setAppConf();
    return true;
}

void AppNunchukPage::initializePage()
{
    mParamTab->setRowCount(0);

    mParamTab->addParamRow(mVesc->appConfig(), "app_chuk_conf.ctrl_type");
    mParamTab->addParamRow(mVesc->appConfig(), "app_chuk_conf.ramp_time_pos");
    mParamTab->addParamRow(mVesc->appConfig(), "app_chuk_conf.ramp_time_neg");
    mParamTab->addParamRow(mVesc->appConfig(), "app_chuk_conf.stick_erpm_per_s_in_cc");
    mParamTab->addParamRow(mVesc->appConfig(), "app_chuk_conf.hyst");
    mParamTab->addParamRow(mVesc->appConfig(), "app_chuk_conf.use_smart_rev");
    mParamTab->addParamRow(mVesc->appConfig(), "app_chuk_conf.smart_rev_max_duty");
    mParamTab->addParamRow(mVesc->appConfig(), "app_chuk_conf.smart_rev_ramp_time");

    mParamTab->addRowSeparator(tr("Multiple VESCs over CAN-bus"));
    mParamTab->addParamRow(mVesc->appConfig(), "app_chuk_conf.tc");
    mParamTab->addParamRow(mVesc->appConfig(), "app_chuk_conf.tc_max_diff");
    mVesc->appConfig()->updateParamBool("app_chuk_conf.multi_esc", true);

    if (field("Input").toInt() == SetupWizardApp::Input_Nunchuk) {
        setSubTitle(tr("Configure your nyko kama nunchuk."));
        mNrfPair->setVisible(false);        
        mVesc->appConfig()->updateParamEnum("app_to_use", 6);
        mVesc->commands()->setAppConf();
        Utility::waitSignal(mVesc->commands(), SIGNAL(ackReceived(QString)), 2000);
        // TODO: Figure out why setting the conf twice is required...
        mVesc->commands()->setAppConf();
        Utility::waitSignal(mVesc->commands(), SIGNAL(ackReceived(QString)), 2000);
    } else {
        setSubTitle(tr("Pair and configure your NRF nunchuk."));
        mNrfPair->setVisible(true);
//        mVesc->appConfig()->updateParamEnum("app_to_use", 7);
        mVesc->appConfig()->updateParamEnum("app_to_use", 3); // Assume permanent NRF or NRF51 on UART
        mVesc->commands()->setAppConf();
        Utility::waitSignal(mVesc->commands(), SIGNAL(ackReceived(QString)), 2000);
        // TODO: Figure out why setting the conf twice is required...
        mVesc->commands()->setAppConf();
        Utility::waitSignal(mVesc->commands(), SIGNAL(ackReceived(QString)), 2000);

        QMessageBox::information(this,
                                 tr("NRF Pairing"),
                                 tr("You are about to start pairing your NRF nunchuk. After clicking OK "
                                    "you can use any of the buttons on the nunchuk to finish the pairing "
                                    "process. Notice that the nunchuk has to be switched off for the "
                                    "pairing to succeed. After the pairing is done you might need to use "
                                    "one of the nunchuk buttons again to switch the nunchuk on before you "
                                    "can use it."
                                    "<br><br>"
                                    "<font color=\"red\">Warning: </font>"
                                    "After the pairing is done the nunchuk will become activated, so if "
                                    "you move the joystick the motor will start spinning. Make sure that "
                                    "nothing is in the way."));

        mNrfPair->startPairing();
    }

    mTimer->start(40);
}

void AppNunchukPage::decodedChukReceived(double value)
{
    double p = value * 100.0;
    mDisplay->setValue(p);
    mDisplay->setText(tr("%1 %").
                      arg(p, 0, 'f', 1));
}

void AppNunchukPage::timerSlot()
{
    mVesc->commands()->getDecodedChuk();
}

AppPpmMapPage::AppPpmMapPage(VescInterface *vesc, QWidget *parent)
    : QWizardPage(parent)
{
    mVesc = vesc;

    setTitle(tr("PPM Mapping"));
    setSubTitle(tr("Map your PPM receiver."));

    mParamTab = new ParamTable;
    mPpmMap = new PpmMap;
    mTimer = new QTimer(this);

    mPpmMap->setVesc(mVesc);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(mParamTab);
    layout->addWidget(mPpmMap);
    setLayout(layout);

    connect(mTimer, SIGNAL(timeout()), this, SLOT(timerSlot()));
    connect(mVesc->appConfig(), SIGNAL(paramChangedDouble(QObject*,QString,double)),
            this, SLOT(paramChangedDouble(QObject*,QString,double)));
}

int AppPpmMapPage::nextId() const
{
    return SetupWizardApp::Page_Ppm;
}

bool AppPpmMapPage::validatePage()
{
    mVesc->commands()->setAppConf();
    return true;
}

void AppPpmMapPage::initializePage()
{
    mParamTab->setRowCount(0);

    mParamTab->addParamRow(mVesc->appConfig(), "app_ppm_conf.pulse_start");
    mParamTab->addParamRow(mVesc->appConfig(), "app_ppm_conf.pulse_end");
    mParamTab->addParamRow(mVesc->appConfig(), "app_ppm_conf.pulse_center");
    mParamTab->addParamRow(mVesc->appConfig(), "app_ppm_conf.hyst");

    mVesc->appConfig()->updateParamEnum("app_ppm_conf.ctrl_type", 0);
    mVesc->appConfig()->updateParamEnum("app_to_use", 4);
    mVesc->commands()->setAppConf();
    Utility::waitSignal(mVesc->commands(), SIGNAL(ackReceived(QString)), 2000);
    // TODO: Figure out why setting the conf twice is required...
    mVesc->commands()->setAppConf();
    Utility::waitSignal(mVesc->commands(), SIGNAL(ackReceived(QString)), 2000);
    mTimer->start(40);
}

void AppPpmMapPage::paramChangedDouble(QObject *src, QString name, double newParam)
{
    (void)src;
    (void)newParam;

    if (name == "app_ppm_conf.pulse_start" ||
            name == "app_ppm_conf.pulse_end" ||
            name == "app_ppm_conf.pulse_center" ||
            name == "app_ppm_conf.hyst") {
        mVesc->commands()->setAppConf();
    }
}

void AppPpmMapPage::timerSlot()
{
    mVesc->commands()->getDecodedPpm();
}

AppPpmPage::AppPpmPage(VescInterface *vesc, QWidget *parent)
    : QWizardPage(parent)
{
    mVesc = vesc;

    setTitle(tr("PPM Configuration"));
    setSubTitle(tr("Configure your PPM receiver."));

    mParamTab = new ParamTable;
    mWriteButton = new QPushButton(tr(" | Write Configuration To Vesc"));
    mWriteButton->setIcon(QIcon("://res/icons/app_down.png"));
    mWriteButton->setIconSize(QSize(24, 24));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(mParamTab);
    layout->addWidget(mWriteButton);
    setLayout(layout);

    connect(mWriteButton, SIGNAL(clicked(bool)),
            mVesc->commands(), SLOT(setAppConf()));
}

int AppPpmPage::nextId() const
{
    return SetupWizardApp::Page_Conclusion;
}

bool AppPpmPage::validatePage()
{
    mVesc->commands()->setAppConf();
    return true;
}

void AppPpmPage::initializePage()
{
    mParamTab->setRowCount(0);

    mParamTab->addRowSeparator(tr("General"));
    mParamTab->addParamRow(mVesc->appConfig(), "app_ppm_conf.ctrl_type");
    mParamTab->addParamRow(mVesc->appConfig(), "app_ppm_conf.median_filter");
    mParamTab->addParamRow(mVesc->appConfig(), "app_ppm_conf.safe_start");
    mParamTab->addParamRow(mVesc->appConfig(), "app_ppm_conf.ramp_time_pos");
    mParamTab->addParamRow(mVesc->appConfig(), "app_ppm_conf.ramp_time_neg");
    mParamTab->addParamRow(mVesc->appConfig(), "app_ppm_conf.pid_max_erpm");
    mParamTab->addParamRow(mVesc->appConfig(), "app_ppm_conf.max_erpm_for_dir");
    mParamTab->addParamRow(mVesc->appConfig(), "app_ppm_conf.smart_rev_max_duty");
    mParamTab->addParamRow(mVesc->appConfig(), "app_ppm_conf.smart_rev_ramp_time");

    mParamTab->addRowSeparator(tr("Multiple VESCs over CAN-bus"));
    mParamTab->addParamRow(mVesc->appConfig(), "app_ppm_conf.tc");
    mParamTab->addParamRow(mVesc->appConfig(), "app_ppm_conf.tc_max_diff");
    mVesc->appConfig()->updateParamBool("app_ppm_conf.multi_esc", true);

    mVesc->appConfig()->updateParamEnum("app_ppm_conf.ctrl_type", 3);
    mVesc->commands()->setAppConf();
}

AppAdcMapPage::AppAdcMapPage(VescInterface *vesc, QWidget *parent)
    : QWizardPage(parent)
{
    mVesc = vesc;

    setTitle(tr("ADC Mapping"));
    setSubTitle(tr("Map your analog throttle."));

    mParamTab = new ParamTable;
    mAdcMap = new AdcMap;
    mTimer = new QTimer(this);

    mAdcMap->setVesc(mVesc);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(mParamTab);
    layout->addWidget(mAdcMap);
    setLayout(layout);

    connect(mTimer, SIGNAL(timeout()), this, SLOT(timerSlot()));
    connect(mVesc->appConfig(), SIGNAL(paramChangedDouble(QObject*,QString,double)),
            this, SLOT(paramChangedDouble(QObject*,QString,double)));
    connect(mVesc->appConfig(), SIGNAL(paramChangedBool(QObject*,QString,bool)),
            this, SLOT(paramChangedBool(QObject*,QString,bool)));
}

int AppAdcMapPage::nextId() const
{
    return SetupWizardApp::Page_Adc;
}

bool AppAdcMapPage::validatePage()
{
    mVesc->commands()->setAppConf();
    return true;
}

void AppAdcMapPage::initializePage()
{
    mParamTab->setRowCount(0);

    mParamTab->addParamRow(mVesc->appConfig(), "app_adc_conf.voltage_start");
    mParamTab->addParamRow(mVesc->appConfig(), "app_adc_conf.voltage_end");
    mParamTab->addParamRow(mVesc->appConfig(), "app_adc_conf.voltage_center");
    mParamTab->addParamRow(mVesc->appConfig(), "app_adc_conf.voltage_inverted");
    mParamTab->addParamRow(mVesc->appConfig(), "app_adc_conf.voltage2_start");
    mParamTab->addParamRow(mVesc->appConfig(), "app_adc_conf.voltage2_end");
    mParamTab->addParamRow(mVesc->appConfig(), "app_adc_conf.voltage2_inverted");
    mParamTab->addParamRow(mVesc->appConfig(), "app_adc_conf.hyst");

    mVesc->appConfig()->updateParamEnum("app_to_use", 5);
    mVesc->appConfig()->updateParamEnum("app_adc_conf.ctrl_type", 0);
    mVesc->commands()->setAppConf();
    Utility::waitSignal(mVesc->commands(), SIGNAL(ackReceived(QString)), 2000);
    // TODO: Figure out why setting the conf twice is required...
    mVesc->commands()->setAppConf();
    Utility::waitSignal(mVesc->commands(), SIGNAL(ackReceived(QString)), 2000);
    mTimer->start(40);
}

void AppAdcMapPage::paramChangedDouble(QObject *src, QString name, double newParam)
{
    (void)src;
    (void)newParam;

    if (name == "app_adc_conf.voltage_start" ||
            name == "app_adc_conf.voltage_end" ||
            name == "app_adc_conf.voltage_center" ||
            name == "app_adc_conf.voltage2_start" ||
            name == "app_adc_conf.voltage2_end" ||
            name == "app_adc_conf.hyst") {
        mVesc->commands()->setAppConf();
    }
}

void AppAdcMapPage::paramChangedBool(QObject *src, QString name, bool newParam)
{
    (void)src;
    (void)newParam;

    if (name == "app_adc_conf.voltage_inverted" ||
            name == "app_adc_conf.voltage2_inverted") {
        mVesc->commands()->setAppConf();
    }
}

void AppAdcMapPage::timerSlot()
{
    mVesc->commands()->getDecodedAdc();
}

AppAdcPage::AppAdcPage(VescInterface *vesc, QWidget *parent)
    : QWizardPage(parent)
{
    mVesc = vesc;

    setTitle(tr("ADC Configuration"));
    setSubTitle(tr("Configure your analog throttle."));

    mParamTab = new ParamTable;
    mWriteButton = new QPushButton(tr(" | Write Configuration To Vesc"));
    mWriteButton->setIcon(QIcon("://res/icons/app_down.png"));
    mWriteButton->setIconSize(QSize(24, 24));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(mParamTab);
    layout->addWidget(mWriteButton);
    setLayout(layout);

    connect(mWriteButton, SIGNAL(clicked(bool)),
            mVesc->commands(), SLOT(setAppConf()));
}

int AppAdcPage::nextId() const
{
    return SetupWizardApp::Page_Conclusion;
}

bool AppAdcPage::validatePage()
{
    mVesc->commands()->setAppConf();
    return true;
}

void AppAdcPage::initializePage()
{
    mParamTab->setRowCount(0);

    mParamTab->addRowSeparator(tr("General"));
    mParamTab->addParamRow(mVesc->appConfig(), "app_adc_conf.ctrl_type");
    mParamTab->addParamRow(mVesc->appConfig(), "app_adc_conf.use_filter");
    mParamTab->addParamRow(mVesc->appConfig(), "app_adc_conf.safe_start");
    mParamTab->addParamRow(mVesc->appConfig(), "app_adc_conf.cc_button_inverted");
    mParamTab->addParamRow(mVesc->appConfig(), "app_adc_conf.rev_button_inverted");
    mParamTab->addParamRow(mVesc->appConfig(), "app_adc_conf.ramp_time_pos");
    mParamTab->addParamRow(mVesc->appConfig(), "app_adc_conf.ramp_time_neg");

    mParamTab->addRowSeparator(tr("Multiple VESCs over CAN-bus"));
    mParamTab->addParamRow(mVesc->appConfig(), "app_adc_conf.tc");
    mParamTab->addParamRow(mVesc->appConfig(), "app_adc_conf.tc_max_diff");
    mVesc->appConfig()->updateParamBool("app_adc_conf.multi_esc", true);

    mVesc->appConfig()->updateParamEnum("app_adc_conf.ctrl_type", 1);
    mVesc->commands()->setAppConf();
}

AppConclusionPage::AppConclusionPage(VescInterface *vesc, QWidget *parent)
    : QWizardPage(parent)
{
    mVesc = vesc;
    setTitle(tr("Conclusion"));

    mLabel = new QLabel;
    mLabel->setWordWrap(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(mLabel);
    setLayout(layout);
}

int AppConclusionPage::nextId() const
{
    return -1;
}

void AppConclusionPage::initializePage()
{
    mLabel->setText(tr("You have finished the app setup. At this point "
                       "everything should be ready to run."));
}
