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

#include "setupwizardmotor.h"
#include <QVBoxLayout>
#include <QMessageBox>
#include "utility.h"

SetupWizardMotor::SetupWizardMotor(VescInterface *vesc, QWidget *parent)
    : QWizard(parent)
{
    setPage(Page_Intro, new IntroPage(vesc));
    setPage(Page_Connection, new ConnectionPage(vesc));
    setPage(Page_Firmware, new FirmwarePage(vesc));
    setPage(Page_MotorType, new MotorTypePage(vesc));
    setPage(Page_Currents, new CurrentsPage(vesc));
    setPage(Page_Voltages, new VoltagesPage(vesc));
    setPage(Page_Sensors, new SensorsPage(vesc));
    setPage(Page_Bldc, new BldcPage(vesc));
    setPage(Page_Foc, new FocPage(vesc));
    setPage(Page_FocEncoder, new FocEncoderPage(vesc));
    setPage(Page_FocHall, new FocHallPage(vesc));
    setPage(Page_Conclusion, new ConclusionPage(vesc));

    setStartId(Page_Intro);
    setWizardStyle(ModernStyle);
    setPixmap(QWizard::LogoPixmap, QPixmap("://res/icon.png").
              scaled(40, 40,
                     Qt::KeepAspectRatio,
                     Qt::SmoothTransformation));
    resize(800, 450);

    setWindowTitle(tr("Motor Setup Wizard"));

    mSideLabel = new AspectImgLabel(Qt::Vertical);
    mSideLabel->setPixmap(QPixmap("://res/logo_wizard.png"));
    mSideLabel->setScaledContents(true);
    setSideWidget(mSideLabel);

    connect(this, SIGNAL(currentIdChanged(int)),
            this, SLOT(idChanged(int)));
}

void SetupWizardMotor::idChanged(int id)
{
    if (id == Page_Intro || id == Page_Conclusion) {
        setSideWidget(mSideLabel);
        mSideLabel->setVisible(true);
    } else {
        setSideWidget(0);
    }
}

IntroPage::IntroPage(VescInterface *vesc, QWidget *parent)
    : QWizardPage(parent)
{
    mVesc = vesc;
    setTitle(tr("VESC® Motor Setup Wizard"));

    mLabel = new QLabel(tr("This wizard will guide you through the motor setup of the VESC® "
                           "step by step. Notice that only the required options for "
                           "getting the motor running are shown. For tweaking the advanced "
                           "settings, the configuration pages have to be entered after "
                           "finishing this wizard.<br><br>"
                           ""
                           "To get more information about the parameters and tools in the "
                           "wizard, click on the questionmark next to them.<br><br>"
                           ""
                           "After finishing the motor setup, you can use the input setup wizard "
                           "to configure the apps for input to the VESC."));
    mLabel->setWordWrap(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(mLabel);
    setLayout(layout);
}

int IntroPage::nextId() const
{
    if (mVesc->isPortConnected()) {
        if (mVesc->commands()->isLimitedMode()) {
            return SetupWizardMotor::Page_Firmware;
        } else {
            return SetupWizardMotor::Page_MotorType;
        }
    } else {
        return SetupWizardMotor::Page_Connection;
    }
}

bool IntroPage::validatePage()
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

ConnectionPage::ConnectionPage(VescInterface *vesc, QWidget *parent)
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

int ConnectionPage::nextId() const
{
    if (mVesc->commands()->isLimitedMode()) {
        return SetupWizardMotor::Page_Firmware;
    } else {
        return SetupWizardMotor::Page_MotorType;
    }
}

bool ConnectionPage::isComplete() const
{
    return mVesc->fwRx();
}

FirmwarePage::FirmwarePage(VescInterface *vesc, QWidget *parent)
    : QWizardPage(parent)
{
    mVesc = vesc;

    setTitle(tr("Update Firmware"));
    setSubTitle(tr("You need to update the firmware on the VESC in order "
                   "to use it with this version of VESC Tool."));

    mPageFirmware = new PageFirmware;
    mPageFirmware->setVesc(mVesc);

    connect(mVesc, SIGNAL(portConnectedChanged()),
            this, SIGNAL(completeChanged()));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(mPageFirmware);
    setLayout(layout);
}

int FirmwarePage::nextId() const
{
    // Going to a previous page here does not seem to work,
    // so this is done in the validatePage function.
    return SetupWizardMotor::Page_MotorType;
}

bool FirmwarePage::isComplete() const
{
    return !mVesc->isPortConnected();
}

bool FirmwarePage::validatePage()
{
    wizard()->back();
    return false;
}

void FirmwarePage::initializePage()
{
    mVesc->commands()->getFwVersion();
}

MotorTypePage::MotorTypePage(VescInterface *vesc, QWidget *parent)
    : QWizardPage(parent)
{
    mVesc = vesc;
    mLoadDefaultAsked = false;

    setTitle(tr("Choose Motor Type"));
    setSubTitle(tr("This choise will help the wizard show the relevant "
                   "configuration pages."));

    mParamTab = new ParamTable;
    mParamTab->addParamRow(mVesc->mcConfig(), "motor_type");

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(mParamTab);
    setLayout(layout);
}

int MotorTypePage::nextId() const
{
    return SetupWizardMotor::Page_Currents;
}

bool MotorTypePage::validatePage()
{
    mVesc->commands()->setMcconf();
    return true;
}

void MotorTypePage::showEvent(QShowEvent *event)
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
            mVesc->commands()->getMcconfDefault();
        }
    }

    QWidget::showEvent(event);
}

CurrentsPage::CurrentsPage(VescInterface *vesc, QWidget *parent)
    : QWizardPage(parent)
{
    mVesc = vesc;
    mWarningShown = false;

    setTitle(tr("Set Current Limits"));
    setSubTitle(tr("It is important to set correct current limits, both for correct "
                   "parameter detection and for safe operation."));

    mParamTab = new ParamTable;
    mParamTab->addRowSeparator(tr("Motor"));
    mParamTab->addParamRow(mVesc->mcConfig(), "l_current_max");
    mParamTab->addParamRow(mVesc->mcConfig(), "l_current_min");
    mParamTab->addRowSeparator(tr("Battery"));
    mParamTab->addParamRow(mVesc->mcConfig(), "l_in_current_max");
    mParamTab->addParamRow(mVesc->mcConfig(), "l_in_current_min");
    mConfigureBatteryCutoff = true;

    mLabel = new QLabel(tr("<font color=\"red\">WARNING: </font>"
                           "Using too high current settings can damage the motor, the VESC "
                           "and/or the battery. Make sure to read the specifications of your "
                           "system if you are not sure about how to configure the current limits."));
    mLabel->setWordWrap(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(mParamTab);
    layout->addWidget(mLabel);
    setLayout(layout);
}

int CurrentsPage::nextId() const
{
    if (mConfigureBatteryCutoff) {
        return SetupWizardMotor::Page_Voltages;
    } else {
        return SetupWizardMotor::Page_Sensors;
    }
}

bool CurrentsPage::validatePage()
{
    mVesc->commands()->setMcconf();

    mConfigureBatteryCutoff = QMessageBox::information(this,
                                     tr("Configure Battery Cutoff"),
                                     tr("Would you like to configure a soft battery cutoff voltage "
                                        "to prevent the battery from over-discharging?"
                                        "<br><br>"
                                        "If you are using a power supply instead of a battery you can "
                                        "skip this step."),
                                     QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes)
            == QMessageBox::Yes;

    return true;
}

void CurrentsPage::showEvent(QShowEvent *event)
{
    if (!mWarningShown) {
        mWarningShown = true;

        QMessageBox::warning(this,
                             tr("Important Notice"),
                             tr("You are about to configure the current limits. Keep in mind that "
                                "using too high current settings can damage the motor, the VESC "
                                "and/or the battery. Make sure to read the specifications of your "
                                "system if you are not sure about how to configure the current limits."));

    }

    QWidget::showEvent(event);
}

VoltagesPage::VoltagesPage(VescInterface *vesc, QWidget *parent)
    : QWizardPage(parent)
{
    mVesc = vesc;

    setTitle(tr("Set Voltage Limits"));
    setSubTitle(tr("Set soft voltage limits to prevent overdischarging your battery."));

    mParamTab = new ParamTable;
    mParamTab->addParamRow(mVesc->mcConfig(), "l_battery_cut_start");
    mParamTab->addParamRow(mVesc->mcConfig(), "l_battery_cut_end");

    mCalc = new BatteryCalculator(this);
    mCalc->setVesc(vesc);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(mParamTab);
    layout->addWidget(mCalc);
    setLayout(layout);
}

int VoltagesPage::nextId() const
{
    return SetupWizardMotor::Page_Sensors;
}

bool VoltagesPage::validatePage()
{
    mVesc->commands()->setMcconf();
    return true;
}

SensorsPage::SensorsPage(VescInterface *vesc, QWidget *parent)
    : QWizardPage(parent)
{
    mVesc = vesc;

    setTitle(tr("Choose sensor mode"));
    setSubTitle(tr("Select what type of sensor (if any) your motor has."));

    mSensorMode = new QComboBox;
    mSensorMode->addItem("No Sensor (Sensorless)", SetupWizardMotor::Sensor_Sensorless);
    mParamTab = new ParamTable;
    mTypeBefore = -1;

    mParamTab->addParamRow(mVesc->mcConfig(), "m_encoder_counts");
    registerField("SensorMode", mSensorMode, "currentData");

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(mSensorMode);
    layout->addWidget(mParamTab);
    setLayout(layout);

    connect(mSensorMode, SIGNAL(currentIndexChanged(int)),
            this, SLOT(indexChanged(int)));

    indexChanged(0);
}

int SensorsPage::nextId() const
{
    int retval = SetupWizardMotor::Page_Conclusion;

    switch (mVesc->mcConfig()->getParamEnum("motor_type")) {
    case 0: // BLDC
        retval = SetupWizardMotor::Page_Bldc;
        break;

    case 1: // DC
        retval = SetupWizardMotor::Page_Conclusion;
        break;

    case 2: // FOC
        retval = SetupWizardMotor::Page_Foc;
        break;

    default:
        break;
    }

    return retval;
}

bool SensorsPage::validatePage()
{
    switch (mSensorMode->currentData().toInt()) {
    case SetupWizardMotor::Sensor_Sensorless:
        mVesc->mcConfig()->updateParamEnum("m_sensor_port_mode", 0);
        mVesc->mcConfig()->updateParamEnum("sensor_mode", 0);
        mVesc->mcConfig()->updateParamEnum("foc_sensor_mode", 0);
        break;

    case SetupWizardMotor::Sensor_Hall:
        mVesc->mcConfig()->updateParamEnum("m_sensor_port_mode", 0);
        mVesc->mcConfig()->updateParamEnum("sensor_mode", 2);
        mVesc->mcConfig()->updateParamEnum("foc_sensor_mode", 2);
        break;

    case SetupWizardMotor::Sensor_EncoderAbi:
        mVesc->mcConfig()->updateParamEnum("m_sensor_port_mode", 1);
        mVesc->mcConfig()->updateParamEnum("sensor_mode", 0);
        mVesc->mcConfig()->updateParamEnum("foc_sensor_mode", 1);
        break;

    case SetupWizardMotor::Sensor_EncoderAs:
        mVesc->mcConfig()->updateParamEnum("m_sensor_port_mode", 2);
        mVesc->mcConfig()->updateParamEnum("sensor_mode", 0);
        mVesc->mcConfig()->updateParamEnum("foc_sensor_mode", 1);
        break;

    case SetupWizardMotor::Sensor_Resolver_AD2S1205:
        mVesc->mcConfig()->updateParamEnum("m_sensor_port_mode", 3);
        mVesc->mcConfig()->updateParamEnum("sensor_mode", 0);
        mVesc->mcConfig()->updateParamEnum("foc_sensor_mode", 1);
        break;

    case SetupWizardMotor::Sensor_Encoder_SinCos:
        mVesc->mcConfig()->updateParamEnum("m_sensor_port_mode", 4);
        mVesc->mcConfig()->updateParamEnum("sensor_mode", 0);
        mVesc->mcConfig()->updateParamEnum("foc_sensor_mode", 1);
        break;

    default:
        break;
    }

    mVesc->commands()->setMcconf();
    return true;
}

void SensorsPage::initializePage()
{
    int typeNow = mVesc->mcConfig()->getParamEnum("motor_type");

    if (mTypeBefore != typeNow) {
        while (mSensorMode->count() > 1) {
            mSensorMode->removeItem(1);
        }

        switch (typeNow) {
        case 0: // BLDC
            mSensorMode->addItem("Hall Sensors", SetupWizardMotor::Sensor_Hall);
            break;

        case 1: // DC
            mSensorMode->addItem("ABI Encoder", SetupWizardMotor::Sensor_EncoderAbi);
            mSensorMode->addItem("AS5047 Encoder", SetupWizardMotor::Sensor_EncoderAs);
            break;

        case 2: // FOC
            mSensorMode->addItem("Hall Sensors", SetupWizardMotor::Sensor_Hall);
            mSensorMode->addItem("ABI Encoder", SetupWizardMotor::Sensor_EncoderAbi);
            mSensorMode->addItem("AS5047 Encoder", SetupWizardMotor::Sensor_EncoderAs);
            mSensorMode->addItem("Resolver", SetupWizardMotor::Sensor_Resolver_AD2S1205);
            mSensorMode->addItem("Sin/Cos Encoder", SetupWizardMotor::Sensor_Encoder_SinCos);
            mSensorMode->addItem("BiSS Encoder", SetupWizardMotor::Sensor_EncoderBiSS);
            break;

        default:
            break;
        }
    }

    mTypeBefore = typeNow;
}

void SensorsPage::cleanupPage()
{
    // Do nothing here, but override so that fields are not reset.
}

void SensorsPage::indexChanged(int ind)
{
    (void)ind;
    if (mSensorMode->currentData() == SetupWizardMotor::Sensor_EncoderAbi) {
        mParamTab->setVisible(true);
    } else {
        mParamTab->setVisible(false);
    }
}

BldcPage::BldcPage(VescInterface *vesc, QWidget *parent)
    : QWizardPage(parent)
{
    mVesc = vesc;

    setTitle(tr("BLDC Settings"));
    setSubTitle(tr("Run detection and get the required parameters for BLDC commutation."));

    mParamTab = new ParamTable;
    mDetect = new DetectBldc(this);
    mDetect->setVesc(vesc);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(mParamTab);
    layout->addWidget(mDetect);
    layout->setStretch(0, 1);
    setLayout(layout);
}

int BldcPage::nextId() const
{
    return SetupWizardMotor::Page_Conclusion;
}

bool BldcPage::validatePage()
{
    mVesc->commands()->setMcconf();
    return true;
}

void BldcPage::initializePage()
{
    mParamTab->setRowCount(0);
    mParamTab->addParamRow(mVesc->mcConfig(), "sl_cycle_int_limit");
    mParamTab->addParamRow(mVesc->mcConfig(), "sl_bemf_coupling_k");
    mParamTab->addRowSeparator(tr("Hall Sensor Settings"));

    if (field("SensorMode").toInt() == SetupWizardMotor::Sensor_Hall) {
        mParamTab->addParamRow(mVesc->mcConfig(), "hall_sl_erpm");
        for (int i = 0;i < 8;i++) {
            QString str;
            str.sprintf("hall_table_%d", i);
            mParamTab->addParamRow(mVesc->mcConfig(), str);
        }
    }
}

FocPage::FocPage(VescInterface *vesc, QWidget *parent)
    : QWizardPage(parent)
{
    mVesc = vesc;

    setTitle(tr("FOC Settings"));
    setSubTitle(tr("Run detection and get the required parameters for FOC."));

    mParamTab = new ParamTable;
    mDetect = new DetectFoc(this);
    mDetect->setVesc(vesc);

    mParamTab->addParamRow(mVesc->mcConfig(), "foc_motor_r");
    mParamTab->addParamRow(mVesc->mcConfig(), "foc_motor_l");
    mParamTab->addParamRow(mVesc->mcConfig(), "foc_motor_flux_linkage");
    mParamTab->addParamRow(mVesc->mcConfig(), "foc_current_kp");
    mParamTab->addParamRow(mVesc->mcConfig(), "foc_current_ki");
    mParamTab->addParamRow(mVesc->mcConfig(), "foc_observer_gain");

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(mParamTab);
    layout->addWidget(mDetect);
    setLayout(layout);
}

int FocPage::nextId() const
{
    int retval = SetupWizardMotor::Page_Conclusion;

    switch (field("SensorMode").toInt()) {
    case SetupWizardMotor::Sensor_Sensorless:
        retval = SetupWizardMotor::Page_Conclusion;
        break;

    case SetupWizardMotor::Sensor_Hall:
        retval = SetupWizardMotor::Page_FocHall;
        break;

    case SetupWizardMotor::Sensor_EncoderAbi:
    case SetupWizardMotor::Sensor_EncoderBiSS:
    case SetupWizardMotor::Sensor_Resolver_AD2S1205:
    case SetupWizardMotor::Sensor_Encoder_SinCos:
    case SetupWizardMotor::Sensor_EncoderAs:
        retval = SetupWizardMotor::Page_FocEncoder;
        break;

    default:
        break;
    }

    return retval;
}

bool FocPage::validatePage()
{
    bool res = false;

    if (mDetect->lastOkValuesApplied()) {
        res = true;
    } else {
        QString msg;

        if (mDetect->allValuesOk()) {
            msg = tr("You have not applied the detection result. Would you like to "
                     "continue with the wizard without using the detected values?"
                     "<br><br>"
                     "<font color=\"red\">WARNING: </font>"
                     "Using the wrong motor parameters will most likely damage your "
                     "VESC and/or motor. Answering <b>No</b> and applying the dection result "
                     "is recommended.");
        } else {
            msg = tr("You have not finished the detection. Would you like to continue "
                     "with the wizard anyway?"
                     "<br><br>"
                     "<font color=\"red\">WARNING: </font>"
                     "Using the wrong motor parameters will most likely damage your "
                     "VESC and/or motor. Answering <b>No</b> and finishing the detection "
                     "is recommended.");
        }

        QMessageBox::StandardButton reply;
        reply = QMessageBox::warning(this,
                                     tr("Continue Without Detection Result?"),
                                     msg,
                                     QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

        res = reply == QMessageBox::Yes;
    }

    if (res) {
        mVesc->commands()->setMcconf();
    }

    return res;
}

FocEncoderPage::FocEncoderPage(VescInterface *vesc, QWidget *parent)
    : QWizardPage(parent)
{
    mVesc = vesc;

    setTitle(tr("FOC Encoder Settings"));
    setSubTitle(tr("Detect and set the required settings for running FOC with an encoder."));

    mParamTab = new ParamTable;
    mDetect = new DetectFocEncoder(this);
    mDetect->setVesc(vesc);

    mParamTab->addParamRow(mVesc->mcConfig(), "foc_sl_erpm");
    mParamTab->addParamRow(mVesc->mcConfig(), "foc_encoder_offset");
    mParamTab->addParamRow(mVesc->mcConfig(), "foc_encoder_ratio");
    mParamTab->addParamRow(mVesc->mcConfig(), "foc_encoder_inverted");

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(mParamTab);
    layout->addWidget(mDetect);
    setLayout(layout);
}

int FocEncoderPage::nextId() const
{
    return SetupWizardMotor::Page_Conclusion;
}

bool FocEncoderPage::validatePage()
{
    mVesc->commands()->setMcconf();
    return true;
}

FocHallPage::FocHallPage(VescInterface *vesc, QWidget *parent)
    : QWizardPage(parent)
{
    mVesc = vesc;

    setTitle(tr("FOC Hall Sensor Settings"));
    setSubTitle(tr("Detect and set the required settings for running FOC with hall sensors."));

    mParamTab = new ParamTable;
    mDetect = new DetectFocHall(this);
    mDetect->setVesc(vesc);

    mParamTab->addParamRow(mVesc->mcConfig(), "foc_sl_erpm");
    mParamTab->addParamRow(mVesc->mcConfig(), "foc_hall_table__0");
    mParamTab->addParamRow(mVesc->mcConfig(), "foc_hall_table__1");
    mParamTab->addParamRow(mVesc->mcConfig(), "foc_hall_table__2");
    mParamTab->addParamRow(mVesc->mcConfig(), "foc_hall_table__3");
    mParamTab->addParamRow(mVesc->mcConfig(), "foc_hall_table__4");
    mParamTab->addParamRow(mVesc->mcConfig(), "foc_hall_table__5");
    mParamTab->addParamRow(mVesc->mcConfig(), "foc_hall_table__6");
    mParamTab->addParamRow(mVesc->mcConfig(), "foc_hall_table__7");

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(mParamTab);
    layout->addWidget(mDetect);
    setLayout(layout);
}

int FocHallPage::nextId() const
{
    return SetupWizardMotor::Page_Conclusion;
}

bool FocHallPage::validatePage()
{
    mVesc->commands()->setMcconf();
    return true;
}

ConclusionPage::ConclusionPage(VescInterface *vesc, QWidget *parent)
    : QWizardPage(parent)
{
    mVesc = vesc;
    setTitle(tr("Conclusion"));

    mLabel = new QLabel(tr("You have finished the motor setup for the VESC®. The next step "
                           "is to run the app setup to configure what type of interface "
                           "to use with your VESC."));
    mLabel->setWordWrap(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(mLabel);
    setLayout(layout);
}

int ConclusionPage::nextId() const
{
    return -1;
}
