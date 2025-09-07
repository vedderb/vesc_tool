/*
    Copyright 2016 - 2018 Benjamin Vedder	benjamin@vedder.se

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

#ifndef DATATYPES_H
#define DATATYPES_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QDateTime>
#include <QTime>
#include <QMap>
#include <cmath>
#include <cstdint>
#include "tcphub.h"

struct VSerialInfo_t {
    Q_GADGET

    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(QString systemPath MEMBER systemPath)
    Q_PROPERTY(bool isVesc MEMBER isVesc)
    Q_PROPERTY(bool isEsp MEMBER isEsp)

public:
    VSerialInfo_t() {
        isVesc = false;
        isEsp = false;
    }

    QString name;
    QString systemPath;
    bool isVesc;
    bool isEsp;
};

Q_DECLARE_METATYPE(VSerialInfo_t)

typedef enum {
    CFG_T_UNDEFINED = 0,
    CFG_T_DOUBLE,
    CFG_T_INT,
    CFG_T_QSTRING,
    CFG_T_ENUM,
    CFG_T_BOOL,
    CFG_T_BITFIELD
} CFG_T;

typedef enum {
    VESC_TX_UNDEFINED = 0,
    VESC_TX_UINT8,
    VESC_TX_INT8,
    VESC_TX_UINT16,
    VESC_TX_INT16,
    VESC_TX_UINT32,
    VESC_TX_INT32,
    VESC_TX_DOUBLE16,
    VESC_TX_DOUBLE32,
    VESC_TX_DOUBLE32_AUTO
} VESC_TX_T;

// General purpose drive output mode
typedef enum {
    GPD_OUTPUT_MODE_NONE = 0,
    GPD_OUTPUT_MODE_MODULATION,
    GPD_OUTPUT_MODE_VOLTAGE,
    GPD_OUTPUT_MODE_CURRENT
} gpd_output_mode;

typedef enum {
    HW_TYPE_VESC = 0,
    HW_TYPE_VESC_BMS,
    HW_TYPE_CUSTOM_MODULE
} HW_TYPE;

typedef enum {
    FAULT_CODE_NONE = 0,
    FAULT_CODE_OVER_VOLTAGE,
    FAULT_CODE_UNDER_VOLTAGE,
    FAULT_CODE_DRV,
    FAULT_CODE_ABS_OVER_CURRENT,
    FAULT_CODE_OVER_TEMP_FET,
    FAULT_CODE_OVER_TEMP_MOTOR,
    FAULT_CODE_GATE_DRIVER_OVER_VOLTAGE,
    FAULT_CODE_GATE_DRIVER_UNDER_VOLTAGE,
    FAULT_CODE_MCU_UNDER_VOLTAGE,
    FAULT_CODE_BOOTING_FROM_WATCHDOG_RESET,
    FAULT_CODE_ENCODER_SPI,
    FAULT_CODE_ENCODER_SINCOS_BELOW_MIN_AMPLITUDE,
    FAULT_CODE_ENCODER_SINCOS_ABOVE_MAX_AMPLITUDE,
    FAULT_CODE_FLASH_CORRUPTION,
    FAULT_CODE_HIGH_OFFSET_CURRENT_SENSOR_1,
    FAULT_CODE_HIGH_OFFSET_CURRENT_SENSOR_2,
    FAULT_CODE_HIGH_OFFSET_CURRENT_SENSOR_3,
    FAULT_CODE_UNBALANCED_CURRENTS,
    FAULT_CODE_BRK,
    FAULT_CODE_RESOLVER_LOT,
    FAULT_CODE_RESOLVER_DOS,
    FAULT_CODE_RESOLVER_LOS,
    FAULT_CODE_FLASH_CORRUPTION_APP_CFG,
    FAULT_CODE_FLASH_CORRUPTION_MC_CFG,
    FAULT_CODE_ENCODER_NO_MAGNET,
    FAULT_CODE_ENCODER_MAGNET_TOO_STRONG,
    FAULT_CODE_PHASE_FILTER,
    FAULT_CODE_ENCODER_FAULT,
    FAULT_CODE_LV_OUTPUT_FAULT,
    FAULT_CODE_PHASE_OUTPUT_ERROR,
} mc_fault_code;

typedef enum {
    DISP_POS_MODE_NONE = 0,
    DISP_POS_MODE_INDUCTANCE,
    DISP_POS_MODE_OBSERVER,
    DISP_POS_MODE_ENCODER,
    DISP_POS_MODE_PID_POS,
    DISP_POS_MODE_PID_POS_ERROR,
    DISP_POS_MODE_ENCODER_OBSERVER_ERROR,
    DISP_POS_MODE_HALL_OBSERVER_ERROR
} disp_pos_mode;

Q_DECLARE_METATYPE(disp_pos_mode)

// ADC control types. Remember to add new types here when adding them to the firmware.
typedef enum {
    ADC_CTRL_TYPE_NONE = 0,
    ADC_CTRL_TYPE_CURRENT,
    ADC_CTRL_TYPE_CURRENT_REV_CENTER,
    ADC_CTRL_TYPE_CURRENT_REV_BUTTON,
    ADC_CTRL_TYPE_CURRENT_REV_BUTTON_BRAKE_ADC,
    ADC_CTRL_TYPE_CURRENT_REV_BUTTON_BRAKE_CENTER,
    ADC_CTRL_TYPE_CURRENT_NOREV_BRAKE_CENTER,
    ADC_CTRL_TYPE_CURRENT_NOREV_BRAKE_BUTTON,
    ADC_CTRL_TYPE_CURRENT_NOREV_BRAKE_ADC,
    ADC_CTRL_TYPE_DUTY,
    ADC_CTRL_TYPE_DUTY_REV_CENTER,
    ADC_CTRL_TYPE_DUTY_REV_BUTTON,
    ADC_CTRL_TYPE_PID,
    ADC_CTRL_TYPE_PID_REV_CENTER,
    ADC_CTRL_TYPE_PID_REV_BUTTON
} adc_control_type;

struct MC_VALUES {
    Q_GADGET

    Q_PROPERTY(double v_in MEMBER v_in)
    Q_PROPERTY(double temp_mos MEMBER temp_mos)
    Q_PROPERTY(double temp_mos_1 MEMBER temp_mos_1)
    Q_PROPERTY(double temp_mos_2 MEMBER temp_mos_2)
    Q_PROPERTY(double temp_mos_3 MEMBER temp_mos_3)
    Q_PROPERTY(double temp_motor MEMBER temp_motor)
    Q_PROPERTY(double current_motor MEMBER current_motor)
    Q_PROPERTY(double current_in MEMBER current_in)
    Q_PROPERTY(double id MEMBER id)
    Q_PROPERTY(double iq MEMBER iq)
    Q_PROPERTY(double rpm MEMBER rpm)
    Q_PROPERTY(double duty_now MEMBER duty_now)
    Q_PROPERTY(double amp_hours MEMBER amp_hours)
    Q_PROPERTY(double amp_hours_charged MEMBER amp_hours_charged)
    Q_PROPERTY(double watt_hours MEMBER watt_hours)
    Q_PROPERTY(double watt_hours_charged MEMBER watt_hours_charged)
    Q_PROPERTY(int tachometer MEMBER tachometer)
    Q_PROPERTY(int tachometer_abs MEMBER tachometer_abs)
    Q_PROPERTY(double position MEMBER position)
    Q_PROPERTY(mc_fault_code fault_code MEMBER fault_code)
    Q_PROPERTY(int vesc_id MEMBER vesc_id)
    Q_PROPERTY(QString fault_str MEMBER fault_str)
    Q_PROPERTY(double vd MEMBER vd)
    Q_PROPERTY(double vq MEMBER vq)
    Q_PROPERTY(bool has_timeout MEMBER has_timeout)
    Q_PROPERTY(bool kill_sw_active MEMBER kill_sw_active)

public:
    MC_VALUES() {
        v_in = 0.0;
        temp_mos = 0.0;
        temp_mos_1 = 0.0;
        temp_mos_2 = 0.0;
        temp_mos_3 = 0.0;
        temp_motor = 0.0;
        current_motor = 0.0;
        current_in = 0.0;
        id = 0.0;
        iq = 0.0;
        rpm = 0.0;
        duty_now = 0.0;
        amp_hours = 0.0;
        amp_hours_charged = 0.0;
        watt_hours = 0.0;
        watt_hours_charged = 0.0;
        tachometer = 0;
        tachometer_abs = 0;
        position = 0.0;
        fault_code = FAULT_CODE_NONE;
        vesc_id = 0;
        vd = 0.0;
        vq = 0.0;
        has_timeout = false;
        kill_sw_active = false;
    }

    bool operator==(const MC_VALUES &other) const {
        (void)other;
        // compare members
        return true;
    }

    bool operator!=(MC_VALUES const &other) const {
        return !(*this == other);
    }

    double v_in;
    double temp_mos;
    double temp_mos_1;
    double temp_mos_2;
    double temp_mos_3;
    double temp_motor;
    double current_motor;
    double current_in;
    double id;
    double iq;
    double rpm;
    double duty_now;
    double amp_hours;
    double amp_hours_charged;
    double watt_hours;
    double watt_hours_charged;
    int tachometer;
    int tachometer_abs;
    double position;
    mc_fault_code fault_code;
    int vesc_id;
    QString fault_str;
    double vd;
    double vq;
    bool has_timeout;
    bool kill_sw_active;
};

Q_DECLARE_METATYPE(MC_VALUES)

struct SETUP_VALUES {
    Q_GADGET

    Q_PROPERTY(double temp_mos MEMBER temp_mos)
    Q_PROPERTY(double temp_motor MEMBER temp_motor)
    Q_PROPERTY(double current_motor MEMBER current_motor)
    Q_PROPERTY(double current_in MEMBER current_in)
    Q_PROPERTY(double duty_now MEMBER duty_now)
    Q_PROPERTY(double rpm MEMBER rpm)
    Q_PROPERTY(double speed MEMBER speed)
    Q_PROPERTY(double v_in MEMBER v_in)
    Q_PROPERTY(double battery_level MEMBER battery_level)
    Q_PROPERTY(double amp_hours MEMBER amp_hours)
    Q_PROPERTY(double amp_hours_charged MEMBER amp_hours_charged)
    Q_PROPERTY(double watt_hours MEMBER watt_hours)
    Q_PROPERTY(double watt_hours_charged MEMBER watt_hours_charged)
    Q_PROPERTY(double tachometer MEMBER tachometer)
    Q_PROPERTY(double tachometer_abs MEMBER tachometer_abs)
    Q_PROPERTY(double position MEMBER position)
    Q_PROPERTY(mc_fault_code fault_code MEMBER fault_code)
    Q_PROPERTY(int vesc_id MEMBER vesc_id)
    Q_PROPERTY(int num_vescs MEMBER num_vescs)
    Q_PROPERTY(double battery_wh MEMBER battery_wh)
    Q_PROPERTY(QString fault_str MEMBER fault_str)
    Q_PROPERTY(unsigned odometer MEMBER odometer)
    Q_PROPERTY(unsigned uptime_ms MEMBER uptime_ms)

public:
    SETUP_VALUES() {
        temp_mos = 0.0;
        temp_motor = 0.0;
        current_motor = 0.0;
        current_in = 0.0;
        duty_now = 0.0;
        rpm = 0.0;
        speed = 0.0;
        v_in = 0.0;
        battery_level = 0.0;
        amp_hours = 0.0;
        amp_hours_charged = 0.0;
        watt_hours = 0.0;
        watt_hours_charged = 0.0;
        tachometer = 0.0;
        tachometer_abs = 0.0;
        position = 0.0;
        fault_code = FAULT_CODE_NONE;
        vesc_id = 0;
        num_vescs = 0;
        battery_wh = 0.0;
        odometer = 0;
        uptime_ms = 0;
    }

    bool operator==(const SETUP_VALUES &other) const {
        (void)other;
        // compare members
        return true;
    }

    bool operator!=(SETUP_VALUES const &other) const {
        return !(*this == other);
    }

    Q_INVOKABLE QString uptimeString() {
        QTime t(0, 0);
        t = t.addMSecs(uptime_ms);
        return t.toString("hh:mm:ss");
    }

    double temp_mos;
    double temp_motor;
    double current_motor;
    double current_in;
    double duty_now;
    double rpm;
    double speed;
    double v_in;
    double battery_level;
    double amp_hours;
    double amp_hours_charged;
    double watt_hours;
    double watt_hours_charged;
    double tachometer;
    double tachometer_abs;
    double position;
    mc_fault_code fault_code;
    int vesc_id;
    int num_vescs;
    double battery_wh;
    QString fault_str;
    unsigned odometer;
    unsigned uptime_ms;
};

Q_DECLARE_METATYPE(SETUP_VALUES)

struct IMU_VALUES {
    Q_GADGET

    Q_PROPERTY(double roll MEMBER roll)
    Q_PROPERTY(double pitch MEMBER pitch)
    Q_PROPERTY(double yaw MEMBER yaw)

    Q_PROPERTY(double accX MEMBER accX)
    Q_PROPERTY(double accY MEMBER accY)
    Q_PROPERTY(double accZ MEMBER accZ)

    Q_PROPERTY(double gyroX MEMBER gyroX)
    Q_PROPERTY(double gyroY MEMBER gyroY)
    Q_PROPERTY(double gyroZ MEMBER gyroZ)

    Q_PROPERTY(double magX MEMBER magX)
    Q_PROPERTY(double magY MEMBER magY)
    Q_PROPERTY(double magZ MEMBER magZ)

    Q_PROPERTY(double q0 MEMBER q0)
    Q_PROPERTY(double q1 MEMBER q1)
    Q_PROPERTY(double q2 MEMBER q2)
    Q_PROPERTY(double q3 MEMBER q3)

    Q_PROPERTY(int vesc_id MEMBER vesc_id)

public:
    IMU_VALUES() {
        roll = 0; pitch = 0; yaw = 0;
        accX = 0; accY = 0; accZ = 0;
        gyroX = 0; gyroY = 0; gyroZ = 0;
        magX = 0; magY = 0; magZ = 0;
        q0 = 1; q1 = 0; q2 = 0; q3 = 0;
        vesc_id = 0;
    }

    bool operator==(const IMU_VALUES &other) const {
        (void)other;
        // compare members
        return true;
    }

    bool operator!=(IMU_VALUES const &other) const {
        return !(*this == other);
    }

    double roll;
    double pitch;
    double yaw;

    double accX;
    double accY;
    double accZ;

    double gyroX;
    double gyroY;
    double gyroZ;

    double magX;
    double magY;
    double magZ;

    double q0;
    double q1;
    double q2;
    double q3;

    int vesc_id;
};

Q_DECLARE_METATYPE(IMU_VALUES)

struct STAT_VALUES {
    Q_GADGET

    Q_PROPERTY(double speed_avg MEMBER speed_avg)
    Q_PROPERTY(double speed_max MEMBER speed_max)
    Q_PROPERTY(double power_avg MEMBER power_avg)
    Q_PROPERTY(double power_max MEMBER power_max)
    Q_PROPERTY(double temp_motor_avg MEMBER temp_motor_avg)
    Q_PROPERTY(double temp_motor_max MEMBER temp_motor_max)
    Q_PROPERTY(double temp_mos_avg MEMBER temp_mos_avg)
    Q_PROPERTY(double temp_mos_max MEMBER temp_mos_max)
    Q_PROPERTY(double current_avg MEMBER current_avg)
    Q_PROPERTY(double current_max MEMBER current_max)
    Q_PROPERTY(double count_time MEMBER count_time)

public:
    STAT_VALUES() {
        speed_avg = 0.0;
        speed_max = 0.0;
        power_avg = 0.0;
        power_max = 0.0;
        temp_motor_avg = 0.0;
        temp_motor_max = 0.0;
        temp_mos_avg = 0.0;
        temp_mos_max = 0.0;
        current_avg = 0.0;
        current_max = 0.0;
        count_time = 0.0;
    }

    bool operator==(const STAT_VALUES &other) const {
        (void)other;
        // compare members
        return true;
    }

    bool operator!=(STAT_VALUES const &other) const {
        return !(*this == other);
    }

    Q_INVOKABLE double distance() { // Meters
        return speed_avg * count_time;
    }

    Q_INVOKABLE double energy() { // Wh
        return power_avg * count_time / 60.0 / 60.0;
    }

    Q_INVOKABLE double ah() {
        return current_avg * count_time / 60.0 / 60.0;
    }

    Q_INVOKABLE double efficiency() { // Wh / km
        return energy() / (distance() / 1000.0);
    }

    double speed_avg;
    double speed_max;
    double power_avg;
    double power_max;
    double temp_motor_avg;
    double temp_motor_max;
    double temp_mos_avg;
    double temp_mos_max;
    double current_avg;
    double current_max;
    double count_time;
};

Q_DECLARE_METATYPE(STAT_VALUES)

struct LOG_HEADER {
    Q_GADGET

    Q_PROPERTY(QString key MEMBER key)
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(QString unit MEMBER unit)
    Q_PROPERTY(int precision MEMBER precision)
    Q_PROPERTY(bool isTimeStamp MEMBER isTimeStamp)
    Q_PROPERTY(bool isRelativeToFirst MEMBER isRelativeToFirst)
    Q_PROPERTY(double scaleStep MEMBER scaleStep)
    Q_PROPERTY(double scaleMax MEMBER scaleMax)

public:
    LOG_HEADER() {
        precision = 2;
        isRelativeToFirst = false;
        isTimeStamp = false;
        scaleStep = 0.1;
        scaleMax = 99.99;
    }

    LOG_HEADER(QString key,
              QString name,
              QString unit,
              int precision = 2,
              bool isRelativeToFirst = false,
              bool isTimeStamp = false,
              double scaleStep = 0.1,
              double scaleMax = 99.99) {
        this->key = key;
        this->name = name;
        this->unit = unit;
        this->precision = precision;
        this->isRelativeToFirst = isRelativeToFirst;
        this->isTimeStamp = isTimeStamp;
        this->scaleStep = scaleStep;
        this->scaleMax = scaleMax;
    }

    QString key;
    QString name;
    QString unit;
    int precision;
    bool isRelativeToFirst;
    bool isTimeStamp;
    double scaleStep;
    double scaleMax;

};

Q_DECLARE_METATYPE(LOG_HEADER)

struct MCCONF_TEMP {
    Q_GADGET

    Q_PROPERTY(double current_min_scale MEMBER current_min_scale)
    Q_PROPERTY(double current_max_scale MEMBER current_max_scale)
    Q_PROPERTY(double erpm_or_speed_min MEMBER erpm_or_speed_min)
    Q_PROPERTY(double erpm_or_speed_max MEMBER erpm_or_speed_max)
    Q_PROPERTY(double duty_min MEMBER duty_min)
    Q_PROPERTY(double duty_max MEMBER duty_max)
    Q_PROPERTY(double watt_min MEMBER watt_min)
    Q_PROPERTY(double watt_max MEMBER watt_max)
    Q_PROPERTY(QString name MEMBER name)

public:
    double current_min_scale;
    double current_max_scale;
    double erpm_or_speed_min;
    double erpm_or_speed_max;
    double duty_min;
    double duty_max;
    double watt_min;
    double watt_max;
    QString name;
};

Q_DECLARE_METATYPE(MCCONF_TEMP)

struct CONFIG_BACKUP {
    Q_GADGET

    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(QString vesc_uuid MEMBER vesc_uuid)
    Q_PROPERTY(QString mcconf_xml_compressed MEMBER mcconf_xml_compressed)
    Q_PROPERTY(QString appconf_xml_compressed MEMBER appconf_xml_compressed)
    Q_PROPERTY(QString customconf_xml_compressed MEMBER customconf_xml_compressed)

public:
    QString name;
    QString vesc_uuid;
    QString mcconf_xml_compressed;
    QString appconf_xml_compressed;
    QString customconf_xml_compressed;
};

Q_DECLARE_METATYPE(CONFIG_BACKUP)

struct FW_RX_PARAMS {
    Q_GADGET

    Q_PROPERTY(int major MEMBER major)
    Q_PROPERTY(int minor MEMBER minor)
    Q_PROPERTY(QString fwName MEMBER fwName)
    Q_PROPERTY(QString hw MEMBER hw)
    Q_PROPERTY(QByteArray uuid MEMBER uuid)
    Q_PROPERTY(bool isPaired MEMBER isPaired)
    Q_PROPERTY(int isTestFw MEMBER isTestFw)
    Q_PROPERTY(HW_TYPE hwType MEMBER hwType)
    Q_PROPERTY(int customConfigNum MEMBER customConfigNum)
    Q_PROPERTY(bool hasQmlHw MEMBER hasQmlHw)
    Q_PROPERTY(bool qmlHwFullscreen MEMBER qmlHwFullscreen)
    Q_PROPERTY(bool hasQmlApp MEMBER hasQmlApp)
    Q_PROPERTY(bool qmlAppFullscreen MEMBER qmlAppFullscreen)
    Q_PROPERTY(bool nrfNameSupported MEMBER nrfNameSupported)
    Q_PROPERTY(bool nrfPinSupported MEMBER nrfPinSupported)
    Q_PROPERTY(quint32 hwConfCrc MEMBER hwConfCrc)

public:
    FW_RX_PARAMS() {
        major = -1;
        minor = -1;
        isPaired = false;
        isTestFw = false;
        hwType = HW_TYPE_VESC;
        customConfigNum = 0;
        hasPhaseFilters = false;
        hasQmlHw = false;
        qmlHwFullscreen = false;
        hasQmlApp = false;
        qmlAppFullscreen = false;
        nrfNameSupported = false;
        nrfPinSupported = false;
        hwConfCrc = 0;
    }

    Q_INVOKABLE QString hwTypeStr() {
        QString res = "Unknown Hardware";
        switch (hwType) {
        case HW_TYPE_VESC:
            res = "VESC";
            break;

        case HW_TYPE_VESC_BMS:
            res = "VESC BMS";
            break;

        case HW_TYPE_CUSTOM_MODULE:
            res = "Custom Module";
            break;
        }

        return res;
    }

    int major;
    int minor;
    QString fwName;
    QString hw;
    QByteArray uuid;
    bool isPaired;
    int isTestFw;
    HW_TYPE hwType;
    int customConfigNum;
    bool hasPhaseFilters;
    bool hasQmlHw;
    bool qmlHwFullscreen;
    bool hasQmlApp;
    bool qmlAppFullscreen;
    bool nrfNameSupported;
    bool nrfPinSupported;
    quint32 hwConfCrc;

};

Q_DECLARE_METATYPE(FW_RX_PARAMS)

struct BMS_VALUES {
    Q_GADGET

    Q_PROPERTY(double v_tot MEMBER v_tot)
    Q_PROPERTY(double v_charge MEMBER v_charge)
    Q_PROPERTY(double i_in MEMBER i_in)
    Q_PROPERTY(double i_in_ic MEMBER i_in_ic)
    Q_PROPERTY(double ah_cnt MEMBER ah_cnt)
    Q_PROPERTY(double wh_cnt MEMBER wh_cnt)
    Q_PROPERTY(QVector<qreal> v_cells MEMBER v_cells)
    Q_PROPERTY(QVector<qreal> temps MEMBER temps)
    Q_PROPERTY(QVector<bool> is_balancing MEMBER is_balancing)
    Q_PROPERTY(double temp_ic MEMBER temp_ic)
    Q_PROPERTY(double humidity MEMBER humidity)
    Q_PROPERTY(double pressure MEMBER pressure)
    Q_PROPERTY(double temp_hum_sensor MEMBER temp_hum_sensor)
    Q_PROPERTY(double temp_cells_highest MEMBER temp_cells_highest)
    Q_PROPERTY(double soc MEMBER soc)
    Q_PROPERTY(double soh MEMBER soh)
    Q_PROPERTY(int can_id MEMBER can_id)
    Q_PROPERTY(double ah_cnt_chg_total MEMBER ah_cnt_chg_total)
    Q_PROPERTY(double wh_cnt_chg_total MEMBER wh_cnt_chg_total)
    Q_PROPERTY(double ah_cnt_dis_total MEMBER ah_cnt_dis_total)
    Q_PROPERTY(double wh_cnt_dis_total MEMBER wh_cnt_dis_total)
    Q_PROPERTY(int data_version MEMBER data_version)
    Q_PROPERTY(QString status MEMBER status)

public:
    BMS_VALUES() {
        v_tot = 0.0;
        v_charge = 0.0;
        i_in = 0.0;
        i_in_ic = 0.0;
        ah_cnt = 0.0;
        wh_cnt = 0.0;
        humidity = 0.0;
        pressure = 0.0;
        temp_hum_sensor = 0.0;
        temp_cells_highest = 0.0;
        soc = 0.0;
        soh = 0.0;
        can_id = -1;
        ah_cnt_chg_total = 0.0;
        wh_cnt_chg_total = 0.0;
        ah_cnt_dis_total = 0.0;
        wh_cnt_dis_total = 0.0;
        data_version = 0;
        updateTime = -1;
    }

    Q_INVOKABLE double age() {
        double res = -1.0;

        if (updateTime > 0) {
            auto ms = QDateTime::currentDateTime().toMSecsSinceEpoch() - updateTime;
            res = double(ms) / 1000.0;
        }

        return res;
    }

    Q_INVOKABLE void updateTimeStamp() {
        updateTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
    }

    double v_tot;
    double v_charge;
    double i_in;
    double i_in_ic;
    double ah_cnt;
    double wh_cnt;
    QVector<qreal> v_cells;
    QVector<qreal> temps;
    QVector<bool> is_balancing;
    double temp_ic;
    double humidity;
    double pressure;
    double temp_hum_sensor;
    double temp_cells_highest;
    double soc;
    double soh;
    int can_id;
    double ah_cnt_chg_total;
    double wh_cnt_chg_total;
    double ah_cnt_dis_total;
    double wh_cnt_dis_total;
    int data_version;
    QString status;
    qint64 updateTime;
};

Q_DECLARE_METATYPE(BMS_VALUES)

struct PSW_STATUS {
    Q_GADGET

    Q_PROPERTY(int id MEMBER id)
    Q_PROPERTY(int psws_num MEMBER psws_num)
    Q_PROPERTY(double age_seconds MEMBER age_seconds)
    Q_PROPERTY(double v_in MEMBER v_in)
    Q_PROPERTY(double v_out MEMBER v_out)
    Q_PROPERTY(double temp MEMBER temp)
    Q_PROPERTY(bool is_out_on MEMBER is_out_on)
    Q_PROPERTY(bool is_pch_on MEMBER is_pch_on)
    Q_PROPERTY(bool is_dsc_on MEMBER is_dsc_on)

public:
    PSW_STATUS() {
        id = -1;
        psws_num = 0;
        age_seconds = -1.0;
        v_in = 0.0;
        v_out = 0.0;
        temp = 0.0;
        is_out_on = false;
        is_pch_on = false;
        is_dsc_on = false;
    }

    int id;
    int psws_num;
    double age_seconds;
    double v_in;
    double v_out;
    double temp;
    bool is_out_on;
    bool is_pch_on;
    bool is_dsc_on;

};

Q_DECLARE_METATYPE(PSW_STATUS)

struct IO_BOARD_VALUES {
    Q_GADGET

    Q_PROPERTY(int id MEMBER id)
    Q_PROPERTY(QVector<qreal> adc_1_4 MEMBER adc_1_4)
    Q_PROPERTY(QVector<qreal> adc_5_8 MEMBER adc_5_8)
    Q_PROPERTY(QVector<bool> digital MEMBER digital)
    Q_PROPERTY(double adc_1_4_age MEMBER adc_1_4_age)
    Q_PROPERTY(double adc_5_8_age MEMBER adc_5_8_age)
    Q_PROPERTY(double digital_age MEMBER digital_age)

public:
    IO_BOARD_VALUES() {
        id = -1;
        adc_1_4_age = -1.0;
        adc_5_8_age = -1.0;
        digital_age = -1.0;
    }

    int id;
    QVector<qreal> adc_1_4;
    QVector<qreal> adc_5_8;
    QVector<bool> digital;
    double adc_1_4_age;
    double adc_5_8_age;
    double digital_age;
};

Q_DECLARE_METATYPE(IO_BOARD_VALUES)

typedef enum {
    DEBUG_SAMPLING_OFF = 0,
    DEBUG_SAMPLING_NOW,
    DEBUG_SAMPLING_START,
    DEBUG_SAMPLING_TRIGGER_START,
    DEBUG_SAMPLING_TRIGGER_FAULT,
    DEBUG_SAMPLING_TRIGGER_START_NOSEND,
    DEBUG_SAMPLING_TRIGGER_FAULT_NOSEND,
    DEBUG_SAMPLING_SEND_LAST_SAMPLES,
    DEBUG_SAMPLING_SEND_SINGLE_SAMPLE
} debug_sampling_mode;

typedef enum {
	COMM_FW_VERSION							= 0,
	COMM_JUMP_TO_BOOTLOADER					= 1,
	COMM_ERASE_NEW_APP						= 2,
	COMM_WRITE_NEW_APP_DATA					= 3,
	COMM_GET_VALUES							= 4,
	COMM_SET_DUTY							= 5,
	COMM_SET_CURRENT						= 6,
	COMM_SET_CURRENT_BRAKE					= 7,
	COMM_SET_RPM							= 8,
	COMM_SET_POS							= 9,
	COMM_SET_HANDBRAKE						= 10,
	COMM_SET_DETECT							= 11,
	COMM_SET_SERVO_POS						= 12,
	COMM_SET_MCCONF							= 13,
	COMM_GET_MCCONF							= 14,
	COMM_GET_MCCONF_DEFAULT					= 15,
	COMM_SET_APPCONF						= 16,
	COMM_GET_APPCONF						= 17,
	COMM_GET_APPCONF_DEFAULT				= 18,
	COMM_SAMPLE_PRINT						= 19,
	COMM_TERMINAL_CMD						= 20,
	COMM_PRINT								= 21,
	COMM_ROTOR_POSITION						= 22,
	COMM_EXPERIMENT_SAMPLE					= 23,
	COMM_DETECT_MOTOR_PARAM					= 24,
	COMM_DETECT_MOTOR_R_L					= 25,
	COMM_DETECT_MOTOR_FLUX_LINKAGE			= 26,
	COMM_DETECT_ENCODER						= 27,
	COMM_DETECT_HALL_FOC					= 28,
	COMM_REBOOT								= 29,
	COMM_ALIVE								= 30,
	COMM_GET_DECODED_PPM					= 31,
	COMM_GET_DECODED_ADC					= 32,
	COMM_GET_DECODED_CHUK					= 33,
	COMM_FORWARD_CAN						= 34,
	COMM_SET_CHUCK_DATA						= 35,
	COMM_CUSTOM_APP_DATA					= 36,
	COMM_NRF_START_PAIRING					= 37,
	COMM_GPD_SET_FSW						= 38,
	COMM_GPD_BUFFER_NOTIFY					= 39,
	COMM_GPD_BUFFER_SIZE_LEFT				= 40,
	COMM_GPD_FILL_BUFFER					= 41,
	COMM_GPD_OUTPUT_SAMPLE					= 42,
	COMM_GPD_SET_MODE						= 43,
	COMM_GPD_FILL_BUFFER_INT8				= 44,
	COMM_GPD_FILL_BUFFER_INT16				= 45,
	COMM_GPD_SET_BUFFER_INT_SCALE			= 46,
	COMM_GET_VALUES_SETUP					= 47,
	COMM_SET_MCCONF_TEMP					= 48,
	COMM_SET_MCCONF_TEMP_SETUP				= 49,
	COMM_GET_VALUES_SELECTIVE				= 50,
	COMM_GET_VALUES_SETUP_SELECTIVE			= 51,
	COMM_EXT_NRF_PRESENT					= 52,
	COMM_EXT_NRF_ESB_SET_CH_ADDR			= 53,
	COMM_EXT_NRF_ESB_SEND_DATA				= 54,
	COMM_EXT_NRF_ESB_RX_DATA				= 55,
	COMM_EXT_NRF_SET_ENABLED				= 56,
	COMM_DETECT_MOTOR_FLUX_LINKAGE_OPENLOOP	= 57,
	COMM_DETECT_APPLY_ALL_FOC				= 58,
	COMM_JUMP_TO_BOOTLOADER_ALL_CAN			= 59,
	COMM_ERASE_NEW_APP_ALL_CAN				= 60,
	COMM_WRITE_NEW_APP_DATA_ALL_CAN			= 61,
	COMM_PING_CAN							= 62,
	COMM_APP_DISABLE_OUTPUT					= 63,
	COMM_TERMINAL_CMD_SYNC					= 64,
	COMM_GET_IMU_DATA						= 65,
	COMM_BM_CONNECT							= 66,
	COMM_BM_ERASE_FLASH_ALL					= 67,
	COMM_BM_WRITE_FLASH						= 68,
	COMM_BM_REBOOT							= 69,
	COMM_BM_DISCONNECT						= 70,
	COMM_BM_MAP_PINS_DEFAULT				= 71,
	COMM_BM_MAP_PINS_NRF5X					= 72,
	COMM_ERASE_BOOTLOADER					= 73,
	COMM_ERASE_BOOTLOADER_ALL_CAN			= 74,
	COMM_PLOT_INIT							= 75,
	COMM_PLOT_DATA							= 76,
	COMM_PLOT_ADD_GRAPH						= 77,
	COMM_PLOT_SET_GRAPH						= 78,
	COMM_GET_DECODED_BALANCE				= 79,
	COMM_BM_MEM_READ						= 80,
	COMM_WRITE_NEW_APP_DATA_LZO				= 81,
	COMM_WRITE_NEW_APP_DATA_ALL_CAN_LZO		= 82,
	COMM_BM_WRITE_FLASH_LZO					= 83,
	COMM_SET_CURRENT_REL					= 84,
	COMM_CAN_FWD_FRAME						= 85,
	COMM_SET_BATTERY_CUT					= 86,
	COMM_SET_BLE_NAME						= 87,
	COMM_SET_BLE_PIN						= 88,
	COMM_SET_CAN_MODE						= 89,
	COMM_GET_IMU_CALIBRATION				= 90,
	COMM_GET_MCCONF_TEMP					= 91,

	// Custom configuration for hardware
	COMM_GET_CUSTOM_CONFIG_XML				= 92,
	COMM_GET_CUSTOM_CONFIG					= 93,
	COMM_GET_CUSTOM_CONFIG_DEFAULT			= 94,
	COMM_SET_CUSTOM_CONFIG					= 95,

	// BMS commands
	COMM_BMS_GET_VALUES						= 96,
	COMM_BMS_SET_CHARGE_ALLOWED				= 97,
	COMM_BMS_SET_BALANCE_OVERRIDE			= 98,
	COMM_BMS_RESET_COUNTERS					= 99,
	COMM_BMS_FORCE_BALANCE					= 100,
	COMM_BMS_ZERO_CURRENT_OFFSET			= 101,

	// FW updates commands for different HW types
	COMM_JUMP_TO_BOOTLOADER_HW				= 102,
	COMM_ERASE_NEW_APP_HW					= 103,
	COMM_WRITE_NEW_APP_DATA_HW				= 104,
	COMM_ERASE_BOOTLOADER_HW				= 105,
	COMM_JUMP_TO_BOOTLOADER_ALL_CAN_HW		= 106,
	COMM_ERASE_NEW_APP_ALL_CAN_HW			= 107,
	COMM_WRITE_NEW_APP_DATA_ALL_CAN_HW		= 108,
	COMM_ERASE_BOOTLOADER_ALL_CAN_HW		= 109,

	COMM_SET_ODOMETER						= 110,

	// Power switch commands
	COMM_PSW_GET_STATUS						= 111,
	COMM_PSW_SWITCH							= 112,

	COMM_BMS_FWD_CAN_RX						= 113,
	COMM_BMS_HW_DATA						= 114,
	COMM_GET_BATTERY_CUT					= 115,
	COMM_BM_HALT_REQ						= 116,
	COMM_GET_QML_UI_HW						= 117,
	COMM_GET_QML_UI_APP						= 118,
	COMM_CUSTOM_HW_DATA						= 119,
	COMM_QMLUI_ERASE						= 120,
	COMM_QMLUI_WRITE						= 121,

	// IO Board
	COMM_IO_BOARD_GET_ALL					= 122,
	COMM_IO_BOARD_SET_PWM					= 123,
	COMM_IO_BOARD_SET_DIGITAL				= 124,

	COMM_BM_MEM_WRITE						= 125,
	COMM_BMS_BLNC_SELFTEST					= 126,
	COMM_GET_EXT_HUM_TMP					= 127,
	COMM_GET_STATS							= 128,
	COMM_RESET_STATS						= 129,

	// Lisp
	COMM_LISP_READ_CODE						= 130,
	COMM_LISP_WRITE_CODE					= 131,
	COMM_LISP_ERASE_CODE					= 132,
	COMM_LISP_SET_RUNNING					= 133,
	COMM_LISP_GET_STATS						= 134,
	COMM_LISP_PRINT							= 135,

	COMM_BMS_SET_BATT_TYPE					= 136,
	COMM_BMS_GET_BATT_TYPE					= 137,

	COMM_LISP_REPL_CMD						= 138,
	COMM_LISP_STREAM_CODE					= 139,

	COMM_FILE_LIST							= 140,
	COMM_FILE_READ							= 141,
	COMM_FILE_WRITE							= 142,
	COMM_FILE_MKDIR							= 143,
	COMM_FILE_REMOVE						= 144,

	COMM_LOG_START							= 145,
	COMM_LOG_STOP							= 146,
	COMM_LOG_CONFIG_FIELD					= 147,
	COMM_LOG_DATA_F32						= 148,

	COMM_SET_APPCONF_NO_STORE				= 149,
	COMM_GET_GNSS							= 150,

	COMM_LOG_DATA_F64						= 151,

	COMM_LISP_RMSG							= 152,

	//Placeholders for pinlock commands
	//COMM_PINLOCK1							= 153,
	//COMM_PINLOCK2							= 154,
	//COMM_PINLOCK3							= 155,

    COMM_SHUTDOWN							= 156,

    COMM_FW_INFO							= 157,

    COMM_CAN_UPDATE_BAUD_ALL				= 158,

    COMM_MOTOR_ESTOP						= 159,
} COMM_PACKET_ID;

// CAN commands
typedef enum {
    CAN_PACKET_SET_DUTY						= 0,
    CAN_PACKET_SET_CURRENT					= 1,
    CAN_PACKET_SET_CURRENT_BRAKE			= 2,
    CAN_PACKET_SET_RPM						= 3,
    CAN_PACKET_SET_POS						= 4,
    CAN_PACKET_FILL_RX_BUFFER				= 5,
    CAN_PACKET_FILL_RX_BUFFER_LONG			= 6,
    CAN_PACKET_PROCESS_RX_BUFFER			= 7,
    CAN_PACKET_PROCESS_SHORT_BUFFER			= 8,
    CAN_PACKET_STATUS						= 9,
    CAN_PACKET_SET_CURRENT_REL				= 10,
    CAN_PACKET_SET_CURRENT_BRAKE_REL		= 11,
    CAN_PACKET_SET_CURRENT_HANDBRAKE		= 12,
    CAN_PACKET_SET_CURRENT_HANDBRAKE_REL	= 13,
    CAN_PACKET_STATUS_2						= 14,
    CAN_PACKET_STATUS_3						= 15,
    CAN_PACKET_STATUS_4						= 16,
    CAN_PACKET_PING							= 17,
    CAN_PACKET_PONG							= 18,
    CAN_PACKET_DETECT_APPLY_ALL_FOC			= 19,
    CAN_PACKET_DETECT_APPLY_ALL_FOC_RES		= 20,
    CAN_PACKET_CONF_CURRENT_LIMITS			= 21,
    CAN_PACKET_CONF_STORE_CURRENT_LIMITS	= 22,
    CAN_PACKET_CONF_CURRENT_LIMITS_IN		= 23,
    CAN_PACKET_CONF_STORE_CURRENT_LIMITS_IN	= 24,
    CAN_PACKET_CONF_FOC_ERPMS				= 25,
    CAN_PACKET_CONF_STORE_FOC_ERPMS			= 26,
    CAN_PACKET_STATUS_5						= 27,
    CAN_PACKET_POLL_TS5700N8501_STATUS		= 28,
    CAN_PACKET_CONF_BATTERY_CUT				= 29,
    CAN_PACKET_CONF_STORE_BATTERY_CUT		= 30,
    CAN_PACKET_SHUTDOWN						= 31,
    CAN_PACKET_IO_BOARD_ADC_1_TO_4			= 32,
    CAN_PACKET_IO_BOARD_ADC_5_TO_8			= 33,
    CAN_PACKET_IO_BOARD_ADC_9_TO_12			= 34,
    CAN_PACKET_IO_BOARD_DIGITAL_IN			= 35,
    CAN_PACKET_IO_BOARD_SET_OUTPUT_DIGITAL	= 36,
    CAN_PACKET_IO_BOARD_SET_OUTPUT_PWM		= 37,
    CAN_PACKET_BMS_V_TOT					= 38,
    CAN_PACKET_BMS_I						= 39,
    CAN_PACKET_BMS_AH_WH					= 40,
    CAN_PACKET_BMS_V_CELL					= 41,
    CAN_PACKET_BMS_BAL						= 42,
    CAN_PACKET_BMS_TEMPS					= 43,
    CAN_PACKET_BMS_HUM						= 44,
    CAN_PACKET_BMS_SOC_SOH_TEMP_STAT		= 45,
    CAN_PACKET_PSW_STAT						= 46,
    CAN_PACKET_PSW_SWITCH					= 47,
    CAN_PACKET_BMS_HW_DATA_1				= 48,
    CAN_PACKET_BMS_HW_DATA_2				= 49,
    CAN_PACKET_BMS_HW_DATA_3				= 50,
    CAN_PACKET_BMS_HW_DATA_4				= 51,
    CAN_PACKET_BMS_HW_DATA_5				= 52,
    CAN_PACKET_BMS_AH_WH_CHG_TOTAL			= 53,
    CAN_PACKET_BMS_AH_WH_DIS_TOTAL			= 54,
    CAN_PACKET_UPDATE_PID_POS_OFFSET		= 55,
    CAN_PACKET_POLL_ROTOR_POS				= 56,
    CAN_PACKET_NOTIFY_BOOT					= 57,
    CAN_PACKET_STATUS_6						= 58,
    CAN_PACKET_GNSS_TIME					= 59,
    CAN_PACKET_GNSS_LAT						= 60,
    CAN_PACKET_GNSS_LON						= 61,
    CAN_PACKET_GNSS_ALT_SPEED_HDOP			= 62,
    CAN_PACKET_UPDATE_BAUD					= 63,
    CAN_PACKET_MAKE_ENUM_32_BITS = 0xFFFFFFFF,
} CAN_PACKET_ID;

typedef struct {
    int js_x;
    int js_y;
    int acc_x;
    int acc_y;
    int acc_z;
    bool bt_c;
    bool bt_z;
} chuck_data;

struct bldc_detect {
    Q_GADGET

    Q_PROPERTY(double cycle_int_limit MEMBER cycle_int_limit)
    Q_PROPERTY(double bemf_coupling_k MEMBER bemf_coupling_k)
    Q_PROPERTY(QVector<int> hall_table MEMBER hall_table)
    Q_PROPERTY(int hall_res MEMBER hall_res)

public:
    double cycle_int_limit;
    double bemf_coupling_k;
    QVector<int> hall_table;
    int hall_res;
};

Q_DECLARE_METATYPE(bldc_detect)

typedef enum {
    NRF_PAIR_STARTED = 0,
    NRF_PAIR_OK,
    NRF_PAIR_FAIL
} NRF_PAIR_RES;

struct LISP_STATS {
    Q_GADGET

public:
    Q_PROPERTY(double cpu_use MEMBER cpu_use)
    Q_PROPERTY(double heap_use MEMBER heap_use)
    Q_PROPERTY(double mem_use MEMBER mem_use)
    Q_PROPERTY(double stack_use MEMBER stack_use)
    Q_PROPERTY(QString done_ctx_r MEMBER done_ctx_r)

    LISP_STATS() {
        cpu_use = 0.0;
        heap_use = 0.0;
        mem_use = 0.0;
        stack_use = 0.0;
    }

    double cpu_use;
    double heap_use;
    double mem_use;
    double stack_use;
    QString done_ctx_r;
    QVector<QPair<QString, double>> number_bindings;
};

Q_DECLARE_METATYPE(LISP_STATS)

struct ENCODER_DETECT_RES {
    Q_GADGET

public:
    Q_PROPERTY(double offset MEMBER offset)
    Q_PROPERTY(double ratio MEMBER ratio)
    Q_PROPERTY(bool inverted MEMBER inverted)
    Q_PROPERTY(bool detect_rx MEMBER detect_rx)

    ENCODER_DETECT_RES() {
        offset = 0.0;
        ratio = 0.0;
        inverted = false;
        detect_rx = false;
    }

    double offset;
    double ratio;
    bool inverted;
    bool detect_rx;
};

Q_DECLARE_METATYPE(ENCODER_DETECT_RES)

struct FILE_LIST_ENTRY {
    Q_GADGET

public:
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(bool isDir MEMBER isDir)
    Q_PROPERTY(qint32 size MEMBER size)

    FILE_LIST_ENTRY() {
        isDir = false;
        size = false;
    }

    QString name;
    bool isDir;
    qint32 size;
};

Q_DECLARE_METATYPE(FILE_LIST_ENTRY)

struct VescPackage {
    Q_GADGET

public:
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(QString description MEMBER description)
    Q_PROPERTY(QString description_md MEMBER description_md)
    Q_PROPERTY(QByteArray lispData MEMBER lispData)
    Q_PROPERTY(QString qmlFile MEMBER qmlFile)
    Q_PROPERTY(QString pkgDescQml MEMBER pkgDescQml)
    Q_PROPERTY(bool qmlIsFullscreen MEMBER qmlIsFullscreen)
    Q_PROPERTY(bool isLibrary MEMBER isLibrary)
    Q_PROPERTY(bool loadOk MEMBER loadOk)
    Q_PROPERTY(QByteArray compressedData MEMBER compressedData)

    VescPackage () {
        name = "VESC Package Name";
        qmlIsFullscreen = false;
        isLibrary = false;
        loadOk = false;
    }

    QByteArray compressedData;
    QString name;
    QString description;
    QString description_md;
    QByteArray lispData;
    QString qmlFile;
    QString pkgDescQml;
    bool qmlIsFullscreen;
    bool isLibrary;
    bool loadOk;

};

Q_DECLARE_METATYPE(VescPackage)

struct TCP_HUB_DEVICE {
    Q_GADGET

public:
    Q_PROPERTY(QString server MEMBER server)
    Q_PROPERTY(int port MEMBER port)
    Q_PROPERTY(QString id MEMBER id)
    Q_PROPERTY(QString password MEMBER password)

    TCP_HUB_DEVICE() {
        port = 65101;
    }

    Q_INVOKABLE bool ping() {
        return TcpHub::ping(server, port, id);
    }

    Q_INVOKABLE QString uuid() {
        return QString("%1:%2:%3").arg(server).arg(port).arg(id);
    }

    QString server;
    int port;
    QString id;
    QString password;
};

Q_DECLARE_METATYPE(TCP_HUB_DEVICE)

#ifndef FE_WGS84
#define FE_WGS84        (1.0/298.257223563) // earth flattening (WGS84)
#endif
#ifndef RE_WGS84
#define RE_WGS84        6378137.0           // earth semimajor axis (WGS84) (m)
#endif
#ifndef SQ
#define SQ(x) ((x) * (x))
#endif

struct LOG_DATA {
    Q_GADGET

    Q_PROPERTY(MC_VALUES values MEMBER values)
    Q_PROPERTY(SETUP_VALUES setupValues MEMBER setupValues)
    Q_PROPERTY(IMU_VALUES imuValues MEMBER imuValues)
    Q_PROPERTY(int valTime MEMBER valTime)
    Q_PROPERTY(int posTime MEMBER posTime)
    Q_PROPERTY(double lat MEMBER lat)
    Q_PROPERTY(double lon MEMBER lon)
    Q_PROPERTY(double alt MEMBER alt)
    Q_PROPERTY(double gVel MEMBER gVel)
    Q_PROPERTY(double vVel MEMBER vVel)
    Q_PROPERTY(double hAcc MEMBER hAcc)
    Q_PROPERTY(double vAcc MEMBER vAcc)

public:
    LOG_DATA() {
        posTime = -1;
        setupValTime = -1;
        imuValTime = -1;
        lat = 0.0;
        lon = 0.0;
        alt = 0.0;
        gVel = 0.0;
        vVel = 0.0;
        hAcc = 0.0;
        vAcc = 0.0;
    }

    double distanceTo(double latOther, double lonOther, double heightOther) const
    {
        double xyz[3];
        llhToXyz(lat, lon, alt, xyz);
        double xyzOther[3];
        llhToXyz(latOther, lonOther, heightOther, xyzOther);
        return sqrt(SQ(xyzOther[0] - xyz[0]) + SQ(xyzOther[1] - xyz[1]) + SQ(xyzOther[2] - xyz[2]));
    }

    double distanceTo(const LOG_DATA &other) const
    {
        return distanceTo(other.lat, other.lon, other.alt);
    }

    void llhToXyz(double lat, double lon, double height, double *xyz) const
    {
        double sinp = sin(lat * M_PI / 180.0);
        double cosp = cos(lat * M_PI / 180.0);
        double sinl = sin(lon * M_PI / 180.0);
        double cosl = cos(lon * M_PI / 180.0);
        double e2 = FE_WGS84 * (2.0 - FE_WGS84);
        double v = RE_WGS84 / sqrt(1.0 - e2 * sinp * sinp);

        xyz[0] = (v + height) * cosp * cosl;
        xyz[1] = (v + height) * cosp * sinl;
        xyz[2] = (v * (1.0 - e2) + height) * sinp;
    }

    MC_VALUES values;
    SETUP_VALUES setupValues;
    IMU_VALUES imuValues;
    int valTime;
    int setupValTime;
    int imuValTime;
    int posTime;
    double lat;
    double lon;
    double alt;
    double gVel;
    double vVel;
    double hAcc;
    double vAcc;
};

Q_DECLARE_METATYPE(LOG_DATA)

struct GNSS_DATA {
    Q_GADGET

public:
    Q_PROPERTY(double lat MEMBER lat)
    Q_PROPERTY(double lon MEMBER lon)
    Q_PROPERTY(double height MEMBER height)
    Q_PROPERTY(double speed MEMBER speed)
    Q_PROPERTY(double hdop MEMBER hdop)
    Q_PROPERTY(qint32 ms_today MEMBER ms_today)
    Q_PROPERTY(int yy MEMBER yy)
    Q_PROPERTY(int mo MEMBER mo)
    Q_PROPERTY(int dd MEMBER dd)
    Q_PROPERTY(double age_s MEMBER age_s)

    GNSS_DATA() {
        lat = 0.0;
        lon = 0.0;
        height = 0.0;
        speed = 0.0;
        hdop = 0.0;
        ms_today = 0;
        yy = 0;
        mo = 0;
        dd = 0;
        age_s = 0.0;
    }

    double distanceTo(double latOther, double lonOther, double heightOther) const
    {
        double xyz[3];
        llhToXyz(lat, lon, height, xyz);
        double xyzOther[3];
        llhToXyz(latOther, lonOther, heightOther, xyzOther);
        return sqrt(SQ(xyzOther[0] - xyz[0]) + SQ(xyzOther[1] - xyz[1]) + SQ(xyzOther[2] - xyz[2]));
    }

    double distanceTo(const GNSS_DATA &other) const
    {
        return distanceTo(other.lat, other.lon, other.height);
    }

    double lat;
    double lon;
    double height;
    double speed;
    double hdop;
    qint32 ms_today;
    int yy;
    int mo;
    int dd;
    double age_s;

    void llhToXyz(double lat, double lon, double height, double *xyz) const
    {
        double sinp = sin(lat * M_PI / 180.0);
        double cosp = cos(lat * M_PI / 180.0);
        double sinl = sin(lon * M_PI / 180.0);
        double cosl = cos(lon * M_PI / 180.0);
        double e2 = FE_WGS84 * (2.0 - FE_WGS84);
        double v = RE_WGS84 / sqrt(1.0 - e2 * sinp * sinp);

        xyz[0] = (v + height) * cosp * cosl;
        xyz[1] = (v + height) * cosp * sinl;
        xyz[2] = (v * (1.0 - e2) + height) * sinp;
    }
};

Q_DECLARE_METATYPE(GNSS_DATA)

#endif // DATATYPES_H
