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

#include "commands.h"
#include <QDebug>

Commands::Commands(QObject *parent) : QObject(parent)
{
    mSendCan = false;
    mCanId = 0;
    mIsLimitedMode = false;

    // Firmware state
    mFirmwareIsUploading = false;
    mFirmwareState = 0;
    mFimwarePtr = 0;
    mFirmwareTimer = 0;
    mFirmwareRetries = 0;
    mFirmwareUploadStatus = "FW Upload Status";
    mCheckNextMcConfig = false;

    mTimer = new QTimer(this);
    mTimer->setInterval(10);
    mTimer->start();

    mMcConfig = 0;
    mAppConfig = 0;

    mTimeoutCount = 50;
    mTimeoutFwVer = 0;
    mTimeoutMcconf = 0;
    mTimeoutAppconf = 0;
    mTimeoutValues = 0;
    mTimeoutDecPpm = 0;
    mTimeoutDecAdc = 0;
    mTimeoutDecChuk = 0;

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

void Commands::setSendCan(bool sendCan, int id)
{
    mSendCan = sendCan;

    if (id >= 0) {
        mCanId = id;
    }
}

bool Commands::getSendCan()
{
    return mSendCan;
}

void Commands::setCanSendId(unsigned int id)
{
    mCanId = id;
}

int Commands::getCanSendId()
{
    return mCanId;
}

void Commands::processPacket(QByteArray data)
{
    VByteArray vb(data);
    COMM_PACKET_ID id = (COMM_PACKET_ID)vb.vbPopFrontUint8();

    switch (id) {
    case COMM_FW_VERSION: {
        mTimeoutFwVer = 0;
        int fw_major;
        int fw_minor;
        QString hw;
        QByteArray uuid;

        if (vb.size() >= 2) {
            fw_major = vb.vbPopFrontInt8();
            fw_minor = vb.vbPopFrontInt8();
            hw = vb.vbPopFrontString();
            if (vb.size() >= 12) {
                uuid.append(vb.left(12));
                vb.remove(0, 12);
            }
        } else {
            fw_major = -1;
            fw_minor = -1;
        }

        emit fwVersionReceived(fw_major, fw_minor, hw, uuid);
    } break;

    case COMM_ERASE_NEW_APP:
    case COMM_WRITE_NEW_APP_DATA:
        firmwareUploadUpdate(!vb.at(0));
        break;

    case COMM_GET_VALUES: {
        mTimeoutValues = 0;
        MC_VALUES values;
        values.temp_mos = vb.vbPopFrontDouble16(1e1);
        values.temp_motor = vb.vbPopFrontDouble16(1e1);
        values.current_motor = vb.vbPopFrontDouble32(1e2);
        values.current_in = vb.vbPopFrontDouble32(1e2);
        values.id = vb.vbPopFrontDouble32(1e2);
        values.iq = vb.vbPopFrontDouble32(1e2);
        values.duty_now = vb.vbPopFrontDouble16(1e3);
        values.rpm = vb.vbPopFrontDouble32(1e0);
        values.v_in = vb.vbPopFrontDouble16(1e1);
        values.amp_hours = vb.vbPopFrontDouble32(1e4);
        values.amp_hours_charged = vb.vbPopFrontDouble32(1e4);
        values.watt_hours = vb.vbPopFrontDouble32(1e4);
        values.watt_hours_charged = vb.vbPopFrontDouble32(1e4);
        values.tachometer = vb.vbPopFrontInt32();
        values.tachometer_abs = vb.vbPopFrontInt32();
        values.fault_code = (mc_fault_code)vb.vbPopFrontInt8();
        values.fault_str = faultToStr(values.fault_code);

        if (vb.size() >= 4) {
            values.position = vb.vbPopFrontDouble32(1e6);
        } else {
            values.position = -1.0;
        }

        if (vb.size() >= 1) {
            values.vesc_id = vb.vbPopFrontUint8();
        } else {
            values.vesc_id = 255;
        }

        emit valuesReceived(values);
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
            mMcConfig->deSerialize(vb);
            mMcConfig->updateDone();

            if (mCheckNextMcConfig) {
                mCheckNextMcConfig = false;
                emit mcConfigCheckResult(mMcConfig->checkDifference(&mMcConfigLast));
            }
        }
        break;

    case COMM_GET_APPCONF:
    case COMM_GET_APPCONF_DEFAULT:
        mTimeoutAppconf = 0;
        if (mAppConfig) {
            mAppConfig->deSerialize(vb);
            mAppConfig->updateDone();
        }
        break;

    case COMM_DETECT_MOTOR_PARAM: {
        bldc_detect param;
        param.cycle_int_limit = vb.vbPopFrontDouble32(1e3);
        param.bemf_coupling_k = vb.vbPopFrontDouble32(1e3);
        for (int i = 0;i < 8;i++) {
            param.hall_table.append((int)vb.vbPopFrontUint8());
        }
        param.hall_res = (int)vb.vbPopFrontUint8();
        emit bldcDetectReceived(param);
    } break;

    case COMM_DETECT_MOTOR_R_L: {
        double r = vb.vbPopFrontDouble32(1e6);
        double l = vb.vbPopFrontDouble32(1e3);
        emit motorRLReceived(r, l);
    } break;

    case COMM_DETECT_MOTOR_FLUX_LINKAGE: {
        emit motorLinkageReceived(vb.vbPopFrontDouble32(1e7));
    } break;

    case COMM_DETECT_ENCODER: {
        double offset = vb.vbPopFrontDouble32(1e6);
        double ratio = vb.vbPopFrontDouble32(1e6);
        bool inverted = vb.vbPopFrontInt8();
        emit encoderParamReceived(offset, ratio, inverted);
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

    case COMM_CUSTOM_APP_DATA:
        emit customAppDataReceived(vb);
        break;

    case COMM_NRF_START_PAIRING:
        emit nrfPairingRes((NRF_PAIR_RES)vb.vbPopFrontInt8());
        break;

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

void Commands::sendTerminalCmd(QString cmd)
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_TERMINAL_CMD);
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

void Commands::samplePrint(debug_sampling_mode mode, int sample_len, int dec)
{
    VByteArray vb;
    vb.vbAppendInt8(COMM_SAMPLE_PRINT);
    vb.vbAppendInt8(mode);
    vb.vbAppendUint16(sample_len);
    vb.vbAppendUint8(dec);
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
    vb.vbAppendDouble16(current, 1e3);
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

void Commands::timerSlot()
{
    if (mFirmwareIsUploading) {
        if (mFirmwareTimer) {
            mFirmwareTimer--;
        } else {
            firmwareUploadUpdate(true);
        }
    }

    if (mTimeoutFwVer > 0) mTimeoutFwVer--;
    if (mTimeoutMcconf > 0) mTimeoutMcconf--;
    if (mTimeoutAppconf > 0) mTimeoutAppconf--;
    if (mTimeoutValues > 0) mTimeoutValues--;
    if (mTimeoutDecPpm > 0) mTimeoutDecPpm--;
    if (mTimeoutDecAdc > 0) mTimeoutDecAdc--;
    if (mTimeoutDecChuk > 0) mTimeoutDecChuk--;
}

void Commands::emitData(QByteArray data)
{
    // Only allow firmware commands in limited mode
    if (mIsLimitedMode && data.at(0) > COMM_WRITE_NEW_APP_DATA) {
        return;
    }

    if (mSendCan) {
        data.prepend((char)mCanId);
        data.prepend((char)COMM_FORWARD_CAN);
    }

    emit dataToSend(data);
}

void Commands::firmwareUploadUpdate(bool isTimeout)
{
    if (!mFirmwareIsUploading) {
        return;
    }

    const int app_packet_size = 200;
    const int retries = 5;
    const int timeout = 350;

    if (mFirmwareState == 0) {
        mFirmwareUploadStatus = "Buffer Erase";
        if (isTimeout) {
            // Erase timed out, abort.
            mFirmwareIsUploading = false;
            mFimwarePtr = 0;
            mFirmwareUploadStatus = "Buffer Erase Timeout";
        } else {
            mFirmwareState++;
            mFirmwareRetries = retries;
            mFirmwareTimer = timeout;
            firmwareUploadUpdate(true);
        }
    } else if (mFirmwareState == 1) {
        mFirmwareUploadStatus = "CRC/Size Write";
        if (isTimeout) {
            if (mFirmwareRetries > 0) {
                mFirmwareRetries--;
                mFirmwareTimer = timeout;
            } else {
                mFirmwareIsUploading = false;
                mFimwarePtr = 0;
                mFirmwareState = 0;
                mFirmwareUploadStatus = "CRC/Size Write Timeout";
                return;
            }

            quint16 crc = Packet::crc16((const unsigned char*)mNewFirmware.constData(), mNewFirmware.size());

            VByteArray vb;
            vb.append((char)COMM_WRITE_NEW_APP_DATA);
            vb.vbAppendUint32(0);
            vb.vbAppendUint32(mNewFirmware.size());
            vb.vbAppendUint16(crc);
            emitData(vb);
        } else {
            mFirmwareState++;
            mFirmwareRetries = retries;
            mFirmwareTimer = timeout;
            firmwareUploadUpdate(true);
        }
    } else if (mFirmwareState == 2) {
        mFirmwareUploadStatus = "FW Data Write";
        if (isTimeout) {
            if (mFirmwareRetries > 0) {
                mFirmwareRetries--;
                mFirmwareTimer = timeout;
            } else {
                mFirmwareIsUploading = false;
                mFimwarePtr = 0;
                mFirmwareState = 0;
                mFirmwareUploadStatus = "FW Data Write Timeout";
                return;
            }

            int fw_size_left = mNewFirmware.size() - mFimwarePtr;
            int send_size = fw_size_left > app_packet_size ? app_packet_size : fw_size_left;

            VByteArray vb;
            vb.append((char)COMM_WRITE_NEW_APP_DATA);

            if (mFirmwareIsBootloader) {
                vb.vbAppendUint32(mFimwarePtr + (1024 * 128 * 3));
            } else {
                vb.vbAppendUint32(mFimwarePtr + 6);
            }

            vb.append(mNewFirmware.mid(mFimwarePtr, send_size));
            emitData(vb);
        } else {
            mFirmwareRetries = retries;
            mFirmwareTimer = timeout;
            mFimwarePtr += app_packet_size;

            if (mFimwarePtr >= mNewFirmware.size()) {
                mFirmwareIsUploading = false;
                mFimwarePtr = 0;
                mFirmwareState = 0;
                mFirmwareUploadStatus = "FW Upload Done";

                // Upload done. Enter bootloader!
                if (!mFirmwareIsBootloader) {
                    QByteArray buffer;
                    buffer.append((char)COMM_JUMP_TO_BOOTLOADER);
                    emitData(buffer);
                }
            } else {
                firmwareUploadUpdate(true);
            }
        }
    }
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
    default: return "Unknown fault";
    }
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

void Commands::startFirmwareUpload(QByteArray &newFirmware, bool isBootloader)
{
    mFirmwareIsBootloader = isBootloader;
    mFirmwareIsUploading = true;
    mFirmwareState = mFirmwareIsBootloader ? 2 : 0;
    mFimwarePtr = 0;
    mFirmwareTimer = 500;
    mFirmwareRetries = 5;
    mNewFirmware.clear();
    mNewFirmware.append(newFirmware);
    mFirmwareUploadStatus = "Buffer Erase";

    if (mFirmwareIsBootloader) {
        firmwareUploadUpdate(true);
    } else {
        VByteArray vb;
        vb.vbAppendInt8(COMM_ERASE_NEW_APP);
        vb.vbAppendUint32(mNewFirmware.size());
        emitData(vb);
    }
}

double Commands::getFirmwareUploadProgress()
{
    if (mFirmwareIsUploading) {
        return (double)mFimwarePtr / (double)mNewFirmware.size();
    } else {
        return -1.0;
    }
}

QString Commands::getFirmwareUploadStatus()
{
    return mFirmwareUploadStatus;
}

void Commands::cancelFirmwareUpload()
{
    if (mFirmwareIsUploading) {
        mFirmwareIsUploading = false;
        mFimwarePtr = 0;
        mFirmwareState = 0;
        mFirmwareUploadStatus = "Cancelled";
    }
}

void Commands::checkMcConfig()
{
    mCheckNextMcConfig = true;
    VByteArray vb;
    vb.vbAppendInt8(COMM_GET_MCCONF);
    emitData(vb);
}
