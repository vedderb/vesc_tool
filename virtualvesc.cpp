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
#include "datatypes.h"
#include "utility.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDebug>

namespace {
const char *VIRTUAL_HW_NAME = "60";
const char *VIRTUAL_FW_NAME = "VIRTUAL";
const char VIRTUAL_UUID[12] = {'V', 'I', 'R', 'T', 'U', 'A', 'L', 'V', 'E', 'S', 'C', '1'};

// Same limits as the firmware
const int CODE_SLOT_MAX_SIZE = 1024 * 128;
const int PACKET_MAX_PL_LEN = 512;

// Field sizes in bytes for COMM_GET_VALUES and COMM_GET_VALUES_SETUP, indexed
// by mask bit. See commands_process_packet in the firmware.
const int VALUES_FIELD_SIZES[] = {
    2, 2, 4, 4, 4, 4, 2, 4, 2, 4, 4, 4, 4, 4, 4, 1, 4, 1, 6, 4, 4, 1
};
const int VALUES_SETUP_FIELD_SIZES[] = {
    2, 2, 4, 4, 2, 4, 4, 2, 2, 4, 4, 4, 4, 4, 4, 4, 1, 1, 1, 4, 4, 4
};
}

VirtualVesc::VirtualVesc(QObject *parent) : QObject(parent)
{
    mPacket = new Packet(this);
    mLispRunning = false;

    auto fw = Utility::configLatestSupported();
    mFwMajor = fw.first;
    mFwMinor = fw.second;

    connect(mPacket, &Packet::packetReceived, this, &VirtualVesc::processPacket);
    connect(mPacket, &Packet::dataToSend, this, [this](QByteArray &data) {
        emit dataToSend(data);
    });

    // Code is written in many small chunks, so save shortly after the last change
    mCodeDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/virtual_vesc";
    mSaveTimer = new QTimer(this);
    mSaveTimer->setSingleShot(true);
    mSaveTimer->setInterval(1000);
    connect(mSaveTimer, &QTimer::timeout, this, &VirtualVesc::saveCode);

    loadMainConfigs();
    loadCode();
}

VirtualVesc::~VirtualVesc()
{
    if (mSaveTimer->isActive()) {
        saveCode();
    }
}

void VirtualVesc::processData(const QByteArray &data)
{
    mPacket->processData(data);
}

void VirtualVesc::resetState()
{
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

    switch (id) {
    case COMM_FW_VERSION: {
        quint16 qmlFlags = 0;
        if (!codeSlotPayload(mQmlSlot).isEmpty()) {
            VByteArray flags(mQmlSlot.mid(6, 2));
            qmlFlags = flags.vbPopFrontUint16();
        }

        reply.vbAppendInt8(mFwMajor);
        reply.vbAppendInt8(mFwMinor);
        reply.vbAppendString(VIRTUAL_HW_NAME);
        reply.append(VIRTUAL_UUID, 12);
        reply.vbAppendUint8(0); // Pairing done
        reply.vbAppendUint8(0); // Test version
        reply.vbAppendUint8(HW_TYPE_VESC);
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

    case COMM_GET_VALUES:
    case COMM_GET_VALUES_SELECTIVE:
    case COMM_GET_VALUES_SETUP:
    case COMM_GET_VALUES_SETUP_SELECTIVE: {
        bool setup = id == COMM_GET_VALUES_SETUP || id == COMM_GET_VALUES_SETUP_SELECTIVE;
        const int *sizes = setup ? VALUES_SETUP_FIELD_SIZES : VALUES_FIELD_SIZES;

        quint32 mask = 0xFFFFFFFF;
        if (id == COMM_GET_VALUES_SELECTIVE || id == COMM_GET_VALUES_SETUP_SELECTIVE) {
            mask = vb.vbPopFrontUint32();
            reply.vbAppendUint32(mask);
        }

        // All values are 0, except for the controller ID
        for (int i = 0;i < 22;i++) {
            if (!(mask & (quint32(1) << i))) {
                continue;
            }

            if (i == 17) {
                reply.vbAppendUint8(mAppConf.getParamInt("controller_id"));
            } else {
                reply.append(QByteArray(sizes[i], '\0'));
            }
        }

        sendReply(reply);
    } break;

    case COMM_PING_CAN:
        // No other devices on the CAN bus
        sendReply(reply);
        break;

    case COMM_TERMINAL_CMD:
    case COMM_TERMINAL_CMD_SYNC: {
        VByteArray print;
        print.vbAppendUint8(COMM_PRINT);
        print.append("Virtual VESC: the terminal is not available");
        sendReply(print);
    } break;

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
    mPacket->sendPacket(vb);
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
