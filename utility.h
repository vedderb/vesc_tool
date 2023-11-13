/*
    Copyright 2017 Benjamin Vedder	benjamin@vedder.se

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

#ifndef UTILITY_H
#define UTILITY_H

#include <QObject>
#include <QMetaEnum>
#include <cstdint>
#include <QQuickWindow>
#include <QtGui/qpa/qplatformwindow.h>
#include "vescinterface.h"
#include "widgets/qcustomplot.h"
#include "datatypes.h"

#define FE_WGS84        (1.0/298.257223563) // earth flattening (WGS84)
#define RE_WGS84        6378137.0           // earth semimajor axis (WGS84) (m)

#define SIGN(x)         ((x < 0) ? -1 : 1)
#define SQ(x)           ((x) * (x))

class Utility : public QObject
{
    Q_OBJECT

public:
    explicit Utility(QObject *parent = nullptr);

    static double map(double x, double in_min, double in_max, double out_min, double out_max);
    static float throttle_curve(float val, float curve_acc, float curve_brake, int mode);
    static bool autoconnectBlockingWithProgress(VescInterface *vesc, QWidget *parent = nullptr);
    Q_INVOKABLE static void checkVersion(VescInterface *vesc = nullptr);
    Q_INVOKABLE static QString fwChangeLog();
    Q_INVOKABLE static QString vescToolChangeLog();
    Q_INVOKABLE static QString aboutText();
    Q_INVOKABLE static QString uuid2Str(QByteArray uuid, bool space);
    Q_INVOKABLE static bool requestFilePermission();
    Q_INVOKABLE static bool hasLocationPermission();
    Q_INVOKABLE static bool requestBleScanPermission();
    Q_INVOKABLE static bool requestBleConnectPermission();
    Q_INVOKABLE static void keepScreenOn(bool on);
    Q_INVOKABLE static void allowScreenRotation(bool enabled);
    Q_INVOKABLE static bool waitSignal(QObject *sender, QString signal, int timeoutMs);
    Q_INVOKABLE static void sleepWithEventLoop(int timeMs);
    Q_INVOKABLE static QString detectAllFoc(VescInterface *vesc,
                                            bool detect_can, double max_power_loss, double min_current_in,
                                            double max_current_in, double openloop_rpm, double sl_erpm);
    Q_INVOKABLE static QVector<double> measureRLBlocking(VescInterface *vesc);
    Q_INVOKABLE static double measureLinkageOpenloopBlocking(VescInterface *vesc, double current, double erpm_per_sec, double low_duty,
                                                             double resistance, double inductance);
    Q_INVOKABLE static QVector<int> measureHallFocBlocking(VescInterface *vesc, double current);
    Q_INVOKABLE static ENCODER_DETECT_RES measureEncoderBlocking(VescInterface *vesc, double current);
    Q_INVOKABLE static bool waitMotorStop(VescInterface *vesc, double erpmTres, int timeoutMs);
    Q_INVOKABLE static bool resetInputCan(VescInterface *vesc, QVector<int> canIds);
    Q_INVOKABLE static bool setBatteryCutCan(VescInterface *vesc, QVector<int> canIds, double cutStart, double cutEnd);
    Q_INVOKABLE static bool setBatteryCutCanFromCurrentConfig(VescInterface *vesc, QVector<int> canIds, bool cautious);
    Q_INVOKABLE static bool setMcParamsFromCurrentConfigAllCan(VescInterface *vesc, QVector<int> canIds, QStringList params);
    Q_INVOKABLE static bool setInvertDirection(VescInterface *vesc, int canId, bool inverted);
    Q_INVOKABLE static bool getInvertDirection(VescInterface *vesc, int canId);
    Q_INVOKABLE static QString testDirection(VescInterface *vesc, int canId, double duty, int ms);
    Q_INVOKABLE static bool restoreConfAll(VescInterface *vesc, bool can, bool mc, bool app);
    Q_INVOKABLE static bool almostEqual(double A, double B, double eps);
    static bool createParamParserC(VescInterface *vesc, QString filename);
    static bool createParamParserC(ConfigParams *params, QString configName, QString filename);
    static bool createCompressedConfigC(ConfigParams *params, QString configName, QString filename);
    static uint32_t crc32c(uint8_t *data, uint32_t len);
    static bool getFwVersionBlocking(VescInterface *vesc, FW_RX_PARAMS *params);
    static bool getFwVersionBlockingCan(VescInterface *vesc, FW_RX_PARAMS *params, int canId);
    Q_INVOKABLE static bool isConnectedToHwVesc(VescInterface *vesc);
    Q_INVOKABLE static FW_RX_PARAMS getFwVersionBlocking(VescInterface *vesc);
    Q_INVOKABLE static FW_RX_PARAMS getFwVersionBlockingCan(VescInterface *vesc, int canId);
    Q_INVOKABLE static MC_VALUES getMcValuesBlocking(VescInterface *vesc);
    static bool checkFwCompatibility(VescInterface *vesc);
    Q_INVOKABLE static QVariantList getNetworkAddresses();
    Q_INVOKABLE static void startGnssForegroundService();
    Q_INVOKABLE static void stopGnssForegroundService();
    Q_INVOKABLE static bool isBleScanEnabled();
    Q_INVOKABLE static QString strCrc32(QString str);
    Q_INVOKABLE static QString readInternalImuType(VescInterface *vesc);

    static void llhToXyz(double lat, double lon, double height, double *x, double *y, double *z);
    static void xyzToLlh(double x, double y, double z, double *lat, double *lon, double *height);
    static void createEnuMatrix(double lat, double lon, double *enuMat);
    static void llhToEnu(const double *iLlh, const double *llh, double *xyz);
    static void enuToLlh(const double *iLlh, const double *xyz, double *llh);

    static bool configCheckCompatibility(int fwMajor, int fwMinor);
    static bool configLoad(VescInterface *vesc, int fwMajor, int fwMinor);
    static QPair<int, int> configLatestSupported();
    static bool configLoadLatest(VescInterface *vesc);
    static QVector<QPair<int, int>> configSupportedFws();
    static bool configLoadCompatible(VescInterface *vesc, QString &uuidRx);

    Q_INVOKABLE static QVector<int> scanCanVescOnly(VescInterface *vesc);
    Q_INVOKABLE static void setAppQColor(QString colorName, QColor color);
    Q_INVOKABLE static QColor getAppQColor(QString colorName);
    Q_INVOKABLE static QString getAppHexColor(QString colorName);
    Q_INVOKABLE static void setDarkMode(bool isDark);
    Q_INVOKABLE static bool isDarkMode();
    Q_INVOKABLE static QString getThemePath();
    Q_INVOKABLE static QVariantMap getSafeAreaMargins(QQuickWindow *window);

    static void setPlotColors(QCustomPlot* plot);
    static void plotSavePdf(QCustomPlot* plot, int width = 1280, int height = 720, QString title = "");
    static void plotSavePng(QCustomPlot* plot, int width = 1280, int height = 720, QString title = "");

    template<typename QEnum>
    static QString QEnumToQString (const QEnum value) {
        return QString(QMetaEnum::fromType<QEnum>().valueToKey(value));
    }

    Q_INVOKABLE static QString arr2str(QByteArray a) {return QString(a);}

    Q_INVOKABLE static bool hasSerialport() {
#ifdef HAS_SERIALPORT
        return true;
#else
        return false;
#endif
    };

    static QString waitForLine(QTcpSocket *socket, int timeoutMs);

    static QPixmap getIcon(QString path);

    Q_INVOKABLE static bool downloadUrlEventloop(QString path, QString dest);

    Q_INVOKABLE static QString md2html(QString md);

signals:

public slots:

private:
    static void serialFunc(ConfigParams *params, QTextStream &s);
    static void deserialFunc(ConfigParams *params, QTextStream &s);
    static void defaultFunc(ConfigParams *params, QTextStream &s);

    static QMap<QString,QColor> mAppColors;
    static bool isDark;
};

#endif // UTILITY_H
