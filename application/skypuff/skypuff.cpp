/*
    Copyright 2020 Kirill Kostiuchenko	kisel2626@gmail.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <QSound>
#include <QThread>
#include <QTimerEvent>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include "skypuff.h"

Skypuff::Skypuff(VescInterface *v) : QObject(),
    vesc(v),
    aliveTimerId(0),
    aliveTimeoutTimerId(0),
    getConfTimeoutTimerId(0)
{
    player = new QMediaPlayer(this);
    playlist = new QMediaPlaylist(player);
    player->setPlaylist(playlist);

    connect(vesc, SIGNAL(portConnectedChanged()), this, SLOT(portConnectedChanged()));
    connect(vesc, SIGNAL(fwRxChanged(bool, bool)), this, SLOT(firmwareVersionReceived(bool, bool)));
    connect(vesc, SIGNAL(messageDialog(QString,QString,bool,bool)), this, SLOT(logVescDialog(const QString&,const QString&)));
    connect(vesc->commands(), SIGNAL(customAppDataReceived(QByteArray)), this, SLOT(customAppDataReceived(QByteArray)));

    setState(DISCONNECTED);
    clearStats();

    statsResponseTimes.setCapacity(aliveAvgN);
}

void Skypuff::logVescDialog(const QString & title, const QString & text)
{
    qWarning() << "-- VESC:" << title;
    qWarning() << "  --" << text;
}

void Skypuff::setState(const skypuff_state newState)
{
    if(state != newState) {
        state = newState;
        emit stateChanged(state_str(state));

        // Translate to UI
        switch (newState) {
        case DISCONNECTED:
            stateText = tr("Disconnected");
            break;
        case BRAKING:
            stateText = tr("Braking");
            break;
        case MANUAL_BRAKING:
            stateText = tr("Manual Braking");
            break;
        case MANUAL_SLOW_SPEED_UP:
            stateText = tr("Speed Up");
            break;
        case MANUAL_SLOW:
            stateText = tr("Manual Slow");
            break;
        case MANUAL_SLOW_BACK_SPEED_UP:
            stateText = tr("Speed Up Back");
            break;
        case MANUAL_SLOW_BACK:
            stateText = tr("Slow Back");
            break;
        case BRAKING_EXTENSION:
            stateText = tr("Braking Ext");
            break;
        case UNWINDING:
            stateText = tr("Unwinding");
            break;
        case REWINDING:
            stateText = tr("Rewinding");
            break;
        case SLOWING:
            stateText = tr("Slowing");
            break;
        case SLOW:
            stateText = tr("Slow");
            break;
        case PRE_PULL:
            stateText = tr("Pre Pull");
            break;
        case TAKEOFF_PULL:
            stateText = tr("Takeoff Pull");
            break;
        case PULL:
            stateText = tr("Pull");
            break;
        case FAST_PULL:
            stateText = tr("Fast Pull");
            break;
        case UNINITIALIZED:
            stateText = tr("No Valid Settings");
            break;
        default:
            stateText = state_str(newState);
        }

        emit stateTextChanged(stateText);
    }
}

void Skypuff::firmwareVersionReceived(bool rx, bool /*limited*/)
{
    if(rx)
        sendGetConf();
}

void Skypuff::portConnectedChanged()
{
    if(vesc->isPortConnected())
        return;

    // Stop timers
    if(aliveTimerId) {
        killTimer(aliveTimerId);
        aliveTimerId = 0;
    }
    if(getConfTimeoutTimerId) {
        killTimer(getConfTimeoutTimerId);
        getConfTimeoutTimerId = 0;
    }
    if(this->aliveTimeoutTimerId) {
        killTimer(aliveTimeoutTimerId);
        aliveTimeoutTimerId = 0;
    }

    // No more fault :)
    setState(DISCONNECTED);

    // Reset scales
    cfg.clearScales();
    clearStats();
    scalesUpdated();
}

void Skypuff::clearStats()
{
    // Alive ping stats
    statsResponseTimes.clear();
    sumResponceTime = 0;
    minResponceTime = INT_MAX;
    maxResponceTime = INT_MIN;

    aliveStep = 0;
    vBat = 0;
    curTac = 0;
    erpm = 0;
    motorAmps = 0;
    batteryAmps = 0;
    tempFets = 0;
    tempMotor = 0;

    setFault(FAULT_CODE_NONE);
    playingFault = FAULT_CODE_NONE;
    status.clear();
}

void Skypuff::timerEvent(QTimerEvent *event)
{
    if(event->timerId() == getConfTimeoutTimerId) {
        killTimer(getConfTimeoutTimerId);
        getConfTimeoutTimerId = 0;
        vesc->emitMessageDialog(tr("Command 'get_conf' timeout"),
                                tr("Skypuff MCU did not answered within %2ms timeout.<br/><br/>"
                                   "Make sure connection is stable. VESC running Skypuff enabled firmware. "
                                   "Motor configured, Custom User App started.<br/><br/>Use vesc_tool terminal 'skypuff' command.").arg(commandTimeout),
                                true);
        vesc->disconnectPort();
        player->setMedia(QUrl("qrc:/res/sounds/alert.mp3"));
        player->play();
    }
    else if(event->timerId() == aliveTimeoutTimerId) {
        killTimer(aliveTimeoutTimerId);
        aliveTimeoutTimerId = 0;
        vesc->emitMessageDialog(tr("Command 'alive' timeout"),
                                tr("Skypuff MCU did not answered within %2ms timeout.<br/><br/>"
                                   "Make sure connection is stable. VESC running Skypuff enabled firmware. "
                                   "Motor configured, Custom User App started. Check it with 'skypuff' command.").arg(commandTimeout),
                                true);
        vesc->disconnectPort();
    }
    else if(event->timerId() == this->aliveTimerId) {
        requestStats();
    }
    else
        qFatal("Skypuff::timerEvent(): unknown timer id %d", event->timerId());
}

void Skypuff::sendGetConf()
{
    if(getConfTimeoutTimerId) {
        vesc->emitMessageDialog(tr("Can't send 'get_conf' command"),
                                tr("Timer already activated: '%2'..").arg(getConfTimeoutTimerId),
                                true);
        vesc->disconnectPort();
        return;
    }

    vesc->commands()->sendTerminalCmd("get_conf");
    getConfTimeoutTimerId = startTimer(commandTimeout, Qt::PreciseTimer);
}

void Skypuff::requestStats()
{
    VByteArray vb;

    vb.vbAppendUint8((aliveStep % aliveStepsForTemps) ? SK_COMM_POWER_STATS : SK_COMM_TEMP_STATS);
    aliveStep++;

    vesc->commands()->sendCustomAppData(vb);

    if(!aliveTimeoutTimerId) {
        aliveResponseDelay.start();
        aliveTimeoutTimerId = startTimer(commandTimeout, Qt::PreciseTimer);
    } else {
        QString wrn = tr("Alive response %1ms timeout!").arg(aliveResponseDelay.elapsed());
        qWarning() << wrn;
        emit statusChanged(wrn, true);
    }
}

QMLable_skypuff_config Skypuff::loadSettings(const QString & fileName)
{
    QMLable_skypuff_config c;
    QSettings f(QUrl(fileName).toLocalFile(), QSettings::IniFormat);
    c.loadV1(f);

    return c;
}

QString Skypuff::defaultSettingsFileName()
{
    QFileInfo fi(QDir(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)), "skypuff.ini");

    return fi.absoluteFilePath();
}

bool Skypuff::isFileExists(const QString & fileName)
{
    QFileInfo fi(fileName);
    return fi.exists();
}

bool Skypuff::saveSettings(const QString & fileName, const QMLable_skypuff_config& c)
{
    QSettings f(fileName, QSettings::IniFormat);
    if(f.status() != QSettings::NoError)
        return false;

    return c.saveV1(f);
}

// Will not make it too complicate with timeout
void Skypuff::sendSettings(const QMLable_skypuff_config& cfg)
{
    vesc->commands()->sendCustomAppData(cfg.serializeV1());
}

void Skypuff::customAppDataReceived(QByteArray data)
{
    VByteArray vb(data);

    if(vb.length() < 1) {
        vesc->emitMessageDialog(tr("Can't deserialize"),
                                tr("Not enough data to deserialize command byte"),
                                true);
        vesc->disconnectPort();
        return;
    }

    skypuff_custom_app_data_command command = (skypuff_custom_app_data_command)vb.vbPopFrontUint8();

    switch(command) {
    case SK_COMM_POWER_STATS:
        processStats(vb, false);
        break;
    case SK_COMM_TEMP_STATS:
        processStats(vb, true);
        break;
    case SK_COMM_PULLING_TOO_HIGH:
        processPullingTooHigh(vb);
        break;
    case SK_COMM_FORCE_IS_SET:
        processForceIsSet(vb);
        break;
    case SK_COMM_UNWINDED_TO_OPPOSITE:
        processUnwindedToOpposite(vb);
        break;
    case SK_COMM_UNWINDED_FROM_SLOWING:
        processUnwindedFromSlowing(vb);
        break;
    case SK_COMM_SETTINGS_APPLIED:
        processSettingsApplied(vb);
        break;
    case SK_COMM_DETECTING_MOTION:
        processDetectingMotion(vb);
        break;
    case SK_COMM_TOO_SLOW_SPEED_UP:
        processTooSlowSpeedUp(vb);
        break;
    case SK_COMM_ZERO_IS_SET:
        processZeroIsSet(vb);
        break;
    case SK_COMM_MSG:
        processMsg(vb);
        break;
    case SK_COMM_SETTINGS_V1:
        processSettingsV1(vb);
        break;
    default:
        vesc->emitMessageDialog(tr("Unknown CUSTOM_APP_DATA command"),
                                tr("Received command: '%1'.").arg((int)command),
                                true);
        vesc->disconnectPort();
        return;
    }

}

void Skypuff::updateStatsResponse(const int millis)
{
    sumResponceTime += millis;

    // Get old value if buffer is full
    if(statsResponseTimes.count() == aliveAvgN)
        sumResponceTime -= statsResponseTimes.takeFirst();

    statsResponseTimes.append(millis);

    emit avgResponseMillisChanged(getAvgResponseMillis());

    if(millis < minResponceTime) {
        minResponceTime = millis;
        emit minResponseMillisChanged(millis);
    }

    if(millis > maxResponceTime) {
        maxResponceTime = millis;
        emit maxResponseMillisChanged(millis);
    }
}

void Skypuff::processPullingTooHigh(VByteArray &vb)
{
    // Enough data?
    const int pulling_too_high_packet_length = 2;
    if(vb.length() < pulling_too_high_packet_length) {
        vesc->emitMessageDialog(tr("Can't deserialize pulling to high command packet"),
                                tr("Received %1 bytes, expected %2 bytes!").arg(vb.length()).arg(pulling_too_high_packet_length),
                                true);
        vesc->disconnectPort();
    }

    float current = vb.vbPopFrontDouble16(1e1);

    if(vb.length()) {
        vesc->emitMessageDialog(tr("Extra bytes received with pulling too high command packet"),
                                tr("Received %1 extra bytes!").arg(vb.length()),
                                true);
        vesc->disconnectPort();
        return;
    }

    setStatus(tr("Pulling too high - %1Kg").arg((double)(current / cfg.amps_per_kg), 0, 'f', 2));
}

void Skypuff::processUnwindedToOpposite(VByteArray &vb)
{
    if(vb.length()) {
        vesc->emitMessageDialog(tr("Extra bytes received with unwinded to opposite command packet"),
                                tr("Received %1 extra bytes!").arg(vb.length()),
                                true);
        vesc->disconnectPort();
        return;
    }

    setStatus(tr("Opposite braking zone"));
}

void Skypuff::processUnwindedFromSlowing(VByteArray &vb)
{
    if(vb.length()) {
        vesc->emitMessageDialog(tr("Extra bytes received with unwinded from slowing command packet"),
                                tr("Received %1 extra bytes!").arg(vb.length()),
                                true);
        vesc->disconnectPort();
        return;
    }

    setStatus(tr("Slowing zone passed"));
}

void Skypuff::processDetectingMotion(VByteArray &vb)
{
    if(vb.length()) {
        vesc->emitMessageDialog(tr("Extra bytes received with detecting motion command packet"),
                                tr("Received %1 extra bytes!").arg(vb.length()),
                                true);
        vesc->disconnectPort();
        return;
    }

    setStatus(tr("Detecting motion..."));
}

void Skypuff::processTooSlowSpeedUp(VByteArray &vb)
{
    if(vb.length()) {
        vesc->emitMessageDialog(tr("Extra bytes received with too slow speed up command packet"),
                                tr("Received %1 extra bytes!").arg(vb.length()),
                                true);
        vesc->disconnectPort();
        return;
    }

    setStatus(tr("Too slow speed up"));
}

void Skypuff::processForceIsSet(VByteArray &vb)
{
    // Enough data?
    const int force_is_set_packet_length = 4;
    if(vb.length() < force_is_set_packet_length) {
        vesc->emitMessageDialog(tr("Can't deserialize force is set command packet"),
                                tr("Received %1 bytes, expected %2 bytes!").arg(vb.length()).arg(force_is_set_packet_length),
                                true);
        vesc->disconnectPort();
    }

    // Calculate amps_per_sec
    float pull_current = vb.vbPopFrontDouble16(1e1);
    float amps_per_sec = vb.vbPopFrontDouble16(1e1);

    setStatus(tr("%1Kg (%2A, %3A/sec)").
                arg(pull_current / cfg.amps_per_kg, 0, 'f', 2).
                arg(pull_current, 0, 'f', 1).
                arg(amps_per_sec, 0, 'f', 1));

    //setStatus(tr("%1Kg (%2A) is set").
    //            arg(lastForceKg, 0, 'f', 2).
    //            arg(pull_current, 0, 'f', 1));

    if(vb.length()) {
        vesc->emitMessageDialog(tr("Extra bytes received with force is set command packet"),
                                tr("Received %1 extra bytes!").arg(vb.length()),
                                true);
        vesc->disconnectPort();
        return;
    }
}

void Skypuff::processZeroIsSet(VByteArray &vb)
{
    if(vb.length()) {
        vesc->emitMessageDialog(tr("Extra bytes received with zero is set command packet"),
                                tr("Received %1 extra bytes!").arg(vb.length()),
                                true);
        vesc->disconnectPort();
        return;
    }

    setStatus(tr("Zero is set"));
}

void Skypuff::processMsg(VByteArray &vb)
{
    if(!vb.length()) {
        vesc->emitMessageDialog(tr("No bytes with SK_COMM_MSG command packet"),
                                tr("Received %1 bytes!").arg(vb.length()),
                                true);
        vesc->disconnectPort();
        return;
    }

    QString msg(vb);

    setStatus(vb);
}

void Skypuff::processSettingsApplied(VByteArray &vb)
{
    if(vb.length()) {
        vesc->emitMessageDialog(tr("Extra bytes received with unwinded to opposite command packet"),
                                tr("Received %1 extra bytes!").arg(vb.length()),
                                true);
        vesc->disconnectPort();
        return;
    }

    vesc->emitMessageDialog(tr("Settings are set"), tr("Have a nice puffs"), true);
}

void Skypuff::processStats(VByteArray &vb, bool isTempsPacket)
{
    // alive could be set from UI, timer is not necessary but possible
    if(aliveTimeoutTimerId) {
        updateStatsResponse(aliveResponseDelay.elapsed());
        killTimer(aliveTimeoutTimerId);
        aliveTimeoutTimerId = 0;
    }
    else {
        qWarning() << "stats received, but timeout timer is not set";
    }

    // Enough data?
    const int stats_power_packet_length = 1 + 4 * 2  + 2 * 2;
    if(vb.length() < stats_power_packet_length) {
        vesc->emitMessageDialog(tr("Can't deserialize stats power command packet"),
                                tr("Received %1 bytes, expected %2 bytes!").arg(vb.length()).arg(stats_power_packet_length),
                                true);
        vesc->disconnectPort();
    }

    setState((skypuff_state)vb.vbPopFrontUint8());

    int newTac = vb.vbPopFrontInt32();
    float newErpm = vb.vbPopFrontInt16() * 4;
    float newMotorAmps = vb.vbPopFrontDouble16(1e1);
    float newBatteryAmps = vb.vbPopFrontDouble32(1e3);

    setPos(newTac);
    setSpeed(newErpm);
    setPower(newMotorAmps, newBatteryAmps);

    if(isTempsPacket) {
        const int stats_temp_packet_length = 2 * 3;
        if(vb.length() < stats_temp_packet_length) {
            vesc->emitMessageDialog(tr("Can't deserialize alive temp command packet"),
                                    tr("Received %1 bytes, expected %2 bytes!").arg(vb.length()).arg(stats_temp_packet_length),
                                    true);
            vesc->disconnectPort();
        }

        float newVBat = vb.vbPopFrontDouble16(1e1);
        float newFetsTemp = vb.vbPopFrontDouble16(1e1);
        float newMotorTemp = vb.vbPopFrontDouble16(1e1);

        setVBat(newVBat);
        setTempFets(newFetsTemp);
        setTempMotor(newMotorTemp);
    }

    // alive message may contain extra data (error messages)
    while(true) {
        if(!vb.length()) {
            break;
        }

        skypuff_custom_app_data_command extraCommand = (skypuff_custom_app_data_command)vb.vbPopFrontUint8();
        switch(extraCommand) {
            case SK_COMM_FAULT: {
                // Enough data?
                const int fault_packet_length = 1;
                if(vb.length() < fault_packet_length) {
                    vesc->emitMessageDialog(tr("Can't deserialize fault command packet"),
                                            tr("Received %1 bytes, expected %2 bytes!").arg(vb.length()).arg(fault_packet_length),
                                            true);
                    vesc->disconnectPort();
                }
                setFault((mc_fault_code)vb.vbPopFrontUint8());
                break;
            }
            case SK_COMM_OUT_OF_LIMITS: {
                // Enough data?
                const int message_length = vb.vbPopFrontUint8();
                if(vb.length() < message_length) {
                    vesc->emitMessageDialog(tr("Can't deserialize out of limits command packet"),
                                            tr("Received %1 bytes, expected %2 bytes!").arg(vb.length()).arg(message_length),
                                            true);
                    vesc->disconnectPort();
                }

                QString msg = QString::fromUtf8(vb, message_length);
                vb.remove(0, message_length);
                vesc->emitMessageDialog(tr("Configuration is out of limits"), msg, false);
                break;
            }
            default: {
                vesc->emitMessageDialog(tr("Can't deserialize alive temp command packet"),
                                        tr("Received message with unknown type %1!").arg(extraCommand),
                                        true);
                vesc->disconnectPort();
            }
        }
    }
}

void Skypuff::processSettingsV1(VByteArray &vb)
{
    // Configuration could be set from console
    // Timeout timer is not necessary, but possible
    if(getConfTimeoutTimerId) {
        killTimer(getConfTimeoutTimerId);
        getConfTimeoutTimerId = 0;
    }
    else {
        qWarning() << "settings received, but get_conf timeout timer is not set";
    }

    // Enough data?
    const int v1_settings_length = 111;
    if(vb.length() < v1_settings_length) {
        vesc->emitMessageDialog(tr("Can't deserialize V1 settings"),
                                tr("Received %1 bytes, expected %2 bytes!").arg(vb.length()).arg(v1_settings_length),
                                true);
        vesc->disconnectPort();
    }

    // Get state and pos
    skypuff_state mcu_state;

    mcu_state = (skypuff_state)vb.vbPopFrontUint8();
    fault = (mc_fault_code)vb.vbPopFrontUint8(); // Will be updated with scales

    cfg.motor_max_current = vb.vbPopFrontDouble16(1e1);
    cfg.discharge_max_current = vb.vbPopFrontDouble16(1e1);
    cfg.charge_max_current = vb.vbPopFrontDouble16(1e1);
    cfg.fet_temp_max = vb.vbPopFrontDouble16(1e1);
    cfg.motor_temp_max = vb.vbPopFrontDouble16(1e1);
    cfg.v_in_min = vb.vbPopFrontDouble32(1e2);
    cfg.v_in_max = vb.vbPopFrontDouble32(1e2);

    cfg.deserializeV1(vb);

    setState(mcu_state);

    emit settingsChanged(cfg);

    if(vb.length()) {
        vesc->emitMessageDialog(tr("Extra bytes received with V1 settings"),
                                tr("Received %1 extra bytes!").arg(vb.length()),
                                true);
        vesc->disconnectPort();
    }

    scalesUpdated();

    // Start alives sequence after first get_conf
    if(!aliveTimerId)
        aliveTimerId = startTimer(aliveTimerDelay, Qt::PreciseTimer);
}

void Skypuff::scalesUpdated()
{
    // Some new warnings or faults?
    playAudio();

    emit faultChanged(getFaultTranslation());
    emit tempFetsChanged(tempFets);
    emit tempMotorChanged(tempMotor);

    // Battery
    emit batteryChanged(getBatteryPercents());
    emit batteryScalesChanged(isBatteryScaleValid());
    emit batteryWarningChanged(isBatteryWarning());
    emit batteryBlinkingChanged(isBatteryBlinking());

    // Position
    emit posChanged(cfg.tac_steps_to_meters(curTac));
    emit brakingRangeChanged(isBrakingRange());
    emit brakingExtensionRangeChanged(isBrakingExtensionRange());
}


void Skypuff::playWarning(const char *resPath)
{
    // Still playing something?
    if(player->state() == QMediaPlayer::PlayingState)
        return;

    // Just play resPath once
    playlist->setPlaybackMode(QMediaPlaylist::Sequential);
    playlist->clear();
    playlist->addMedia(QUrl(tr(resPath)));
    player->play();
}

// Priority to faults, play warnings only once if no fault
void Skypuff::playAudio()
{
    if(fault == FAULT_CODE_NONE) {
        // In case of fault gone play prev fault once
        playlist->setPlaybackMode(QMediaPlaylist::Sequential);

        // Still playing something?
        if(player->state() == QMediaPlayer::PlayingState)
            return;

        playlist->clear();

        // Play some warnings?
        if(isBatteryTooHigh()) {
            playlist->addMedia(QUrl(tr("qrc:/res/sounds/Battery charged too high.mp3")));
            emit statusChanged(tr("Battery charged too high"), true);
        }

        if(isBatteryTooLow()) {
            playlist->addMedia(QUrl(tr("qrc:/res/sounds/Battery charged too low.mp3")));
            emit statusChanged(tr("Battery charged too low"), true);
        }

        if(!playlist->isEmpty())
            player->play();

        return;
    }

    playlist->setPlaybackMode(QMediaPlaylist::Loop);

    // Still playing the same fault?
    if(playingFault == fault && player->state() == QMediaPlayer::PlayingState) {
        // Do not interrupt playing
        return;
    }

    playingFault = fault;

    playlist->clear();
    playlist->addMedia(QUrl("qrc:/res/sounds/Alert.mp3"));

    // Known specific alerts?
    switch (fault) {
    case FAULT_CODE_OVER_VOLTAGE:
        playlist->addMedia(QUrl(tr("qrc:/res/sounds/faults/Overvoltage.mp3")));
        break;
    case FAULT_CODE_UNDER_VOLTAGE:
        playlist->addMedia(QUrl(tr("qrc:/res/sounds/faults/Undervoltage.mp3")));
        break;
    case FAULT_CODE_DRV:
        playlist->addMedia(QUrl(tr("qrc:/res/sounds/faults/DRV.mp3")));
        break;
    case FAULT_CODE_ABS_OVER_CURRENT:
        playlist->addMedia(QUrl(tr("qrc:/res/sounds/faults/Absolute over current.mp3")));
        break;
    case FAULT_CODE_OVER_TEMP_FET:
        playlist->addMedia(QUrl(tr("qrc:/res/sounds/faults/Over temp FET.mp3")));
        break;
    case FAULT_CODE_OVER_TEMP_MOTOR:
        playlist->addMedia(QUrl(tr("qrc:/res/sounds/faults/Over temp motor.mp3")));
        break;
    case FAULT_CODE_GATE_DRIVER_OVER_VOLTAGE:
        playlist->addMedia(QUrl(tr("qrc:/res/sounds/faults/Gate driver over voltage.mp3")));
        break;
    case FAULT_CODE_GATE_DRIVER_UNDER_VOLTAGE:
        playlist->addMedia(QUrl(tr("qrc:/res/sounds/faults/Gate driver under voltage.mp3")));
        break;
    case FAULT_CODE_MCU_UNDER_VOLTAGE:
        playlist->addMedia(QUrl(tr("qrc:/res/sounds/faults/MCU under voltage.mp3")));
        break;
    case FAULT_CODE_BOOTING_FROM_WATCHDOG_RESET:
        playlist->addMedia(QUrl(tr("qrc:/res/sounds/faults/Booting from watchdog reset.mp3")));
        break;
    case FAULT_CODE_ENCODER_SPI:
        playlist->addMedia(QUrl(tr("qrc:/res/sounds/faults/Encoder SPI fault.mp3")));
        break;
    case FAULT_CODE_ENCODER_SINCOS_BELOW_MIN_AMPLITUDE:
        playlist->addMedia(QUrl(tr("qrc:/res/sounds/faults/Encoder below min amplitude.mp3")));
        break;
    case FAULT_CODE_ENCODER_SINCOS_ABOVE_MAX_AMPLITUDE:
        playlist->addMedia(QUrl(tr("qrc:/res/sounds/faults/Encoder above max amplitude.mp3")));
        break;
    case FAULT_CODE_FLASH_CORRUPTION:
        playlist->addMedia(QUrl(tr("qrc:/res/sounds/faults/Flash corruption.mp3")));
        break;
    case FAULT_CODE_HIGH_OFFSET_CURRENT_SENSOR_1:
        playlist->addMedia(QUrl(tr("qrc:/res/sounds/faults/High offset current sensor 1.mp3")));
        break;
    case FAULT_CODE_HIGH_OFFSET_CURRENT_SENSOR_2:
        playlist->addMedia(QUrl(tr("qrc:/res/sounds/faults/High offset current sensor 2.mp3")));
        break;
    case FAULT_CODE_HIGH_OFFSET_CURRENT_SENSOR_3:
        playlist->addMedia(QUrl(tr("qrc:/res/sounds/faults/High offset current sensor 3.mp3")));
        break;
    case FAULT_CODE_UNBALANCED_CURRENTS:
        playlist->addMedia(QUrl(tr("qrc:/res/sounds/faults/Unbalanced currents.mp3")));
        break;
    case FAULT_CODE_BRK:
        playlist->addMedia(QUrl(tr("qrc:/res/sounds/faults/BRK fault.mp3")));
        break;
    case FAULT_CODE_RESOLVER_DOS:
        playlist->addMedia(QUrl(tr("qrc:/res/sounds/faults/Resolver DOS fault.mp3")));
        break;
    case FAULT_CODE_RESOLVER_LOS:
        playlist->addMedia(QUrl(tr("qrc:/res/sounds/faults/Resolver LOS fault.mp3")));
        break;
    case FAULT_CODE_RESOLVER_LOT:
        playlist->addMedia(QUrl(tr("qrc:/res/sounds/faults/Resolver LOT fault.mp3")));
        break;
    case FAULT_CODE_FLASH_CORRUPTION_APP_CFG:
        playlist->addMedia(QUrl(tr("qrc:/res/sounds/faults/Flash corruption application config.mp3")));
        break;
    case FAULT_CODE_FLASH_CORRUPTION_MC_CFG:
        playlist->addMedia(QUrl(tr("qrc:/res/sounds/faults/Flash corruption motor config.mp3")));
        break;
    case FAULT_CODE_ENCODER_NO_MAGNET:
        playlist->addMedia(QUrl(tr("qrc:/res/sounds/faults/Encoder no magnet.mp3")));
        break;
    case FAULT_CODE_ENCODER_MAGNET_TOO_STRONG:
        playlist->addMedia(QUrl(tr("qrc:/res/sounds/faults/Encoder magnet too strong.mp3")));
        break;
    default:
        playlist->addMedia(QUrl(tr("qrc:/res/sounds/Fault code.mp3")));
        break;
    }

    player->play();
}

QString Skypuff::getFaultTranslation()
{
    switch (fault) {
    case FAULT_CODE_NONE:
        return QString();
    case FAULT_CODE_OVER_VOLTAGE:
        return tr("Battery overvoltage %1V").arg(vBat);
    default:
        return Commands::faultToStr(fault);
    }
}

void Skypuff::setFault(const mc_fault_code newFault)
{
    if(fault != newFault) {
        fault = newFault;
        playAudio();
        emit faultChanged(getFaultTranslation());
    }
}

// overrideChanged after new seetings to update ranges flags
void Skypuff::setPos(const int newTac)
{
    if(newTac == curTac)
        return;

    // Does the tachometer jump over zero in any direction?
    if ((curTac > 0) != (newTac > 0)) {
        emit positiveTachometerChanged(newTac > 0);
    }

    bool wasBrakingRange = isBrakingRange();
    bool wasBrakingExtensionRange = isBrakingExtensionRange();

    // Play sound warnings
    if(curTac <= 0) {
        float oldMetersLeft = cfg.tac_steps_to_meters(cfg.rope_length - abs(curTac));
        float newMetersLeft = cfg.tac_steps_to_meters(cfg.rope_length - abs(newTac));

        if(oldMetersLeft > 0 && newMetersLeft <= 0)
            playWarning("qrc:/res/sounds/Tow line is over.mp3");

        if(oldMetersLeft > 100 && newMetersLeft <= 100)
            playWarning("qrc:/res/sounds/Alert.mp3");

        if(oldMetersLeft > 200 && newMetersLeft <= 200)
            playWarning("qrc:/res/sounds/200 meters left.mp3");
    }

    curTac = newTac;
    emit posChanged(cfg.tac_steps_to_meters(newTac));

    bool newBrakingRange = isBrakingRange();
    bool newBrakingExtensionRange = isBrakingExtensionRange();

    if(wasBrakingRange != newBrakingRange)
        emit brakingRangeChanged(newBrakingRange);

    if(wasBrakingExtensionRange != newBrakingExtensionRange)
        emit brakingExtensionRangeChanged(newBrakingExtensionRange);
}

void Skypuff::setSpeed(float new_erpm)
{
    if(erpm != new_erpm) {
        erpm = new_erpm;
        emit speedChanged(cfg.erpm_to_ms(new_erpm));
    }
}

void Skypuff::setPower(const float newMotorAmps, const float newBatteryAmps)
{
    if(motorAmps != newMotorAmps) {
        motorAmps = newMotorAmps;

        emit motorKgChanged(motorAmps / cfg.amps_per_kg);
    }

    if(batteryAmps != newBatteryAmps) {
        batteryAmps = newBatteryAmps;

        emit powerChanged(newBatteryAmps * vBat);
    }
}

void Skypuff::setTempFets(const float newTempFets)
{
    if(tempFets != newTempFets) {
        tempFets = newTempFets;
        emit tempFetsChanged(newTempFets);
    }
}

void Skypuff::setTempMotor(const float newTempMotor)
{
    if(tempMotor != newTempMotor) {
        tempMotor = newTempMotor;
        emit tempMotorChanged(newTempMotor);
    }
}

bool Skypuff::isBatteryTooHigh()
{
    if(!isBatteryScaleValid())
        return false;

    return getBatteryPercents() > (float)95;
}

bool Skypuff::isBatteryTooLow()
{
    if(!isBatteryScaleValid())
        return false;

    return getBatteryPercents() < (float)5;
}

bool Skypuff::isBatteryBlinking()
{
    if(!isBatteryScaleValid())
        return false;

    float p = getBatteryPercents();
    return  p  > (float)97 || p < (float)3;
}

void Skypuff::setVBat(const float newVBat)
{
    if(vBat != newVBat) {
        bool oldWarning = isBatteryWarning();
        bool oldBlinking = isBatteryBlinking();

        vBat = newVBat;
        emit batteryChanged(getBatteryPercents());

        bool newWarning = isBatteryWarning();
        bool newBlinking = isBatteryBlinking();

        if(oldWarning != newWarning) {
            emit batteryWarningChanged(newWarning);
        }

        if(newWarning)
            playAudio();

        if(oldBlinking != newBlinking)
            emit batteryBlinkingChanged(newBlinking);
    }
}

void Skypuff::setStatus(const QString& newStatus)
{
    // Do not filter same messages!
    //if(status != newStatus) {
        status = newStatus;
        emit statusChanged(newStatus);
    //}
}

QVariantList Skypuff::serialPortsToQml()
{
    QVariantList res;
    QVariantMap v;

    auto ports = vesc->listSerialPorts();
    for(auto it = ports.constBegin(); it != ports.constEnd(); it++) {
        if((*it).name.isEmpty())
            v["name"] = (*it).systemPath;
        else
            v["name"] = (*it).name + " [ " + (*it).systemPath + " ]";
        v["addr"] = (*it).systemPath;
        v["isVesc"] = (*it).isVesc;
        res.append(v);
    }

    return res;
}

bool Skypuff::connectSerial(QString port, int baudrate)
{
    return vesc->connectSerial(port, baudrate);
}
