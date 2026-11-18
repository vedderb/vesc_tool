/*
    Copyright 2026 Dado Mista

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

#include "virtualvesc.h"
#include "commands.h"
#include "utility.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDateTime>
#include <QTextStream>
#include <QRandomGenerator>
#include <QDebug>
#include <cmath>

namespace {
const char *VIRTUAL_HW_NAME = "60";
const char *VIRTUAL_FW_NAME = "VIRTUAL";
const char VIRTUAL_UUID[12] = {'V', 'I', 'R', 'T', 'U', 'A', 'L', 'V', 'E', 'S', 'C', '1'};

// Same limits as the firmware
const int CODE_SLOT_MAX_SIZE = 1024 * 128;
const int PACKET_MAX_PL_LEN = 512;
const int SAMPLE_MAX_LEN = 1000;
const int FAULT_LOG_LEN = 25;
const int BMS_CAN_ID = 10;

// Detection results have a bit of measurement noise
double measured(double value)
{
    return value * (1.0 + (QRandomGenerator::global()->generateDouble() - 0.5) * 0.02);
}
}

VirtualVesc::VirtualVesc(QObject *parent) :
    VirtualVesc(nullptr, -1, HW_TYPE_VESC, VIRTUAL_HW_NAME,
                QByteArray(VIRTUAL_UUID, 12), parent)
{
}

VirtualVesc::VirtualVesc(VirtualVesc *main, int canId, HW_TYPE hwType, const QString &hwName,
                         const QByteArray &uuid, QObject *parent) :
    QObject(parent), mMain(main), mCanId(canId), mHwType(hwType), mHwName(hwName), mUuid(uuid)
{
    mPacket = new Packet(this);
    mLispRunning = false;
    mCanOn = true;
    mSimTimer = nullptr;
    mSimSteps = 0;
    mDispPosMode = DISP_POS_MODE_NONE;

    auto fw = Utility::configLatestSupported();
    mFwMajor = fw.first;
    mFwMinor = fw.second;

    connect(mPacket, &Packet::packetReceived, this, &VirtualVesc::processPacket);
    connect(mPacket, &Packet::dataToSend, this, [this](QByteArray &data) {
        emit dataToSend(data);
    });

    // Code is written in many small chunks, so save shortly after the last change
    mSaveTimer = new QTimer(this);
    mSaveTimer->setSingleShot(true);
    mSaveTimer->setInterval(1000);
    connect(mSaveTimer, &QTimer::timeout, this, &VirtualVesc::saveCode);

    if (mMain) {
        mPlant = mMain->mPlant;
        if (mHwType == HW_TYPE_VESC) {
            mMcConf = mMain->mMcConf;
            mMcConfDefault = mMain->mMcConfDefault;
            mAppConf = mMain->mAppConf;
            mAppConfDefault = mMain->mAppConfDefault;
            mAppConf.updateParamInt("controller_id", mCanId);
        }
        return;
    }

    loadMainConfigs();
    mCodeDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/virtual_vesc";
    loadCode();

    mPlant = new VirtualPlant(&mMcConf, &mAppConf);
    mSimTimer = new QTimer(this);
    mSimTimer->setInterval(10);
    connect(mSimTimer, &QTimer::timeout, this, &VirtualVesc::simStep);

    // The rest of the vehicle, on the CAN-bus
    mCanNodes.append(new VirtualVesc(this, 1, HW_TYPE_VESC, VIRTUAL_HW_NAME, "VIRTUALVESC2", this));
    mCanNodes.append(new VirtualVesc(this, 2, HW_TYPE_CUSTOM_MODULE, "Express", "VIRTUALEXPR1", this));
    mCanNodes.append(new VirtualVesc(this, BMS_CAN_ID, HW_TYPE_VESC_BMS, "BMS", "VIRTUALBMS01", this));
    for (auto node: mCanNodes) {
        connect(node, &VirtualVesc::dataToSend, this, &VirtualVesc::dataToSend);
    }
}

VirtualVesc::~VirtualVesc()
{
    if (mSaveTimer->isActive()) {
        saveCode();
    }

    if (!mMain) {
        delete mPlant;
    }
}

void VirtualVesc::processData(const QByteArray &data)
{
    if (mSimTimer && !mSimTimer->isActive()) {
        mSimTimer->start();
    }

    mPacket->processData(data);
}

void VirtualVesc::resetState()
{
    if (mSimTimer) {
        mSimTimer->stop();
    }

    mPacket->resetState();
}

QByteArray VirtualVesc::findConfigXml(const QByteArray &data)
{
    // The XML is stored as a qCompress-blob: [uint32 size][zlib stream]. Look
    // for the zlib header (78 DA, best compression) and validate candidates.
    int ind = data.indexOf("\x78\xDA", 4);
    while (ind >= 4) {
        VByteArray sizeBytes(data.mid(ind - 4, 4));
        quint32 size = sizeBytes.vbPopFrontUint32();

        if (size >= 100 && size <= 4 * 1024 * 1024) {
            QByteArray blob = data.mid(ind - 4);
            QByteArray xml = qUncompress(blob);

            if (quint32(xml.size()) == size && xml.left(300).contains("<ConfigParams")) {
                ConfigParams params;
                QByteArray compressed = qCompress(xml, 9);
                if (params.loadCompressedParamsXml(compressed) &&
                        !params.getSerializeOrder().isEmpty()) {
                    return compressed;
                }
            }
        }

        ind = data.indexOf("\x78\xDA", ind + 1);
    }

    return QByteArray();
}

void VirtualVesc::processPacket(QByteArray &data)
{
    VByteArray vb(data);
    COMM_PACKET_ID id = COMM_PACKET_ID(vb.vbPopFrontUint8());

    VByteArray reply;
    reply.vbAppendUint8(id);

    VirtualPlant *p = mPlant;
    int ownId = mMain ? mCanId : mAppConf.getParamInt("controller_id");
    bool fileCmd = id >= COMM_FILE_LIST && id <= COMM_FILE_REMOVE;

    // The BMS and the Express only answer what they have
    if ((mHwType == HW_TYPE_VESC_BMS && id != COMM_FW_VERSION && id != COMM_BMS_GET_VALUES) ||
            (mHwType == HW_TYPE_CUSTOM_MODULE && id != COMM_FW_VERSION && !fileCmd)) {
        return;
    }

    if (fileCmd && (!mMain || mHwType == HW_TYPE_CUSTOM_MODULE)) {
        processFileCmd(id, vb, reply);
        sendReply(reply);
        return;
    }

    switch (id) {
    case COMM_FW_VERSION: {
        quint16 qmlFlags = 0;
        if (!codeSlotPayload(mQmlSlot).isEmpty()) {
            VByteArray flags(mQmlSlot.mid(6, 2));
            qmlFlags = flags.vbPopFrontUint16();
        }

        reply.vbAppendInt8(mFwMajor);
        reply.vbAppendInt8(mFwMinor);
        reply.vbAppendString(mHwName);
        reply.append(mUuid);
        reply.vbAppendUint8(0); // Pairing done
        reply.vbAppendUint8(0); // Test version
        reply.vbAppendUint8(mHwType);
        reply.vbAppendUint8((mLispRunning && !mCustomConfXml.isEmpty()) ? 1 : 0);
        reply.vbAppendUint8(0); // Phase filters
        reply.vbAppendUint8(0); // QML HW
        reply.vbAppendUint8(qmlFlags); // QML APP
        reply.vbAppendUint8(0); // NRF flags
        reply.vbAppendString(VIRTUAL_FW_NAME);
        // Note: No HW config CRC so that VESC Tool does not cache custom configs
        sendReply(reply);
    } break;

    case COMM_GET_MCCONF:
        mMcConf.serialize(reply);
        sendReply(reply);
        break;

    case COMM_GET_MCCONF_DEFAULT:
        mMcConfDefault.serialize(reply);
        sendReply(reply);
        break;

    case COMM_SET_MCCONF:
        if (mMcConf.deSerialize(vb)) {
            sendReply(reply);
        }
        break;

    case COMM_SET_MCCONF_TEMP:
    case COMM_SET_MCCONF_TEMP_SETUP:
        // The temporary limits are not applied, only acknowledged
        sendReply(reply);
        break;

    case COMM_GET_APPCONF:
        mAppConf.serialize(reply);
        sendReply(reply);
        break;

    case COMM_GET_APPCONF_DEFAULT:
        mAppConfDefault.serialize(reply);
        sendReply(reply);
        break;

    case COMM_SET_APPCONF:
    case COMM_SET_APPCONF_NO_STORE:
        if (mAppConf.deSerialize(vb)) {
            sendReply(reply);
        }
        break;

    // Both motors share the state of the plant. The values are per controller
    // and the setup values for the whole vehicle.
    case COMM_GET_VALUES:
    case COMM_GET_VALUES_SELECTIVE: {
        quint32 mask = 0xFFFFFFFF;
        if (id == COMM_GET_VALUES_SELECTIVE) {
            mask = vb.vbPopFrontUint32();
            reply.vbAppendUint32(mask);
        }

        auto has = [mask](int bit) { return mask & (quint32(1) << bit); };
        if (has(0)) { reply.vbAppendDouble16(p->tempFet, 1e1); }
        if (has(1)) { reply.vbAppendDouble16(p->tempMotor, 1e1); }
        if (has(2)) { reply.vbAppendDouble32(p->iq, 1e2); }
        if (has(3)) { reply.vbAppendDouble32(p->iIn, 1e2); }
        if (has(4)) { reply.vbAppendDouble32(0.0, 1e2); } // id
        if (has(5)) { reply.vbAppendDouble32(p->iq, 1e2); }
        if (has(6)) { reply.vbAppendDouble16(p->duty, 1e3); }
        if (has(7)) { reply.vbAppendDouble32(p->erpm, 1e0); }
        if (has(8)) { reply.vbAppendDouble16(p->vBus, 1e1); }
        if (has(9)) { reply.vbAppendDouble32(p->ah, 1e4); }
        if (has(10)) { reply.vbAppendDouble32(p->ahCharged, 1e4); }
        if (has(11)) { reply.vbAppendDouble32(p->wh, 1e4); }
        if (has(12)) { reply.vbAppendDouble32(p->whCharged, 1e4); }
        if (has(13)) { reply.vbAppendInt32(qint32(p->tacho)); }
        if (has(14)) { reply.vbAppendInt32(qint32(p->tachoAbs)); }
        if (has(15)) { reply.vbAppendUint8(p->fault); }
        if (has(16)) { reply.vbAppendDouble32(p->angleE * 180.0 / M_PI, 1e6); }
        if (has(17)) { reply.vbAppendUint8(ownId); }
        if (has(18)) {
            for (int i = 0;i < 3;i++) {
                reply.vbAppendDouble16(p->tempFet - 0.4 * i, 1e1);
            }
        }
        if (has(19)) { reply.vbAppendDouble32(p->vd, 1e3); }
        if (has(20)) { reply.vbAppendDouble32(p->vq, 1e3); }
        if (has(21)) { reply.vbAppendUint8(0); } // No timeout or kill switch
        sendReply(reply);
    } break;

    case COMM_GET_VALUES_SETUP:
    case COMM_GET_VALUES_SETUP_SELECTIVE: {
        quint32 mask = 0xFFFFFFFF;
        if (id == COMM_GET_VALUES_SETUP_SELECTIVE) {
            mask = vb.vbPopFrontUint32();
            reply.vbAppendUint32(mask);
        }

        const int n = VirtualPlant::motorCount;
        double whLeft = p->soc * p->cells() * 3.7 * mMcConf.getParamDouble("si_battery_ah");

        auto has = [mask](int bit) { return mask & (quint32(1) << bit); };
        if (has(0)) { reply.vbAppendDouble16(p->tempFet, 1e1); }
        if (has(1)) { reply.vbAppendDouble16(p->tempMotor, 1e1); }
        if (has(2)) { reply.vbAppendDouble32(p->iq * n, 1e2); }
        if (has(3)) { reply.vbAppendDouble32(p->iIn * n, 1e2); }
        if (has(4)) { reply.vbAppendDouble16(p->duty, 1e3); }
        if (has(5)) { reply.vbAppendDouble32(p->erpm, 1e0); }
        if (has(6)) { reply.vbAppendDouble32(p->speed, 1e3); }
        if (has(7)) { reply.vbAppendDouble16(p->vBus, 1e1); }
        if (has(8)) { reply.vbAppendDouble16(p->soc, 1e3); }
        if (has(9)) { reply.vbAppendDouble32(p->ah * n, 1e4); }
        if (has(10)) { reply.vbAppendDouble32(p->ahCharged * n, 1e4); }
        if (has(11)) { reply.vbAppendDouble32(p->wh * n, 1e4); }
        if (has(12)) { reply.vbAppendDouble32(p->whCharged * n, 1e4); }
        if (has(13)) { reply.vbAppendDouble32(p->dist, 1e3); }
        if (has(14)) { reply.vbAppendDouble32(p->distAbs, 1e3); }
        if (has(15)) { reply.vbAppendDouble32(p->angleE * 180.0 / M_PI, 1e6); }
        if (has(16)) { reply.vbAppendUint8(p->fault); }
        if (has(17)) { reply.vbAppendUint8(ownId); }
        if (has(18)) { reply.vbAppendUint8(n); }
        if (has(19)) { reply.vbAppendDouble32(whLeft, 1e3); }
        if (has(20)) { reply.vbAppendUint32(quint32(p->odometer)); }
        if (has(21)) { reply.vbAppendUint32(quint32(p->timeS * 1000.0)); }
        sendReply(reply);
    } break;

    case COMM_ALIVE:
        p->alive();
        break;

    case COMM_SET_DUTY:
        p->setControl(VirtualPlant::CTRL_DUTY, vb.vbPopFrontInt32() / 1e5);
        break;

    case COMM_SET_CURRENT:
        p->setControl(VirtualPlant::CTRL_CURRENT, vb.vbPopFrontInt32() / 1e3);
        break;

    case COMM_SET_CURRENT_BRAKE:
        p->setControl(VirtualPlant::CTRL_BRAKE, vb.vbPopFrontInt32() / 1e3);
        break;

    case COMM_SET_RPM:
        p->setControl(VirtualPlant::CTRL_RPM, vb.vbPopFrontInt32());
        break;

    case COMM_SET_HANDBRAKE:
        p->setControl(VirtualPlant::CTRL_BRAKE, vb.vbPopFrontDouble32(1e3));
        break;

    case COMM_APP_DISABLE_OUTPUT: {
        vb.vbPopFrontInt8(); // Forward to CAN, they share the plant
        int time = vb.vbPopFrontInt32();
        p->appDisabledS = time == -1 ? 1e9 : time / 1000.0;
    } break;

    case COMM_SET_DETECT:
        mDispPosMode = vb.vbPopFrontInt8();
        break;

    case COMM_GET_DECODED_PPM:
        p->inputPolledS = p->timeS;
        reply.vbAppendDouble32(p->decodedPpm, 1e6);
        reply.vbAppendDouble32(p->ppmPulse, 1e6);
        sendReply(reply);
        break;

    case COMM_GET_DECODED_ADC:
        p->inputPolledS = p->timeS;
        reply.vbAppendDouble32(p->decodedAdc, 1e6);
        reply.vbAppendDouble32(p->adc1, 1e6);
        reply.vbAppendDouble32(p->decodedAdc2, 1e6);
        reply.vbAppendDouble32(p->adc2, 1e6);
        sendReply(reply);
        break;

    case COMM_GET_DECODED_CHUK:
        p->inputPolledS = p->timeS;
        reply.vbAppendDouble32(p->decodedChuk, 1e6);
        sendReply(reply);
        break;

    case COMM_NRF_START_PAIRING: {
        reply.vbAppendInt8(NRF_PAIR_STARTED);
        sendReply(reply);

        // The remote is switched on after a moment
        VByteArray res;
        res.vbAppendUint8(id);
        res.vbAppendInt8(NRF_PAIR_OK);
        sendLater(2000, res);
    } break;

    case COMM_GET_IMU_DATA: {
        // Leaning in the turn of the circle, pitch from slope and acceleration
        double latAcc = p->speed * p->speed / VirtualPlant::gnssRadius;
        double roll = atan(latAcc / 9.81);
        double pitch = p->slope + 0.02 * p->accel;
        double yaw = p->heading;
        auto noise = []() { return (QRandomGenerator::global()->generateDouble() - 0.5) * 0.2; };
        double cr = cos(roll / 2), sr = sin(roll / 2), cp = cos(pitch / 2);
        double sp = sin(pitch / 2), cy = cos(yaw / 2), sy = sin(yaw / 2);

        double values[16] = {
            roll, pitch, yaw,
            p->accel / 9.81, latAcc / 9.81, 1.0,
            noise(), noise(), p->speed / VirtualPlant::gnssRadius * 180.0 / M_PI + noise(),
            cos(yaw), -sin(yaw), 0.4,
            cr * cp * cy + sr * sp * sy, sr * cp * cy - cr * sp * sy,
            cr * sp * cy + sr * cp * sy, cr * cp * sy - sr * sp * cy
        };

        quint16 mask = vb.vbPopFrontUint16();
        reply.vbAppendUint16(mask);
        for (int i = 0;i < 16;i++) {
            if (mask & (1 << i)) {
                reply.vbAppendDouble32Auto(values[i]);
            }
        }
        reply.vbAppendUint8(ownId);
        sendReply(reply);
    } break;

    case COMM_GET_IMU_CALIBRATION: {
        double yaw = vb.vbPopFrontDouble32(1e3);
        double cal[9] = {1.2, -0.8, yaw, 0.01, -0.02, 0.0, 0.31, -0.24, 0.12};
        for (double c: cal) {
            reply.vbAppendDouble32(c, 1e6);
        }
        sendLater(1000, reply);
    } break;

    case COMM_GET_STATS: {
        quint16 mask = vb.vbPopFrontUint16();
        double t = qMax(p->statTime, 1e-3);
        double values[11] = {
            p->statSpeedSum / t, p->statSpeedMax, p->statPowerSum / t, p->statPowerMax,
            p->statCurrSum / t, p->statCurrMax, p->statFetSum / t, p->statFetMax,
            p->statMotorSum / t, p->statMotorMax, p->statTime
        };

        reply.vbAppendUint32(mask);
        for (int i = 0;i < 11;i++) {
            if (mask & (1 << i)) {
                reply.vbAppendDouble32Auto(values[i]);
            }
        }
        sendReply(reply);
    } break;

    case COMM_RESET_STATS:
        p->resetStats();
        if (!vb.isEmpty() && vb.vbPopFrontUint8()) {
            sendReply(reply);
        }
        break;

    case COMM_GET_GNSS: {
        QDateTime now = QDateTime::currentDateTime();
        quint16 mask = vb.vbPopFrontUint16();
        reply.vbAppendUint32(mask);
        if (mask & (1 << 0)) { reply.vbAppendDouble64(p->lat, 1e16); }
        if (mask & (1 << 1)) { reply.vbAppendDouble64(p->lon, 1e16); }
        if (mask & (1 << 2)) { reply.vbAppendDouble32Auto(12.0); } // Height
        if (mask & (1 << 3)) { reply.vbAppendDouble32Auto(fabs(p->speed)); }
        if (mask & (1 << 4)) { reply.vbAppendDouble32Auto(0.9); } // HDOP
        if (mask & (1 << 5)) { reply.vbAppendInt32(now.time().msecsSinceStartOfDay()); }
        if (mask & (1 << 6)) { reply.vbAppendInt16(now.date().year() - 2000); }
        if (mask & (1 << 7)) { reply.vbAppendUint8(now.date().month()); }
        if (mask & (1 << 8)) { reply.vbAppendUint8(now.date().day()); }
        if (mask & (1 << 9)) { reply.vbAppendDouble32Auto(0.1); } // Age
        sendReply(reply);
    } break;

    case COMM_BMS_GET_VALUES: {
        const int n = VirtualPlant::motorCount;
        int cells = p->cells();
        double vMin = 10.0;
        for (int i = 0;i < cells;i++) {
            vMin = qMin(vMin, p->cellVoltage(i));
        }

        reply.vbAppendDouble32(p->vBus, 1e6);
        reply.vbAppendDouble32(0.0, 1e6); // Charge input
        reply.vbAppendDouble32(p->iIn * n, 1e6);
        reply.vbAppendDouble32(p->iIn * n, 1e6);
        reply.vbAppendDouble32(p->ah * n, 1e3);
        reply.vbAppendDouble32(p->wh * n, 1e3);

        reply.vbAppendUint8(cells);
        for (int i = 0;i < cells;i++) {
            reply.vbAppendDouble16(p->cellVoltage(i), 1e3);
        }
        for (int i = 0;i < cells;i++) {
            reply.vbAppendUint8(p->cellVoltage(i) > (vMin + 0.005));
        }

        // The pack warms up a bit with the electronics
        double tempPack = 24.0 + (p->tempFet - 22.0) * 0.3;
        reply.vbAppendUint8(4);
        for (int i = 0;i < 4;i++) {
            reply.vbAppendDouble16(tempPack + 0.4 * i, 1e2);
        }

        reply.vbAppendDouble16(tempPack + 4.0, 1e2); // IC
        reply.vbAppendDouble16(24.0, 1e2); // Humidity sensor temperature
        reply.vbAppendDouble16(41.0, 1e2); // Humidity
        reply.vbAppendDouble16(tempPack + 1.2, 1e2); // Hottest cell
        reply.vbAppendDouble16(p->soc, 1e3);
        reply.vbAppendDouble16(0.97, 1e3); // SoH
        reply.vbAppendUint8(BMS_CAN_ID);
        reply.vbAppendDouble32Auto(52.3); // Totals
        reply.vbAppendDouble32Auto(2010.0);
        reply.vbAppendDouble32Auto(61.7 + p->ah * n);
        reply.vbAppendDouble32Auto(2370.0 + p->wh * n);
        sendReply(reply);
    } break;

    case COMM_PING_CAN:
        if ((mMain ? mMain : this)->mCanOn) {
            for (auto node: (mMain ? mMain : this)->mCanNodes) {
                reply.vbAppendUint8(node->mCanId);
            }
        }
        sendReply(reply);
        break;

    case COMM_FORWARD_CAN: {
        int target = vb.vbPopFrontUint8();
        for (auto node: mCanNodes) {
            if (mCanOn && node->mCanId == target) {
                node->processPacket(vb);
            }
        }
    } break;

    // Detection measures the true motor of the plant. It takes a few seconds,
    // like on real hardware.
    case COMM_DETECT_MOTOR_R_L:
        reply.vbAppendDouble32(measured(VirtualPlant::motorR), 1e6);
        reply.vbAppendDouble32(measured(VirtualPlant::motorL * 1e6), 1e3);
        reply.vbAppendDouble32(measured(VirtualPlant::motorLdLqDiff * 1e6), 1e3);
        sendLater(2000, reply);
        break;

    case COMM_DETECT_MOTOR_FLUX_LINKAGE:
        reply.vbAppendDouble32(measured(VirtualPlant::motorLambda), 1e7);
        sendLater(4000, reply);
        break;

    case COMM_DETECT_MOTOR_FLUX_LINKAGE_OPENLOOP:
    case COMM_DETECT_ENCODER: {
        // The encoder is found when the sensor port is set to one. Without
        // encoder the firmware sends -1 after open loop and 1001 otherwise.
        bool encoder = mMcConf.getParamEnum("m_sensor_port_mode") != 0;
        bool openLoop = id == COMM_DETECT_MOTOR_FLUX_LINKAGE_OPENLOOP;
        double ratio = mMcConf.getParamInt("si_motor_poles") / 2.0;

        if (openLoop) {
            reply.vbAppendDouble32(measured(VirtualPlant::motorLambda), 1e7);
        }
        reply.vbAppendDouble32(encoder ? VirtualPlant::encOffset : (openLoop ? -1.0 : 1001.0), 1e6);
        reply.vbAppendDouble32(encoder ? ratio : (openLoop ? -1.0 : 0.0), 1e6);
        reply.vbAppendInt8(0);
        sendLater(4000, reply);
    } break;

    case COMM_DETECT_HALL_FOC: {
        bool hall = mMcConf.getParamEnum("m_sensor_port_mode") == 0;
        for (int i = 0;i < 8;i++) {
            reply.vbAppendUint8(hall ? VirtualPlant::hallTable[i] : 255);
        }
        reply.vbAppendUint8(hall ? 0 : 1);
        sendLater(3000, reply);
    } break;

    case COMM_DETECT_MOTOR_PARAM: {
        // BLDC: commutation step for each hall state
        const int bldcTable[8] = {-1, 1, 3, 2, 5, 6, 4, -1};
        reply.vbAppendDouble32(measured(62.4), 1e3); // Cycle integrator limit
        reply.vbAppendDouble32(measured(615.0), 1e3); // BEMF coupling
        for (int i = 0;i < 8;i++) {
            reply.vbAppendInt8(bldcTable[i]);
        }
        reply.vbAppendUint8(0);
        sendLater(4000, reply);
    } break;

    case COMM_DETECT_APPLY_ALL_FOC: {
        bool detectCan = vb.vbPopFrontInt8();
        double maxLoss = vb.vbPopFrontDouble32(1e3);
        double minCurrIn = vb.vbPopFrontDouble32(1e3);
        double maxCurrIn = vb.vbPopFrontDouble32(1e3);
        double iMax = qMin(sqrt(maxLoss / VirtualPlant::motorR), 150.0);

        // Applied like the firmware, the plant has hall sensors
        auto apply = [&](ConfigParams &mc) {
            mc.updateParamEnum("motor_type", 2);
            mc.updateParamDouble("foc_motor_r", measured(VirtualPlant::motorR));
            mc.updateParamDouble("foc_motor_l", measured(VirtualPlant::motorL));
            mc.updateParamDouble("foc_motor_ld_lq_diff", measured(VirtualPlant::motorLdLqDiff));
            mc.updateParamDouble("foc_motor_flux_linkage", measured(VirtualPlant::motorLambda));
            mc.updateParamDouble("l_current_max", iMax);
            mc.updateParamDouble("l_current_min", -iMax);
            mc.updateParamDouble("l_abs_current_max", iMax * 1.5);
            mc.updateParamEnum("m_sensor_port_mode", 0);
            mc.updateParamEnum("foc_sensor_mode", 2);
            for (int i = 0;i < 8;i++) {
                mc.updateParamInt(QString("foc_hall_table__%1").arg(i), VirtualPlant::hallTable[i]);
            }
            if (fabs(minCurrIn) > 0.001) {
                mc.updateParamDouble("l_in_current_min", minCurrIn);
            }
            if (fabs(maxCurrIn) > 0.001) {
                mc.updateParamDouble("l_in_current_max", maxCurrIn);
            }
        };

        int time = 6000;
        apply(mMcConf);
        if (detectCan && mCanOn) {
            for (auto node: mCanNodes) {
                if (node->mHwType == HW_TYPE_VESC) {
                    apply(node->mMcConf);
                    time += 4000;
                }
            }
        }

        // The firmware sends the new configuration before the result
        reply.vbAppendInt16(1); // Hall sensors found
        QTimer::singleShot(time, this, [this, reply]() {
            VByteArray conf;
            conf.vbAppendUint8(COMM_GET_MCCONF);
            mMcConf.serialize(conf);
            sendReply(conf);
            sendReply(reply);
        });
    } break;

    case COMM_SAMPLE_PRINT: {
        int mode = vb.vbPopFrontInt8();
        int len = vb.vbPopFrontUint16();
        int dec = qMax(1, int(vb.vbPopFrontUint8()));

        if (mode == DEBUG_SAMPLING_SEND_SINGLE_SAMPLE) {
            if (len < mSamples.size()) {
                sendReply(mSamples.at(len));
            }
            break;
        }

        if (mode == DEBUG_SAMPLING_OFF) {
            break;
        }

        // Phase currents and voltages from the current state of the motor
        if (mode != DEBUG_SAMPLING_SEND_LAST_SAMPLES) {
            double fSw = mMcConf.getParamDouble("foc_f_zv");
            double dt = dec / fSw;
            double omega = p->erpm / 60.0 * 2.0 * M_PI;
            double vAmp = hypot(p->vd, p->vq);
            double vAngle = atan2(p->vd, p->vq);

            mSamples.clear();
            for (int i = 0;i < SAMPLE_MAX_LEN;i++) {
                double th = p->angleE + omega * dt * i;
                VByteArray s;
                s.vbAppendUint8(COMM_SAMPLE_PRINT);
                s.vbAppendInt16(i);
                for (int ph = 0;ph < 3;ph++) {
                    s.vbAppendDouble32Auto(p->iq * cos(th - ph * 2.0 * M_PI / 3.0));
                }
                for (int ph = 0;ph < 3;ph++) {
                    s.vbAppendDouble32Auto(p->vBus / 2.0 + vAmp * cos(th + vAngle - ph * 2.0 * M_PI / 3.0));
                }
                s.vbAppendDouble32Auto(p->vBus);
                s.vbAppendDouble32Auto(p->iq);
                s.vbAppendDouble32Auto(fSw);
                s.vbAppendInt8(0); // Status
                s.vbAppendInt8(0); // Phase
                s.vbAppendInt32(i);
                mSamples.append(s);
            }
        }

        // Like the firmware, the trigger modes send the whole buffer
        if (mode != DEBUG_SAMPLING_TRIGGER_START_NOSEND && mode != DEBUG_SAMPLING_TRIGGER_FAULT_NOSEND) {
            bool now = mode == DEBUG_SAMPLING_NOW || mode == DEBUG_SAMPLING_START;
            int count = now ? qMin(len, SAMPLE_MAX_LEN) : mSamples.size();
            for (int i = 0;i < count;i++) {
                sendReply(mSamples.at(i));
            }
        }
    } break;

    case COMM_TERMINAL_CMD:
    case COMM_TERMINAL_CMD_SYNC:
        terminalCmd(QString::fromLatin1(vb));
        break;

    case COMM_ERASE_NEW_APP:
    case COMM_WRITE_NEW_APP_DATA:
    case COMM_WRITE_NEW_APP_DATA_LZO:
        // Firmware updates are not supported
        reply.vbAppendUint8(0);
        sendReply(reply);
        break;

    case COMM_QMLUI_ERASE:
    case COMM_LISP_ERASE_CODE:
        if (id == COMM_QMLUI_ERASE) {
            mQmlSlot.clear();
        } else {
            mLispSlot.clear();
            mLispRunning = false;
            updateCustomConfig();
        }
        mSaveTimer->start();
        reply.vbAppendUint8(1);
        sendReply(reply);
        break;

    case COMM_QMLUI_WRITE:
    case COMM_LISP_WRITE_CODE: {
        quint32 offset = vb.vbPopFrontUint32();
        bool ok = writeCodeSlot(id == COMM_QMLUI_WRITE ? mQmlSlot : mLispSlot, offset, vb);
        mSaveTimer->start();
        reply.vbAppendUint8(ok ? 1 : 0);
        reply.vbAppendUint32(offset);
        sendReply(reply);
    } break;

    case COMM_GET_QML_UI_APP:
    case COMM_LISP_READ_CODE: {
        qint32 len = vb.vbPopFrontInt32();
        qint32 offset = vb.vbPopFrontInt32();
        QByteArray code = codeSlotPayload(id == COMM_GET_QML_UI_APP ? mQmlSlot : mLispSlot);

        if (code.isEmpty()) {
            reply.vbAppendInt32(0);
            reply.vbAppendInt32(0);
            sendReply(reply);
            break;
        }

        if (len < 0 || offset < 0 || (len + offset) > code.size() ||
                len > (PACKET_MAX_PL_LEN - 10)) {
            break;
        }

        reply.vbAppendInt32(code.size());
        reply.vbAppendInt32(offset);
        reply.append(code.mid(offset, len));
        sendReply(reply);
    } break;

    case COMM_LISP_SET_RUNNING:
        mLispRunning = vb.vbPopFrontUint8();
        updateCustomConfig();
        reply.vbAppendUint8(1);
        sendReply(reply);
        break;

    case COMM_LISP_GET_STATS:
        reply.vbAppendDouble16(0.0, 1e2); // CPU
        reply.vbAppendDouble16(0.0, 1e2); // Heap
        reply.vbAppendDouble16(0.0, 1e2); // Memory
        reply.vbAppendDouble16(0.0, 1e2); // Stack
        reply.vbAppendString(""); // Result
        sendReply(reply);
        break;

    case COMM_GET_CUSTOM_CONFIG:
    case COMM_GET_CUSTOM_CONFIG_DEFAULT: {
        int confInd = vb.vbPopFrontUint8();
        if (confInd == 0 && !mCustomConfXml.isEmpty()) {
            reply.vbAppendUint8(confInd);
            if (id == COMM_GET_CUSTOM_CONFIG_DEFAULT) {
                mCustomConfDefault.serialize(reply);
            } else {
                mCustomConf.serialize(reply);
            }
            sendReply(reply);
        }
    } break;

    case COMM_SET_CUSTOM_CONFIG: {
        int confInd = vb.vbPopFrontUint8();
        if (confInd == 0 && !mCustomConfXml.isEmpty() && mCustomConf.deSerialize(vb)) {
            sendReply(reply);
        }
    } break;

    case COMM_GET_CUSTOM_CONFIG_XML: {
        int confInd = vb.vbPopFrontUint8();
        qint32 len = vb.vbPopFrontInt32();
        qint32 offset = vb.vbPopFrontInt32();

        if (confInd != 0 || mCustomConfXml.isEmpty() || len < 0 || offset < 0 ||
                (len + offset) > mCustomConfXml.size() || len > (PACKET_MAX_PL_LEN - 10)) {
            break;
        }

        reply.vbAppendUint8(confInd);
        reply.vbAppendInt32(mCustomConfXml.size());
        reply.vbAppendInt32(offset);
        reply.append(mCustomConfXml.mid(offset, len));
        sendReply(reply);
    } break;

    default:
        // Everything else needs hardware, ignore it
        break;
    }
}

void VirtualVesc::sendReply(const VByteArray &vb)
{
    // Delayed replies are dropped after disconnecting
    if (!(mMain ? mMain : this)->mSimTimer->isActive()) {
        return;
    }

    mPacket->sendPacket(vb);
}

void VirtualVesc::sendLater(int ms, const VByteArray &vb)
{
    QTimer::singleShot(ms, this, [this, vb]() {
        sendReply(vb);
    });
}

void VirtualVesc::print(const QString &str)
{
    VByteArray vb;
    vb.vbAppendUint8(COMM_PRINT);
    vb.append(str.toLatin1());
    sendReply(vb);
}

void VirtualVesc::simStep()
{
    int faultLast = mPlant->fault;
    mPlant->step(mSimTimer->interval() / 1000.0);

    if (mPlant->fault != faultLast && mPlant->fault != FAULT_CODE_NONE) {
        mFaultLog.append(QString("Fault            : %1\n"
                                 "Current          : %2\n"
                                 "Voltage          : %3\n"
                                 "Duty             : %4\n"
                                 "RPM              : %5").
                         arg(Commands::faultToStr(mc_fault_code(mPlant->fault))).
                         arg(mPlant->iq, 0, 'f', 1).arg(mPlant->vBus, 0, 'f', 2).
                         arg(mPlant->duty, 0, 'f', 3).arg(mPlant->erpm, 0, 'f', 1));
        if (mFaultLog.size() > FAULT_LOG_LEN) {
            mFaultLog.removeFirst();
        }
    }

    // The firmware streams the rotor position while a display mode is set
    if (mDispPosMode != DISP_POS_MODE_NONE && (++mSimSteps % 5) == 0) {
        VByteArray vb;
        vb.vbAppendUint8(COMM_ROTOR_POSITION);
        vb.vbAppendDouble32(mPlant->angleE * 180.0 / M_PI, 1e5);
        sendReply(vb);
    }
}

void VirtualVesc::terminalCmd(const QString &cmd)
{
    VirtualVesc *root = mMain ? mMain : this;
    VirtualPlant *p = mPlant;
    QStringList args = cmd.split(" ", Qt::SkipEmptyParts);
    QString sub = args.value(1);
    QString val = args.value(2);
    const QStringList scenarios = {"cruise", "hill", "stopgo", "idle"};

    print(QString("-> %1 \n").arg(cmd));

    if (args.isEmpty()) {
        return;
    }

    if (args.at(0) == "help") {
        print("Valid commands are:\n"
              "help\n  Show this help\n"
              "fault\n  Prints the current fault code\n"
              "faults\n  Prints all stored fault codes and conditions when they arrived\n"
              "sim\n  Show the state of the simulation\n"
              "sim throttle [-1.0 - 1.0 | auto]\n  Hold the throttle of the remote, or let the rider ride\n"
              "sim scenario [cruise | hill | stopgo | idle]\n  What the rider does\n"
              "sim fault [code]\n  Set a fault code, 0 clears it\n"
              "sim cell [cell] [voltage | off]\n  Override the voltage of a cell, starting at 1\n"
              "sim soc [percent]\n  Set the state of charge of the battery\n"
              "sim can [on | off]\n  Connect or disconnect the CAN-bus");
    } else if (args.at(0) == "fault") {
        print(Commands::faultToStr(mc_fault_code(p->fault)));
    } else if (args.at(0) == "faults") {
        if (root->mFaultLog.isEmpty()) {
            print("No faults registered since startup");
        } else {
            print("Active fault: " + Commands::faultToStr(mc_fault_code(p->fault)));
            print("The following faults were registered since start:");
            for (const auto &f: root->mFaultLog) {
                print(f + "\n");
            }
        }
    } else if (args.at(0) == "sim") {
        if (sub == "throttle" && !val.isEmpty()) {
            p->throttleOverride = val == "auto" ? NAN : qBound(-1.0, val.toDouble(), 1.0);
        } else if (sub == "scenario" && scenarios.contains(val)) {
            p->scenario = VirtualPlant::Scenario(scenarios.indexOf(val));
        } else if (sub == "fault" && !val.isEmpty()) {
            p->faultForced = val.toInt();
        } else if (sub == "cell" && args.size() == 4 && val.toInt() >= 1 &&
                   val.toInt() <= p->cellOverride.size()) {
            p->cellOverride[val.toInt() - 1] = args.at(3) == "off" ? NAN : args.at(3).toDouble();
        } else if (sub == "soc" && !val.isEmpty()) {
            p->soc = qBound(0.0, val.toDouble() / 100.0, 1.0);
        } else if (sub == "can" && (val == "on" || val == "off")) {
            root->mCanOn = val == "on";
        } else if (!sub.isEmpty()) {
            print("Invalid argument, type help to list all available commands");
            return;
        }

        print(QString("Scenario: %1, throttle: %2").arg(scenarios.at(p->scenario)).
              arg(std::isfinite(p->throttleOverride) ? QString::number(p->throttleOverride) : "auto"));
        print(QString("Speed: %1 km/h, motor current: %2 A, battery: %3 V, %4 %").
              arg(p->speed * 3.6, 0, 'f', 1).arg(p->iq, 0, 'f', 1).
              arg(p->vBus, 0, 'f', 1).arg(p->soc * 100.0, 0, 'f', 0));
        print(QString("Fault: %1, CAN-bus: %2").
              arg(Commands::faultToStr(mc_fault_code(p->fault))).arg(root->mCanOn ? "on" : "off"));
    } else {
        print("Invalid command: " + args.at(0) + "\n"
              "type help to list all available commands");
    }
}

void VirtualVesc::processFileCmd(COMM_PACKET_ID id, VByteArray &vb, VByteArray &reply)
{
    QString path = sdPath(vb.vbPopFrontString());

    switch (id) {
    case COMM_FILE_LIST: {
        QString from = vb.vbPopFrontString();
        generateLog();

        reply.vbAppendUint8(0); // Has more
        if (path.isEmpty()) {
            break;
        }

        bool fromFound = from.isEmpty();
        auto entries = QDir(path).entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::Name);
        for (const auto &fi: entries) {
            if (!fromFound) {
                fromFound = fi.fileName() == from;
                continue;
            }

            // Same packet size as the Express
            if ((reply.size() + fi.fileName().toLocal8Bit().size()) >= 400) {
                reply[1] = 1;
                break;
            }

            reply.vbAppendInt8(fi.isDir());
            reply.vbAppendInt32(fi.isDir() ? QDir(fi.filePath()).count() : fi.size());
            reply.vbAppendString(fi.fileName());
        }
    } break;

    case COMM_FILE_READ: {
        qint32 offset = vb.vbPopFrontInt32();
        QFile file(path);
        reply.vbAppendInt32(offset);
        if (!path.isEmpty() && file.open(QIODevice::ReadOnly) && file.seek(offset)) {
            reply.vbAppendInt32(file.size());
            reply.append(file.read(400));
        } else {
            reply.vbAppendInt32(0);
        }
    } break;

    case COMM_FILE_WRITE: {
        qint32 offset = vb.vbPopFrontInt32();
        vb.vbPopFrontInt32(); // Total size
        QFile file(path);
        bool ok = !path.isEmpty() &&
                file.open(offset == 0 ? QIODevice::WriteOnly : QIODevice::ReadWrite) &&
                file.seek(offset) && file.write(vb) == vb.size();
        reply.vbAppendInt32(offset);
        reply.vbAppendInt8(ok);
    } break;

    case COMM_FILE_MKDIR:
        reply.vbAppendInt8(!path.isEmpty() && QDir().mkpath(path));
        break;

    case COMM_FILE_REMOVE:
        reply.vbAppendInt8(!path.isEmpty() &&
                           (QFileInfo(path).isDir() ? QDir(path).removeRecursively() : QFile::remove(path)));
        break;

    default:
        break;
    }
}

QString VirtualVesc::sdPath(const QString &path) const
{
    // Only inside the SD card folder
    QString root = (mMain ? mMain : this)->mCodeDir + "/sd";
    QString res = QDir::cleanPath(root + "/" + path);
    return (res == root || res.startsWith(root + "/")) ? res : QString();
}

void VirtualVesc::generateLog()
{
    // A ride for the log browser, like an Express that logs to SD
    QString dir = sdPath("log_can");
    if (dir.isEmpty() || QDir(dir).exists() || !QDir().mkpath(dir)) {
        return;
    }

    QDateTime start = QDateTime::currentDateTime().addSecs(-3600);
    QFile file(dir + "/" + start.toString("yyyy-MM-dd_HH-mm-ss") + ".csv");
    if (!file.open(QIODevice::WriteOnly)) {
        return;
    }

    QTextStream out(&file);
    out << "t_day:Time:s:3:0:1;kmh_vesc:Speed VESC:km/h:2:0:0;v_in:Input Voltage:V:2:0:0;"
           "setup_curr_motor:Current Motors:A:2:0:0;setup_curr_battery:Current Battery:A:2:0:0;"
           "setup_power:Power:W:0:0:0;erpm:ERPM::0:0:0;duty:Duty:%:1:0:0;"
           "soc:Battery Level:%:1:0:0;t_mosfet:Temp MOSFET:°C:1:0:0;t_motor:Temp Motor:°C:1:0:0;"
           "cnt_ah:Ah Used:Ah:3:0:0;cnt_wh:Wh Used:Wh:3:0:0;trip_vesc_abs:Trip VESC ABS:m:3:1:0;"
           "t_day_pos:Time GNSS:s:3:0:1;gnss_lat:Latitude:deg:7:0:0;gnss_lon:Longitude:deg:7:0:0\n";

    // Five minutes at 10 Hz, from the same model with the current configuration
    VirtualVesc *root = mMain ? mMain : this;
    VirtualPlant p(&root->mMcConf, &root->mAppConf);
    const int n = VirtualPlant::motorCount;
    double t0 = start.time().msecsSinceStartOfDay() / 1000.0;

    for (int i = 0;i < 3000;i++) {
        for (int j = 0;j < 5;j++) {
            p.step(0.02);
        }

        double t = fmod(t0 + p.timeS, 86400.0);
        out << QString::asprintf("%.3f;%.2f;%.2f;%.2f;%.2f;%.0f;%.0f;%.1f;%.1f;%.1f;%.1f;"
                                 "%.3f;%.3f;%.3f;%.3f;%.7f;%.7f\n",
                                 t, p.speed * 3.6, p.vBus, p.iq * n, p.iIn * n,
                                 p.iIn * n * p.vBus, p.erpm, p.duty * 100.0, p.soc * 100.0,
                                 p.tempFet, p.tempMotor, p.ah * n, p.wh * n, p.distAbs,
                                 t, p.lat, p.lon);
    }
}

void VirtualVesc::loadMainConfigs()
{
    // Same lookup as Utility::configLoad, but into our own config instances
    QDirIterator it(Utility::configPath(""));

    while (it.hasNext()) {
        QFileInfo fi(it.next());

        if (!fi.isDir()) {
            continue;
        }

        foreach (auto name, fi.fileName().split("_o_")) {
            auto parts = name.split(".");
            if (parts.size() == 2 && parts.at(0).toInt() == mFwMajor &&
                    parts.at(1).toInt() == mFwMinor) {
                QString mcPath = fi.absoluteFilePath() + "/parameters_mcconf.xml";
                QString appPath = fi.absoluteFilePath() + "/parameters_appconf.xml";

                mMcConf.loadParamsXml(mcPath);
                mMcConfDefault.loadParamsXml(mcPath);
                mAppConf.loadParamsXml(appPath);
                mAppConfDefault.loadParamsXml(appPath);

                // A set up 12S skateboard, the defaults stay as in the firmware
                mMcConf.updateParamInt("si_battery_cells", 12);
                mMcConf.updateParamDouble("si_battery_ah", 10.0);
                mMcConf.updateParamDouble("l_battery_cut_start", 37.2);
                mMcConf.updateParamDouble("l_battery_cut_end", 36.0);
                mMcConf.updateParamDouble("foc_motor_r", VirtualPlant::motorR);
                mMcConf.updateParamDouble("foc_motor_l", VirtualPlant::motorL);
                mMcConf.updateParamDouble("foc_motor_ld_lq_diff", VirtualPlant::motorLdLqDiff);
                mMcConf.updateParamDouble("foc_motor_flux_linkage", VirtualPlant::motorLambda);
                return;
            }
        }
    }

    qWarning() << "VirtualVesc: no configurations found for firmware" << mFwMajor << mFwMinor;
}

void VirtualVesc::updateCustomConfig()
{
    QByteArray code = codeSlotPayload(mLispSlot);

    if (code.isEmpty()) {
        mCustomConfXml.clear();
        mCustomConfSource.clear();
        return;
    }

    // Keep the current values if the code did not change
    if (code == mCustomConfSource) {
        return;
    }

    mCustomConfSource = code;
    mCustomConfXml = findConfigXml(code);

    if (!mCustomConfXml.isEmpty()) {
        mCustomConf.loadCompressedParamsXml(mCustomConfXml);
        mCustomConfDefault.loadCompressedParamsXml(mCustomConfXml);
    }
}

void VirtualVesc::loadCode()
{
    QFile lispFile(mCodeDir + "/lisp.bin");
    if (lispFile.open(QIODevice::ReadOnly)) {
        mLispSlot = lispFile.readAll();
    }

    QFile qmlFile(mCodeDir + "/qml.bin");
    if (qmlFile.open(QIODevice::ReadOnly)) {
        mQmlSlot = qmlFile.readAll();
    }

    // Like the firmware, start valid Lisp code on boot
    mLispRunning = !codeSlotPayload(mLispSlot).isEmpty();
    updateCustomConfig();
}

void VirtualVesc::saveCode()
{
    // The code of the CAN nodes is not stored
    if (mCodeDir.isEmpty()) {
        return;
    }

    QDir().mkpath(mCodeDir);

    auto save = [this](const QString &name, const QByteArray &slot) {
        QString path = mCodeDir + "/" + name;
        if (codeSlotPayload(slot).isEmpty()) {
            QFile::remove(path);
            return;
        }

        QFile file(path);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(slot);
        }
    };

    save("lisp.bin", mLispSlot);
    save("qml.bin", mQmlSlot);
}

QByteArray VirtualVesc::codeSlotPayload(const QByteArray &slot) const
{
    if (slot.size() < 8) {
        return QByteArray();
    }

    VByteArray header(slot.left(6));
    quint32 len = header.vbPopFrontUint32();
    quint16 crc = header.vbPopFrontUint16();

    if (len > quint32(CODE_SLOT_MAX_SIZE) || int(len) + 8 > slot.size()) {
        return QByteArray();
    }

    // The CRC includes the 2 byte flags
    if (Packet::crc16(reinterpret_cast<const unsigned char*>(slot.constData()) + 6, len + 2) != crc) {
        return QByteArray();
    }

    return slot.mid(8, int(len));
}

bool VirtualVesc::writeCodeSlot(QByteArray &slot, quint32 offset, const QByteArray &data)
{
    if (offset + quint32(data.size()) > quint32(CODE_SLOT_MAX_SIZE)) {
        return false;
    }

    if (slot.size() < int(offset) + data.size()) {
        slot.append(QByteArray(int(offset) + data.size() - slot.size(), char(0xFF)));
    }

    slot.replace(int(offset), data.size(), data);
    return true;
}
