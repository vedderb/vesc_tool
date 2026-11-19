/*
    Copyright 2026 Jeffrey M. Friesen

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

// LispBM on the devices of the Virtual ESC. The extensions in lispBM/vl_node.c
// call the host functions here from the evaluator thread, so they lock the
// plant and the configurations. The Qt thread does not hold the lock while
// it waits for an evaluator, see processPacket.

#include "virtualvesc.h"
#include "commands.h"
#include <QElapsedTimer>
#include <QPainter>
#include <cmath>

extern "C" {
uint64_t vl_time_us(void);

uint32_t lbm_timestamp(void)
{
    return uint32_t(vl_time_us());
}

void vl_sleep_us(uint32_t us)
{
    QThread::usleep(us);
}
}

extern "C" uint64_t vl_time_us(void)
{
    static QElapsedTimer timer = [] { QElapsedTimer t; t.start(); return t; }();
    return uint64_t(timer.nsecsElapsed() / 1000);
}

namespace {
#ifdef HAS_LISPBM
// One per device, in the order of devices()
const vl_api_t *const lbmApis[] = {&vl0_api, &vl1_api, &vl2_api, &vl3_api, &vl4_api};
#endif

QMutex displayLock;
QHash<int, QImage> displays; // By CAN-ID
int displayFrame = 0;
}

QList<VirtualVesc*> VirtualVesc::devices()
{
    QList<VirtualVesc*> res = root()->mCanNodes;
    res.prepend(root());
    return res;
}

int VirtualVesc::canId()
{
    return mMain ? mCanId : mAppConf.getParamInt("controller_id");
}

VirtualVesc *VirtualVesc::vescOnBus(int canId)
{
    for (auto v: devices()) {
        if (v != this && root()->mCanOn && v->mHwType == HW_TYPE_VESC && v->canId() == canId) {
            return v;
        }
    }
    return nullptr;
}

void VirtualVesc::lispStart(bool loadCode)
{
    static const vl_host_t fns = {hostPrint, hostSend, hostGet, hostSet, hostConf, hostEeprom, hostDisplay};

    lispStop();

#ifdef HAS_LISPBM
    mLbm = lbmApis[devices().indexOf(this)];
#endif
    if (!mLbm || !mLbm->init(&fns, this)) {
        mLbm = nullptr;
        return;
    }

    mLbmThread = QThread::create(mLbm->run);
    mLbmThread->start();

    mLbmCode = codeSlotPayload(mLispSlot);
    if (loadCode && !mLbmCode.isEmpty()) {
        mLbm->load(mLbmCode.constData(), mLbmCode.size());
    }
}

void VirtualVesc::lispStop()
{
    if (mLbmThread) {
        mLbm->kill();
        mLbmThread->wait();
        delete mLbmThread;
        mLbmThread = nullptr;
    }
    mLbm = nullptr;

    QMutexLocker locker(&displayLock);
    displays.remove(mCanId);
    displayFrame++;
}

// The values that the get-extensions return on the motor controllers
void VirtualVesc::lispMock()
{
    VirtualPlant *p = mPlant;
    const int n = mCanOn ? VirtualPlant::motorCount : 1;

    vesc_mock_state_t s = {};
    s.rpm = s.rpm_fast = s.rpm_faster = p->erpm;
    s.current = s.current_dir = s.iq = p->iMotor();
    s.current_in = p->iIn;
    s.duty = p->duty;
    s.duty_abs = fabs(p->duty);
    s.vin = p->vBus;
    s.batt = p->soc;
    s.vd = p->vd;
    s.vq = p->vq;
    s.speed = p->speed;
    s.ah = p->ah;
    s.ah_chg = p->ahCharged;
    s.wh = p->wh;
    s.wh_chg = p->whCharged;
    s.temp_fet = p->tempFet;
    s.temp_mot = p->tempMotor;
    s.dist = p->dist;
    s.dist_abs = p->distAbs;
    s.ppm = p->decodedPpm;
    s.adc[0] = p->adc1;
    s.adc[1] = p->adc2;
    s.adc_decoded[0] = p->decodedAdc;
    s.adc_decoded[1] = p->decodedAdc2;
    s.imu_rpy[1] = p->slope;
    s.imu_rpy[2] = p->heading;
    s.gnss_lat = p->lat;
    s.gnss_lon = p->lon;
    s.gnss_speed = p->speed;
    s.setup_current = p->iMotor() * n;
    s.setup_current_in = p->iIn * n;
    s.setup_ah = p->ah * n;
    s.setup_ah_chg = p->ahCharged * n;
    s.setup_wh = p->wh * n;
    s.setup_wh_chg = p->whCharged * n;
    s.setup_num_vescs = n;
    strncpy(s.fault_name, Commands::faultToStr(mc_fault_code(p->fault)).toLatin1().constData(),
            sizeof(s.fault_name) - 1);

    for (auto v: devices()) {
        if (v->mLbm && v->mHwType == HW_TYPE_VESC) {
            v->mLbm->mock(&s);
        }
    }
}

// Lisp commands that wait for the evaluator, handled without the lock.
// Returns false when processPacket should continue.
bool VirtualVesc::processLisp(COMM_PACKET_ID id, VByteArray &vb)
{
    VByteArray reply;
    reply.vbAppendUint8(id);

    switch (id) {
    case COMM_LISP_ERASE_CODE:
        lispStop();
        return false;

    case COMM_LISP_SET_RUNNING:
        mLispRunning = vb.vbPopFrontUint8();
        updateCustomConfig();
        if (mLispRunning) {
            lispStart(true);
        } else {
            lispStop();
        }
        reply.vbAppendUint8(1);
        sendReply(reply);
        return true;

    case COMM_LISP_GET_STATS: {
        if (!mLbm) {
            return true;
        }

        bool all = vb.isEmpty() || vb.vbPopFrontUint8();
        float heap = 0.0, mem = 0.0;
        QVector<QPair<QString, double>> globals;
        mLbm->stats(&heap, &mem, &globals, [](void *h, const char *name, double val) {
            static_cast<QVector<QPair<QString, double>>*>(h)->append({name, val});
        });

        reply.vbAppendDouble16(0.0, 1e2); // CPU
        reply.vbAppendDouble16(heap, 1e2);
        reply.vbAppendDouble16(mem, 1e2);
        reply.vbAppendDouble16(0.0, 1e2); // Stack
        reply.vbAppendString(""); // Result
        for (const auto &g: globals) {
            if (reply.size() > 300) {
                break;
            }
            if (all || g.first.startsWith("vt", Qt::CaseInsensitive)) {
                reply.vbAppendString(g.first);
                reply.vbAppendDouble32Auto(g.second);
            }
        }
        sendReply(reply);
    } return true;

    case COMM_LISP_REPL_CMD:
        if (!mLbm) {
            lispStart(false);
        }
        if (vb.startsWith(':')) {
            print("The special commands are not available in the Virtual ESC", COMM_LISP_PRINT);
        } else if (vb.size() <= 1) {
            print(">", COMM_LISP_PRINT);
        } else if (mLbm) {
            mLbm->eval(vb.constData(), false);
        } else {
            print("LispBM is not running", COMM_LISP_PRINT);
        }
        return true;

    case COMM_LISP_STREAM_CODE: {
        qint32 offset = vb.vbPopFrontInt32();
        qint32 totLen = vb.vbPopFrontInt32();
        qint8 restart = vb.vbPopFrontInt8();

        if (offset == 0) {
            mLbmStream.clear();
            if (!mLbm || restart > 0) {
                lispStart(restart == 2);
            }
        }

        mLbmStream.append(vb);
        reply.vbAppendInt32(offset);
        reply.vbAppendInt16(mLbm ? 0 : -1);
        sendReply(reply);

        if (mLbm && mLbmStream.size() >= totLen) {
            print("Stream done, starting...", COMM_LISP_PRINT);
            mLbm->eval(mLbmStream.constData(), true);
        }
    } return true;

    case COMM_CUSTOM_APP_DATA:
        if (mLbm) {
            mLbm->event("data-rx", 0, 0, reinterpret_cast<const uint8_t*>(vb.constData()), vb.size());
        }
        return true;

    default:
        return false;
    }
}

// Like a full link, prints and sends are dropped when the Qt thread falls behind
static bool lbmQueue(QAtomicInt &pending)
{
    if (pending.fetchAndAddRelaxed(1) >= 500) {
        pending.fetchAndAddRelaxed(-1);
        return false;
    }
    return true;
}

void VirtualVesc::hostPrint(void *host, const char *str)
{
    auto v = static_cast<VirtualVesc*>(host);
    if (!lbmQueue(v->mLbmPending)) {
        return;
    }
    QMetaObject::invokeMethod(v, [v, s = QString::fromLatin1(str)]() {
        v->mLbmPending.fetchAndAddRelaxed(-1);
        v->print(s, COMM_LISP_PRINT);
    }, Qt::QueuedConnection);
}

void VirtualVesc::hostSend(void *host, const char *what, int id, int arg, const uint8_t *data, int len)
{
    auto v = static_cast<VirtualVesc*>(host);
    if (!lbmQueue(v->mLbmPending)) {
        return;
    }
    QMetaObject::invokeMethod(v, [v, w = QString(what), id, arg, d = QByteArray((const char*)data, len)]() {
        v->mLbmPending.fetchAndAddRelaxed(-1);
        if (w == "send-data") {
            VByteArray vb;
            vb.vbAppendUint8(COMM_CUSTOM_APP_DATA);
            vb.append(d);
            v->sendReply(vb);
            return;
        }

        QMutexLocker locker(&v->root()->mLock);
        for (auto t: v->devices()) {
            if (t == v || !t->mLbm || !v->root()->mCanOn) {
                continue;
            }

            auto data = reinterpret_cast<const uint8_t*>(d.constData());
            if (w == "canmsg-send" && t->canId() == id) {
                t->mLbm->event("rmsg", id, arg, data, d.size());
            } else if (w.startsWith("can-send-")) {
                t->mLbm->event(w.endsWith("eid") ? "can-eid" : "can-sid", id, 0, data, d.size());
            }
        }
    }, Qt::QueuedConnection);
}

double VirtualVesc::hostGet(void *host, const char *what, int id)
{
    auto v = static_cast<VirtualVesc*>(host);
    VirtualVesc *root = v->root();
    QMutexLocker locker(&root->mLock);
    VirtualPlant *p = v->mPlant;
    const QString w(what);

    // The ADS1115 of the Dash: buttons 0 - 2 divide channel 1, button 3 pulls channel 0 down
    if (w == "ads-volts") {
        if (id == 0) {
            return (root->mButtons & 8) ? 1.0 : 3.3;
        }
        const double volts[] = {1.24, 1.65, 2.25};
        for (int i = 0;i < 3;i++) {
            if (root->mButtons & (1 << i)) {
                return volts[i];
            }
        }
        return 3.3;
    }

    if (w == "systime-hz") {
        return v->mHwType == HW_TYPE_CUSTOM_MODULE ? 1000 : 10000;
    } else if (w == "can-local-id") {
        return v->canId();
    } else if (w == "fault") {
        return p->fault;
    } else if (w == "odometer") {
        return p->odometer;
    } else if (w == "fw-major" || w == "fw-minor") {
        return w == "fw-major" ? v->mFwMajor : v->mFwMinor;
    } else if (w == "bms-msg-age") {
        return root->mCanOn ? 0.1 : 1000.0;
    }

    // Same values as COMM_BMS_GET_VALUES
    if (w.startsWith("bms-")) {
        const int n = VirtualPlant::motorCount;
        const int cells = int(p->packCells);
        if (!root->mCanOn) {
            return NAN;
        } else if (w == "bms-v-cell" || w == "bms-bal-state") {
            return id >= 0 && id < cells ? (w == "bms-v-cell" ? p->cellVoltage(id) : p->cellBalancing(id)) : NAN;
        } else if (w == "bms-temps-adc") {
            return id >= 0 && id < 4 ? p->packTempNow() + 0.4 * id : NAN;
        }
        const QHash<QString, double> bms = {
            {"bms-v-tot", p->vBus}, {"bms-v-charge", 0.0}, {"bms-i-in", p->iIn * n},
            {"bms-i-in-ic", p->iIn * n}, {"bms-ah-cnt", p->ah * n}, {"bms-wh-cnt", p->wh * n},
            {"bms-cell-num", cells}, {"bms-temp-adc-num", 4}, {"bms-temp-ic", p->packTempNow() + 4.0},
            {"bms-temp-hum", 24.0}, {"bms-hum", 41.0}, {"bms-temp-cell-max", p->packTempNow() + 1.2},
            {"bms-soc", p->soc}, {"bms-soh", p->soh}, {"bms-can-id", BMS_CAN_ID}};
        return bms.value(w, NAN);
    }

    // canget-* and the status messages of the motor controllers. The Express
    // reports speed and distance unconverted, as ERPM and tachometer counts.
    if (!v->vescOnBus(id)) {
        return NAN;
    }
    bool raw = v->mHwType == HW_TYPE_CUSTOM_MODULE;
    const QHash<QString, double> status = {
        {"can-msg-age", 0.01}, {"rpm", p->erpm}, {"current", p->iMotor()}, {"current-dir", p->iMotor()},
        {"current-in", p->iIn}, {"duty", p->duty}, {"temp-fet", p->tempFet}, {"temp-motor", p->tempMotor},
        {"vin", p->vBus}, {"speed", raw ? p->erpm : p->speed}, {"dist", raw ? p->tacho : p->dist},
        {"ppm", p->decodedPpm}, {"adc", p->adc1}};
    return status.value(w, NAN);
}

void VirtualVesc::hostSet(void *host, const char *what, int id, double value)
{
    auto v = static_cast<VirtualVesc*>(host);
    const QString w(what);

    if (w == "reboot") {
        QMetaObject::invokeMethod(v, [v]() { v->lispStart(true); }, Qt::QueuedConnection);
        return;
    }

    QMutexLocker locker(&v->root()->mLock);
    VirtualPlant *p = v->mPlant;
    VirtualVesc *t = id >= 0 ? v->vescOnBus(id) : (v->mHwType == HW_TYPE_VESC ? v : nullptr);
    if (!t) {
        return;
    }

    double iMax = t->mMcConf.getParamDouble("l_current_max");
    if (w == "alive") {
        p->alive();
    } else if (w == "current" || w == "current-rel") {
        p->setControl(VirtualPlant::CTRL_CURRENT, w == "current" ? value : value * iMax);
    } else if (w == "brake" || w == "brake-rel") {
        p->setControl(VirtualPlant::CTRL_BRAKE, w == "brake" ? value : value * iMax);
    } else if (w == "duty") {
        p->setControl(VirtualPlant::CTRL_DUTY, value);
    } else if (w == "rpm") {
        p->setControl(VirtualPlant::CTRL_RPM, value);
    }
}

bool VirtualVesc::hostConf(void *host, const char *name, double *value, bool set, bool def)
{
    auto v = static_cast<VirtualVesc*>(host);
    QMutexLocker locker(&v->root()->mLock);
    const QString n(name);

    for (auto c: {&v->mMcConf, &v->mAppConf}) {
        if (!c->hasParam(n)) {
            continue;
        }

        ConfigParams *src = !def ? c : (c == &v->mMcConf ? &v->mMcConfDefault : &v->mAppConfDefault);
        ConfigParam *p = src->getParam(n);
        bool isInt = p->type != CFG_T_DOUBLE;
        if (!set) {
            *value = isInt ? p->valInt : p->valDouble;
        } else if (p->type == CFG_T_DOUBLE) {
            c->updateParamDouble(n, *value);
        } else if (p->type == CFG_T_ENUM) {
            c->updateParamEnum(n, int(*value));
        } else if (p->type == CFG_T_BOOL) {
            c->updateParamBool(n, *value != 0.0);
        } else {
            c->updateParamInt(n, int(*value));
        }
        return isInt;
    }

    if (set) {
        v->mOtherConf[n] = *value;
    } else {
        *value = v->mOtherConf.value(n);
    }
    return false;
}

bool VirtualVesc::hostEeprom(void *host, int addr, uint32_t *value, bool store)
{
    auto v = static_cast<VirtualVesc*>(host);
    if (addr < 0 || addr >= (v->mHwType == HW_TYPE_CUSTOM_MODULE ? 512 : 256)) {
        return false;
    }

    QMutexLocker locker(&v->root()->mLock);
    if (store) {
        v->mEeprom[addr] = *value;
        QMetaObject::invokeMethod(v->mSaveTimer, "start", Qt::QueuedConnection);
    } else if (v->mEeprom.contains(addr)) {
        *value = v->mEeprom.value(addr);
    } else if (v->mHwType == HW_TYPE_CUSTOM_MODULE) {
        *value = 0xFFFFFFFF; // Erased flash on the Express, -1 or NaN
    } else {
        return false;
    }
    return true;
}

// All displays are the 480x320 of the Dash 35B
void VirtualVesc::hostDisplay(void *host, int x, int y, int w, int h, const uint32_t *rgb, uint32_t color)
{
    auto v = static_cast<VirtualVesc*>(host);
    QMutexLocker locker(&displayLock);
    QImage &img = displays[v->mCanId];
    if (img.isNull()) {
        img = QImage(480, 320, QImage::Format_RGB32);
        img.fill(Qt::black);
    }

    if (!rgb) {
        img.fill(QColor(QRgb(color)));
    } else {
        for (int j = qMax(0, -y);j < h && y + j < img.height();j++) {
            QRgb *line = reinterpret_cast<QRgb*>(img.scanLine(y + j));
            for (int i = qMax(0, -x);i < w && x + i < img.width();i++) {
                line[x + i] = 0xFF000000 | rgb[j * w + i];
            }
        }
    }
    displayFrame++;
}

VirtualDisplay::VirtualDisplay(QQuickItem *parent) :
    QQuickPaintedItem(parent), mCanId(-1), mFrame(-1)
{
    auto timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]() {
        QMutexLocker locker(&displayLock);
        if (displayFrame != mFrame) {
            mFrame = displayFrame;
            update();
        }
    });
    timer->start(50);
}

void VirtualDisplay::paint(QPainter *painter)
{
    QMutexLocker locker(&displayLock);
    QImage img = displays.value(mCanId);
    locker.unlock();
    if (!img.isNull()) {
        painter->drawImage(boundingRect(), img);
    }
}
