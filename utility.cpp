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

#ifdef Q_OS_ANDROID
#include <QtAndroid>
#include <QAndroidJniObject>
#endif

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
}

QString Utility::fwChangeLog()
{
    QFile cl("://res/firmwares/CHANGELOG");
    if (cl.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString::fromUtf8(cl.readAll());
    } else {
        return "";
    }
}

QString Utility::vescToolChangeLog()
{
    QFile cl("://res/CHANGELOG");
    if (cl.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString::fromUtf8(cl.readAll());
    } else {
        return "";
    }
}

QString Utility::aboutText()
{
    return tr("<b>VESC® Tool %1</b><br>"
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
              "&copy; Benjamin Vedder 2016 - 2019<br>"
              "<a href=\"mailto:benjamin@vedder.se\">benjamin@vedder.se</a><br>"
              "<a href=\"https://vesc-project.com/\">https://vesc-project.com/</a>").
            arg(QString::number(VT_VERSION, 'f', 2));
}

QString Utility::uuid2Str(QByteArray uuid, bool space)
{
    QString strUuid;
    for (int i = 0;i < uuid.size();i++) {
        QString str = QString::number(uuid.at(i), 16).
                rightJustified(2, '0').toUpper();
        strUuid.append(((i > 0 && space) ? " " : "") + str);
    }

    return strUuid;
}

bool Utility::requestFilePermission()
{
#ifdef Q_OS_ANDROID
    // Note: The following should work on Qt 5.10
    // https://codereview.qt-project.org/#/c/199162/
//    QtAndroid::PermissionResult r = QtAndroid::checkPermission("android.permission.WRITE_EXTERNAL_STORAGE");
//    if(r == QtAndroid::PermissionResult::Denied) {
//        QtAndroid::requestPermissionsSync( QStringList() << "android.permission.WRITE_EXTERNAL_STORAGE" );
//        r = QtAndroid::checkPermission("android.permission.WRITE_EXTERNAL_STORAGE");
//        if(r == QtAndroid::PermissionResult::Denied) {
//            return false;
//        }
//    }
    return true;
#else
    return true;
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

QString Utility::detectAllFoc(VescInterface *vesc,
                              bool detect_can, double max_power_loss, double min_current_in,
                              double max_current_in, double openloop_rpm, double sl_erpm)
{
    QString res;
    bool detectOk = true;

    vesc->commands()->detectAllFoc(detect_can, max_power_loss, min_current_in,
                                   max_current_in, openloop_rpm, sl_erpm);

    QEventLoop loop;
    QTimer timeoutTimer;
    QTimer pollTimer;
    timeoutTimer.setSingleShot(true);
    timeoutTimer.start(180000);
    pollTimer.start(100);

    vesc->commands()->disableAppOutput(180000, true);

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
            waitSignal(ap, SIGNAL(updated()), 1500);

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
                               "Motor Flux Linkage : %5 mWb\n"
                               "Temp Comp          : %6\n"
                               "Sensors            : %7").
                        arg(ap->getParamInt("controller_id")).
                        arg(p->getParamDouble("l_current_max"), 0, 'f', 2).
                        arg(p->getParamDouble("foc_motor_r") * 1e3, 0, 'f', 2).
                        arg(p->getParamDouble("foc_motor_l") * 1e6, 0, 'f', 2).
                        arg(p->getParamDouble("foc_motor_flux_linkage") * 1e3, 0, 'f', 2).
                        arg(p->getParamBool("foc_temp_comp") ? "True" : "False").
                        arg(sensors);
            };

            QVector<int> canDevs = vesc->scanCan();
            res = genRes();

            int canLastFwd = vesc->commands()->getSendCan();
            int canLastId = vesc->commands()->getCanSendId();
            vesc->ignoreCanChange(true);

            if (!canDevs.empty()) {
                res += "\n\nVESCs on CAN-bus:";
            }

            for (int id: canDevs) {
                vesc->commands()->setSendCan(true, id);
                vesc->commands()->getMcconf();
                waitSignal(p, SIGNAL(updated()), 1500);
                vesc->commands()->getAppConf();
                waitSignal(ap, SIGNAL(updated()), 1500);
                res += "\n\n" + genRes();
            }

            vesc->commands()->setSendCan(canLastFwd, canLastId);
            vesc->ignoreCanChange(false);
            vesc->commands()->getMcconf();
            waitSignal(p, SIGNAL(updated()), 1500);
            vesc->commands()->getAppConf();
            waitSignal(ap, SIGNAL(updated()), 1500);
        } else {
            QString reason;
            switch (resDetect) {
            case -1: reason = "Sensor detection failed"; break;
            case -10: reason = "Flux linkage detection failed"; break;
            case -50: reason = "CAN detection timeout"; break;
            case -51: reason = "CAN detection failed"; break;
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

bool Utility::resetInputCan(VescInterface *vesc, QVector<int> canIds)
{
    bool res = true;

    bool canLastFwd = vesc->commands()->getSendCan();
    int canLastId = vesc->commands()->getCanSendId();

    vesc->ignoreCanChange(true);

    // Local VESC first
    ConfigParams *ap = vesc->appConfig();
    vesc->commands()->setSendCan(false);

    vesc->commands()->getAppConf();
    res = waitSignal(ap, SIGNAL(updated()), 1500);

    if (res) {
        int canId = ap->getParamInt("controller_id");
        int canStatus = ap->getParamEnum("send_can_status");
        vesc->commands()->getAppConfDefault();
        res = waitSignal(ap, SIGNAL(updated()), 1500);

        if (res) {
            ap->updateParamInt("controller_id", canId);
            ap->updateParamEnum("send_can_status", canStatus);
            vesc->commands()->setAppConf();
            res = waitSignal(vesc->commands(), SIGNAL(ackReceived(QString)), 2000);
        }
    }

    // All VESCs on CAN-bus
    if (res) {
        for (int id: canIds) {
            vesc->commands()->setSendCan(true, id);

            vesc->commands()->getAppConf();
            res = waitSignal(ap, SIGNAL(updated()), 1500);

            if (!res) {
                break;
            }

            int canId = ap->getParamInt("controller_id");
            int canStatus = ap->getParamEnum("send_can_status");
            vesc->commands()->getAppConfDefault();
            res = waitSignal(ap, SIGNAL(updated()), 1500);

            if (!res) {
                break;
            }

            ap->updateParamInt("controller_id", canId);
            ap->updateParamEnum("send_can_status", canStatus);
            vesc->commands()->setAppConf();
            res = waitSignal(vesc->commands(), SIGNAL(ackReceived(QString)), 2000);

            if (!res) {
                break;
            }
        }
    }

    vesc->commands()->setSendCan(canLastFwd, canLastId);
    vesc->commands()->getAppConf();
    if (!waitSignal(ap, SIGNAL(updated()), 1500)) {
        res = false;
    }

    vesc->ignoreCanChange(false);

    return res;
}

bool Utility::setBatteryCutCan(VescInterface *vesc, QVector<int> canIds,
                               double cutStart, double cutEnd)
{
    bool res = true;

    bool canLastFwd = vesc->commands()->getSendCan();
    int canLastId = vesc->commands()->getCanSendId();

    vesc->ignoreCanChange(true);

    // Local VESC first
    ConfigParams *p = vesc->mcConfig();
    vesc->commands()->setSendCan(false);

    vesc->commands()->getMcconf();
    res = waitSignal(p, SIGNAL(updated()), 1500);

    if (res) {
        p->updateParamDouble("l_battery_cut_start", cutStart);
        p->updateParamDouble("l_battery_cut_end", cutEnd);
        vesc->commands()->setMcconf(false);
        res = waitSignal(vesc->commands(), SIGNAL(ackReceived(QString)), 2000);
    }

    // All VESCs on CAN-bus
    if (res) {
        for (int id: canIds) {
            vesc->commands()->setSendCan(true, id);

            vesc->commands()->getMcconf();
            res = waitSignal(p, SIGNAL(updated()), 1500);

            if (!res) {
                break;
            }

            p->updateParamDouble("l_battery_cut_start", cutStart);
            p->updateParamDouble("l_battery_cut_end", cutEnd);
            vesc->commands()->setMcconf(false);
            res = waitSignal(vesc->commands(), SIGNAL(ackReceived(QString)), 2000);

            if (!res) {
                break;
            }
        }
    }

    vesc->commands()->setSendCan(canLastFwd, canLastId);
    vesc->commands()->getMcconf();
    if (!waitSignal(p, SIGNAL(updated()), 1500)) {
        res = false;
    }

    vesc->ignoreCanChange(false);

    return res;
}

bool Utility::setBatteryCutCanFromCurrentConfig(VescInterface *vesc, QVector<int> canIds)
{
    ConfigParams *p = vesc->mcConfig();

    int battType = p->getParamEnum("si_battery_type");
    int cells = p->getParamInt("si_battery_cells");
    double start = -1.0;
    double end = -1.0;

    if (battType == 0) {
        start = 3.4;
        end = 3.0;
    } else if (battType == 1) {
        start = 2.9;
        end = 2.6;
    } else {
        return false;
    }

    start *= (double)cells;
    end *= (double)cells;

    return setBatteryCutCan(vesc, canIds, start, end);
}

bool Utility::setInvertDirection(VescInterface *vesc, int canId, bool inverted)
{
    bool res = true;

    bool canLastFwd = vesc->commands()->getSendCan();
    int canLastId = vesc->commands()->getCanSendId();

    vesc->ignoreCanChange(true);
    vesc->commands()->setSendCan(canId >= 0, canId);

    ConfigParams *p = vesc->mcConfig();
    vesc->commands()->getMcconf();
    res = waitSignal(p, SIGNAL(updated()), 1500);

    if (res) {
        p->updateParamBool("m_invert_direction", inverted);
        vesc->commands()->setMcconf(false);
        res = waitSignal(vesc->commands(), SIGNAL(ackReceived(QString)), 2000);
    }

    vesc->commands()->setSendCan(canLastFwd, canLastId);
    vesc->commands()->getMcconf();
    if (!waitSignal(p, SIGNAL(updated()), 1500)) {
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

    ConfigParams *p = vesc->mcConfig();
    vesc->commands()->getMcconf();
    waitSignal(p, SIGNAL(updated()), 1500);
    res = p->getParamBool("m_invert_direction");

    vesc->commands()->setSendCan(canLastFwd, canLastId);
    vesc->commands()->getMcconf();
    waitSignal(p, SIGNAL(updated()), 1500);

    vesc->ignoreCanChange(false);

    return res;
}

QString Utility::testDirection(VescInterface *vesc, int canId, double duty, int ms)
{
    bool canLastFwd = vesc->commands()->getSendCan();
    int canLastId = vesc->commands()->getCanSendId();

    vesc->commands()->disableAppOutput(ms, true);

    vesc->ignoreCanChange(true);
    vesc->commands()->setSendCan(canId >= 0, canId);

    QEventLoop loop;
    QTimer timeoutTimer;
    QTimer pollTimer;
    timeoutTimer.setSingleShot(true);
    timeoutTimer.start(ms);
    pollTimer.start(20);

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

    vesc->commands()->setSendCan(canLastFwd, canLastId);
    vesc->ignoreCanChange(false);

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

    if (can) {
        vesc->ignoreCanChange(true);
        vesc->commands()->setSendCan(false);
    }

    if (mc) {
        ConfigParams *p = vesc->mcConfig();
        vesc->commands()->getMcconfDefault();
        res = waitSignal(p, SIGNAL(updated()), 1500);

        if (res) {
            vesc->commands()->setMcconf(false);
            res = waitSignal(vesc->commands(), SIGNAL(ackReceived(QString)), 2000);
        }
    }

    if (app) {
        ConfigParams *p = vesc->appConfig();
        vesc->commands()->getAppConfDefault();
        res = waitSignal(p, SIGNAL(updated()), 1500);

        if (res) {
            vesc->commands()->setAppConf();
            res = waitSignal(vesc->commands(), SIGNAL(ackReceived(QString)), 2000);
        }
    }

    if (res && can) {
        QVector<int> canDevs = vesc->scanCan();

        for (int d: canDevs) {
            vesc->commands()->setSendCan(true, d);

            if (mc) {
                ConfigParams *p = vesc->mcConfig();
                vesc->commands()->getMcconfDefault();
                res = waitSignal(p, SIGNAL(updated()), 1500);

                if (!res) {
                    break;
                }

                vesc->commands()->setMcconf(false);
                res = waitSignal(vesc->commands(), SIGNAL(ackReceived(QString)), 2000);

                if (!res) {
                    break;
                }
            }

            if (app) {
                ConfigParams *p = vesc->appConfig();
                vesc->commands()->getAppConfDefault();
                res = waitSignal(p, SIGNAL(updated()), 1500);

                if (!res) {
                    break;
                }

                vesc->commands()->setAppConf();
                res = waitSignal(vesc->commands(), SIGNAL(ackReceived(QString)), 2000);

                if (!res) {
                    break;
                }
            }
        }
    }

    if (can) {
        vesc->commands()->setSendCan(canLastFwd, canLastId);

        if (mc) {
            ConfigParams *p = vesc->mcConfig();
            vesc->commands()->getMcconf();
            if (!waitSignal(p, SIGNAL(updated()), 1500)) {
                res = false;
                qWarning() << "Could not restore mc conf";
            }
        }

        if (app) {
            ConfigParams *p = vesc->appConfig();
            vesc->commands()->getAppConf();
            if (!waitSignal(p, SIGNAL(updated()), 1500)) {
                res = false;
                qWarning() << "Could not restore app conf";
            }
        }

        vesc->ignoreCanChange(false);
    }

    return res;
}

bool Utility::almostEqual(double A, double B, double eps)
{
    return fabs(A - B) <= eps * fmax(1.0f, fmax(fabs(A), fabs(B)));
}
