/*
    Copyright 2017 - 2019 Benjamin Vedder	benjamin@vedder.se

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

#include "utility.h"
#ifdef Q_OS_IOS
#include "ios/src/setIosParameters.h"
#endif
#include <cmath>
#include <QProgressDialog>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QNetworkRequest>
#include <QDebug>
#include <QNetworkReply>
#include <QMessageBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QtGlobal>
#include <QNetworkInterface>
#include <QDirIterator>
#include <QPixmapCache>

#include "maddy/parser.h"

#ifdef Q_OS_ANDROID
#include <QtAndroid>
#include <QAndroidJniObject>
#include <QAndroidJniEnvironment>
#endif

QMap<QString, QColor> Utility::mAppColors = {
    {"lightestBackground", QColor(80,80,80)},
    {"lightBackground", QColor(66,66,66)},
    {"normalBackground", QColor(48,48,48)},
    {"darkBackground", QColor(39,39,39)},
    {"normalText", QColor(180,180,180)},
    {"lightText", QColor(220,220,220)},
    {"disabledText", QColor(127,127,127)},
    {"lightAccent", QColor(129,212,250)},
    {"darkAccent", QColor(71,117,137)},
    {"pink", QColor(219,98,139)},
    {"red", QColor(200,52,52)},
    {"orange", QColor(206,125,44)},
    {"yellow", QColor(210,210,127)},
    {"green", QColor(127,200,127)},
    {"cyan", QColor(79,203,203)},
    {"blue", QColor(77,127,196)},
    {"magenta", QColor(157,127,210)},
    {"white", QColor(255,255,255)},
    {"black", QColor(0,0,0)},
    {"plot_graph1", QColor(77,127,196)},
    {"plot_graph2", QColor(200,52,52)},
    {"plot_graph3", QColor(127,200,127)},
    {"plot_graph4", QColor(206,125,44)},
    {"plot_graph5", QColor(210,210,127)},
    {"plot_graph6", QColor(79,203,203)},
    {"plot_graph7", QColor(157,127,210)},
    {"plot_graph8", QColor(129,212,250)},
    {"plot_graph9", QColor(180,180,180)},
    {"plot_graph10", QColor(219,98,139)},
    {"plot_graph11", QColor(250,250,200)},
};

bool Utility::isDark = false;

Utility::Utility(QObject *parent) : QObject(parent)
{

}

double Utility::map(double x, double in_min, double in_max, double out_min, double out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

float Utility::throttle_curve(float val, float curve_acc, float curve_brake, int mode)
{
    float ret = 0.0;
    float val_a = fabsf(val);

    if (val < -1.0) {
        val = -1.0;
    }

    if (val > 1.0) {
        val = 1.0;
    }

    float curve;
    if (val >= 0.0) {
        curve = curve_acc;
    } else {
        curve = curve_brake;
    }

    // See
    // http://math.stackexchange.com/questions/297768/how-would-i-create-a-exponential-ramp-function-from-0-0-to-1-1-with-a-single-val
    if (mode == 0) { // Power
        if (curve >= 0.0) {
            ret = 1.0 - powf(1.0 - val_a, 1.0 + curve);
        } else {
            ret = powf(val_a, 1.0 - curve);
        }
    } else if (mode == 1) { // Exponential
        if (fabsf(curve) < 1e-10) {
            ret = val_a;
        } else {
            if (curve >= 0.0) {
                ret = 1.0 - ((expf(curve * (1.0 - val_a)) - 1.0) / (expf(curve) - 1.0));
            } else {
                ret = (expf(-curve * val_a) - 1.0) / (expf(-curve) - 1.0);
            }
        }
    } else if (mode == 2) { // Polynomial
        if (curve >= 0.0) {
            ret = 1.0 - ((1.0 - val_a) / (1.0 + curve * val_a));
        } else {
            ret = val_a / (1.0 - curve * (1.0 - val_a));
        }
    } else { // Linear
        ret = val_a;
    }

    if (val < 0.0) {
        ret = -ret;
    }

    return ret;
}

bool Utility::autoconnectBlockingWithProgress(VescInterface *vesc, QWidget *parent)
{
    if (!vesc) {
        return false;
    }

    QProgressDialog dialog("Autoconnecting...", QString(), 0, 0, parent);
    dialog.setWindowModality(Qt::WindowModal);
    dialog.show();

    bool res = vesc->autoconnect();

    if (!res) {
        vesc->emitMessageDialog(QObject::tr("Autoconnect"),
                                QObject::tr("Could not autoconnect. Make sure that the USB cable is plugged in "
                                            "and that the VESC is powered."),
                                false);
    }

    return res;
}

void Utility::checkVersion(VescInterface *vesc)
{
    QString version = QString::number(VT_VERSION);
    QUrl url("https://vesc-project.com/vesctool-version.html");
    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    QNetworkReply *reply = manager.get(request);
    QEventLoop loop;
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    QString res = QString::fromUtf8(reply->readAll());

    if (res.startsWith("vesctoolversion")) {
        res.remove(0, 15);
        res.remove(res.indexOf("vesctoolversion"), res.size());

        if (res.toDouble() > version.toDouble()) {
            if (vesc) {
                vesc->emitStatusMessage("A new version of VESC Tool is available", true);
                vesc->emitMessageDialog(QObject::tr("New Software Available"),
                                        QObject::tr("A new version of VESC Tool is available. Go to "
                                                    "<a href=\"http://vesc-project.com/\">http://vesc-project.com/</a>"
                                                    " to download it and get all the latest features."),
                                        true);
            } else {
                qDebug() << "A new version of VESC Tool is available. Go to vesc-project.com to download it "
                            "and get all the latest features.";
            }
        }
    } else {
        qWarning() << res;
    }

    reply->abort();
    reply->deleteLater();
}

QString Utility::fwChangeLog()
{
    QFile cl("://res/firmwares/CHANGELOG.md");
    if (cl.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString::fromUtf8(cl.readAll());
    } else {
        return "";
    }
}

QString Utility::vescToolChangeLog()
{
    QFile cl("://res/CHANGELOG.md");
    if (cl.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString::fromUtf8(cl.readAll());
    } else {
        return "";
    }
}

QString Utility::aboutText()
{
    return tr("<b>VESC® Tool %1</b><br>"
          #if VT_IS_TEST_VERSION
              "Test Version %2<br>"
          #endif
          #if defined(VER_ORIGINAL)
              "Original Version<br>"
          #elif defined(VER_PLATINUM)
              "Platinum Version<br>"
          #elif defined(VER_GOLD)
              "Gold Version<br>"
          #elif defined(VER_SILVER)
              "Silver Version<br>"
          #elif defined(VER_BRONZE)
              "Bronze Version<br>"
          #elif defined(VER_FREE)
              "Free of Charge Version<br>"
          #endif
              "&copy; Benjamin Vedder 2016 - 2023<br>"
              "<a href=\"mailto:benjamin@vedder.se\">benjamin@vedder.se</a><br>"
              "<a href=\"https://vesc-project.com/\">https://vesc-project.com/</a>").
            arg(QString::number(VT_VERSION, 'f', 2))
        #if VT_IS_TEST_VERSION
            .arg(QString::number(VT_IS_TEST_VERSION))
        #endif
            ;
}

QString Utility::uuid2Str(QByteArray uuid, bool space)
{
    QString strUuid;
    for (int i = 0;i < uuid.size();i++) {
        QString str = QString::number((uint8_t)uuid.at(i), 16).
                rightJustified(2, '0').toUpper();
        strUuid.append(((i > 0 && space) ? " " : "") + str);
    }

    return strUuid;
}

bool Utility::requestFilePermission()
{
#ifdef Q_OS_ANDROID
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    // https://codereview.qt-project.org/#/c/199162/
    QtAndroid::PermissionResult r = QtAndroid::checkPermission("android.permission.WRITE_EXTERNAL_STORAGE");
    if(r == QtAndroid::PermissionResult::Denied) {
        QtAndroid::requestPermissionsSync( QStringList() << "android.permission.WRITE_EXTERNAL_STORAGE", 10000);
        r = QtAndroid::checkPermission("android.permission.WRITE_EXTERNAL_STORAGE");
        if(r == QtAndroid::PermissionResult::Denied) {
            return false;
        }
    }

    return true;
#else
    return true;
#endif
#else
    return true;
#endif
}

bool Utility::requestBleScanPermission()
{
#ifdef Q_OS_ANDROID
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    QtAndroid::PermissionResult r = QtAndroid::checkPermission("android.permission.BLUETOOTH_SCAN");
    if(r == QtAndroid::PermissionResult::Denied) {
        QtAndroid::requestPermissionsSync( QStringList() << "android.permission.BLUETOOTH_SCAN", 10000);
        r = QtAndroid::checkPermission("android.permission.BLUETOOTH_SCAN");
        if(r == QtAndroid::PermissionResult::Denied) {
            return false;
        }
    }

    return true;
#else
    return true;
#endif
#else
    return true;
#endif
}

bool Utility::requestBleConnectPermission()
{
#ifdef Q_OS_ANDROID
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    QtAndroid::PermissionResult r = QtAndroid::checkPermission("android.permission.BLUETOOTH_CONNECT");
    if(r == QtAndroid::PermissionResult::Denied) {
        QtAndroid::requestPermissionsSync( QStringList() << "android.permission.BLUETOOTH_CONNECT", 10000);
        r = QtAndroid::checkPermission("android.permission.BLUETOOTH_CONNECT");
        if(r == QtAndroid::PermissionResult::Denied) {
            return false;
        }
    }

    return true;
#else
    return true;
#endif
#else
    return true;
#endif
}

bool Utility::hasLocationPermission()
{
#ifdef Q_OS_ANDROID
    QtAndroid::PermissionResult r = QtAndroid::checkPermission("android.permission.ACCESS_FINE_LOCATION");
    if (r == QtAndroid::PermissionResult::Denied) {
        return false;
    } else {
        return true;
    }
#else
    return true;
#endif
}

void Utility::keepScreenOn(bool on)
{
#ifdef Q_OS_IOS

#endif
#ifdef Q_OS_ANDROID
    QtAndroid::runOnAndroidThread([on]{
        QAndroidJniObject activity = QtAndroid::androidActivity();
        if (activity.isValid()) {
            QAndroidJniObject window =
                    activity.callObjectMethod("getWindow", "()Landroid/view/Window;");

            if (window.isValid()) {
                const int FLAG_KEEP_SCREEN_ON = 128;
                if (on) {
                    window.callMethod<void>("addFlags", "(I)V", FLAG_KEEP_SCREEN_ON);
                } else {
                    window.callMethod<void>("clearFlags", "(I)V", FLAG_KEEP_SCREEN_ON);
                }
            }
        }
        QAndroidJniEnvironment env;
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
        }
    });
#else
    (void)on;
#endif
}

void Utility::allowScreenRotation(bool enabled)
{
#ifdef Q_OS_ANDROID
    if (enabled) {
        QAndroidJniObject activity = QtAndroid::androidActivity();
        activity.callMethod<void>("setRequestedOrientation", "(I)V",
                                  QAndroidJniObject::getStaticField<int>("android.content.pm.ActivityInfo",
                                                                         "SCREEN_ORIENTATION_UNSPECIFIED"));
    } else {
        QAndroidJniObject activity = QtAndroid::androidActivity();
        activity.callMethod<void>("setRequestedOrientation", "(I)V",
                                  QAndroidJniObject::getStaticField<int>("android.content.pm.ActivityInfo",
                                                                         "SCREEN_ORIENTATION_PORTRAIT"));
    }
#else
    (void)enabled;
#endif
}

bool Utility::waitSignal(QObject *sender, QString signal, int timeoutMs)
{
    QEventLoop loop;
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    timeoutTimer.start(timeoutMs);
    auto conn1 = QObject::connect(sender, signal.toLocal8Bit().data(), &loop, SLOT(quit()));
    auto conn2 = QObject::connect(&timeoutTimer, SIGNAL(timeout()), &loop, SLOT(quit()));
    loop.exec();

    QObject::disconnect(conn1);
    QObject::disconnect(conn2);

    return timeoutTimer.isActive();
}

void Utility::sleepWithEventLoop(int timeMs)
{
    QEventLoop loop;
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    timeoutTimer.start(timeMs);
    auto conn1 = QObject::connect(&timeoutTimer, SIGNAL(timeout()), &loop, SLOT(quit()));
    loop.exec();
    QObject::disconnect(conn1);
}

QString Utility::detectAllFoc(VescInterface *vesc,
                              bool detect_can, double max_power_loss, double min_current_in,
                              double max_current_in, double openloop_rpm, double sl_erpm)
{
    QString res;
    bool detectOk = true;

    vesc->commands()->disableAppOutput(180000, true);
    Utility::sleepWithEventLoop(100);

    vesc->commands()->detectAllFoc(detect_can, max_power_loss, min_current_in,
                                   max_current_in, openloop_rpm, sl_erpm);

    QEventLoop loop;
    QTimer timeoutTimer;
    QTimer pollTimer;
    timeoutTimer.setSingleShot(true);
    timeoutTimer.start(180000);
    pollTimer.start(100);

    int resDetect = 0;
    auto conn = connect(vesc->commands(), &Commands::detectAllFocReceived,
                        [&resDetect, &loop](int result) {
        resDetect = result;
        loop.quit();
    });

    QString pollRes;
    auto conn2 = connect(&pollTimer, &QTimer::timeout,
                        [&pollRes, &loop, &vesc]() {
        if (!vesc->isPortConnected()) {
            pollRes = "VESC disconnected during detection.";
            loop.quit();
        }
    });

    connect(&timeoutTimer, SIGNAL(timeout()), &loop, SLOT(quit()));
    loop.exec();

    disconnect(conn);
    disconnect(conn2);

    if (timeoutTimer.isActive() && pollRes.isEmpty()) {
        if (resDetect >= 0) {
            ConfigParams *p = vesc->mcConfig();
            ConfigParams *ap = vesc->appConfig();

            // MCConf should have been sent after the detection
            vesc->commands()->getAppConf();
            waitSignal(ap, SIGNAL(updated()), 4000);

            auto genRes = [&p, &ap]() {
                QString sensors;
                switch (p->getParamEnum("foc_sensor_mode")) {
                case 0: sensors = "Sensorless"; break;
                case 1: sensors = "Encoder"; break;
                case 2: sensors = "Hall Sensors"; break;
                default: break; }
                return QString("VESC ID            : %1\n"
                               "Motor current      : %2 A\n"
                               "Motor R            : %3 mΩ\n"
                               "Motor L            : %4 µH\n"
                               "Motor Lq-Ld        : %5 µH\n"
                               "Motor Flux Linkage : %6 mWb\n"
                               "Temp Comp          : %7\n"
                               "Sensors            : %8").
                        arg(ap->getParamInt("controller_id")).
                        arg(p->getParamDouble("l_current_max"), 0, 'f', 2).
                        arg(p->getParamDouble("foc_motor_r") * 1e3, 0, 'f', 2).
                        arg(p->getParamDouble("foc_motor_l") * 1e6, 0, 'f', 2).
                        arg(p->getParamDouble("foc_motor_ld_lq_diff") * 1e6, 0, 'f', 2).
                        arg(p->getParamDouble("foc_motor_flux_linkage") * 1e3, 0, 'f', 2).
                        arg(p->getParamBool("foc_temp_comp") ? "True" : "False").
                        arg(sensors);
            };

            QVector<int> canDevs;
            if (detect_can) {
                if (!vesc->commands()->getSendCan()) {
                    res = genRes();
                }

                canDevs = Utility::scanCanVescOnly(vesc);
            } else {
                res = genRes();
            }

            int canLastFwd = vesc->commands()->getSendCan();
            int canLastId = vesc->commands()->getCanSendId();
            vesc->ignoreCanChange(true);

            if (!canDevs.empty()) {
                res += "\n\nVESCs on CAN-bus:";
            }

            for (int id: canDevs) {
                vesc->commands()->setSendCan(true, id);
                if (!checkFwCompatibility(vesc)) {
                    vesc->emitMessageDialog("FW Versions",
                                            "All VESCs must have the latest firmware to perform this operation.",
                                            false, false);
                    break;
                }

                vesc->commands()->getMcconf();
                waitSignal(p, SIGNAL(updated()), 4000);
                vesc->commands()->getAppConf();
                waitSignal(ap, SIGNAL(updated()), 4000);
                res += "\n\n" + genRes();
            }

            vesc->commands()->setSendCan(canLastFwd, canLastId);
            vesc->ignoreCanChange(false);
            vesc->commands()->getMcconf();
            waitSignal(p, SIGNAL(updated()), 4000);
            vesc->commands()->getAppConf();
            waitSignal(ap, SIGNAL(updated()), 4000);
        } else {
            QString reason;
            switch (resDetect) {
            case -1: reason = "Peristent fault, check realtime data page"; break;
            case -10: reason = "Flux linkage detection failed"; break;
            case -50: reason = "CAN detection timeout"; break;
            case -51: reason = "CAN detection failed"; break;
            case -100 + FAULT_CODE_NONE: reason = "No fault, detection failed for an unknown reason"; break;
            case -100 + FAULT_CODE_OVER_VOLTAGE: reason = "Over voltage fault, check voltage is below set limit"; break;
            case -100 + FAULT_CODE_UNDER_VOLTAGE: reason = "Under voltage fault, check voltage is above set limit. If using a power supply make sure the current limit is high enough."; break;
            case -100 + FAULT_CODE_DRV: reason = "DRV fault, hardware fault occured. Check there are no shorts"; break;
            case -100 + FAULT_CODE_ABS_OVER_CURRENT: reason = "Overcurrent fault, Check there are no shorts and ABS Overcurrent limit is sensible"; break;
            case -100 + FAULT_CODE_OVER_TEMP_FET: reason = "Mosfet Overtemperature fault, Mosfets overheated, check for shorts. Cool down device"; break;
            case -100 + FAULT_CODE_OVER_TEMP_MOTOR: reason = "Motor Overtemperature fault, Motor overheaded, is the current limit OK?"; break;
            case -100 + FAULT_CODE_GATE_DRIVER_OVER_VOLTAGE: reason = "Gate Driver over voltage, check for hardware failure"; break;
            case -100 + FAULT_CODE_GATE_DRIVER_UNDER_VOLTAGE: reason = "Gate Driver under voltage, check for hardware failure"; break;
            case -100 + FAULT_CODE_MCU_UNDER_VOLTAGE: reason = "MCU under voltage, check for hardware failure, shorts on outputs"; break;
            case -100 + FAULT_CODE_BOOTING_FROM_WATCHDOG_RESET: reason = "Boot from watchdog reset, software locked up check for firmware corruption"; break;
            case -100 + FAULT_CODE_ENCODER_SPI: reason = "Encoder SPI fault, check encoder connections"; break;
            case -100 + FAULT_CODE_ENCODER_SINCOS_BELOW_MIN_AMPLITUDE: reason = "Encoder SINCOS below min amplitude, check encoder connections and magnet alignment / distance"; break;
            case -100 + FAULT_CODE_ENCODER_SINCOS_ABOVE_MAX_AMPLITUDE: reason = "Encoder SINCOS above max amplitude, check encoder connections and magnet alignment / distance"; break;
            case -100 + FAULT_CODE_FLASH_CORRUPTION: reason = "Flash corruption, reflash firmware immediately!"; break;
            case -100 + FAULT_CODE_HIGH_OFFSET_CURRENT_SENSOR_1: reason = "High offset on current sensor 1, check for hardware failure"; break;
            case -100 + FAULT_CODE_HIGH_OFFSET_CURRENT_SENSOR_2: reason = "High offset on current sensor 2, check for hardware failure"; break;
            case -100 + FAULT_CODE_HIGH_OFFSET_CURRENT_SENSOR_3: reason = "High offset on current sensor 3, check for hardware failure"; break;
            case -100 + FAULT_CODE_UNBALANCED_CURRENTS: reason = "Unbalanced currents, check for hardware failure"; break;
            case -100 + FAULT_CODE_BRK: reason = "BRK, hardware protection triggered, check for shorts or possible hardware failure"; break;
            case -100 + FAULT_CODE_RESOLVER_LOT: reason = "Encoder/Resolver: Loss of tracking"; break;
            case -100 + FAULT_CODE_RESOLVER_DOS: reason = "Encoder/Resolver: Degradation of signal"; break;
            case -100 + FAULT_CODE_RESOLVER_LOS: reason = "Encoder/Resolver: Loss of signal"; break;
            case -100 + FAULT_CODE_FLASH_CORRUPTION_APP_CFG: reason = "Flash corruption, App config corrupt, rewrite app config to restore"; break;
            case -100 + FAULT_CODE_FLASH_CORRUPTION_MC_CFG: reason = "Flash corruption, Motor config corrupt, rewrite motor config to restore"; break;
            case -100 + FAULT_CODE_ENCODER_NO_MAGNET: reason = "Encoder no magnet, magnet is too weak or too far from the encoder"; break;
            case -100 + FAULT_CODE_ENCODER_MAGNET_TOO_STRONG: reason = "Magnet too strong, magnet is too strong or too close to the encoder"; break;
            case -100 + FAULT_CODE_PHASE_FILTER: reason = "Phase filter fault, invalid phase filter readings"; break;
            case -100 + FAULT_CODE_ENCODER_FAULT: reason = "Encoder fault, check encoder connections and alignment"; break;

            default: reason = QString::number(resDetect); break;
            }

            res = QString("Detection failed. Reason:\n%1").arg(reason);
            detectOk = false;
        }
    } else {
        if (!pollRes.isEmpty()) {
            res = QString("Detection failed. Reason:\n%1").arg(pollRes);
        } else {
            res = "Detection timed out.";
        }
        detectOk = false;
    }

    if (detectOk) {
        res.prepend("Success!\n\n");
    }

    vesc->commands()->disableAppOutput(0, true);

    return res;
}

QVector<double> Utility::measureRLBlocking(VescInterface *vesc)
{
    QVector<double> res;

    vesc->commands()->measureRL();

    auto conn = connect(vesc->commands(), &Commands::motorRLReceived, [&res](double r, double l, double ld_lq_diff) {
        res.append(r);
        res.append(l);
        res.append(ld_lq_diff);
    });

    waitSignal(vesc->commands(), SIGNAL(motorRLReceived(double, double, double)), 8000);
    disconnect(conn);

    return res;
}

double Utility::measureLinkageOpenloopBlocking(VescInterface *vesc, double current,
                                               double erpm_per_sec, double low_duty, double resistance, double inductance)
{
    double res = -1.0;

    vesc->commands()->measureLinkageOpenloop(current, erpm_per_sec, low_duty, resistance, inductance);

    auto conn = connect(vesc->commands(), &Commands::motorLinkageReceived, [&res](double flux_linkage) {
        res = flux_linkage;
    });

    waitSignal(vesc->commands(), SIGNAL(motorLinkageReceived(double)), 12000);
    disconnect(conn);

    return res;
}

QVector<int> Utility::measureHallFocBlocking(VescInterface *vesc, double current)
{
    QVector<int> resDetect;
    vesc->commands()->measureHallFoc(current);

    auto conn = connect(vesc->commands(), &Commands::focHallTableReceived,
                        [&resDetect](QVector<int> hall_table, int res) {
            resDetect.append(res);
            resDetect.append(hall_table);
    });

    bool rx = waitSignal(vesc->commands(), SIGNAL(focHallTableReceived(QVector<int>, int)), 25000);
    disconnect(conn);

    if (!rx) {
        resDetect.append(-10);
    }

    return resDetect;
}

ENCODER_DETECT_RES Utility::measureEncoderBlocking(VescInterface *vesc, double current)
{
    ENCODER_DETECT_RES resDetect;
    vesc->commands()->measureEncoder(current);

    auto conn = connect(vesc->commands(), &Commands::encoderParamReceived,
                        [&resDetect](ENCODER_DETECT_RES res) {
            resDetect = res;
    });

    waitSignal(vesc->commands(), SIGNAL(encoderParamReceived(ENCODER_DETECT_RES)), 50000);
    disconnect(conn);

    return resDetect;
}

bool Utility::waitMotorStop(VescInterface *vesc, double erpmTres, int timeoutMs)
{
    QTimer t;
    t.start(timeoutMs);
    t.setSingleShot(true);

    auto val = getMcValuesBlocking(vesc);
    while (t.isActive() && fabs(val.rpm) > erpmTres) {
        val = getMcValuesBlocking(vesc);
        sleepWithEventLoop(100);
    }

    return t.isActive();
}

bool Utility::resetInputCan(VescInterface *vesc, QVector<int> canIds)
{
    bool res = true;

    bool canLastFwd = vesc->commands()->getSendCan();
    int canLastId = vesc->commands()->getCanSendId();

    vesc->ignoreCanChange(true);

    auto update = [vesc]() {
        bool res = true;
        ConfigParams *ap = vesc->appConfig();

        if (isConnectedToHwVesc(vesc)) {
            if (!checkFwCompatibility(vesc)) {
                vesc->emitMessageDialog("FW Versions",
                                        "All VESCs must have the latest firmware to perform this operation.",
                                        false, false);
                res = false;
                qWarning() << "Incompatible firmware";
                return false;
            }

            vesc->commands()->getAppConf();
            res = waitSignal(ap, SIGNAL(updated()), 4000);

            if (!res) {
                qWarning() << "Appconf not received";
                return false;
            }

            int canId = ap->getParamInt("controller_id");

            int canStatus;
            int canStatus2;
            bool has_bitfield_params = ap->hasParam("can_status_msgs_r1");
            if (has_bitfield_params) {
                canStatus = ap->getParamInt("can_status_msgs_r1");
                canStatus2 = ap->getParamInt("can_status_msgs_r2");
            } else {
                canStatus = ap->getParamEnum("send_can_status");
            }

            vesc->commands()->getAppConfDefault();
            res = waitSignal(ap, SIGNAL(updated()), 4000);

            if (!res) {
                qWarning() << "Default appconf not received";
                return false;
            }

            ap->updateParamInt("controller_id", canId);
            if (has_bitfield_params) {
                ap->updateParamInt("can_status_msgs_r1", canStatus);
                ap->updateParamInt("can_status_msgs_r2", canStatus2);
            } else {
                ap->updateParamEnum("send_can_status", canStatus);
            }

            vesc->commands()->setAppConf();
            res = waitSignal(vesc->commands(), SIGNAL(ackReceived(QString)), 4000);

            if (!res) {
                qWarning() << "Appconf set no ack received";
                return false;
            }
        }

        return res;
    };

    // Local VESC first
    vesc->commands()->setSendCan(false);
    res = update();

    // All VESCs on CAN-bus
    if (res) {
        for (int id: canIds) {
            vesc->commands()->setSendCan(true, id);

            res = update();
            if (!res) {
                break;
            }
        }
    }

    vesc->commands()->setSendCan(canLastFwd, canLastId);

    if (isConnectedToHwVesc(vesc)) {
        vesc->commands()->getAppConf();
        ConfigParams *ap = vesc->appConfig();
        if (!waitSignal(ap, SIGNAL(updated()), 4000)) {
            qWarning() << "Appconf not received";
            res = false;
        }
    }

    vesc->ignoreCanChange(false);

    return res;
}

bool Utility::setBatteryCutCan(VescInterface *vesc, QVector<int> canIds, double cutStart, double cutEnd)
{
    vesc->mcConfig()->updateParamDouble("l_battery_cut_start", cutStart);
    vesc->mcConfig()->updateParamDouble("l_battery_cut_end", cutEnd);
    return setMcParamsFromCurrentConfigAllCan(vesc, canIds, {"l_battery_cut_start", "l_battery_cut_end"});
}

bool Utility::setBatteryCutCanFromCurrentConfig(VescInterface *vesc, QVector<int> canIds, bool cautious)
{
    ConfigParams *p = vesc->mcConfig();

    int battType = p->getParamEnum("si_battery_type");
    int cells = p->getParamInt("si_battery_cells");
    double start = -1.0;
    double end = -1.0;

    if (cautious) {
        if (battType == 0) {
            start = 3.4;
            end = 3.0;
        } else if (battType == 1) {
            start = 2.9;
            end = 2.6;
        } else {
            return false;
        }
    } else {
        if (battType == 0) {
            start = 2.7;
            end = 2.5;
        } else if (battType == 1) {
            start = 2.5;
            end = 2.2;
        } else {
            return false;
        }
    }

    start *= (double)cells;
    end *= (double)cells;

    vesc->mcConfig()->updateParamDouble("l_battery_cut_start", start);
    vesc->mcConfig()->updateParamDouble("l_battery_cut_end", end);
    return setMcParamsFromCurrentConfigAllCan(vesc, canIds, {"l_battery_cut_start", "l_battery_cut_end",
                                              "si_battery_type", "si_battery_cells"});
}

/**
 * @brief Utility::setMcParamsFromCurrentConfigAllCan
 * Take the list of parameters from the current config and apply them on all VESCs on the CAN-bus (including the local one)
 *
 * @param vesc
 * VescInterface pointer
 *
 * @param canIds
 * A list with CAN-IDs to send the parameters to
 *
 * @param params
 * The motor configuration parameters to set
 *
 * @return
 * True for success, false otherwise.
 */
bool Utility::setMcParamsFromCurrentConfigAllCan(VescInterface *vesc, QVector<int> canIds, QStringList params)
{
    bool res = true;

    ConfigParams *config = vesc->mcConfig();
    QVector<QPair<QString, ConfigParam>> paramVec;

    for (auto s: params) {
        paramVec.append(qMakePair(s, config->getParamCopy(s)));
    }

    auto updateConf = [&vesc, &config, &paramVec]() {
        if (!checkFwCompatibility(vesc)) {
            vesc->emitMessageDialog("FW Versions",
                                    "All VESCs must have the latest firmware to perform this operation.",
                                    false, false);
            return false;
        }

        vesc->commands()->getMcconf();

        if (!waitSignal(config, SIGNAL(updated()), 4000)) {
            vesc->emitMessageDialog("Read Motor Configuration",
                                    "Could not read motor configuration.",
                                    false, false);

            return false;
        }

        for (auto p: paramVec) {
            config->updateParamFromOther(p.first, p.second, nullptr);
        }

        vesc->commands()->setMcconf(false);

        if (!waitSignal(vesc->commands(), SIGNAL(ackReceived(QString)), 4000)) {
            vesc->emitMessageDialog("Write Motor Configuration",
                                    "Could not write motor configuration.",
                                    false, false);

            return false;
        }

        return true;
    };

    // Start with local VESC
    vesc->canTmpOverride(false, 0);

    if (isConnectedToHwVesc(vesc)) {
        res = updateConf();
    }

    // Now every VESC on the CAN-bus
    if (res) {
        for (int id: canIds) {
            vesc->canTmpOverride(true, id);

            if (isConnectedToHwVesc(vesc)) {
                res = updateConf();
                if (!res) {
                    break;
                }
            }
        }
    }

    vesc->canTmpOverrideEnd();

    vesc->commands()->getMcconf();
    if (!waitSignal(config, SIGNAL(updated()), 4000)) {
        res = false;

        if (!res) {
            vesc->emitMessageDialog("Read Motor Configuration",
                                    "Could not read motor configuration.",
                                    false, false);
        }
    }


    return res;
}

bool Utility::setInvertDirection(VescInterface *vesc, int canId, bool inverted)
{
    bool res = true;

    bool canLastFwd = vesc->commands()->getSendCan();
    int canLastId = vesc->commands()->getCanSendId();

    vesc->ignoreCanChange(true);
    vesc->commands()->setSendCan(canId >= 0, canId);

    if (!checkFwCompatibility(vesc)) {
        vesc->emitMessageDialog("FW Versions",
                                "All VESCs must have the latest firmware to perform this operation.",
                                false, false);
        res = false;
    }

    ConfigParams *p = vesc->mcConfig();

    if (res) {
        vesc->commands()->getMcconf();
        res = waitSignal(p, SIGNAL(updated()), 4000);
    }

    if (res) {
        p->updateParamBool("m_invert_direction", inverted);
        vesc->commands()->setMcconf(false);
        res = waitSignal(vesc->commands(), SIGNAL(ackReceived(QString)), 4000);
    }

    vesc->commands()->setSendCan(canLastFwd, canLastId);
    vesc->commands()->getMcconf();
    if (!waitSignal(p, SIGNAL(updated()), 4000)) {
        res = false;
    }

    vesc->ignoreCanChange(false);

    return res;
}

bool Utility::getInvertDirection(VescInterface *vesc, int canId)
{
    bool res = false;

    bool canLastFwd = vesc->commands()->getSendCan();
    int canLastId = vesc->commands()->getCanSendId();

    vesc->ignoreCanChange(true);
    vesc->commands()->setSendCan(canId >= 0, canId);

    if (!checkFwCompatibility(vesc)) {
        vesc->emitMessageDialog("FW Versions",
                                "All VESCs must have the latest firmware to perform this operation.",
                                false, false);
        vesc->commands()->setSendCan(canLastFwd, canLastId);
        vesc->ignoreCanChange(false);
        return false;
    }

    ConfigParams *p = vesc->mcConfig();
    vesc->commands()->getMcconf();
    waitSignal(p, SIGNAL(updated()), 4000);
    res = p->getParamBool("m_invert_direction");

    vesc->commands()->setSendCan(canLastFwd, canLastId);
    vesc->commands()->getMcconf();
    waitSignal(p, SIGNAL(updated()), 4000);

    vesc->ignoreCanChange(false);

    return res;
}

QString Utility::testDirection(VescInterface *vesc, int canId, double duty, int ms)
{
    vesc->commands()->disableAppOutput(ms, true);
    Utility::sleepWithEventLoop(100);

    vesc->canTmpOverride(canId >= 0, canId);

    if (!checkFwCompatibility(vesc)) {
        vesc->emitMessageDialog("FW Versions",
                                "All VESCs must have the latest firmware to perform this operation.",
                                false, false);
        vesc->canTmpOverrideEnd();
        return "FW not up to date";
    }

    QEventLoop loop;
    QTimer timeoutTimer;
    QTimer pollTimer;
    timeoutTimer.setSingleShot(true);
    timeoutTimer.start(ms);
    pollTimer.start(40);

    QString pollRes = "Ok";
    auto conn = connect(&pollTimer, &QTimer::timeout,
                        [&pollRes, &loop, &vesc, &duty, &ms, &timeoutTimer]() {
        if (!vesc->isPortConnected()) {
            pollRes = "VESC disconnected.";
            loop.quit();
        } else {
            double d = 2.0 * duty * (double)(ms - timeoutTimer.remainingTime()) / (double)ms;
            if (fabs(d) > fabs(duty)) {
                d = duty;
            }
            vesc->commands()->setDutyCycle(d);
        }
    });

    connect(&timeoutTimer, SIGNAL(timeout()), &loop, SLOT(quit()));
    loop.exec();

    disconnect(conn);
    vesc->commands()->setCurrent(0.0);
    vesc->canTmpOverrideEnd();

    return pollRes;
}

/**
 * @brief Utility::restoreConfAll
 * Restore the VESC configuration to the default values.
 *
 * @param vesc
 * Pointer to a connected VescInterface instance.
 *
 * @param can
 * Scan CAN-bus and restore all devices.
 *
 * @param mc
 * Restore mc configuration.
 *
 * @param app
 * Restore app configuration.
 *
 * @return
 * true for success, false otherwise.
 */
bool Utility::restoreConfAll(VescInterface *vesc, bool can, bool mc, bool app)
{
    bool res = true;

    bool canLastFwd = vesc->commands()->getSendCan();
    int canLastId = vesc->commands()->getCanSendId();

    vesc->ignoreCanChange(true);
    vesc->commands()->setSendCan(false);

    auto update = [mc, app, vesc]() {
        bool res = true;
        if (!isConnectedToHwVesc(vesc)) {
            return true;
        }

        if (!checkFwCompatibility(vesc)) {
            vesc->emitMessageDialog("FW Versions",
                                    "All VESCs must have the latest firmware to perform this operation.",
                                    false, false);
            return false;
        }

        if (mc) {
            ConfigParams *p = vesc->mcConfig();
            vesc->commands()->getMcconfDefault();
            res = waitSignal(p, SIGNAL(updated()), 4000);

            if (res) {
                vesc->commands()->setMcconf(false);
                res = waitSignal(vesc->commands(), SIGNAL(ackReceived(QString)), 4000);
            } else {
                return false;
            }
        }

        if (app) {
            ConfigParams *p = vesc->appConfig();
            vesc->commands()->getAppConfDefault();
            res = waitSignal(p, SIGNAL(updated()), 4000);

            if (res) {
                vesc->commands()->setAppConf();
                res = waitSignal(vesc->commands(), SIGNAL(ackReceived(QString)), 4000);
            } else {
                return false;
            }
        }

        return res;
    };

    res = update();

    if (res && can) {
        QVector<int> canDevs = Utility::scanCanVescOnly(vesc);

        for (int d: canDevs) {
            vesc->commands()->setSendCan(true, d);
            res = update();
            if (!res) {
                break;
            }
        }
    }

    vesc->commands()->setSendCan(canLastFwd, canLastId);
    vesc->ignoreCanChange(false);

    if (can) {
        if (!isConnectedToHwVesc(vesc)) {
            return res;
        }

        if (mc) {
            ConfigParams *p = vesc->mcConfig();
            vesc->commands()->getMcconf();
            if (!waitSignal(p, SIGNAL(updated()), 4000)) {
                res = false;
                qWarning() << "Could not restore mc conf";
            }
        }

        if (app) {
            ConfigParams *p = vesc->appConfig();
            vesc->commands()->getAppConf();
            if (!waitSignal(p, SIGNAL(updated()), 4000)) {
                res = false;
                qWarning() << "Could not restore app conf";
            }
        }
    }

    return res;
}

bool Utility::almostEqual(double A, double B, double eps)
{
    return fabs(A - B) <= eps * fmax(1.0f, fmax(fabs(A), fabs(B)));
}

bool Utility::createParamParserC(VescInterface *vesc, QString filename)
{
    if (filename.toLower().endsWith(".c") || filename.toLower().endsWith(".h")) {
        filename.chop(2);
    }

    QString sourceFileName = filename + ".c";
    QString headerFileName = filename + ".h";

    QFile sourceFile(sourceFileName);
    if (!sourceFile.open(QIODevice::WriteOnly)) {
        qWarning() << tr("Could not open %1 for writing").arg(sourceFileName);
        return false;
    }

    QFile headerFile(headerFileName);
    if (!headerFile.open(QIODevice::WriteOnly)) {
        qWarning() << tr("Could not open %1 for writing").arg(headerFileName);
        return false;
    }

    QTextStream outSource(&sourceFile);
    QTextStream outHeader(&headerFile);
    QFileInfo headerInfo(headerFile);
    QString headerNameStr = headerInfo.fileName().toUpper().replace(".", "_") + "_";
    QString prefix = headerInfo.fileName();
    prefix.chop(2);

    outHeader << "// This file is autogenerated by VESC Tool\n\n";

    outHeader << "#ifndef " + headerNameStr + "\n";
    outHeader << "#define " + headerNameStr + "\n\n";

    outHeader << "#include \"datatypes.h\"\n";
    outHeader << "#include <stdint.h>\n";
    outHeader << "#include <stdbool.h>\n\n";

    outHeader << "// Constants\n";
    outHeader << "#define MCCONF_SIGNATURE\t\t" << vesc->mcConfig()->getSignature() << "\n";
    outHeader << "#define APPCONF_SIGNATURE\t\t" << vesc->appConfig()->getSignature() << "\n\n";

    outHeader << "// Functions\n";
    outHeader << "int32_t " << prefix << "_serialize_mcconf(uint8_t *buffer, const mc_configuration *conf);\n";
    outHeader << "int32_t " << prefix << "_serialize_appconf(uint8_t *buffer, const app_configuration *conf);\n\n";

    outHeader << "bool " << prefix << "_deserialize_mcconf(const uint8_t *buffer, mc_configuration *conf);\n";
    outHeader << "bool " << prefix << "_deserialize_appconf(const uint8_t *buffer, app_configuration *conf);\n\n";

    outHeader << "void " << prefix << "_set_defaults_mcconf(mc_configuration *conf);\n";
    outHeader << "void " << prefix << "_set_defaults_appconf(app_configuration *conf);\n\n";

    outHeader << "// " + headerNameStr + "\n";
    outHeader << "#endif\n";

    outSource << "// This file is autogenerated by VESC Tool\n\n";
    outSource << "#include <string.h>\n";
    outSource << "#include \"buffer.h\"\n";
    outSource << "#include \"conf_general.h\"\n";
    outSource << "#include \"" << headerInfo.fileName() << "\"\n\n";

    outSource << "int32_t " << prefix << "_serialize_mcconf(uint8_t *buffer, const mc_configuration *conf) {\n";
    outSource << "\t" << "int32_t ind = 0;\n\n";
    outSource << "\t" << "buffer_append_uint32(buffer, MCCONF_SIGNATURE, &ind);\n\n";
    serialFunc(vesc->mcConfig(), outSource);
    outSource << "\n\t" << "return ind;\n";
    outSource << "}\n\n";

    outSource << "int32_t " << prefix << "_serialize_appconf(uint8_t *buffer, const app_configuration *conf) {\n";
    outSource << "\t" << "int32_t ind = 0;\n\n";
    outSource << "\t" << "buffer_append_uint32(buffer, APPCONF_SIGNATURE, &ind);\n\n";
    serialFunc(vesc->appConfig(), outSource);
    outSource << "\n\t" << "return ind;\n";
    outSource << "}\n\n";

    outSource << "bool " << prefix << "_deserialize_mcconf(const uint8_t *buffer, mc_configuration *conf) {\n";
    outSource << "\t" << "int32_t ind = 0;\n\n";
    outSource << "\t" << "uint32_t signature = buffer_get_uint32(buffer, &ind);\n";
    outSource << "\t" << "if (signature != MCCONF_SIGNATURE) {\n";
    outSource << "\t\t" << "return false;\n";
    outSource << "\t" << "}\n\n";
    deserialFunc(vesc->mcConfig(), outSource);
    outSource << "\n\t" << "return true;\n";
    outSource << "}\n\n";

    outSource << "bool " << prefix << "_deserialize_appconf(const uint8_t *buffer, app_configuration *conf) {\n";
    outSource << "\t" << "int32_t ind = 0;\n\n";
    outSource << "\t" << "uint32_t signature = buffer_get_uint32(buffer, &ind);\n";
    outSource << "\t" << "if (signature != APPCONF_SIGNATURE) {\n";
    outSource << "\t\t" << "return false;\n";
    outSource << "\t" << "}\n\n";
    deserialFunc(vesc->appConfig(), outSource);
    outSource << "\n\t" << "return true;\n";
    outSource << "}\n\n";

    outSource << "void " << prefix << "_set_defaults_mcconf(mc_configuration *conf) {\n";
    defaultFunc(vesc->mcConfig(), outSource);
    outSource << "}\n\n";

    outSource << "void " << prefix << "_set_defaults_appconf(app_configuration *conf) {\n";
    defaultFunc(vesc->appConfig(), outSource);
    outSource << "}\n";

    outHeader.flush();
    outSource.flush();
    headerFile.close();
    sourceFile.close();

    return true;
}

bool Utility::createParamParserC(ConfigParams *params, QString configName, QString filename)
{
    if (filename.toLower().endsWith(".c") || filename.toLower().endsWith(".h")) {
        filename.chop(2);
    }

    QString sourceFileName = filename + ".c";
    QString headerFileName = filename + ".h";

    QFile sourceFile(sourceFileName);
    if (!sourceFile.open(QIODevice::WriteOnly)) {
        qWarning() << tr("Could not open %1 for writing").arg(sourceFileName);
        return false;
    }

    QFile headerFile(headerFileName);
    if (!headerFile.open(QIODevice::WriteOnly)) {
        qWarning() << tr("Could not open %1 for writing").arg(headerFileName);
        return false;
    }

    QTextStream outSource(&sourceFile);
    QTextStream outHeader(&headerFile);
    QFileInfo headerInfo(headerFile);
    QString headerNameStr = headerInfo.fileName().toUpper().replace(".", "_") + "_";
    QString prefix = headerInfo.fileName();
    prefix.chop(2);

    QString signatureString = QString("%1_SIGNATURE").arg(configName.toUpper());

    outHeader << "// This file is autogenerated by VESC Tool\n\n";

    outHeader << "#ifndef " + headerNameStr + "\n";
    outHeader << "#define " + headerNameStr + "\n\n";

    outHeader << "#include \"datatypes.h\"\n";
    outHeader << "#include <stdint.h>\n";
    outHeader << "#include <stdbool.h>\n\n";

    outHeader << "// Constants\n";
    outHeader << "#define " << signatureString << "\t\t" << params->getSignature() << "\n\n";

    outHeader << "// Functions\n";
    outHeader << "int32_t " << prefix << "_serialize_" << configName.toLower() << "(uint8_t *buffer, const " << configName << " *conf);\n";
    outHeader << "bool " << prefix << "_deserialize_" << configName.toLower() << "(const uint8_t *buffer, " << configName << " *conf);\n";
    outHeader << "void " << prefix << "_set_defaults_" << configName.toLower() << "(" << configName << " *conf);\n\n";

    outHeader << "// " + headerNameStr + "\n";
    outHeader << "#endif\n";

    outSource << "// This file is autogenerated by VESC Tool\n\n";
    outSource << "#include <string.h>\n";
    outSource << "#include \"buffer.h\"\n";
    outSource << "#include \"conf_general.h\"\n";
    outSource << "#include \"" << headerInfo.fileName() << "\"\n\n";

    outSource << "int32_t " << prefix << "_serialize_" << configName.toLower() << "(uint8_t *buffer, const " << configName << " *conf) {\n";
    outSource << "\t" << "int32_t ind = 0;\n\n";
    outSource << "\t" << "buffer_append_uint32(buffer, " << signatureString << ", &ind);\n\n";
    serialFunc(params, outSource);
    outSource << "\n\t" << "return ind;\n";
    outSource << "}\n\n";

    outSource << "bool " << prefix << "_deserialize_" << configName.toLower() << "(const uint8_t *buffer, " << configName << " *conf) {\n";
    outSource << "\t" << "int32_t ind = 0;\n\n";
    outSource << "\t" << "uint32_t signature = buffer_get_uint32(buffer, &ind);\n";
    outSource << "\t" << "if (signature != " << signatureString << ") {\n";
    outSource << "\t\t" << "return false;\n";
    outSource << "\t" << "}\n\n";
    deserialFunc(params, outSource);
    outSource << "\n\t" << "return true;\n";
    outSource << "}\n\n";

    outSource << "void " << prefix << "_set_defaults_" << configName.toLower() << "(" << configName << " *conf) {\n";
    defaultFunc(params, outSource);
    outSource << "}\n\n";

    outHeader.flush();
    outSource.flush();
    headerFile.close();
    sourceFile.close();

    return true;
}

bool Utility::createCompressedConfigC(ConfigParams *params, QString configName, QString filename)
{
    if (filename.toLower().endsWith(".c") || filename.toLower().endsWith(".h")) {
        filename.chop(2);
    }

    QString sourceFileName = filename + ".c";
    QString headerFileName = filename + ".h";

    QFile sourceFile(sourceFileName);
    if (!sourceFile.open(QIODevice::WriteOnly)) {
        qWarning() << tr("Could not open %1 for writing").arg(sourceFileName);
        return false;
    }

    QFile headerFile(headerFileName);
    if (!headerFile.open(QIODevice::WriteOnly)) {
        qWarning() << tr("Could not open %1 for writing").arg(headerFileName);
        return false;
    }

    QTextStream outSource(&sourceFile);
    QTextStream outHeader(&headerFile);
    QFileInfo headerInfo(headerFile);
    QString headerNameStr = headerInfo.fileName().toUpper().replace(".", "_") + "_";
    QString configNameStr = configName.toLower().replace(".", "_") + "_";
    QString prefix = headerInfo.fileName();
    prefix.chop(2);

    QByteArray compressed = params->getCompressedParamsXml();

    outHeader << "// This file is autogenerated by VESC Tool\n\n";

    outHeader << "#ifndef " + headerNameStr + "\n";
    outHeader << "#define " + headerNameStr + "\n\n";

    outHeader << "#include \"datatypes.h\"\n";
    outHeader << "#include <stdint.h>\n";
    outHeader << "#include <stdbool.h>\n\n";

    outHeader << "// Constants\n";
    outHeader << "#define DATA_" << configNameStr.toUpper() << "_SIZE\t\t" << compressed.size() << "\n\n";

    outHeader << "// Variables\n";
    outHeader << "extern uint8_t data_" << configNameStr << "[];\n\n";

    outHeader << "// " + headerNameStr + "\n";
    outHeader << "#endif\n";

    outSource << "// This file is autogenerated by VESC Tool\n\n";
    outSource << "#include \"" << headerInfo.fileName() << "\"\n\n";
    outSource << "uint8_t data_" << configNameStr << "[" << compressed.size() << "] = {\n\t";

    int posCnt = 0;
    for (auto b: compressed) {
        if (posCnt >= 16) {
            posCnt = 0;
            outSource << "\n\t";
        }

        outSource << QString("0x%1, ").arg(quint8(b), 2, 16, QLatin1Char('0'));
        posCnt++;
    }

    outSource << "\n};\n";

    outHeader.flush();
    outSource.flush();
    headerFile.close();
    sourceFile.close();

    return true;
}

uint32_t Utility::crc32c(uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFF;

    for (uint32_t i = 0; i < len;i++) {
        uint32_t byte = data[i];
        crc = crc ^ byte;

        for (int j = 7;j >= 0;j--) {
            uint32_t mask = -(crc & 1);
            crc = (crc >> 1) ^ (0x82F63B78 & mask);
        }
    }

    return ~crc;
}

bool Utility::getFwVersionBlocking(VescInterface *vesc, FW_RX_PARAMS *params)
{
    bool res = false;

    auto conn = connect(vesc->commands(), &Commands::fwVersionReceived,
            [&](FW_RX_PARAMS paramsRx) {
        if (params) {
            *params = paramsRx;
        }
        res = true;
    });

    disconnect(vesc->commands(), SIGNAL(fwVersionReceived(FW_RX_PARAMS)),
               vesc, SLOT(fwVersionReceived(FW_RX_PARAMS)));

    vesc->commands()->getFwVersion();
    waitSignal(vesc->commands(), SIGNAL(fwVersionReceived(FW_RX_PARAMS)), 4000);

    disconnect(conn);

    connect(vesc->commands(), SIGNAL(fwVersionReceived(FW_RX_PARAMS)),
            vesc, SLOT(fwVersionReceived(FW_RX_PARAMS)));

    return res;
}

bool Utility::getFwVersionBlockingCan(VescInterface *vesc, FW_RX_PARAMS *params, int canId)
{
    vesc->canTmpOverride(true, canId);
    bool res = getFwVersionBlocking(vesc, params);
    vesc->canTmpOverrideEnd();
    return res;
}

bool Utility::isConnectedToHwVesc(VescInterface *vesc)
{
    FW_RX_PARAMS paramsRx;
    getFwVersionBlocking(vesc, &paramsRx);
    return paramsRx.hwType == HW_TYPE_VESC;
}

FW_RX_PARAMS Utility::getFwVersionBlocking(VescInterface *vesc)
{
    FW_RX_PARAMS params;
    getFwVersionBlocking(vesc, &params);
    return params;
}

FW_RX_PARAMS Utility::getFwVersionBlockingCan(VescInterface *vesc, int canId)
{
    FW_RX_PARAMS params;
    getFwVersionBlockingCan(vesc, &params, canId);
    return params;
}

MC_VALUES Utility::getMcValuesBlocking(VescInterface *vesc)
{
    MC_VALUES res;

    auto conn = connect(vesc->commands(), &Commands::valuesReceived,
                        [&](MC_VALUES val, unsigned int mask) {
            (void)mask;
            res = val;
    });

    vesc->commands()->getValues();
    waitSignal(vesc->commands(), SIGNAL(valuesReceived(MC_VALUES, unsigned int)), 4000);

    disconnect(conn);

    return res;
}

bool Utility::checkFwCompatibility(VescInterface *vesc)
{
    bool res = false;

    FW_RX_PARAMS params;
    if (getFwVersionBlocking(vesc, &params)) {
        res = vesc->getSupportedFirmwarePairs().contains(qMakePair(params.major, params.minor));
    }

    return res;
}

QVariantList Utility::getNetworkAddresses()
{
    QVariantList res;

    for(QHostAddress a: QNetworkInterface::allAddresses()) {
        if(!a.isLoopback()) {
            if (a.protocol() == QAbstractSocket::IPv4Protocol) {
                res << a.toString();
            }
        }
    }

    return res;
}

void Utility::startGnssForegroundService()
{
#ifdef Q_OS_ANDROID
    QAndroidJniObject::callStaticMethod<void>("com/vedder/vesc/Utils",
                                              "startVForegroundService",
                                              "(Landroid/content/Context;)V",
                                              QtAndroid::androidActivity().object());
#endif
}

void Utility::stopGnssForegroundService()
{
#ifdef Q_OS_ANDROID
    QAndroidJniObject::callStaticMethod<void>("com/vedder/vesc/Utils",
                                              "stopVForegroundService",
                                              "(Landroid/content/Context;)V",
                                              QtAndroid::androidActivity().object());
#endif
}

bool Utility::isBleScanEnabled()
{
#ifdef Q_OS_ANDROID
    return QAndroidJniObject::callStaticMethod<jboolean>("com/vedder/vesc/Utils",
                                                         "checkLocationEnabled",
                                                         "(Landroid/content/Context;)Z",
                                                         QtAndroid::androidActivity().object());
#else
    return true;
#endif
}

QString Utility::strCrc32(QString str)
{
    uint32_t crc = 0xFFFFFFFF;
    auto data = str.toLocal8Bit();

    for (auto b: data) {
        uint32_t byte = quint8(b);
        crc = crc ^ byte;

        for (int j = 7;j >= 0;j--) {
            uint32_t mask = -(crc & 1);
            crc = (crc >> 1) ^ (0x82F63B78 & mask);
        }
    }

    crc = ~crc;

    return QString::number(crc);
}

QString Utility::readInternalImuType(VescInterface *vesc)
{
    QString res = "Timeout";
    auto conn = connect(vesc->commands(), &Commands::printReceived, [&](QString str) {
            res = str;
    });

    vesc->commands()->sendTerminalCmdSync("imu_type_internal");
    waitSignal(vesc->commands(), SIGNAL(printReceived(QString)), 2000);

    disconnect(conn);

    return res;
}

void Utility::llhToXyz(double lat, double lon, double height, double *x, double *y, double *z)
{
    double sinp = sin(lat * M_PI / 180.0);
    double cosp = cos(lat * M_PI / 180.0);
    double sinl = sin(lon * M_PI / 180.0);
    double cosl = cos(lon * M_PI / 180.0);
    double e2 = FE_WGS84 * (2.0 - FE_WGS84);
    double v = RE_WGS84 / sqrt(1.0 - e2 * sinp * sinp);

    *x = (v + height) * cosp * cosl;
    *y = (v + height) * cosp * sinl;
    *z = (v * (1.0 - e2) + height) * sinp;
}

void Utility::xyzToLlh(double x, double y, double z, double *lat, double *lon, double *height)
{
    double e2 = FE_WGS84 * (2.0 - FE_WGS84);
    double r2 = x * x + y * y;
    double za = z;
    double zk = 0.0;
    double sinp = 0.0;
    double v = RE_WGS84;

    while (fabs(za - zk) >= 1E-4) {
        zk = za;
        sinp = za / sqrt(r2 + za * za);
        v = RE_WGS84 / sqrt(1.0 - e2 * sinp * sinp);
        za = z + v * e2 * sinp;
    }

    *lat = (r2 > 1E-12 ? atan(za / sqrt(r2)) : (z > 0.0 ? M_PI / 2.0 : -M_PI / 2.0)) * 180.0 / M_PI;
    *lon = (r2 > 1E-12 ? atan2(y, x) : 0.0) * 180.0 / M_PI;
    *height = sqrt(r2 + za * za) - v;
}

void Utility::createEnuMatrix(double lat, double lon, double *enuMat)
{
    double so = sin(lon * M_PI / 180.0);
    double co = cos(lon * M_PI / 180.0);
    double sa = sin(lat * M_PI / 180.0);
    double ca = cos(lat * M_PI / 180.0);

    // ENU
    enuMat[0] = -so;
    enuMat[1] = co;
    enuMat[2] = 0.0;

    enuMat[3] = -sa * co;
    enuMat[4] = -sa * so;
    enuMat[5] = ca;

    enuMat[6] = ca * co;
    enuMat[7] = ca * so;
    enuMat[8] = sa;
}

void Utility::llhToEnu(const double *iLlh, const double *llh, double *xyz)
{
    double ix, iy, iz;
    llhToXyz(iLlh[0], iLlh[1], iLlh[2], &ix, &iy, &iz);

    double x, y, z;
    llhToXyz(llh[0], llh[1], llh[2], &x, &y, &z);

    double enuMat[9];
    createEnuMatrix(iLlh[0], iLlh[1], enuMat);

    double dx = x - ix;
    double dy = y - iy;
    double dz = z - iz;

    xyz[0] = enuMat[0] * dx + enuMat[1] * dy + enuMat[2] * dz;
    xyz[1] = enuMat[3] * dx + enuMat[4] * dy + enuMat[5] * dz;
    xyz[2] = enuMat[6] * dx + enuMat[7] * dy + enuMat[8] * dz;
}

void Utility::enuToLlh(const double *iLlh, const double *xyz, double *llh)
{
    double ix, iy, iz;
    llhToXyz(iLlh[0], iLlh[1], iLlh[2], &ix, &iy, &iz);

    double enuMat[9];
    createEnuMatrix(iLlh[0], iLlh[1], enuMat);

    double x = enuMat[0] * xyz[0] + enuMat[3] * xyz[1] + enuMat[6] * xyz[2] + ix;
    double y = enuMat[1] * xyz[0] + enuMat[4] * xyz[1] + enuMat[7] * xyz[2] + iy;
    double z = enuMat[2] * xyz[0] + enuMat[5] * xyz[1] + enuMat[8] * xyz[2] + iz;

    xyzToLlh(x, y, z, &llh[0], &llh[1], &llh[2]);
}

bool Utility::configCheckCompatibility(int fwMajor, int fwMinor)
{
    QDirIterator it("://res/config");

    while (it.hasNext()) {
        QFileInfo fi(it.next());
        QStringList names = fi.fileName().split("_o_");

        if (fi.isDir()) {
            for(auto name: names) {
                auto parts = name.split(".");
                if (parts.size() == 2) {
                    int major = parts.at(0).toInt();
                    int minor = parts.at(1).toInt();
                    if (major == fwMajor && minor == fwMinor) {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

bool Utility::configLoad(VescInterface *vesc, int fwMajor, int fwMinor)
{
    QDirIterator it("://res/config");

    while (it.hasNext()) {
        QFileInfo fi(it.next());
        QStringList names = fi.fileName().split("_o_");

        if (fi.isDir()) {
            for(auto name: names) {
                auto parts = name.split(".");
                if (parts.size() == 2) {
                    int major = parts.at(0).toInt();
                    int minor = parts.at(1).toInt();
                    if (major == fwMajor && minor == fwMinor) {
                        QFileInfo fMc(it.filePath() + "/parameters_mcconf.xml");
                        QFileInfo fApp(it.filePath() + "/parameters_appconf.xml");
                        QFileInfo fInfo(it.filePath() + "/info.xml");

                        if (fMc.exists() && fApp.exists() && fInfo.exists()) {
                            vesc->mcConfig()->loadParamsXml(fMc.absoluteFilePath());
                            vesc->appConfig()->loadParamsXml(fApp.absoluteFilePath());
                            vesc->infoConfig()->loadParamsXml(fInfo.absoluteFilePath());
                            vesc->emitConfigurationChanged();
                            return true;
                        } else {
                            qWarning() << "Configurations not found in firmware directory" << it.path();
                            return false;
                        }
                    }
                }
            }
        }
    }

    return false;
}

QPair<int, int> Utility::configLatestSupported()
{
    QPair<int, int> res = qMakePair(-1, -1);
    QDirIterator it("://res/config");

    while (it.hasNext()) {
        QFileInfo fi(it.next());
        QStringList names = fi.fileName().split("_o_");

        if (fi.isDir()) {
            foreach (auto name, names) {
                auto parts = name.split(".");
                if (parts.size() == 2) {
                    QPair<int, int> ver = qMakePair(parts.at(0).toInt(), parts.at(1).toInt());
                    if (ver > res) {
                        res = ver;
                    }
                }
            }
        }
    }

    return res;
}

bool Utility::configLoadLatest(VescInterface *vesc)
{
    auto latestSupported = configLatestSupported();

    if (latestSupported.first >= 0) {
        configLoad(vesc, latestSupported.first, latestSupported.second);
        return true;
    } else {
        return false;
    }
}

QVector<QPair<int, int> > Utility::configSupportedFws()
{
    QVector<QPair<int, int>> res;
    QDirIterator it("://res/config");

    while (it.hasNext()) {
        QFileInfo fi(it.next());
        QStringList names = fi.fileName().split("_o_");

        if (fi.isDir()) {
            for(auto name: names) {
                auto parts = name.split(".");
                if (parts.size() == 2) {
                    int major = parts.at(0).toInt();
                    int minor = parts.at(1).toInt();
                    res.append(qMakePair(major, minor));
                }
            }
        }
    }

    return res;
}

bool Utility::configLoadCompatible(VescInterface *vesc, QString &uuidRx)
{
    bool res = false;

    auto conn = connect(vesc->commands(), &Commands::fwVersionReceived,
            [&res, &uuidRx, vesc](FW_RX_PARAMS params) {
        if (vesc->getSupportedFirmwarePairs().contains(qMakePair(params.major, params.minor))) {
            res = true;
        } else {
            res = configLoad(vesc, params.major, params.minor);
        }

        QString uuidStr = uuid2Str(params.uuid, true);
        uuidRx = uuidStr.toUpper();
        uuidRx.replace(" ", "");

        if (!res) {
            vesc->emitMessageDialog("Load Config", "Could not load configuration parser.", false, false);
        }
    });

    disconnect(vesc->commands(), SIGNAL(fwVersionReceived(FW_RX_PARAMS)),
               vesc, SLOT(fwVersionReceived(FW_RX_PARAMS)));

    vesc->commands()->getFwVersion();

    if (!waitSignal(vesc->commands(), SIGNAL(fwVersionReceived(FW_RX_PARAMS)), 4000)) {
        vesc->emitMessageDialog("Load Config", "No response when reading firmware version.", false, false);
    }

    disconnect(conn);

    connect(vesc->commands(), SIGNAL(fwVersionReceived(FW_RX_PARAMS)),
            vesc, SLOT(fwVersionReceived(FW_RX_PARAMS)));

    return res;
}

QVector<int> Utility::scanCanVescOnly(VescInterface *vesc)
{
    auto canDevs = vesc->scanCan();
    QVector<int> res;

    for (auto d: canDevs) {
        FW_RX_PARAMS params;
        getFwVersionBlockingCan(vesc, &params, d);
        if (params.hwType == HW_TYPE_VESC) {
            res.append(d);
        }
    }

    return res;
}

void Utility::serialFunc(ConfigParams *params, QTextStream &s) {
    QStringList serialOrder = params->getSerializeOrder();

    for (int i = 0;i < serialOrder.size();i++) {
        QString name = serialOrder.at(i);

        ConfigParam *p = params->getParam(name);

        int last__ = name.lastIndexOf("__");
        if (last__ > 0) {
            QString end = name.mid(last__ + 2);
            bool ok;
            int endNum = end.toInt(&ok);
            if (ok) {
                name = name.left(last__) + QString("[%1]").arg(endNum);
            }
        }

        if (p) {
            switch (p->type) {
            case CFG_T_BOOL:
            case CFG_T_ENUM:
            case CFG_T_BITFIELD:
                s << "\t" << "buffer[ind++] = conf->" << name << ";\n";
                break;

            case CFG_T_INT:
                switch (p->vTx) {
                case VESC_TX_UINT8:
                case VESC_TX_INT8:
                    s << "\t" << "buffer[ind++] = (uint8_t)conf->" << name << ";\n";
                    break;

                case VESC_TX_UINT16:
                    s << "\t" << "buffer_append_uint16(buffer, conf->" << name << ", &ind);\n";
                    break;

                case VESC_TX_INT16:
                    s << "\t" << "buffer_append_int16(buffer, conf->" << name << ", &ind);\n";
                    break;

                case VESC_TX_UINT32:
                    s << "\t" << "buffer_append_uint32(buffer, conf->" << name << ", &ind);\n";
                    break;

                case VESC_TX_INT32:
                    s << "\t" << "buffer_append_int32(buffer, conf->" << name << ", &ind);\n";
                    break;

                default:
                    qWarning() << "Serialization type not supporter";
                    break;
                }
                break;

            case CFG_T_DOUBLE:
                switch (p->vTx) {
                case VESC_TX_DOUBLE16:
                    s << "\t" << "buffer_append_float16(buffer, conf->" << name << ", " << p->vTxDoubleScale << ", &ind);\n";
                    break;

                case VESC_TX_DOUBLE32:
                    s << "\t" << "buffer_append_float32(buffer, conf->" << name << ", " << p->vTxDoubleScale << ", &ind);\n";
                    break;

                case VESC_TX_DOUBLE32_AUTO:
                    s << "\t" << "buffer_append_float32_auto(buffer, conf->" << name << ", &ind);\n";
                    break;

                default:
                    qWarning() << "Serialization type not supporter";
                    break;
                }
                break;

            case CFG_T_QSTRING:
                s << "\t" << "strcpy((char*)buffer + ind, conf->" << name << ");\n";
                s << "\t" << "ind += strlen(conf->" << name << ") + 1;\n";
                break;

            default:
                qWarning() << name << ": type not supported.";
                break;
            }
        } else {
            qWarning() << name << "not found.";
        }
    }
}

void Utility::deserialFunc(ConfigParams *params, QTextStream &s) {
    QStringList serialOrder = params->getSerializeOrder();

    for (int i = 0;i < serialOrder.size();i++) {
        QString name = serialOrder.at(i);

        ConfigParam *p = params->getParam(name);

        int last__ = name.lastIndexOf("__");
        if (last__ > 0) {
            QString end = name.mid(last__ + 2);
            bool ok;
            int endNum = end.toInt(&ok);
            if (ok) {
                name = name.left(last__) + QString("[%1]").arg(endNum);
            }
        }

        if (p) {
            switch (p->type) {
            case CFG_T_BOOL:
            case CFG_T_ENUM:
            case CFG_T_BITFIELD:
                s << "\tconf->" << name << " = buffer[ind++];\n";
                break;

            case CFG_T_INT:
                switch (p->vTx) {
                case VESC_TX_UINT8:
                    s << "\tconf->" << name << " = buffer[ind++];\n";
                    break;

                case VESC_TX_INT8:
                    s << "\tconf->" << name << " = (int8_t)buffer[ind++];\n";
                    break;

                case VESC_TX_UINT16:
                    s << "\tconf->" << name << " = buffer_get_uint16(buffer, &ind);\n";
                    break;

                case VESC_TX_INT16:
                    s << "\tconf->" << name << " = buffer_get_int16(buffer, &ind);\n";
                    break;

                case VESC_TX_UINT32:
                    s << "\tconf->" << name << " = buffer_get_uint32(buffer, &ind);\n";
                    break;

                case VESC_TX_INT32:
                    s << "\tconf->" << name << " = buffer_get_int32(buffer, &ind);\n";
                    break;

                default:
                    qWarning() << "Serialization type not supporter";
                    break;
                }
                break;
                break;

            case CFG_T_DOUBLE:
                switch (p->vTx) {
                case VESC_TX_DOUBLE16:
                    s << "\tconf->" << name << " = buffer_get_float16(buffer, " << p->vTxDoubleScale << ", &ind);\n";
                    break;

                case VESC_TX_DOUBLE32:
                    s << "\tconf->" << name << " = buffer_get_float32(buffer, " << p->vTxDoubleScale << ", &ind);\n";
                    break;

                case VESC_TX_DOUBLE32_AUTO:
                    s << "\tconf->" << name << " = buffer_get_float32_auto(buffer, &ind);\n";
                    break;

                default:
                    qWarning() << "Serialization type not supporter";
                    break;
                }
                break;

            case CFG_T_QSTRING:
                s << "\t" << "strcpy(conf->" << name << ", (char*)buffer + ind);\n";
                s << "\t" << "ind += strlen(conf->" << name << ") + 1;\n";
                break;

            default:
                qWarning() << name << ": type not supported.";
                break;
            }
        } else {
            qWarning() << name << "not found.";
        }
    }
}

void Utility::defaultFunc(ConfigParams *params, QTextStream &s) {
    QStringList serialOrder = params->getSerializeOrder();

    for (int i = 0;i < serialOrder.size();i++) {
        QString name = serialOrder.at(i);

        ConfigParam *p = params->getParam(name);

        int last__ = name.lastIndexOf("__");
        if (last__ > 0) {
            QString end = name.mid(last__ + 2);
            bool ok;
            int endNum = end.toInt(&ok);
            if (ok) {
                name = name.left(last__) + QString("[%1]").arg(endNum);
            }
        }

        if (p) {
            QString def = p->cDefine;

            if (p->type == CFG_T_QSTRING) {
                s << "\t" << "strcpy(conf->" << name << ", " << def << ");\n";
            } else {
                // Kind of a hack...
                if (name == "controller_id") {
                    def = "HW_DEFAULT_ID";
                }
                s << "\tconf->" << name << " = " << def << ";\n";
            }
        } else {
            qWarning() << name << "not found.";
        }
    }
}

void Utility::setAppQColor(QString colorName, QColor color)
{
    if(mAppColors.contains(colorName)) {
        mAppColors[colorName] = color;
    } else {
        mAppColors.insert(colorName , color);
    }
}

QColor Utility::getAppQColor(QString colorName)
{
    if(mAppColors.contains(colorName)) {
        return mAppColors[colorName];
    } else {
        qDebug() << colorName <<" not found in standard colors";
        return Qt::red;
    }
}

QString Utility::getAppHexColor(QString colorName)
{
    return getAppQColor(colorName).name();
}

void Utility::setPlotColors(QCustomPlot* plot)
{
    plot->setBackground(QBrush(Utility::getAppQColor("plotBackground")));

    plot->xAxis->setLabelColor(Utility::getAppQColor("lightText"));
    plot->xAxis->setBasePen(QPen(Utility::getAppQColor("lightText")));
    plot->xAxis->setTickPen(QPen(Utility::getAppQColor("lightText")));
    plot->xAxis->setSubTickPen(QPen(Utility::getAppQColor("lightText")));
    plot->xAxis->setTickLabelColor(Utility::getAppQColor("lightText"));

    plot->xAxis->setBasePen(QPen(Utility::getAppQColor("lightText")));
    plot->yAxis->setLabelColor(Utility::getAppQColor("lightText"));
    plot->yAxis->setBasePen(QPen(Utility::getAppQColor("lightText")));
    plot->yAxis->setTickPen(QPen(Utility::getAppQColor("lightText")));
    plot->yAxis->setSubTickPen(QPen(Utility::getAppQColor("lightText")));
    plot->yAxis->setTickLabelColor(Utility::getAppQColor("lightText"));
    plot->yAxis->setBasePen(QPen(Utility::getAppQColor("lightText")));

    plot->xAxis2->setLabelColor(Utility::getAppQColor("lightText"));
    plot->xAxis2->setBasePen(QPen(Utility::getAppQColor("lightText")));
    plot->xAxis2->setTickPen(QPen(Utility::getAppQColor("lightText")));
    plot->xAxis2->setSubTickPen(QPen(Utility::getAppQColor("lightText")));
    plot->xAxis2->setTickLabelColor(Utility::getAppQColor("lightText"));
    plot->xAxis2->setBasePen(QPen(Utility::getAppQColor("lightText")));

    plot->yAxis2->setLabelColor(Utility::getAppQColor("lightText"));
    plot->yAxis2->setBasePen(QPen(Utility::getAppQColor("lightText")));
    plot->yAxis2->setTickPen(QPen(Utility::getAppQColor("lightText")));
    plot->yAxis2->setSubTickPen(QPen(Utility::getAppQColor("lightText")));
    plot->yAxis2->setTickLabelColor(Utility::getAppQColor("lightText"));
    plot->yAxis2->setBasePen(QPen(Utility::getAppQColor("lightText")));

    plot->legend->setBrush(Utility::getAppQColor("normalBackground"));
    plot->legend->setTextColor(Utility::getAppQColor("lightText"));
    plot->legend->setBorderPen(QPen(Utility::getAppQColor("darkBackground")));
}

void Utility::plotSavePdf(QCustomPlot *plot, int width, int height, QString title)
{
    QString fileName = QFileDialog::getSaveFileName(plot,
                                                    tr("Save PDF"), "",
                                                    tr("PDF Files (*.pdf)"));

    if (!fileName.isEmpty()) {
        if (!fileName.toLower().endsWith(".pdf")) {
            fileName.append(".pdf");
        }

        if (!title.isEmpty()) {
            auto element = new QCPTextElement(plot,
                                              title,
                                              QFont("sans", 12, QFont::Bold));
            element->setTextColor(Utility::getAppQColor("lightText"));

            plot->plotLayout()->insertRow(0);
            plot->plotLayout()->addElement(0, 0, element);
            plot->replot();
        }

        plot->savePdf(fileName, width, height);

        if (!title.isEmpty()) {
            delete plot->plotLayout()->element(0, 0);
            plot->plotLayout()->simplify();
            plot->replot();
        }
    }
}

void Utility::plotSavePng(QCustomPlot *plot, int width, int height, QString title)
{
    QString fileName = QFileDialog::getSaveFileName(plot,
                                                    tr("Save Image"), "",
                                                    tr("PNG Files (*.png)"));

    if (!fileName.isEmpty()) {
        if (!fileName.toLower().endsWith(".png")) {
            fileName.append(".png");
        }

        if (!title.isEmpty()) {
            auto element = new QCPTextElement(plot,
                                              title,
                                              QFont("sans", 12, QFont::Bold));
            element->setTextColor(Utility::getAppQColor("lightText"));

            plot->plotLayout()->insertRow(0);
            plot->plotLayout()->addElement(0, 0, element);
            plot->replot();
        }

        plot->savePng(fileName, width, height);

        if (!title.isEmpty()) {
            delete plot->plotLayout()->element(0, 0);
            plot->plotLayout()->simplify();
            plot->replot();
        }
    }
}

QString Utility::waitForLine(QTcpSocket *socket, int timeoutMs)
{
    QEventLoop loop;
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    timeoutTimer.start(timeoutMs);
    auto conn = QObject::connect(&timeoutTimer, SIGNAL(timeout()), &loop, SLOT(quit()));

    QByteArray rxLine;
    auto conn2 = connect(socket, &QTcpSocket::readyRead, [&rxLine,socket,&loop]() {
        while (socket->bytesAvailable() > 0) {
            QByteArray rxb = socket->read(1);
            if (rxb.size() == 1) {
                if (rxb[0] != '\n') {
                    rxLine.append(rxb[0]);
                } else {
                    rxLine.append('\0');
                    loop.quit();
                }
            } else {
                break;
            }
        }
    });

    loop.exec();

    disconnect(conn);
    disconnect(conn2);

    auto res = QString::fromLocal8Bit(rxLine);
    if (!timeoutTimer.isActive()) {
        res = "";
    }

    return res;
}

QPixmap Utility::getIcon(QString path)
{
    while (path.startsWith("/")) {
        path.remove(0, 1);
    }

    QPixmap pm;
    if (!QPixmapCache::find(path, &pm)) {
        pm.load(getThemePath() + path);
        QPixmapCache::insert(path, pm);
    }

    return pm;
}

bool Utility::downloadUrlEventloop(QString path, QString dest)
{
    bool res = false;

    QUrl url(path);

    if (!url.isValid()) {
        return res;
    }

    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    QNetworkReply *reply = manager.get(request);

    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        QFile file(dest);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(reply->readAll());
            file.close();
            res = true;
        }
    }

    reply->abort();
    reply->deleteLater();

    return res;
}

QString Utility::md2html(QString md)
{
    std::shared_ptr<maddy::ParserConfig> config = std::make_shared<maddy::ParserConfig>();
    config->enabledParsers = maddy::types::ALL;
    std::shared_ptr<maddy::Parser> parser = std::make_shared<maddy::Parser>(config);

    std::stringstream markdownInput(md.toStdString());
    return QString::fromStdString(parser->Parse(markdownInput));
}

void Utility::setDarkMode(bool isDarkSetting)
{
    isDark = isDarkSetting;
}

bool Utility::isDarkMode()
{
    return isDark;
}

QString Utility::getThemePath()
{
    if (isDark) {
        return ":/res/";
    } else {
        return ":/res/+theme_light/";
    }
}

QVariantMap Utility::getSafeAreaMargins(QQuickWindow *window)
{
    QPlatformWindow *platformWindow = static_cast<QPlatformWindow *>(window->handle());
    QMargins margins = platformWindow->safeAreaMargins();
    QVariantMap map;
    map["top"] = margins.top();
    map["right"] = margins.right();
    map["bottom"] = margins.bottom();
    map["left"] = margins.left();
    return map;
}
