/*
    Copyright 2016 - 2022 Benjamin Vedder	benjamin@vedder.se

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

#include "commands.h"
#include "qelapsedtimer.h"
#include <QDebug>
#include <QEventLoop>

Commands::Commands(QObject *parent) : QObject(parent)
{
    mSendCan = false;
    mCanId = -1;
    mIsLimitedMode = false;
    mLimitedSupportsFwdAllCan = false;
    mLimitedSupportsEraseBootloader = false;
    mMaxPowerLossBug = false;
    mCheckNextMcConfig = false;

    mTimer = new QTimer(this);
    mTimer->setInterval(10);
    mTimer->start();

    mMcConfig = nullptr;
    mAppConfig = nullptr;

    mTimeoutCount = 100;
    mTimeoutFwVer = 0;
    mTimeoutMcconf = 0;
    mTimeoutAppconf = 0;
    mTimeoutValues = 0;
    mTimeoutValuesSetup = 0;
    mTimeoutImuData = 0;
    mTimeoutDecPpm = 0;
    mTimeoutDecAdc = 0;
    mTimeoutDecChuk = 0;
    mTimeoutPingCan = 0;
    mTimeoutCustomConf = 0;
    mTimeoutBmsVal = 0;

    mFilePercentage = 0.0;
    mFileSpeed = 0.0;

    connect(mTimer, SIGNAL(timeout()), this, SLOT(timerSlot()));
}

void Commands::setLimitedMode(bool is_limited)
{
    mIsLimitedMode = is_limited;
}

bool Commands::isLimitedMode()
{
    return mIsLimitedMode;
}

bool Commands::setSendCan(bool sendCan, int id)
{
    if (id >= 0) {
        mCanId = id;
    }

    if (mCanId >= 0) {
        mSendCan = sendCan;
    } else {
        mSendCan = false;
    }

    return mSendCan == sendCan;
}

bool Commands::getSendCan()
{
    return mSendCan;
}

void Commands::setCanSendId(unsigned int id)
{
    mCanId = int(id);
}

int Commands::getCanSendId()
{
    return mCanId;
}

void Commands::processPacket(QByteArray data)
{
    VByteArray vb(data);
    COMM_PACKET_ID id = COMM_PACKET_ID(vb.vbPopFrontUint8());

    switch (id) {
    case COMM_FW_VERSION: {
        mTimeoutFwVer = 0;
        FW_RX_PARAMS params;

        if (vb.size() >= 2) {
            params.major = vb.vbPopFrontInt8();
            params.minor = vb.vbPopFrontInt8();
            params.hw = vb.vbPopFrontString();
        }

        if (vb.size() >= 12) {
            params.uuid.append(vb.left(12));
            vb.remove(0, 12);
        }

        if (vb.size() >= 1) {
            params.isPaired = vb.vbPopFrontInt8();
        }

        if (vb.size() >= 1) {
            params.isTestFw = vb.vbPopFrontInt8();
        }

        if (vb.size() >= 1) {
            params.hwType = HW_TYPE(vb.vbPopFrontInt8());
        }

        if (vb.size() >= 1) {
            params.customConfigNum = vb.vbPopFrontInt8();
        }

        if (vb.size() >= 1) {
            params.hasPhaseFilters = vb.vbPopFrontInt8();
        }

        if (vb.size() >= 2) {
            auto qmlHw = vb.vbPopFrontInt8();
            auto qmlApp = vb.vbPopFrontInt8();
            params.hasQmlHw = qmlHw > 0;
            params.qmlHwFullscreen = qmlHw == 2;
            params.hasQmlApp = qmlApp > 0;
            params.qmlAppFullscreen = qmlApp == 2;
        }

        if (vb.size() >= 1) {
            auto nrfFlags = vb.vbPopFrontUint8();
            params.nrfNameSupported = nrfFlags & 1;
            params.nrfPinSupported = nrfFlags & 2;
        }

        if (vb.size() >= 1) {
            params.fwName = vb.vbPopFrontString();
        }

        emit fwVersionReceived(params);
    } break;

    case COMM_ERASE_NEW_APP:
        emit eraseNewAppResReceived(vb.at(0));
        break;

    case COMM_WRITE_NEW_APP_DATA: {
        bool ok = vb.vbPopFrontInt8();
        bool hasOffset = false;
        quint32 offset = 0;
        if (vb.size() >= 4) {
            hasOffset = true;
            offset = vb.vbPopFrontUint32();
        }
        emit writeNewAppDataResReceived(ok, hasOffset, offset);
    } break;

    case COMM_ERASE_BOOTLOADER:
        emit eraseBootloaderResReceived(vb.at(0));
        break;

    case COMM_GET_VALUES:
    case COMM_GET_VALUES_SELECTIVE: {
        mTimeoutValues = 0;
        MC_VALUES values;

        uint32_t mask = 0xFFFFFFFF;
        if (id == COMM_GET_VALUES_SELECTIVE) {
            mask = vb.vbPopFrontUint32();
        }

        if (mask & (uint32_t(1) << 0)) {
            values.temp_mos = vb.vbPopFrontDouble16(1e1);
        }
        if (mask & (uint32_t(1) << 1)) {
            values.temp_motor = vb.vbPopFrontDouble16(1e1);
        }
        if (mask & (uint32_t(1) << 2)) {
            values.current_motor = vb.vbPopFrontDouble32(1e2);
        }
        if (mask & (uint32_t(1) << 3)) {
            values.current_in = vb.vbPopFrontDouble32(1e2);
        }
        if (mask & (uint32_t(1) << 4)) {
            values.id = vb.vbPopFrontDouble32(1e2);
        }
        if (mask & (uint32_t(1) << 5)) {
            values.iq = vb.vbPopFrontDouble32(1e2);
        }
        if (mask & (uint32_t(1) << 6)) {
            values.duty_now = vb.vbPopFrontDouble16(1e3);
        }
        if (mask & (uint32_t(1) << 7)) {
            values.rpm = vb.vbPopFrontDouble32(1e0);
        }
        if (mask & (uint32_t(1) << 8)) {
            values.v_in = vb.vbPopFrontDouble16(1e1);
        }
        if (mask & (uint32_t(1) << 9)) {
            values.amp_hours = vb.vbPopFrontDouble32(1e4);
        }
        if (mask & (uint32_t(1) << 10)) {
            values.amp_hours_charged = vb.vbPopFrontDouble32(1e4);
        }
        if (mask & (uint32_t(1) << 11)) {
            values.watt_hours = vb.vbPopFrontDouble32(1e4);
        }
        if (mask & (uint32_t(1) << 12)) {
            values.watt_hours_charged = vb.vbPopFrontDouble32(1e4);
        }
        if (mask & (uint32_t(1) << 13)) {
            values.tachometer = vb.vbPopFrontInt32();
        }
        if (mask & (uint32_t(1) << 14)) {
            values.tachometer_abs = vb.vbPopFrontInt32();
        }
        if (mask & (uint32_t(1) << 15)) {
            values.fault_code = mc_fault_code(vb.vbPopFrontInt8());
            values.fault_str = faultToStr(values.fault_code);
        }

        if (vb.size() >= 4) {
            if (mask & (uint32_t(1) << 16)) {
                values.position = vb.vbPopFrontDouble32(1e6);
            }
        } else {
            values.position = -1.0;
        }

        if (vb.size() >= 1) {
            if (mask & (uint32_t(1) << 17)) {
                values.vesc_id = vb.vbPopFrontUint8();
            }
        } else {
            values.vesc_id = 255;
        }

        if (vb.size() >= 6) {
            if (mask & (uint32_t(1) << 18)) {
                values.temp_mos_1 = vb.vbPopFrontDouble16(1e1);
                values.temp_mos_2 = vb.vbPopFrontDouble16(1e1);
                values.temp_mos_3 = vb.vbPopFrontDouble16(1e1);
            }
        }

        if (vb.size() >= 8) {
            if (mask & (uint32_t(1) << 19)) {
                values.vd = vb.vbPopFrontDouble32(1e3);
            }
            if (mask & (uint32_t(1) << 20)) {
                values.vq = vb.vbPopFrontDouble32(1e3);
            }
        }

        if (vb.size() >= 1) {
            if (mask & (uint32_t(1) << 21)) {
                quint8 status = vb.vbPopFrontUint8();
                values.has_timeout = status & 1;
                values.kill_sw_active = (status >> 1) & 1;
            }
        }

        emit valuesReceived(values, mask);
    } break;

    case COMM_PRINT:
        emit printReceived(QString::fromLatin1(vb));
        break;

    case COMM_SAMPLE_PRINT:
        emit samplesReceived(vb);
        break;

    case COMM_ROTOR_POSITION:
        emit rotorPosReceived(vb.vbPopFrontDouble32(1e5));
        break;

    case COMM_EXPERIMENT_SAMPLE: {
        QVector<double> samples;
        while (!vb.isEmpty()) {
            samples.append(vb.vbPopFrontDouble32(1e4));
        }
        emit experimentSamplesReceived(samples);
    } break;

    case COMM_GET_MCCONF:
    case COMM_GET_MCCONF_DEFAULT:
        mTimeoutMcconf = 0;
        if (mMcConfig) {
            if (mMcConfig->deSerialize(vb)) {
                mMcConfig->updateDone();

                if (mCheckNextMcConfig) {
                    mCheckNextMcConfig = false;
                    emit mcConfigCheckResult(mMcConfig->checkDifference(&mMcConfigLast));
                }
            } else {
                emit deserializeConfigFailed(true, false);
            }
        }
        break;

    case COMM_GET_APPCONF:
    case COMM_GET_APPCONF_DEFAULT:
        mTimeoutAppconf = 0;
        if (mAppConfig) {
            if (mAppConfig->deSerialize(vb)) {
                mAppConfig->updateDone();
            } else {
                emit deserializeConfigFailed(false, true);
            }
        }
        break;

    case COMM_DETECT_MOTOR_PARAM: {
        bldc_detect param;
        param.cycle_int_limit = vb.vbPopFrontDouble32(1e3);
        param.bemf_coupling_k = vb.vbPopFrontDouble32(1e3);
        for (int i = 0;i < 8;i++) {
            param.hall_table.append(int(vb.vbPopFrontUint8()));
        }
        param.hall_res = int(vb.vbPopFrontUint8());
        emit bldcDetectReceived(param);
    } break;

    case COMM_DETECT_MOTOR_R_L: {
        double r = vb.vbPopFrontDouble32(1e6);
        double l = vb.vbPopFrontDouble32(1e3);
        double ld_lq_diff = 0.0;
        if (vb.size() >= 4) {
            ld_lq_diff = vb.vbPopFrontDouble32(1e3);
        }
        emit motorRLReceived(r, l, ld_lq_diff);
    } break;

    case COMM_DETECT_MOTOR_FLUX_LINKAGE: {
        emit motorLinkageReceived(vb.vbPopFrontDouble32(1e7));
    } break;

    case COMM_DETECT_ENCODER: {
        ENCODER_DETECT_RES res;
        res.offset = vb.vbPopFrontDouble32(1e6);
        res.ratio = vb.vbPopFrontDouble32(1e6);
        res.inverted = vb.vbPopFrontInt8();
        res.detect_rx = true;
        emit encoderParamReceived(res);
    } break;

    case COMM_DETECT_HALL_FOC: {
        QVector<int> table;
        for (int i = 0;i < 8;i++) {
            table.append(vb.vbPopFrontUint8());
        }
        int res = vb.vbPopFrontUint8();
        emit focHallTableReceived(table, res);
    } break;

    case COMM_GET_DECODED_PPM: {
        mTimeoutDecPpm = 0;
        double dec_ppm = vb.vbPopFrontDouble32(1e6);
        double ppm_last_len = vb.vbPopFrontDouble32(1e6);
        emit decodedPpmReceived(dec_ppm, ppm_last_len);
    } break;

    case COMM_GET_DECODED_ADC: {
        mTimeoutDecAdc = 0;
        double dec_adc = vb.vbPopFrontDouble32(1e6);
        double dec_adc_voltage = vb.vbPopFrontDouble32(1e6);
        double dec_adc2 = vb.vbPopFrontDouble32(1e6);
        double dec_adc_voltage2 = vb.vbPopFrontDouble32(1e6);
        emit decodedAdcReceived(dec_adc, dec_adc_voltage, dec_adc2, dec_adc_voltage2);
    } break;

    case COMM_GET_DECODED_CHUK:
        mTimeoutDecChuk = 0;
        emit decodedChukReceived(vb.vbPopFrontDouble32(1000000.0));
        break;

    case COMM_SET_MCCONF:
        emit ackReceived("MCCONF Write OK");
        break;

    case COMM_SET_APPCONF:
        emit ackReceived("APPCONF Write OK");
        break;

    case COMM_SET_APPCONF_NO_STORE:
        emit ackReceived("APPCONF_NO_STORE Write OK");
        break;

    case COMM_CUSTOM_APP_DATA:
        emit customAppDataReceived(vb);
        break;

    case COMM_CUSTOM_HW_DATA:
        emit customHwDataReceived(vb);
        break;

    case COMM_NRF_START_PAIRING:
        emit nrfPairingRes(NRF_PAIR_RES(vb.vbPopFrontInt8()));
        break;

    case COMM_GPD_BUFFER_NOTIFY:
        emit gpdBufferNotifyReceived();
        break;

    case COMM_GPD_BUFFER_SIZE_LEFT:
        emit gpdBufferSizeLeftReceived(vb.vbPopFrontInt16());
        break;

    case COMM_GET_VALUES_SETUP:
    case COMM_GET_VALUES_SETUP_SELECTIVE: {
        mTimeoutValuesSetup = 0;
        SETUP_VALUES values;

        uint32_t mask = 0xFFFFFFFF;
        if (id == COMM_GET_VALUES_SETUP_SELECTIVE) {
            mask = vb.vbPopFrontUint32();
        }

        if (mask & (uint32_t(1) << 0)) {
            values.temp_mos = vb.vbPopFrontDouble16(1e1);
        }
        if (mask & (uint32_t(1) << 1)) {
            values.temp_motor = vb.vbPopFrontDouble16(1e1);
        }
        if (mask & (uint32_t(1) << 2)) {
            values.current_motor = vb.vbPopFrontDouble32(1e2);
        }
        if (mask & (uint32_t(1) << 3)) {
            values.current_in = vb.vbPopFrontDouble32(1e2);
        }
        if (mask & (uint32_t(1) << 4)) {
            values.duty_now = vb.vbPopFrontDouble16(1e3);
        }
        if (mask & (uint32_t(1) << 5)) {
            values.rpm = vb.vbPopFrontDouble32(1e0);
        }
        if (mask & (uint32_t(1) << 6)) {
            values.speed = vb.vbPopFrontDouble32(1e3);
        }
        if (mask & (uint32_t(1) << 7)) {
            values.v_in = vb.vbPopFrontDouble16(1e1);
        }
        if (mask & (uint32_t(1) << 8)) {
            values.battery_level = vb.vbPopFrontDouble16(1e3);
        }
        if (mask & (uint32_t(1) << 9)) {
            values.amp_hours = vb.vbPopFrontDouble32(1e4);
        }
        if (mask & (uint32_t(1) << 10)) {
            values.amp_hours_charged = vb.vbPopFrontDouble32(1e4);
        }
        if (mask & (uint32_t(1) << 11)) {
            values.watt_hours = vb.vbPopFrontDouble32(1e4);
        }
        if (mask & (uint32_t(1) << 12)) {
            values.watt_hours_charged = vb.vbPopFrontDouble32(1e4);
        }
        if (mask & (uint32_t(1) << 13)) {
            values.tachometer = vb.vbPopFrontDouble32(1e3);
        }
        if (mask & (uint32_t(1) << 14)) {
            values.tachometer_abs = vb.vbPopFrontDouble32(1e3);
        }
        if (mask & (uint32_t(1) << 15)) {
            values.position = vb.vbPopFrontDouble32(1e6);
        }
        if (mask & (uint32_t(1) << 16)) {
            values.fault_code = mc_fault_code(vb.vbPopFrontInt8());
            values.fault_str = faultToStr(values.fault_code);
        }
        if (mask & (uint32_t(1) << 17)) {
            values.vesc_id = vb.vbPopFrontUint8();
        }
        if (mask & (uint32_t(1) << 18)) {
            values.num_vescs = vb.vbPopFrontUint8();
        }
        if (mask & (uint32_t(1) << 19)) {
            values.battery_wh = vb.vbPopFrontDouble32(1e3);
        }
        if (mask & (uint32_t(1) << 20)) {
            values.odometer = vb.vbPopFrontUint32();
        }
        if (mask & (uint32_t(1) << 21)) {
            values.uptime_ms = vb.vbPopFrontUint32();
        }

        emit valuesSetupReceived(values, mask);
    } break;

    case COMM_SET_MCCONF_TEMP:
        emit ackReceived("COMM_SET_MCCONF_TEMP Write OK");
        break;

    case COMM_SET_MCCONF_TEMP_SETUP:
        emit ackReceived("COMM_SET_MCCONF_TEMP_SETUP Write OK");
        break;

    case COMM_DETECT_MOTOR_FLUX_LINKAGE_OPENLOOP:
        emit motorLinkageReceived(vb.vbPopFrontDouble32(1e7));
        break;

    case COMM_DETECT_APPLY_ALL_FOC:
        emit detectAllFocReceived(vb.vbPopFrontInt16());
        break;

    case COMM_PING_CAN: {
        mTimeoutPingCan = 0;
        QVector<int> devs;
        while(vb.size() > 0) {
            devs.append(vb.vbPopFrontUint8());
        }
        emit pingCanRx(devs, false);
    } break;

    case COMM_GET_IMU_DATA: {
        mTimeoutImuData = 0;

        IMU_VALUES values;

        uint32_t mask = vb.vbPopFrontUint16();

        if (mask & (uint32_t(1) << 0)) {
            values.roll = vb.vbPopFrontDouble32Auto();
        }
        if (mask & (uint32_t(1) << 1)) {
            values.pitch = vb.vbPopFrontDouble32Auto();
        }
        if (mask & (uint32_t(1) << 2)) {
            values.yaw = vb.vbPopFrontDouble32Auto();
        }

        if (mask & (uint32_t(1) << 3)) {
            values.accX = vb.vbPopFrontDouble32Auto();
        }
        if (mask & (uint32_t(1) << 4)) {
            values.accY = vb.vbPopFrontDouble32Auto();
        }
        if (mask & (uint32_t(1) << 5)) {
            values.accZ = vb.vbPopFrontDouble32Auto();
        }

        if (mask & (uint32_t(1) << 6)) {
            values.gyroX = vb.vbPopFrontDouble32Auto();
        }
        if (mask & (uint32_t(1) << 7)) {
            values.gyroY = vb.vbPopFrontDouble32Auto();
        }
        if (mask & (uint32_t(1) << 8)) {
            values.gyroZ = vb.vbPopFrontDouble32Auto();
        }

        if (mask & (uint32_t(1) << 9)) {
            values.magX = vb.vbPopFrontDouble32Auto();
        }
        if (mask & (uint32_t(1) << 10)) {
            values.magY = vb.vbPopFrontDouble32Auto();
        }
        if (mask & (uint32_t(1) << 11)) {
            values.magZ = vb.vbPopFrontDouble32Auto();
        }

        if (mask & (uint32_t(1) << 12)) {
            values.q0 = vb.vbPopFrontDouble32Auto();
        }
        if (mask & (uint32_t(1) << 13)) {
            values.q1 = vb.vbPopFrontDouble32Auto();
        }
        if (mask & (uint32_t(1) << 14)) {
            values.q2 = vb.vbPopFrontDouble32Auto();
        }
        if (mask & (uint32_t(1) << 15)) {
            values.q3 = vb.vbPopFrontDouble32Auto();
        }
        if (vb.size() >= 1) {
            values.vesc_id = vb.vbPopFrontUint8();
        }

        emit valuesImuReceived(values, mask);
    } break;

    case COMM_GET_IMU_CALIBRATION: {
        QVector<double> cal;
        for (int i = 0;i < 9;i++) {
            cal.append(vb.vbPopFrontDouble32(1e6));
        }
        emit imuCalibrationReceived(cal);
    } break;

    case COMM_BM_CONNECT:
        emit bmConnRes(vb.vbPopFrontInt16());
        break;

    case COMM_BM_ERASE_FLASH_ALL:
        emit bmEraseFlashAllRes(vb.vbPopFrontInt16());
        break;

    case COMM_BM_WRITE_FLASH:
    case COMM_BM_WRITE_FLASH_LZO:
        emit bmWriteFlashRes(vb.vbPopFrontInt16());
        break;

    case COMM_BM_REBOOT:
        emit bmRebootRes(vb.vbPopFrontInt16());
        break;

    case COMM_BM_DISCONNECT:
        emit ackReceived("COMM_BM_DISCONNECT OK");
        break;

    case COMM_BM_MAP_PINS_DEFAULT:
        emit bmMapPinsDefaultRes(vb.vbPopFrontInt16());
        break;

    case COMM_BM_MAP_PINS_NRF5X:
        emit bmMapPinsNrf5xRes(vb.vbPopFrontInt16());
        break;

    case COMM_PLOT_INIT: {
        QString xL = vb.vbPopFrontString();
        QString yL = vb.vbPopFrontString();
        emit plotInitReceived(xL, yL);
    } break;

    case COMM_PLOT_DATA: {
        double x = vb.vbPopFrontDouble32Auto();
        double y = vb.vbPopFrontDouble32Auto();
        emit plotDataReceived(x, y);
    } break;

    case COMM_PLOT_ADD_GRAPH: {
        emit plotAddGraphReceived(vb.vbPopFrontString());
    } break;

    case COMM_PLOT_SET_GRAPH: {
        emit plotSetGraphReceived(vb.vbPopFrontInt8());
    } break;

    case COMM_BM_MEM_READ: {
        int res = vb.vbPopFrontInt16();
        emit bmReadMemRes(res, vb);
    } break;

    case COMM_CAN_FWD_FRAME: {
        quint32 id = vb.vbPopFrontUint32();
        bool isExtended = vb.vbPopFrontInt8();
        emit canFrameRx(vb, id, isExtended);
    } break;

    case COMM_SET_BATTERY_CUT:
        emit ackReceived("COMM_SET_BATTERY_CUT Write OK");
        break;

    case COMM_BMS_GET_VALUES: {
        mTimeoutBmsVal = 0;
        BMS_VALUES val;
        val.v_tot = vb.vbPopFrontDouble32(1e6);
        val.v_charge = vb.vbPopFrontDouble32(1e6);
        val.i_in = vb.vbPopFrontDouble32(1e6);
        val.i_in_ic = vb.vbPopFrontDouble32(1e6);
        val.ah_cnt = vb.vbPopFrontDouble32(1e3);
        val.wh_cnt = vb.vbPopFrontDouble32(1e3);

        int cells = vb.vbPopFrontUint8();
        for (int i = 0;i < cells;i++) {
            val.v_cells.append(vb.vbPopFrontDouble16(1e3));
        }
        for (int i = 0;i < cells;i++) {
            val.is_balancing.append(vb.vbPopFrontUint8());
        }

        int sensors = vb.vbPopFrontUint8();
        for (int i = 0;i < sensors;i++) {
            val.temps.append(vb.vbPopFrontDouble16(1e2));
        }

        val.temp_ic = vb.vbPopFrontDouble16(1e2);
        val.temp_hum_sensor = vb.vbPopFrontDouble16(1e2);
        val.humidity = vb.vbPopFrontDouble16(1e2);

        val.temp_cells_highest = vb.vbPopFrontDouble16(1e2);

        if (vb.size() >= 2) {
            val.soc = vb.vbPopFrontDouble16(1e3);
        }

        if (vb.size() >= 2) {
            val.soh = vb.vbPopFrontDouble16(1e3);
        }

        if (vb.size() >= 1) {
            val.can_id = vb.vbPopFrontUint8();
        }

        if (vb.size() >= 16) {
            val.ah_cnt_chg_total = vb.vbPopFrontDouble32Auto();
            val.wh_cnt_chg_total = vb.vbPopFrontDouble32Auto();
            val.ah_cnt_dis_total = vb.vbPopFrontDouble32Auto();
            val.wh_cnt_dis_total = vb.vbPopFrontDouble32Auto();
        }

        val.updateTimeStamp();

        emit bmsValuesRx(val);
    } break;

    case COMM_SET_CUSTOM_CONFIG:
        emit ackReceived("COMM_SET_CUSTOM_CONFIG Write OK");
        break;

    case COMM_GET_CUSTOM_CONFIG:
    case COMM_GET_CUSTOM_CONFIG_DEFAULT: {
        mTimeoutCustomConf = 0;
        int confInd = vb.vbPopFrontInt8();
        emit customConfigRx(confInd, vb);
    } break;

    case COMM_GET_CUSTOM_CONFIG_XML: {
        int confInd = vb.vbPopFrontInt8();
        int confSize = vb.vbPopFrontInt32();
        int offset = vb.vbPopFrontInt32();
        emit customConfigChunkRx(confInd, confSize, offset, vb);
    } break;

    case COMM_PSW_GET_STATUS: {
        PSW_STATUS stat;
        stat.id = vb.vbPopFrontInt16();
        stat.psws_num = vb.vbPopFrontInt16();
        stat.age_seconds = vb.vbPopFrontDouble32Auto();
        stat.v_in = vb.vbPopFrontDouble32Auto();
        stat.v_out = vb.vbPopFrontDouble32Auto();
        stat.temp = vb.vbPopFrontDouble32Auto();
        stat.is_out_on = vb.vbPopFrontInt8();
        stat.is_pch_on = vb.vbPopFrontInt8();
        stat.is_dsc_on = vb.vbPopFrontInt8();
        emit pswStatusRx(stat);
    } break;

    case COMM_BMS_FWD_CAN_RX: {
        int id = vb.vbPopFrontUint8();
        CAN_PACKET_ID cmd = CAN_PACKET_ID(vb.vbPopFrontUint8());
        BMS_VALUES &val = mBmsValues[id];

        switch (cmd) {
        case CAN_PACKET_BMS_SOC_SOH_TEMP_STAT: {
            vb.vbPopFrontDouble16(1e3); // V_CELL_MIN
            vb.vbPopFrontDouble16(1e3); // V_CELL_MAX
            val.can_id = id;
            val.soc = double(vb.vbPopFrontUint8()) / 255.0;
            val.soh = double(vb.vbPopFrontUint8()) / 255.0;
            val.temp_cells_highest = double(vb.vbPopFrontUint8());
            val.updateTimeStamp();
        } break;

        case CAN_PACKET_BMS_V_TOT: {
            val.can_id = id;
            val.v_tot = vb.vbPopFrontDouble32Auto();
            val.v_charge = vb.vbPopFrontDouble32Auto();
            val.updateTimeStamp();
        } break;

        case CAN_PACKET_BMS_I: {
            val.can_id = id;
            val.i_in = vb.vbPopFrontDouble32Auto();
            val.i_in_ic = vb.vbPopFrontDouble32Auto();
            val.updateTimeStamp();
        } break;

        case CAN_PACKET_BMS_AH_WH: {
            val.can_id = id;
            val.ah_cnt = vb.vbPopFrontDouble32Auto();
            val.wh_cnt = vb.vbPopFrontDouble32Auto();
            val.updateTimeStamp();
        } break;

        case CAN_PACKET_BMS_V_CELL: {
            val.can_id = id;

            int ofs = vb.vbPopFrontUint8();
            val.v_cells.resize(vb.vbPopFrontUint8());

            while (vb.size() > 1) {
                val.v_cells[ofs++] = vb.vbPopFrontDouble16(1e3);
            }

            val.updateTimeStamp();
        } break;

        case CAN_PACKET_BMS_BAL: {
            val.can_id = id;

            uint64_t bal_state_0 = vb.vbPopFrontUint32();
            int cell_num = (bal_state_0 >> 24) & 0xFF;
            bal_state_0 &= 0x00FFFFFF;
            uint64_t bal_state_1 = vb.vbPopFrontUint32();
            uint64_t bal_state = bal_state_0 << 32 | bal_state_1;
            int32_t ind = 0;

            val.is_balancing.resize(cell_num);
            while (ind < cell_num) {
                val.is_balancing[ind] = (bal_state >> ind) & 1;
                ind++;
            }

            val.updateTimeStamp();
        } break;

        case CAN_PACKET_BMS_TEMPS: {
            val.can_id = id;

            unsigned int ofs = vb.vbPopFrontUint8();
            val.temps.resize(vb.vbPopFrontUint8());

            while (vb.size() > 1) {
                val.temps[ofs++] = vb.vbPopFrontDouble16(1e2);
            }

            val.updateTimeStamp();
        } break;

        case CAN_PACKET_BMS_HUM: {
            val.can_id = id;
            val.temp_hum_sensor = vb.vbPopFrontDouble16(1e2);
            val.humidity = vb.vbPopFrontDouble16(1e2);
            val.temp_ic = vb.vbPopFrontDouble16(1e2);
            val.updateTimeStamp();
        } break;

        case CAN_PACKET_BMS_AH_WH_CHG_TOTAL: {
            val.can_id = id;
            val.ah_cnt_chg_total = vb.vbPopFrontDouble32Auto();
            val.wh_cnt_chg_total = vb.vbPopFrontDouble32Auto();
            val.updateTimeStamp();
        } break;

        case CAN_PACKET_BMS_AH_WH_DIS_TOTAL: {
            val.can_id = id;
            val.ah_cnt_dis_total = vb.vbPopFrontDouble32Auto();
            val.wh_cnt_dis_total = vb.vbPopFrontDouble32Auto();
            val.updateTimeStamp();
        } break;

        default:
            break;
        }
    } break;

    case COMM_GET_QML_UI_HW: {
        int qmlSize = vb.vbPopFrontInt32();
        int offset = vb.vbPopFrontInt32();
        emit qmluiHwRx(qmlSize, offset, vb);
    } break;

    case COMM_GET_QML_UI_APP: {
        int qmlSize = vb.vbPopFrontInt32();
        int offset = vb.vbPopFrontInt32();
        emit qmluiAppRx(qmlSize, offset, vb);
    } break;

    case COMM_QMLUI_ERASE:
        emit eraseQmluiResReceived(vb.at(0));
        break;

    case COMM_QMLUI_WRITE: {
        bool ok = vb.vbPopFrontInt8();
        quint32 offset = vb.vbPopFrontUint32();
        emit writeQmluiResReceived(ok, offset);
    } break;

    case COMM_IO_BOARD_GET_ALL: {
        IO_BOARD_VALUES val;
        val.id = vb.vbPopFrontInt16();

        while (vb.size() > 0) {
            int type = vb.vbPopFrontInt8();
            switch (type) {
            case 1: {
                val.adc_1_4_age = vb.vbPopFrontDouble32Auto();
                val.adc_1_4.append(vb.vbPopFrontDouble16(1e2));
                val.adc_1_4.append(vb.vbPopFrontDouble16(1e2));
                val.adc_1_4.append(vb.vbPopFrontDouble16(1e2));
                val.adc_1_4.append(vb.vbPopFrontDouble16(1e2));
            } break;

            case 2: {
                val.adc_5_8_age = vb.vbPopFrontDouble32Auto();
                val.adc_5_8.append(vb.vbPopFrontDouble16(1e2));
                val.adc_5_8.append(vb.vbPopFrontDouble16(1e2));
                val.adc_5_8.append(vb.vbPopFrontDouble16(1e2));
                val.adc_5_8.append(vb.vbPopFrontDouble16(1e2));
            } break;

            case 3: {
                val.digital_age = vb.vbPopFrontDouble32Auto();
                uint64_t hi = vb.vbPopFrontUint32();
                uint64_t lo = vb.vbPopFrontUint32();
                uint64_t combined = (hi << 32) | lo;

                for (int i = 0;i < 64;i++) {
                    val.digital.append((combined >> i) & 1);
                }
            } break;

            default:
                break;
            }
        }

        emit ioBoardValRx(val);
    } break;

    case COMM_GET_STATS: {
        mTimeoutStats = 0;
        STAT_VALUES values;
        uint32_t mask = vb.vbPopFrontUint32();
        if (mask & ((uint32_t)1 << 0)) { values.speed_avg = vb.vbPopFrontDouble32Auto(); }
        if (mask & ((uint32_t)1 << 1)) { values.speed_max = vb.vbPopFrontDouble32Auto(); }
        if (mask & ((uint32_t)1 << 2)) { values.power_avg = vb.vbPopFrontDouble32Auto(); }
        if (mask & ((uint32_t)1 << 3)) { values.power_max = vb.vbPopFrontDouble32Auto(); }
        if (mask & ((uint32_t)1 << 4)) { values.current_avg = vb.vbPopFrontDouble32Auto(); }
        if (mask & ((uint32_t)1 << 5)) { values.current_max = vb.vbPopFrontDouble32Auto(); }
        if (mask & ((uint32_t)1 << 6)) { values.temp_mos_avg = vb.vbPopFrontDouble32Auto(); }
        if (mask & ((uint32_t)1 << 7)) { values.temp_mos_max = vb.vbPopFrontDouble32Auto(); }
        if (mask & ((uint32_t)1 << 8)) { values.temp_motor_avg = vb.vbPopFrontDouble32Auto(); }
        if (mask & ((uint32_t)1 << 9)) { values.temp_motor_max = vb.vbPopFrontDouble32Auto(); }
        if (mask & ((uint32_t)1 << 10)) { values.count_time = vb.vbPopFrontDouble32Auto(); }
        emit statsRx(values, mask);
    } break;

    case COMM_LISP_READ_CODE: {
        int qmlSize = vb.vbPopFrontInt32();
        int offset = vb.vbPopFrontInt32();
        emit lispReadCodeRx(qmlSize, offset, vb);
    } break;

    case COMM_LISP_ERASE_CODE:
        emit lispEraseCodeRx(vb.at(0));
        break;

    case COMM_LISP_WRITE_CODE: {
        bool ok = vb.vbPopFrontInt8();
        quint32 offset = vb.vbPopFrontUint32();
        emit lispWriteCodeRx(ok, offset);
    } break;

    case COMM_LISP_PRINT:
        emit lispPrintReceived(QString::fromLatin1(vb));
        break;

    case COMM_LISP_GET_STATS: {
        LISP_STATS stats;
        stats.cpu_use = vb.vbPopFrontDouble16(1e2);
        stats.heap_use = vb.vbPopFrontDouble16(1e2);
        stats.mem_use = vb.vbPopFrontDouble16(1e2);
        stats.stack_use = vb.vbPopFrontDouble16(1e2);
        stats.done_ctx_r = vb.vbPopFrontString();
        while (vb.size() > 0) {
            auto name = vb.vbPopFrontString();
            auto num = vb.vbPopFrontDouble32Auto();
            stats.number_bindings.append(qMakePair(name, num));
        }
        emit lispStatsRx(stats);
    } break;

    case COMM_LISP_SET_RUNNING:
        emit lispRunningResRx(vb.at(0));
        break;

    case COMM_LISP_STREAM_CODE: {
        quint32 offset = vb.vbPopFrontInt32();
        quint32 res = vb.vbPopFrontInt16();
        emit lispStreamCodeRx(offset, res);
    } break;

    case COMM_FILE_LIST: {
        auto hasMore = vb.vbPopFrontInt8();
        QList<FILE_LIST_ENTRY> files;
        while (vb.size() > 0) {
            FILE_LIST_ENTRY f;
            f.isDir = vb.vbPopFrontInt8();
            f.size = vb.vbPopFrontInt32();
            f.name = vb.vbPopFrontString();
            files.append(f);
        }
        emit fileListRx(hasMore, files);
    } break;

    case COMM_FILE_READ: {
        auto offset = vb.vbPopFrontInt32();
        auto size = vb.vbPopFrontInt32();
        emit fileReadRx(offset, size, vb);
    } break;

    case COMM_FILE_WRITE: {
        auto offset = vb.vbPopFrontInt32();
        auto ok = vb.vbPopFrontInt8();
        emit fileWriteRx(offset, ok);
    } break;

    case COMM_FILE_MKDIR: {
        auto ok = vb.vbPopFrontInt8();
        emit fileMkdirRx(ok);
    } break;

    case COMM_FILE_REMOVE: {
        auto ok = vb.vbPopFrontInt8();
        emit fileRemoveRx(ok);
    } break;

    case COMM_GET_GNSS: {
        mTimeoutStats = 0;
        GNSS_DATA values;
        uint32_t mask = vb.vbPopFrontUint32();
        if (mask & ((uint32_t)1 << 0)) { values.lat = vb.vbPopFrontDouble64(1e16); }
        if (mask & ((uint32_t)1 << 1)) { values.lon = vb.vbPopFrontDouble64(1e16); }
        if (mask & ((uint32_t)1 << 2)) { values.height = vb.vbPopFrontDouble32Auto(); }
        if (mask & ((uint32_t)1 << 3)) { values.speed = vb.vbPopFrontDouble32Auto(); }
        if (mask & ((uint32_t)1 << 4)) { values.hdop = vb.vbPopFrontDouble32Auto(); }
        if (mask & ((uint32_t)1 << 5)) { values.ms_today = vb.vbPopFrontInt32(); }
        if (mask & ((uint32_t)1 << 6)) { values.yy = vb.vbPopFrontInt16(); }
        if (mask & ((uint32_t)1 << 7)) { values.mo = vb.vbPopFrontInt8(); }
        if (mask & ((uint32_t)1 << 8)) { values.dd = vb.vbPopFrontInt8(); }
        if (mask & ((uint32_t)1 << 9)) { values.age_s = vb.vbPopFrontDouble32Auto(); }
        emit gnssRx(values, mask);
    } break;

    case COMM_LOG_START: {
        int fieldNum = vb.vbPopFrontInt16();
        double rateHz = vb.vbPopFrontDouble32Auto();
        bool appendTime = vb.vbPopFrontInt8();
        bool appendGnss = vb.vbPopFrontInt8();
        bool appendGnssTime = vb.vbPopFrontInt8();
        emit logStart(fieldNum, rateHz, appendTime, appendGnss, appendGnssTime);
    } break;

    case COMM_LOG_STOP: {
        emit logStop();
    } break;

    case COMM_LOG_CONFIG_FIELD: {
        LOG_HEADER h;
        int fieldInd = vb.vbPopFrontInt16();
        h.key = vb.vbPopFrontString();
        h.name = vb.vbPopFrontString();
        h.unit = vb.vbPopFrontString();
        h.precision = vb.vbPopFrontInt8();
        h.isRelativeToFirst = vb.vbPopFrontInt8();
        h.isTimeStamp = vb.vbPopFrontInt8();
        emit logConfigField(fieldInd, h);
    } break;

    case COMM_LOG_DATA_F32: {
        int fieldStart = vb.vbPopFrontInt16();
        QVector<double> samples;
        while (vb.size() >= 4) {
            samples.append(vb.vbPopFrontDouble32Auto());
        }
        emit logSamples(fieldStart, samples);
    } break;

    case COMM_LOG_DATA_F64: {
        int fieldStart = vb.vbPopFrontInt16();
        QVector<double> samples;
        while (vb.size() >= 8) {
            samples.append(vb.vbPopFrontDouble64Auto());
        }
        emit logSamples(fieldStart, samples);
    } break;

    default:
        break;
    }
}

void Commands::getFwVersion()
{
    if (mTimeoutFwVer > 0) {
        return;
    }

    mTimeoutFwVer = mTimeoutCount;

    VByteArray vb;
    vb.vbAppendInt8(COMM_FW_VERSION);
    emitData(vb);
}

void Commands::eraseNewApp(bool fwdCan, quint32 fwSize, HW_TYPE hwType, QString hwName)
{
    VByteArray vb;
    if (!hwName.isEmpty() && hwType != HW_TYPE_VESC && fwdCan) {
        vb.vbAppendInt8(COMM_ERASE_NEW_APP_ALL_CAN_HW);
        vb.vbAppendUint8(hwType);
        vb.vbAppendString(hwName);
    } else {
        vb.vbAppendInt8(fwdCan ? COMM_ERASE_NEW_APP_ALL_CAN :
                                 COMM_ERASE_NEW_APP);
    }

    vb.vbAppendUint32(fwSize);
    emitData(vb);
}

void Commands::eraseBootloader(bool fwdCan, HW_TYPE hwType, QString hwName)
{
    VByteArray vb;
    if (!hwName.isEmpty() && hwType != HW_TYPE_VESC && fwdCan) {
        vb.vbAppendInt8(COMM_ERASE_BOOTLOADER_ALL_CAN_HW);
        vb.vbAppendUint8(hwType);
        vb.vbAppendString(hwName);
    } else {
        vb.vbAppendInt8(fwdCan ? COMM_ERASE_BOOTLOADER_ALL_CAN :
                                 COMM_ERASE_BOOTLOADER);
    }

    emitData(vb);
}

void Commands::writeNewAppData(QByteArray data, quint32 offset, bool fwdCan, HW_TYPE hwType, QString hwName)
{
    VByteArray vb;
    if (!hwName.isEmpty() && hwType != HW_TYPE_VESC && fwdCan) {
        vb.vbAppendInt8(COMM_WRITE_NEW_APP_DATA_ALL_CAN_HW);
        vb.vbAppendUint8(hwType);
        vb.vbAppendString(hwName);
    } else {
        vb.vbAppendInt8(fwdCan ? COMM_WRITE_NEW_APP_DATA_ALL_CAN :
                                 COMM_WRITE_NEW_APP_DATA);
    }

    vb.vbAppendUint32(offset);
    vb.append(data);
    emitData(vb);
}

void Commands::writeNewAppDataLzo(QByteArray data, quint32 offset, quint16 decompressedLen, bool fwdCan)
{
    VByteArray vb;
    vb.vbAppendInt8(fwdCan ? COMM_WRITE_NEW_APP_DATA_ALL_CAN_LZO :
                             COMM_WRITE_NEW_APP_DATA_LZO);
    vb.vbAppendUint32(offset);
    vb.vbAppendUint16(decompressedLen);
    vb.append(data);
    emitData(vb);
}

void Commands::jumpToBootloader(bool fwdCan, HW_TYPE hwType, QString hwName)
{
    VByteArray vb;
    if (!hwName.isEmpty() && hwType != HW_TYPE_VESC && fwdCan) {
        vb.vbAppendInt8(COMM_JUMP_TO_BOOTLOADER_ALL_CAN_HW);
        vb.vbAppendUint8(hwType);
        vb.vbAppendString(hwName);
    } else {
        vb.vbAppendInt8(fwdCan ? COMM_JUMP_TO_BOOTLOADER_ALL_CAN :
                                 COMM_JUMP_TO_BOOTLOADER);
    }
    emitData(vb);
}

void Commands::getValues()
{
    if (mTimeoutValues > 0) {
        return;
    }

    mTimeoutValues = mTimeoutCount;

    VByteArray vb;
    vb.vbAppendInt8(COMM_GET_VALUES);
    emitData(vb);
}

void Commands::setOdometer(unsigned odometer_meters)
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_SET_ODOMETER);
    vb.vbAppendUint32(odometer_meters);
    emitData(vb);
}

int Commands::bmsGetCanDevNum()
{
    return mBmsValues.size();
}

BMS_VALUES Commands::bmsGetCanValues(int can_id)
{
    return mBmsValues.value(can_id);
}

bool Commands::bmsHasCanValues(int can_id)
{
    return mBmsValues.contains(can_id);
}

void Commands::emitPlotInit(QString xLabel, QString yLabel)
{
    emit plotInitReceived(xLabel, yLabel);
}

void Commands::emitPlotData(double x, double y)
{
    emit plotDataReceived(x, y);
}

void Commands::emitPlotAddGraph(QString name)
{
    emit plotAddGraphReceived(name);
}

void Commands::emitPlotSetGraph(int graph)
{
    emit plotSetGraphReceived(graph);
}

void Commands::sendTerminalCmd(QString cmd)
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_TERMINAL_CMD);
    vb.append(cmd.toLatin1());
    emitData(vb);
}

void Commands::sendTerminalCmdSync(QString cmd)
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_TERMINAL_CMD_SYNC);
    vb.append(cmd.toLatin1());
    emitData(vb);
}

void Commands::setDutyCycle(double dutyCycle)
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_SET_DUTY);
    vb.vbAppendDouble32(dutyCycle, 1e5);
    emitData(vb);
}

void Commands::setCurrent(double current)
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_SET_CURRENT);
    vb.vbAppendDouble32(current, 1e3);
    emitData(vb);
}

void Commands::setCurrentBrake(double current)
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_SET_CURRENT_BRAKE);
    vb.vbAppendDouble32(current, 1e3);
    emitData(vb);
}

void Commands::setRpm(int rpm)
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_SET_RPM);
    vb.vbAppendInt32(rpm);
    emitData(vb);
}

void Commands::setPos(double pos)
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_SET_POS);
    vb.vbAppendDouble32(pos, 1e6);
    emitData(vb);
}

void Commands::setHandbrake(double current)
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_SET_HANDBRAKE);
    vb.vbAppendDouble32(current, 1e3);
    emitData(vb);
}

void Commands::setDetect(disp_pos_mode mode)
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_SET_DETECT);
    vb.vbAppendInt8(mode);
    emitData(vb);
}

void Commands::samplePrint(debug_sampling_mode mode, int sample_len, int dec, bool raw)
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_SAMPLE_PRINT);
    vb.vbAppendInt8(mode);
    vb.vbAppendUint16(sample_len);
    vb.vbAppendUint8(dec);
    vb.vbAppendInt8(raw);
    emitData(vb);
}

void Commands::getMcconf()
{
    if (mTimeoutMcconf > 0) {
        return;
    }

    mTimeoutMcconf = mTimeoutCount;

    mCheckNextMcConfig = false;
    VByteArray vb;
    vb.vbAppendInt8(COMM_GET_MCCONF);
    emitData(vb);
}

void Commands::getMcconfDefault()
{
    if (mTimeoutMcconf > 0) {
        return;
    }

    mTimeoutMcconf = mTimeoutCount;

    mCheckNextMcConfig = false;
    VByteArray vb;
    vb.vbAppendInt8(COMM_GET_MCCONF_DEFAULT);
    emitData(vb);
}

void Commands::setMcconf(bool check)
{
    if (mMcConfig) {
        mMcConfigLast = *mMcConfig;
        VByteArray vb;
        vb.vbAppendInt8(COMM_SET_MCCONF);
        mMcConfig->serialize(vb);
        emitData(vb);

        if (check) {
            checkMcConfig();
        }

        emit mcConfigWriteSent(check);
    }
}

void Commands::getAppConf()
{
    if (mTimeoutAppconf > 0) {
        return;
    }

    mTimeoutAppconf = mTimeoutCount;

    VByteArray vb;
    vb.vbAppendInt8(COMM_GET_APPCONF);
    emitData(vb);
}

void Commands::getAppConfDefault()
{
    if (mTimeoutAppconf > 0) {
        return;
    }

    mTimeoutAppconf = mTimeoutCount;

    VByteArray vb;
    vb.vbAppendInt8(COMM_GET_APPCONF_DEFAULT);
    emitData(vb);
}

void Commands::setAppConf()
{
    if (mAppConfig) {
        VByteArray vb;
        vb.vbAppendInt8(COMM_SET_APPCONF);
        mAppConfig->serialize(vb);
        emitData(vb);
    }
}

void Commands::setAppConfNoStore()
{
    if (mAppConfig) {
        VByteArray vb;
        vb.vbAppendInt8(COMM_SET_APPCONF_NO_STORE);
        mAppConfig->serialize(vb);
        emitData(vb);
    }
}

void Commands::detectMotorParam(double current, double min_rpm, double low_duty)
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_DETECT_MOTOR_PARAM);
    vb.vbAppendDouble32(current, 1e3);
    vb.vbAppendDouble32(min_rpm, 1e3);
    vb.vbAppendDouble32(low_duty, 1e3);
    emitData(vb);
}

void Commands::reboot()
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_REBOOT);
    emitData(vb);
}

void Commands::sendAlive()
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_ALIVE);
    emitData(vb);
}

void Commands::getDecodedPpm()
{
    if (mTimeoutDecPpm > 0) {
        return;
    }

    mTimeoutDecPpm = mTimeoutCount;

    VByteArray vb;
    vb.vbAppendInt8(COMM_GET_DECODED_PPM);
    emitData(vb);
}

void Commands::getDecodedAdc()
{
    if (mTimeoutDecAdc > 0) {
        return;
    }

    mTimeoutDecAdc = mTimeoutCount;

    VByteArray vb;
    vb.vbAppendInt8(COMM_GET_DECODED_ADC);
    emitData(vb);
}

void Commands::getDecodedChuk()
{
    if (mTimeoutDecChuk > 0) {
        return;
    }

    mTimeoutDecChuk = mTimeoutCount;

    VByteArray vb;
    vb.vbAppendInt8(COMM_GET_DECODED_CHUK);
    emitData(vb);
}

void Commands::setServoPos(double pos)
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_SET_SERVO_POS);
    vb.vbAppendDouble16(pos, 1e3);
    emitData(vb);
}

void Commands::measureRL()
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_DETECT_MOTOR_R_L);
    emitData(vb);
}

void Commands::measureLinkage(double current, double min_rpm, double low_duty, double resistance)
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_DETECT_MOTOR_FLUX_LINKAGE);
    vb.vbAppendDouble32(current, 1e3);
    vb.vbAppendDouble32(min_rpm, 1e3);
    vb.vbAppendDouble32(low_duty, 1e3);
    vb.vbAppendDouble32(resistance, 1e6);
    emitData(vb);
}

void Commands::measureEncoder(double current)
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_DETECT_ENCODER);
    vb.vbAppendDouble32(current, 1e3);
    emitData(vb);
}

void Commands::measureHallFoc(double current)
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_DETECT_HALL_FOC);
    vb.vbAppendDouble32(current, 1e3);
    emitData(vb);
}

void Commands::sendCustomAppData(QByteArray data)
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_CUSTOM_APP_DATA);
    vb.append(data);
    emitData(vb);
}

void Commands::sendCustomAppData(unsigned char *data, unsigned int len)
{
    QByteArray ba((char*)data, len);
    sendCustomAppData(ba);
}

void Commands::sendCustomHwData(QByteArray data)
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_CUSTOM_HW_DATA);
    vb.append(data);
    emitData(vb);
}

void Commands::setChukData(chuck_data &data)
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_SET_CHUCK_DATA);
    vb.vbAppendUint8(data.js_x);
    vb.vbAppendUint8(data.js_y);
    vb.vbAppendUint8(data.bt_c);
    vb.vbAppendUint8(data.bt_z);
    vb.vbAppendInt16(data.acc_x);
    vb.vbAppendInt16(data.acc_y);
    vb.vbAppendInt16(data.acc_z);
    emitData(vb);
}

void Commands::pairNrf(int ms)
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_NRF_START_PAIRING);
    vb.vbAppendInt32(ms);
    emitData(vb);
}

void Commands::gpdSetFsw(float fsw)
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_GPD_SET_FSW);
    vb.vbAppendInt32((quint32)fsw);
    emitData(vb);
}

void Commands::getGpdBufferSizeLeft()
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_GPD_BUFFER_SIZE_LEFT);
    emitData(vb);
}

void Commands::gpdFillBuffer(QVector<float> samples)
{
    VByteArray vb;

    while (!samples.isEmpty()) {
        vb.vbAppendDouble32Auto(samples.at(0));
        samples.removeFirst();

        if (vb.size() > 400) {
            vb.prepend(COMM_GPD_FILL_BUFFER);
            emitData(vb);
            vb.clear();
        }
    }

    if (vb.size() > 0) {
        vb.prepend(COMM_GPD_FILL_BUFFER);
        emitData(vb);
    }
}

void Commands::gpdOutputSample(float sample)
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_GPD_OUTPUT_SAMPLE);
    vb.vbAppendDouble32Auto(sample);
    emitData(vb);
}

void Commands::gpdSetMode(gpd_output_mode mode)
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_GPD_SET_MODE);
    vb.vbAppendInt8(mode);
    emitData(vb);
}

void Commands::gpdFillBufferInt8(QVector<qint8> samples)
{
    VByteArray vb;

    while (!samples.isEmpty()) {
        vb.vbAppendInt8(samples.at(0));
        samples.removeFirst();

        if (vb.size() > 400) {
            vb.prepend(COMM_GPD_FILL_BUFFER_INT8);
            emitData(vb);
            vb.clear();
        }
    }

    if (vb.size() > 0) {
        vb.prepend(COMM_GPD_FILL_BUFFER_INT8);
        emitData(vb);
    }
}

void Commands::gpdFillBufferInt16(QVector<qint16> samples)
{
    VByteArray vb;

    while (!samples.isEmpty()) {
        vb.vbAppendInt16(samples.at(0));
        samples.removeFirst();

        if (vb.size() > 400) {
            vb.prepend(COMM_GPD_FILL_BUFFER_INT16);
            emitData(vb);
            vb.clear();
        }
    }

    if (vb.size() > 0) {
        vb.prepend(COMM_GPD_FILL_BUFFER_INT16);
        emitData(vb);
    }
}

void Commands::gpdSetBufferIntScale(float scale)
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_GPD_SET_BUFFER_INT_SCALE);
    vb.vbAppendDouble32Auto(scale);
    emitData(vb);
}

void Commands::getValuesSetup()
{
    if (mTimeoutValuesSetup > 0) {
        return;
    }

    mTimeoutValuesSetup = mTimeoutCount;

    VByteArray vb;
    vb.vbAppendInt8(COMM_GET_VALUES_SETUP);
    emitData(vb);
}

void Commands::setMcconfTemp(const MCCONF_TEMP &conf, bool is_setup, bool store,
                             bool forward_can, bool divide_by_controllers, bool ack)
{
    VByteArray vb;
    vb.vbAppendInt8(is_setup ? COMM_SET_MCCONF_TEMP_SETUP : COMM_SET_MCCONF_TEMP);
    vb.vbAppendInt8(store);
    vb.vbAppendInt8(forward_can);
    vb.vbAppendInt8(ack);
    vb.vbAppendInt8(divide_by_controllers);
    vb.vbAppendDouble32Auto(conf.current_min_scale);
    vb.vbAppendDouble32Auto(conf.current_max_scale);
    vb.vbAppendDouble32Auto(conf.erpm_or_speed_min);
    vb.vbAppendDouble32Auto(conf.erpm_or_speed_max);
    vb.vbAppendDouble32Auto(conf.duty_min);
    vb.vbAppendDouble32Auto(conf.duty_max);
    vb.vbAppendDouble32Auto(conf.watt_min);
    vb.vbAppendDouble32Auto(conf.watt_max);
    emitData(vb);
}

void Commands::getValuesSelective(unsigned int mask)
{
    if (mTimeoutValues > 0) {
        return;
    }

    mTimeoutValues = mTimeoutCount;

    VByteArray vb;
    vb.vbAppendInt8(COMM_GET_VALUES_SELECTIVE);
    vb.vbAppendUint32(mask);
    emitData(vb);
}

void Commands::getValuesSetupSelective(unsigned int mask)
{
    if (mTimeoutValuesSetup > 0) {
        return;
    }

    mTimeoutValuesSetup = mTimeoutCount;

    VByteArray vb;
    vb.vbAppendInt8(COMM_GET_VALUES_SETUP_SELECTIVE);
    vb.vbAppendUint32(mask);
    emitData(vb);
}

void Commands::measureLinkageOpenloop(double current, double erpm_per_sec, double low_duty,
                                      double resistance, double inductance)
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_DETECT_MOTOR_FLUX_LINKAGE_OPENLOOP);
    vb.vbAppendDouble32(current, 1e3);
    vb.vbAppendDouble32(erpm_per_sec, 1e3);
    vb.vbAppendDouble32(low_duty, 1e3);
    vb.vbAppendDouble32(resistance, 1e6);
    vb.vbAppendDouble32(inductance, 1e8);
    emitData(vb);
}

void Commands::detectAllFoc(bool detect_can, double max_power_loss, double min_current_in,
                            double max_current_in, double openloop_rpm, double sl_erpm)
{
    if (mMaxPowerLossBug) {
        max_power_loss /= 2.0;
    }

    VByteArray vb;
    vb.vbAppendInt8(COMM_DETECT_APPLY_ALL_FOC);
    vb.vbAppendInt8(detect_can);
    vb.vbAppendDouble32(max_power_loss, 1e3);
    vb.vbAppendDouble32(min_current_in, 1e3);
    vb.vbAppendDouble32(max_current_in, 1e3);
    vb.vbAppendDouble32(openloop_rpm, 1e3);
    vb.vbAppendDouble32(sl_erpm, 1e3);
    emitData(vb);
}

void Commands::pingCan()
{
    if (mTimeoutPingCan > 0) {
        return;
    }

    mTimeoutPingCan = 500;

    VByteArray vb;
    vb.vbAppendInt8(COMM_PING_CAN);
    emitData(vb);
}

/**
 * @brief Commands::disableAppOutput
 * Disable output from apps for a specified amount of time. No ack
 * is sent back.
 *
 * @param time_ms
 * 0: Enable output now
 * -1: Disable forever
 * >0: Amount of milliseconds to disable output
 *
 * @param fwdCan
 * Broadcast the command on the CAN-bus, to affect all VESCs.
 */
void Commands::disableAppOutput(int time_ms, bool fwdCan)
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_APP_DISABLE_OUTPUT);
    vb.vbAppendInt8(fwdCan);
    vb.vbAppendInt32(time_ms);
    emitData(vb);
}

void Commands::getImuData(unsigned int mask)
{
    if (mTimeoutImuData > 0) {
        return;
    }

    mTimeoutImuData = mTimeoutCount;

    VByteArray vb;
    vb.vbAppendInt8(COMM_GET_IMU_DATA);
    vb.vbAppendUint16(mask);
    emitData(vb);
}

void Commands::getImuCalibration(double yaw)
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_GET_IMU_CALIBRATION);
    vb.vbAppendDouble32(yaw, 1e3);
    emitData(vb);
}

void Commands::bmConnect()
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_BM_CONNECT);
    emitData(vb);
}

void Commands::bmEraseFlashAll()
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_BM_ERASE_FLASH_ALL);
    emitData(vb);
}

void Commands::bmWriteFlash(uint32_t addr, QByteArray data)
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_BM_WRITE_FLASH);
    vb.vbAppendUint32(addr);
    vb.append(data);
    emitData(vb);
}

void Commands::bmWriteFlashLzo(uint32_t addr, quint16 decompressedLen, QByteArray data)
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_BM_WRITE_FLASH_LZO);
    vb.vbAppendUint32(addr);
    vb.vbAppendUint16(decompressedLen);
    vb.append(data);
    emitData(vb);
}

void Commands::bmReboot()
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_BM_REBOOT);
    emitData(vb);
}

void Commands::bmDisconnect()
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_BM_DISCONNECT);
    emitData(vb);
}

void Commands::bmMapPinsDefault()
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_BM_MAP_PINS_DEFAULT);
    emitData(vb);
}

void Commands::bmMapPinsNrf5x()
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_BM_MAP_PINS_NRF5X);
    emitData(vb);
}

void Commands::bmReadMem(uint32_t addr, quint16 size)
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_BM_MEM_READ);
    vb.vbAppendUint32(addr);
    vb.vbAppendUint16(size);
    emitData(vb);
}

void Commands::setCurrentRel(double current)
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_SET_CURRENT_REL);
    vb.vbAppendDouble32(current, 1e5);
    emitData(vb);
}

void Commands::forwardCanFrame(QByteArray data, quint32 id, bool isExtended)
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_CAN_FWD_FRAME);
    vb.vbAppendUint32(id);
    vb.vbAppendInt8(isExtended);
    vb.append(data);
    emitData(vb);
}

void Commands::setBatteryCut(double start, double end, bool store, bool fwdCan)
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_SET_BATTERY_CUT);
    vb.vbAppendDouble32(start, 1e3);
    vb.vbAppendDouble32(end, 1e3);
    vb.vbAppendInt8(store);
    vb.vbAppendInt8(fwdCan);
    emitData(vb);
}

void Commands::bmsGetValues()
{
    if (mTimeoutBmsVal > 0) {
        return;
    }

    mTimeoutBmsVal = mTimeoutCount;

    VByteArray vb;
    vb.vbAppendUint8(COMM_BMS_GET_VALUES);
    emitData(vb);
}

void Commands::bmsSetChargeAllowed(bool allowed)
{
    VByteArray vb;
    vb.vbAppendUint8(COMM_BMS_SET_CHARGE_ALLOWED);
    vb.vbAppendUint8(allowed);
    emitData(vb);
}

void Commands::bmsSetBalanceOverride(uint8_t cell, uint8_t override)
{
    VByteArray vb;
    vb.vbAppendUint8(COMM_BMS_SET_BALANCE_OVERRIDE);
    vb.vbAppendUint8(cell);
    vb.vbAppendUint8(override);
    emitData(vb);
}

void Commands::bmsResetCounters(bool ah, bool wh)
{
    VByteArray vb;
    vb.vbAppendUint8(COMM_BMS_RESET_COUNTERS);
    vb.vbAppendUint8(ah);
    vb.vbAppendUint8(wh);
    emitData(vb);
}

void Commands::bmsForceBalance(bool bal_en)
{
    VByteArray vb;
    vb.vbAppendUint8(COMM_BMS_FORCE_BALANCE);
    vb.vbAppendUint8(bal_en);
    emitData(vb);
}

void Commands::bmsZeroCurrentOffset()
{
    VByteArray vb;
    vb.vbAppendUint8(COMM_BMS_ZERO_CURRENT_OFFSET);
    emitData(vb);
}

void Commands::customConfigGetChunk(int confInd, int len, int offset)
{
    VByteArray vb;
    vb.vbAppendUint8(COMM_GET_CUSTOM_CONFIG_XML);
    vb.vbAppendInt8(int8_t(confInd));
    vb.vbAppendInt32(len);
    vb.vbAppendInt32(offset);
    emitData(vb);
}

void Commands::customConfigGet(int confInd, bool isDefault)
{
    if (mTimeoutCustomConf > 0) {
        return;
    }

    mTimeoutCustomConf = mTimeoutCount;

    VByteArray vb;
    vb.vbAppendUint8(isDefault ? COMM_GET_CUSTOM_CONFIG_DEFAULT : COMM_GET_CUSTOM_CONFIG);
    vb.vbAppendInt8(int8_t(confInd));
    emitData(vb);
}

void Commands::customConfigSet(int confInd, ConfigParams *conf)
{
    VByteArray vb;
    vb.vbAppendUint8(COMM_SET_CUSTOM_CONFIG);
    vb.vbAppendInt8(int8_t(confInd));
    conf->serialize(vb);
    emitData(vb);
}

void Commands::pswGetStatus(bool by_id, int id_ind)
{
    VByteArray vb;
    vb.vbAppendUint8(COMM_PSW_GET_STATUS);
    vb.vbAppendInt8(by_id);
    vb.vbAppendInt16(id_ind);
    emitData(vb);
}

void Commands::pswSwitch(int id, bool is_on, bool plot)
{
    VByteArray vb;
    vb.vbAppendUint8(COMM_PSW_SWITCH);
    vb.vbAppendInt16(id);
    vb.vbAppendInt8(is_on);
    vb.vbAppendInt8(plot);
    emitData(vb);
}

void Commands::qmlUiHwGet(int len, int offset)
{
    VByteArray vb;
    vb.vbAppendUint8(COMM_GET_QML_UI_HW);
    vb.vbAppendInt32(len);
    vb.vbAppendInt32(offset);
    emitData(vb);
}

void Commands::qmlUiAppGet(int len, int offset)
{
    VByteArray vb;
    vb.vbAppendUint8(COMM_GET_QML_UI_APP);
    vb.vbAppendInt32(len);
    vb.vbAppendInt32(offset);
    emitData(vb);
}

void Commands::qmlUiErase(int size)
{
    VByteArray vb;
    vb.vbAppendUint8(COMM_QMLUI_ERASE);
    vb.vbAppendInt32(size);
    emitData(vb);
}

void Commands::qmlUiWrite(QByteArray data, quint32 offset)
{
    VByteArray vb;
    vb.vbAppendUint8(COMM_QMLUI_WRITE);
    vb.vbAppendUint32(offset);
    vb.append(data);
    emitData(vb);
}

void Commands::ioBoardGetAll(int id)
{
    VByteArray vb;
    vb.vbAppendUint8(COMM_IO_BOARD_GET_ALL);
    vb.vbAppendInt16(id);
    emitData(vb);
}

void Commands::ioBoardSetPwm(int id, int channel, double duty)
{
    VByteArray vb;
    vb.vbAppendUint8(COMM_IO_BOARD_SET_PWM);
    vb.vbAppendInt16(id);
    vb.vbAppendInt16(channel);
    vb.vbAppendDouble32Auto(duty);
    emitData(vb);
}

void Commands::ioBoardSetDigital(int id, int channel, bool on)
{
    VByteArray vb;
    vb.vbAppendUint8(COMM_IO_BOARD_SET_DIGITAL);
    vb.vbAppendInt16(id);
    vb.vbAppendInt16(channel);
    vb.vbAppendInt8(on);
    emitData(vb);
}

void Commands::getStats(unsigned int mask)
{
    if (mTimeoutStats > 0) {
        return;
    }

    mTimeoutStats = mTimeoutCount;

    VByteArray vb;
    vb.vbAppendInt8(COMM_GET_STATS);
    vb.vbAppendUint16(mask);
    emitData(vb);
}

void Commands::resetStats(bool sendAck)
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_RESET_STATS);
    vb.vbAppendInt8(sendAck);
    emitData(vb);
}

void Commands::getGnss(unsigned int mask)
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_GET_GNSS);
    vb.vbAppendUint16(mask);
    emitData(vb);
}

void Commands::lispReadCode(int len, int offset)
{
    VByteArray vb;
    vb.vbAppendUint8(COMM_LISP_READ_CODE);
    vb.vbAppendInt32(len);
    vb.vbAppendInt32(offset);
    emitData(vb);
}

void Commands::lispWriteCode(QByteArray data, quint32 offset)
{
    VByteArray vb;
    vb.vbAppendUint8(COMM_LISP_WRITE_CODE);
    vb.vbAppendUint32(offset);
    vb.append(data);
    emitData(vb);
}

void Commands::lispStreamCode(QByteArray data, quint32 offset, quint32 totLen, qint8 mode)
{
    VByteArray vb;
    vb.vbAppendUint8(COMM_LISP_STREAM_CODE);
    vb.vbAppendInt32(offset);
    vb.vbAppendInt32(totLen);
    vb.vbAppendInt8(mode);
    vb.append(data);
    emitData(vb);
}

void Commands::lispEraseCode(int size)
{
    VByteArray vb;
    vb.vbAppendUint8(COMM_LISP_ERASE_CODE);
    vb.vbAppendInt32(size);
    emitData(vb);
}

void Commands::lispSetRunning(bool running)
{
    VByteArray vb;
    vb.vbAppendUint8(COMM_LISP_SET_RUNNING);
    vb.vbAppendInt8(running);
    emitData(vb);
}

void Commands::lispGetStats(bool all)
{
    VByteArray vb;
    vb.vbAppendUint8(COMM_LISP_GET_STATS);
    vb.vbAppendInt8(all);
    emitData(vb);
}

void Commands::lispSendReplCmd(QString str)
{
    VByteArray vb;
    vb.vbAppendUint8(COMM_LISP_REPL_CMD);
    vb.vbAppendString(str);
    emitData(vb);
}

void Commands::setBleName(QString name)
{
    VByteArray vb;
    vb.vbAppendUint8(COMM_SET_BLE_NAME);
    vb.vbAppendString(name);
    emitData(vb);
}

void Commands::setBlePin(QString pin)
{
    VByteArray vb;
    vb.vbAppendUint8(COMM_SET_BLE_PIN);
    vb.vbAppendString(pin);
    emitData(vb);
}

void Commands::fileList(QString path, QString from)
{
    VByteArray vb;
    vb.vbAppendUint8(COMM_FILE_LIST);
    vb.vbAppendString(path);
    vb.vbAppendString(from);
    emitData(vb);
}

void Commands::fileRead(QString path, qint32 offset)
{
    VByteArray vb;
    vb.vbAppendUint8(COMM_FILE_READ);
    vb.vbAppendString(path);
    vb.vbAppendInt32(offset);
    emitData(vb);
}

void Commands::fileWrite(QString path, qint32 offset, qint32 size, QByteArray data)
{
    VByteArray vb;
    vb.vbAppendUint8(COMM_FILE_WRITE);
    vb.vbAppendString(path);
    vb.vbAppendInt32(offset);
    vb.vbAppendInt32(size);
    vb.append(data);
    emitData(vb);
}

void Commands::fileMkdir(QString path)
{
    VByteArray vb;
    vb.vbAppendUint8(COMM_FILE_MKDIR);
    vb.vbAppendString(path);
    emitData(vb);
}

void Commands::fileRemove(QString path)
{
    VByteArray vb;
    vb.vbAppendUint8(COMM_FILE_REMOVE);
    vb.vbAppendString(path);
    emitData(vb);
}

void Commands::timerSlot()
{
    if (mTimeoutFwVer > 0) mTimeoutFwVer--;
    if (mTimeoutMcconf > 0) {
        mTimeoutMcconf--;
        if (mTimeoutMcconf == 0) {
            mCheckNextMcConfig = false;
        }
    }
    if (mTimeoutAppconf > 0) mTimeoutAppconf--;
    if (mTimeoutValues > 0) mTimeoutValues--;
    if (mTimeoutValuesSetup > 0) mTimeoutValuesSetup--;
    if (mTimeoutImuData > 0) mTimeoutImuData--;
    if (mTimeoutDecPpm > 0) mTimeoutDecPpm--;
    if (mTimeoutDecAdc > 0) mTimeoutDecAdc--;
    if (mTimeoutDecChuk > 0) mTimeoutDecChuk--;
    if (mTimeoutPingCan > 0) {
        mTimeoutPingCan--;
        if (mTimeoutPingCan == 0) {
            emit pingCanRx(QVector<int>(), true);
            qWarning() << "CAN ping timed out";
        }
    }
    if (mTimeoutCustomConf > 0) mTimeoutCustomConf--;
    if (mTimeoutBmsVal > 0) mTimeoutBmsVal--;
    if (mTimeoutStats > 0) mTimeoutStats--;
}

void Commands::emitData(QByteArray data)
{
    // Only allow firmware commands in limited mode
    if (mIsLimitedMode && data.at(0) > COMM_WRITE_NEW_APP_DATA) {
        if (!mLimitedSupportsFwdAllCan ||
                (data.at(0) != COMM_JUMP_TO_BOOTLOADER_ALL_CAN &&
                data.at(0) != COMM_ERASE_NEW_APP_ALL_CAN &&
                data.at(0) != COMM_WRITE_NEW_APP_DATA_ALL_CAN)) {
            if (!mLimitedSupportsEraseBootloader ||
                    (data.at(0) != COMM_ERASE_BOOTLOADER &&
                     data.at(0) != COMM_ERASE_BOOTLOADER_ALL_CAN)) {

                if (!mCompatibilityCommands.contains(int(data.at(0)))) {
                    return;
                }
            }
        }
    }

    if (mSendCan) {
        data.prepend((char)mCanId);
        data.prepend((char)COMM_FORWARD_CAN);
    }

    emit dataToSend(data);
}

double Commands::getFileSpeed() const
{
    return mFileSpeed;
}

double Commands::getFilePercentage() const
{
    return mFilePercentage;
}

bool Commands::getMaxPowerLossBug() const
{
    return mMaxPowerLossBug;
}

void Commands::setMaxPowerLossBug(bool maxPowerLossBug)
{
    mMaxPowerLossBug = maxPowerLossBug;
}

QVariantList Commands::fileBlockList(QString path)
{
    mFileShouldCancel = false;

    auto waitRes = [this](QList<FILE_LIST_ENTRY> &filesNow) {
        bool res = false;
        bool more = false;

        QEventLoop loop;
        QTimer timeoutTimer;
        timeoutTimer.setSingleShot(true);
        timeoutTimer.start(1500);
        auto conn = connect(this, &Commands::fileListRx,
                            [&res,&loop,&filesNow,&more](bool hasMore, QList<FILE_LIST_ENTRY> files) {
            filesNow.append(files);
            res = true;
            more = hasMore;
            loop.quit();
        });

        connect(&timeoutTimer, SIGNAL(timeout()), &loop, SLOT(quit()));
        loop.exec();

        disconnect(conn);
        return qMakePair(res, more);
    };

    auto waitResRetry = [waitRes,path,this](QList<FILE_LIST_ENTRY> &filesNow) {
        QString from = "";
        if (!filesNow.isEmpty()) {
            from = filesNow.last().name;
        }
        fileList(path, from);
        auto res = waitRes(filesNow);
        int retries = 3;
        while (!res.first && retries > 0) {
            fileList(path, from);
            res = waitRes(filesNow);
            retries--;
        }
        return res;
    };

    QList<FILE_LIST_ENTRY> files;
    auto res = waitResRetry(files);

    while (res.second) {
        res = waitResRetry(files);

        if (mFileShouldCancel) {
            break;
        }
    }

    if (!res.first) {
        qWarning() << "Could not list files";
    }

    std::sort(files.begin(), files.end(), []
              (const FILE_LIST_ENTRY & a, const FILE_LIST_ENTRY & b) {
        return a.name.toLower() < b.name.toLower();
    });

    QVariantList retVal;
    foreach (auto f, files) {
        retVal.append(QVariant::fromValue(f));
    }

    return retVal;
}

QByteArray Commands::fileBlockRead(QString path)
{
    mFileShouldCancel = false;

    auto waitRes = [this](QByteArray &dataNow, qint32 offsetNow) {
        qint32 sizeRet = -1.0;

        QEventLoop loop;
        QTimer timeoutTimer;
        timeoutTimer.setSingleShot(true);
        timeoutTimer.start(1500);
        auto conn = connect(this, &Commands::fileReadRx,
                            [&sizeRet,&loop,&dataNow,&offsetNow]
                            (qint32 offset, qint32 size, QByteArray data) {
            if (offset == offsetNow) {
                sizeRet = size;
                dataNow.append(data);
            } else {
                qWarning() << "Wrong offset";
            }
            loop.quit();
        });

        connect(&timeoutTimer, SIGNAL(timeout()), &loop, SLOT(quit()));
        loop.exec();

        disconnect(conn);
        return sizeRet;
    };

    auto waitResRetry = [waitRes,path,this](QByteArray &dataNow, qint32 offsetNow) {
        fileRead(path, offsetNow);
        auto res = waitRes(dataNow, offsetNow);
        int retries = 3;
        while (res < 0 && retries > 0) {
            fileRead(path, offsetNow);
            res = waitRes(dataNow, offsetNow);
            retries--;
        }
        return res;
    };

    QElapsedTimer t;
    t.start();

    QByteArray data;
    auto res = waitResRetry(data, 0);

    while (res >= 0 && data.size() < res) {
        mFilePercentage = (double(data.size()) / double(res)) * 100.0;
        mFileSpeed = (double(data.size()) / double(t.elapsed())) * 1000.0;
        emit fileProgress(data.size(), res, mFilePercentage, mFileSpeed);
        res = waitResRetry(data, data.size());

        if (mFileShouldCancel) {
            return QByteArray();
            break;
        }
    }

    if (res < 0) {
        qWarning() << "Could not read file";
        return QByteArray();
    }

    return data;
}

bool Commands::fileBlockWrite(QString path, QByteArray data)
{
    mFileShouldCancel = false;

    auto waitRes = [this]() {
        bool res = false;

        QEventLoop loop;
        QTimer timeoutTimer;
        timeoutTimer.setSingleShot(true);
        timeoutTimer.start(1500);
        auto conn = connect(this, &Commands::fileWriteRx,
                            [&res,&loop]
                            (qint32 offset, bool ok) {
            (void)offset;
            res = ok;
            loop.quit();
        });

        connect(&timeoutTimer, SIGNAL(timeout()), &loop, SLOT(quit()));
        loop.exec();
        disconnect(conn);

        return res;
    };

    auto waitResRetry = [waitRes,path,this](qint32 offsetNow, qint32 size, QByteArray dataNow) {
        fileWrite(path, offsetNow, size, dataNow);
        auto res = waitRes();
        int retries = 3;
        while (!res && retries > 0) {
            fileWrite(path, offsetNow, size, dataNow);
            res = waitRes();
            retries--;
        }
        return res;
    };

    const int chunkSize = 384;
    qint32 size = data.size();
    int sz = data.size() > chunkSize ? chunkSize : data.size();

    QElapsedTimer t;
    t.start();

    qint32 offset = 0;
    auto res = waitResRetry(offset, size, data.mid(0, sz));
    offset += sz;
    data.remove(0, sz);

    while (res && data.size() > 0) {
        mFilePercentage = (double(size - data.size()) / double(size)) * 100.0;
        mFileSpeed = (double(size - data.size()) / double(t.elapsed())) * 1000.0;
        emit fileProgress(size - data.size(), size, mFilePercentage, mFileSpeed);

        sz = data.size() > chunkSize ? chunkSize : data.size();
        res = waitResRetry(offset, size, data.mid(0, sz));
        offset += sz;
        data.remove(0, sz);

        if (mFileShouldCancel) {
            return false;
        }
    }

    return res;
}

bool Commands::fileBlockMkdir(QString path)
{
    mFileShouldCancel = false;

    auto waitRes = [this]() {
        bool res = false;

        QEventLoop loop;
        QTimer timeoutTimer;
        timeoutTimer.setSingleShot(true);
        timeoutTimer.start(1500);
        auto conn = connect(this, &Commands::fileMkdirRx,
                            [&res,&loop](bool ok) {
            res = ok;
            loop.quit();
        });

        connect(&timeoutTimer, SIGNAL(timeout()), &loop, SLOT(quit()));
        loop.exec();
        disconnect(conn);

        return res;
    };

    auto waitResRetry = [waitRes,path,this]() {
        fileMkdir(path);
        auto res = waitRes();
        int retries = 3;
        while (!res && retries > 0) {
            fileMkdir(path);
            res = waitRes();
            retries--;
        }
        return res;
    };

    return waitResRetry();
}

bool Commands::fileBlockRemove(QString path)
{
    mFileShouldCancel = false;

    auto waitRes = [this]() {
        bool res = false;

        QEventLoop loop;
        QTimer timeoutTimer;
        timeoutTimer.setSingleShot(true);
        timeoutTimer.start(1500);
        auto conn = connect(this, &Commands::fileRemoveRx,
                            [&res,&loop](bool ok) {
            res = ok;
            loop.quit();
        });

        connect(&timeoutTimer, SIGNAL(timeout()), &loop, SLOT(quit()));
        loop.exec();
        disconnect(conn);

        return res;
    };

    auto waitResRetry = [waitRes,path,this]() {
        fileRemove(path);
        auto res = waitRes();
        int retries = 3;
        while (!res && retries > 0) {
            fileRemove(path);
            res = waitRes();
            retries--;
        }
        return res;
    };

    return waitResRetry();
}

void Commands::fileBlockCancel()
{
    mFileShouldCancel = true;
    mFilePercentage = 0.0;
    mFileSpeed = 0;
    emit fileProgress(0, 0, mFilePercentage, mFileSpeed);
}

bool Commands::fileBlockDidCancel()
{
    return mFileShouldCancel;
}

bool Commands::getLimitedSupportsFwdAllCan() const
{
    return mLimitedSupportsFwdAllCan;
}

void Commands::setLimitedSupportsFwdAllCan(bool limitedSupportsFwdAllCan)
{
    mLimitedSupportsFwdAllCan = limitedSupportsFwdAllCan;
}

bool Commands::getLimitedSupportsEraseBootloader() const
{
    return mLimitedSupportsEraseBootloader;
}

void Commands::setLimitedSupportsEraseBootloader(bool limitedSupportsEraseBootloader)
{
    mLimitedSupportsEraseBootloader = limitedSupportsEraseBootloader;
}

QVector<int> Commands::getLimitedCompatibilityCommands() const
{
    return mCompatibilityCommands;
}

void Commands::setLimitedCompatibilityCommands(QVector<int> compatibilityCommands)
{
    mCompatibilityCommands = compatibilityCommands;
}

QString Commands::faultToStr(mc_fault_code fault)
{
    switch (fault) {
    case FAULT_CODE_NONE: return "FAULT_CODE_NONE";
    case FAULT_CODE_OVER_VOLTAGE: return "FAULT_CODE_OVER_VOLTAGE";
    case FAULT_CODE_UNDER_VOLTAGE: return "FAULT_CODE_UNDER_VOLTAGE";
    case FAULT_CODE_DRV: return "FAULT_CODE_DRV";
    case FAULT_CODE_ABS_OVER_CURRENT: return "FAULT_CODE_ABS_OVER_CURRENT";
    case FAULT_CODE_OVER_TEMP_FET: return "FAULT_CODE_OVER_TEMP_FET";
    case FAULT_CODE_OVER_TEMP_MOTOR: return "FAULT_CODE_OVER_TEMP_MOTOR";
    case FAULT_CODE_GATE_DRIVER_OVER_VOLTAGE: return "FAULT_CODE_GATE_DRIVER_OVER_VOLTAGE";
    case FAULT_CODE_GATE_DRIVER_UNDER_VOLTAGE: return "FAULT_CODE_GATE_DRIVER_UNDER_VOLTAGE";
    case FAULT_CODE_MCU_UNDER_VOLTAGE: return "FAULT_CODE_MCU_UNDER_VOLTAGE";
    case FAULT_CODE_BOOTING_FROM_WATCHDOG_RESET: return "FAULT_CODE_BOOTING_FROM_WATCHDOG_RESET";
    case FAULT_CODE_ENCODER_SPI: return "FAULT_CODE_ENCODER_SPI";
    case FAULT_CODE_ENCODER_SINCOS_BELOW_MIN_AMPLITUDE: return "FAULT_CODE_ENCODER_SINCOS_BELOW_MIN_AMPLITUDE";
    case FAULT_CODE_ENCODER_SINCOS_ABOVE_MAX_AMPLITUDE: return "FAULT_CODE_ENCODER_SINCOS_ABOVE_MAX_AMPLITUDE";
    case FAULT_CODE_FLASH_CORRUPTION: return "FAULT_CODE_FLASH_CORRUPTION";
    case FAULT_CODE_HIGH_OFFSET_CURRENT_SENSOR_1: return "FAULT_CODE_HIGH_OFFSET_CURRENT_SENSOR_1";
    case FAULT_CODE_HIGH_OFFSET_CURRENT_SENSOR_2: return "FAULT_CODE_HIGH_OFFSET_CURRENT_SENSOR_2";
    case FAULT_CODE_HIGH_OFFSET_CURRENT_SENSOR_3: return "FAULT_CODE_HIGH_OFFSET_CURRENT_SENSOR_3";
    case FAULT_CODE_UNBALANCED_CURRENTS: return "FAULT_CODE_UNBALANCED_CURRENTS";
    case FAULT_CODE_BRK: return "FAULT_CODE_BRK";
    case FAULT_CODE_RESOLVER_LOT: return "FAULT_CODE_RESOLVER_LOT";
    case FAULT_CODE_RESOLVER_DOS: return "FAULT_CODE_RESOLVER_DOS";
    case FAULT_CODE_RESOLVER_LOS: return "FAULT_CODE_RESOLVER_LOS";        
    case FAULT_CODE_FLASH_CORRUPTION_APP_CFG: return "FAULT_CODE_FLASH_CORRUPTION_APP_CFG";
    case FAULT_CODE_FLASH_CORRUPTION_MC_CFG: return "FAULT_CODE_FLASH_CORRUPTION_MC_CFG";
    case FAULT_CODE_ENCODER_NO_MAGNET: return "FAULT_CODE_ENCODER_NO_MAGNET";
    case FAULT_CODE_ENCODER_MAGNET_TOO_STRONG: return "FAULT_CODE_ENCODER_MAGNET_TOO_STRONG";
    case FAULT_CODE_PHASE_FILTER: return "FAULT_CODE_PHASE_FILTER";
    case FAULT_CODE_ENCODER_FAULT: return "FAULT_CODE_ENCODER_FAULT";
    }

    return "Unknown fault";
}

QByteArray Commands::bmReadMemWait(uint32_t addr, quint16 size, int timeoutMs)
{
    bmReadMem(addr, size);

    int res = -10;
    QByteArray resData;

    QEventLoop loop;
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    timeoutTimer.start(timeoutMs);
    auto conn = connect(this, &Commands::bmReadMemRes, [&res,&resData,&loop]
                        (int rdRes, QByteArray data) {
        res = rdRes;
        resData = data;
        loop.quit();
    });

    connect(&timeoutTimer, SIGNAL(timeout()), &loop, SLOT(quit()));
    loop.exec();

    disconnect(conn);
    return resData;
}

int Commands::bmWriteMemWait(uint32_t addr, QByteArray data, int timeoutMs)
{
    bmWriteFlash(addr, data);

    int res = -10;

    QEventLoop loop;
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    timeoutTimer.start(timeoutMs);
    auto conn = connect(this, &Commands::bmWriteFlashRes, [&res,&loop](int wrRes) {
        res = wrRes;
        loop.quit();
    });

    connect(&timeoutTimer, SIGNAL(timeout()), &loop, SLOT(quit()));
    loop.exec();

    disconnect(conn);
    return res;
}

void Commands::setAppConfig(ConfigParams *appConfig)
{
    mAppConfig = appConfig;
    connect(mAppConfig, SIGNAL(updateRequested()), this, SLOT(getAppConf()));
    connect(mAppConfig, SIGNAL(updateRequestDefault()), this, SLOT(getAppConfDefault()));
}

void Commands::setMcConfig(ConfigParams *mcConfig)
{
    mMcConfig = mcConfig;
    connect(mMcConfig, SIGNAL(updateRequested()), this, SLOT(getMcconf()));
    connect(mMcConfig, SIGNAL(updateRequestDefault()), this, SLOT(getMcconfDefault()));
}

void Commands::checkMcConfig()
{
    mCheckNextMcConfig = true;
    VByteArray vb;
    vb.vbAppendInt8(COMM_GET_MCCONF);
    emitData(vb);
}

void Commands::emitEmptyValues()
{
    MC_VALUES values;
    values.temp_mos = 0.0;
    values.temp_motor = 0.0;
    values.current_motor = 0.0;
    values.current_in = 0.0;
    values.id = 0.0;
    values.iq = 0.0;
    values.duty_now = 0.0;
    values.rpm = 0.0;
    values.v_in = 45.0;
    values.amp_hours = 0.0;
    values.amp_hours_charged = 0.0;
    values.watt_hours = 0.0;
    values.watt_hours_charged = 0.0;
    values.tachometer = 0;
    values.tachometer_abs = 0;
    values.fault_code = FAULT_CODE_NONE;
    values.fault_str = faultToStr(values.fault_code);
    values.position = 0.0;
    values.vesc_id = 0;

    emit valuesReceived(values, 0xFFFFFFFF);
}

void Commands::emitEmptySetupValues()
{
    SETUP_VALUES values;
    values.temp_mos = 0.0;
    values.temp_motor = 0.0;
    values.current_motor = 0.0;
    values.current_in = 0.0;
    values.duty_now = 0.0;
    values.rpm = 0.0;
    values.speed = 0.0;
    values.v_in = 45.0;
    values.battery_level = 0.0;
    values.amp_hours = 0.0;
    values.amp_hours_charged = 0.0;
    values.watt_hours = 0.0;
    values.watt_hours_charged = 0.0;
    values.tachometer = 0.0;
    values.tachometer_abs = 0.0;
    values.position = 0.0;
    values.fault_code = FAULT_CODE_NONE;
    values.fault_str = faultToStr(values.fault_code);
    values.vesc_id = 0;
    values.num_vescs = 1;
    values.battery_wh = 0.0;

    emit valuesSetupReceived(values, 0xFFFFFFFF);
}

void Commands::emitEmptyStats()
{
    STAT_VALUES stat;

    emit statsRx(stat, 0xFFFFFFFF);
}
