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

#include "setupwizardapp.h"
#include <QVBoxLayout>
#include <QMessageBox>
#include "utility.h"

SetupWizardApp::SetupWizardApp(VescInterface *vesc, QWidget *parent)
    : QWizard(parent)
{
    setPage(Page_Intro, new AppIntroPage(vesc));
    setPage(Page_Connection, new AppConnectionPage(vesc));
    setPage(Page_Firmware, new AppFirmwarePage(vesc));
    setPage(Page_Multi, new AppMultiPage(vesc));
    setPage(Page_MultiId, new AppMultiIdPage(vesc));
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
}

void SetupWizardApp::idChanged(int id)
{
    if (id == Page_Intro || id == Page_Conclusion) {
        setSideWidget(mSideLabel);
        mSideLabel->setVisible(true);
    } else {
        setSideWidget(0);
    }
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
        if (mVesc->commands()->isLimitedMode()) {
            return SetupWizardApp::Page_Firmware;
        } else {
            return SetupWizardApp::Page_Multi;
        }
    } else {
        return SetupWizardApp::Page_Connection;
//        return SetupWizardApp::Page_Multi;
    }
}

bool AppIntroPage::validatePage()
{
    bool res = false;

    if (!mVesc->isPortConnected()) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::information(this,
                                         tr("Connection"),
                                         tr("You are not connected to the VESC. Would you like to try to automatically connect?<br><br>"
                                            ""
                                            "<i>Notice that the USB cable must be plugged in and that the VESC "
                                            "must be powered for the connection to work.</i>"),
                                         QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

        if (reply == QMessageBox::Yes) {
            Utility::autoconnectBlockingWithProgress(mVesc, this);
            res = true;
        }
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

    mLabel = new QLabel(tr("Your VESC has old firmware, and needs to be updated. After that, "
                           "the motor configuration has to be done again. Please run the "
                           "motor configuration wizard to update the firmware and to configure "
                           "the motor."));

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
    mLoadDefaultAsked = false;

    setTitle(tr("Multiple VESCs"));
    setSubTitle(tr("Do you have more than one VESC on your setup?"));

    mModeList = new QListWidget;
    QListWidgetItem *item = new QListWidgetItem;
    item->setText(tr("My setup has a single VESC."));
    item->setIcon(QIcon("://res/images/multi_single.png"));
    item->setData(Qt::UserRole, SetupWizardApp::Multi_Single);
    mModeList->addItem(item);

    item = new QListWidgetItem;
    item->setText(tr("My setup has more than one VESC, and I'm configuring the master VESC now. The "
                  "master VESC is the one that is connected to the input."));
    item->setIcon(QIcon("://res/images/multi_master.png"));
    item->setData(Qt::UserRole, SetupWizardApp::Multi_Master);
    mModeList->addItem(item);

    item = new QListWidgetItem;
    item->setText(tr("My setup has more than one VESC, and I'm configuring one of the slave VESCs now. "
                  "A slave VESC is not connected to any input, only to the other VESCs over CAN-bus."));
    item->setIcon(QIcon("://res/images/multi_slave.png"));
    item->setData(Qt::UserRole, SetupWizardApp::Multi_Slave);
    mModeList->addItem(item);

    mModeList->setIconSize(QSize(200, 200));
    mModeList->setCurrentItem(0);
    mModeList->setWordWrap(true);
    mModeList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    registerField("Multi", mModeList, "currentRow");

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(mModeList);
    setLayout(layout);
}

int AppMultiPage::nextId() const
{
    if (field("Multi").toInt() == SetupWizardApp::Multi_Single) {
        return SetupWizardApp::Page_General;
    } else {
        return SetupWizardApp::Page_MultiId;
    }
}

bool AppMultiPage::validatePage()
{
    if (field("Multi").toInt() == SetupWizardApp::Multi_Single) {
        mVesc->appConfig()->updateParamBool("send_can_status", false);
    } else {
        mVesc->appConfig()->updateParamBool("send_can_status", true);
        mVesc->appConfig()->updateParamInt("send_can_status_rate_hz", 200);
    }

    if (field("Multi").toInt() == SetupWizardApp::Multi_Slave) {
        mVesc->appConfig()->updateParamEnum("app_to_use", 3); // Use UART app on slave VESCs
    }

    mVesc->commands()->setAppConf();
    return true;
}

void AppMultiPage::showEvent(QShowEvent *event)
{
    if (!mLoadDefaultAsked) {
        mLoadDefaultAsked = true;

        QMessageBox::StandardButton reply;
        reply = QMessageBox::information(this,
                                         tr("Load Default Configuration"),
                                         tr("Would you like to load the default configuration from "
                                            "the connected VESC before proceeding with the setup?"),
                                         QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

        if (reply == QMessageBox::Yes) {
            mVesc->commands()->getAppConfDefault();
        }
    }

    QWidget::showEvent(event);
}

void AppMultiPage::cleanupPage()
{
    // Do nothing here, but override so that fields are not reset.
}

AppMultiIdPage::AppMultiIdPage(VescInterface *vesc, QWidget *parent)
    : QWizardPage(parent)
{
    mVesc = vesc;

    setTitle(tr("Set VESC ID"));
    setSubTitle(tr("Make sure that your connected VESCs have unique IDs."));

    mParamTab = new ParamTable;
    mLabel = new QLabel(tr("TIP: After this setup you can connect the USB cable to any of the VESCs "
                           "connected over CAN-bus and access all of them with the CAN forwarding "
                           "function."));
    mLabel->setWordWrap(true);

    mParamTab->addParamRow(mVesc->appConfig(), "controller_id");

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(mParamTab);
    layout->addWidget(mLabel);

    setLayout(layout);
}

int AppMultiIdPage::nextId() const
{
    if (field("Multi").toInt() == SetupWizardApp::Multi_Slave) {
        return SetupWizardApp::Page_Conclusion;
    } else {
        return SetupWizardApp::Page_General;
    }
}

bool AppMultiIdPage::validatePage()
{
    mVesc->commands()->setAppConf();
    return true;
}

void AppMultiIdPage::initializePage()
{
    if (field("Multi").toInt() == SetupWizardApp::Multi_Slave) {
        mVesc->appConfig()->updateParamInt("controller_id", 1);
    } else {
        mVesc->appConfig()->updateParamInt("controller_id", 0);
    }
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
    item->setText(tr("ADC input, such as conventional ebike throttles."));
    item->setIcon(QIcon("://res/images/ebike_throttle.jpg"));
    item->setData(Qt::UserRole, SetupWizardApp::Input_Adc);
    mInputList->addItem(item);

    item = new QListWidgetItem;
    item->setText(tr("Wireless nyko kama nunchuk."));
    item->setIcon(QIcon("://res/images/nunchuk.jpg"));
    item->setData(Qt::UserRole, SetupWizardApp::Input_Nunchuk);
    mInputList->addItem(item);

    item = new QListWidgetItem;
    item->setText(tr("NRF nunchuk."));
    item->setIcon(QIcon("://res/images/vedder_nunchuk.jpg"));
    item->setData(Qt::UserRole, SetupWizardApp::Input_NunchukNrf);
    mInputList->addItem(item);

    mInputList->setIconSize(QSize(60, 60));
    mInputList->setCurrentItem(0);

    registerField("Input", mInputList, "currentRow");

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
        break;

    case SetupWizardApp::Input_Ppm:
        return SetupWizardApp::Page_Ppm_Map;
        break;

    case SetupWizardApp::Input_Adc:
        return SetupWizardApp::Page_Adc_Map;
        break;

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

    if (field("Multi").toInt() == SetupWizardApp::Multi_Master) {
        mParamTab->addRowSeparator(tr("Multiple VESCs over CAN-bus"));
        mParamTab->addParamRow(mVesc->appConfig(), "app_chuk_conf.tc");
        mParamTab->addParamRow(mVesc->appConfig(), "app_chuk_conf.tc_max_diff");
        mVesc->appConfig()->updateParamBool("app_chuk_conf.multi_esc", true);
    } else {
        mVesc->appConfig()->updateParamBool("app_chuk_conf.multi_esc", false);
    }

    if (field("Input").toInt() == SetupWizardApp::Input_Nunchuk) {
        setSubTitle(tr("Configure your nyko kama nunchuk."));
        mNrfPair->setVisible(false);
        mVesc->appConfig()->updateParamEnum("app_to_use", 6);
        mVesc->commands()->setAppConf();
    } else {
        setSubTitle(tr("Pair and configure your NRF nunchuk."));
        mNrfPair->setVisible(true);
        mVesc->appConfig()->updateParamEnum("app_to_use", 7);
        mVesc->commands()->setAppConf();

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

    mTimer->start(20);
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

    mTimer->start(20);
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

    if (field("Multi").toInt() == SetupWizardApp::Multi_Master) {
        mParamTab->addRowSeparator(tr("Multiple VESCs over CAN-bus"));
        mParamTab->addParamRow(mVesc->appConfig(), "app_ppm_conf.tc");
        mParamTab->addParamRow(mVesc->appConfig(), "app_ppm_conf.tc_max_diff");
        mVesc->appConfig()->updateParamBool("app_ppm_conf.multi_esc", true);
    } else {
        mVesc->appConfig()->updateParamBool("app_ppm_conf.multi_esc", false);
    }

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

    mVesc->appConfig()->updateParamEnum("app_to_use", 2);
    mVesc->appConfig()->updateParamEnum("app_adc_conf.ctrl_type", 0);
    mVesc->commands()->setAppConf();

    mTimer->start(20);
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

    if (field("Multi").toInt() == SetupWizardApp::Multi_Master) {
        mParamTab->addRowSeparator(tr("Multiple VESCs over CAN-bus"));
        mParamTab->addParamRow(mVesc->appConfig(), "app_adc_conf.tc");
        mParamTab->addParamRow(mVesc->appConfig(), "app_adc_conf.tc_max_diff");
        mVesc->appConfig()->updateParamBool("app_adc_conf.multi_esc", true);
    } else {
        mVesc->appConfig()->updateParamBool("app_adc_conf.multi_esc", false);
    }

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
    switch (field("Multi").toInt()) {
    case SetupWizardApp::Multi_Master:
        mLabel->setText(tr("You have finished the app setup for the VESC. After configuring "
                           "all the VESCs in your setup you are done."));
        break;

    case SetupWizardApp::Multi_Slave:
        mLabel->setText(tr("You have finished the setup for this VESC. Since this is not the "
                           "master VESC you don't have to configure any app on it. After configuring "
                           "all the VESCs in your setup you are done."));
        break;

    default:
        mLabel->setText(tr("You have finished the app setup for the VESC. At this point "
                           "everything should be ready to run."));
        break;
    }
}
