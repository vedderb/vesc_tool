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
#ifndef SKYPUFF_H
#define SKYPUFF_H

#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QElapsedTimer>
#include <QContiguousCache>
#include "vescinterface.h"
#include "qmlable_skypuff_types.h"

const int aliveTimerDelay = 300; // milliseconds
const int commandTimeout = 1390;
const int aliveAvgN = 10; // number last alive responses to average
const int aliveStepsForTemps = 4; // Get temps every N steps


/*
 * After vesc is connected, Skypuff will ask configuration and
 * will starts sending alive commands.
 *
 * Received MCU terminal prints will be parsed, CUSTOM_APP_DATA
 * deserialized and translated to QML accessible properties and signals.
 *
 * Sorry for some hardcore stings parsing.
 * It's not too much difference from binary in the end.
 *
 * If no answer within commandTimeout, vesc will be disconnected
 * and error message thrown via vesc interface.
 */
class Skypuff : public QObject
{
    Q_OBJECT

    // I tried to make enum skypuff_state QML accesible, but ..
    Q_PROPERTY(QString state READ getState NOTIFY stateChanged)
    // Translated state
    Q_PROPERTY(QString stateText READ getStateText NOTIFY stateTextChanged)
    Q_PROPERTY(QString status READ getStatusText NOTIFY statusChanged)

    // To enable transitions to braking, if pos below or equal braking_length + braking_extension_length
    Q_PROPERTY(bool isBrakingExtensionRange READ isBrakingExtensionRange NOTIFY brakingExtensionRangeChanged)
    Q_PROPERTY(bool isPositiveTachometer READ isPositiveTachometer NOTIFY positiveTachometerChanged)
    // To enable manual_slow buttons
    Q_PROPERTY(bool isBrakingRange READ isBrakingRange NOTIFY brakingRangeChanged)
    Q_PROPERTY(float ropeMeters READ getRopeMeters NOTIFY settingsChanged)
    Q_PROPERTY(float drawnMeters READ getDrawnMeters NOTIFY posChanged)
    Q_PROPERTY(float leftMeters READ getLeftMeters NOTIFY posChanged)
    Q_PROPERTY(float speedMs READ getSpeedMs NOTIFY speedChanged)
    Q_PROPERTY(float motorKg READ getMotorKg NOTIFY motorKgChanged)
    Q_PROPERTY(float power READ getPower NOTIFY powerChanged)
    Q_PROPERTY(float tempFets READ getTempFets NOTIFY tempFetsChanged)
    Q_PROPERTY(float tempMotor READ getTempMotor NOTIFY tempMotorChanged)
    // Battery
    Q_PROPERTY(bool isBatteryScaleValid READ isBatteryScaleValid NOTIFY batteryScalesChanged)
    Q_PROPERTY(bool isBatteryBlinking READ isBatteryBlinking NOTIFY batteryBlinkingChanged)
    Q_PROPERTY(bool isBatteryWarning READ isBatteryWarning NOTIFY batteryWarningChanged)
    Q_PROPERTY(int batteryPercents READ getBatteryPercents NOTIFY batteryChanged)
    Q_PROPERTY(float batteryVolts READ getBatteryVolts NOTIFY batteryChanged)
    Q_PROPERTY(float batteryCellVolts READ getBatteryCellVolts NOTIFY batteryChanged)
    // Readable fault, empty if none
    Q_PROPERTY(QString fault READ getFaultTranslation NOTIFY faultChanged)

    Q_PROPERTY(int minResponseMillis READ getMinResponseMillis NOTIFY minResponseMillisChanged)
    Q_PROPERTY(int maxResponseMillis READ getMaxResponseMillis NOTIFY maxResponseMillisChanged)
    Q_PROPERTY(float avgResponseMillis READ getAvgResponseMillis NOTIFY avgResponseMillisChanged)

public:
    Skypuff(VescInterface *parent = 0);

    Q_INVOKABLE void sendTerminal(const QString &c) {vesc->commands()->sendTerminalCmd(c);}

    // All this types conversion between C++ and QML is very strange...
    Q_INVOKABLE QMLable_skypuff_config emptySettings() {return QMLable_skypuff_config();}
    Q_INVOKABLE void sendSettings(const QMLable_skypuff_config& cfg);
    Q_INVOKABLE QMLable_skypuff_config loadSettings(const QString & fileName);
    Q_INVOKABLE QString defaultSettingsFileName();
    Q_INVOKABLE bool isFileExists(const QString & fileName);
    Q_INVOKABLE bool saveSettings(const QString & fileName, const QMLable_skypuff_config& c);
    Q_INVOKABLE QVariantList serialPortsToQml();
    Q_INVOKABLE bool connectSerial(QString port, int baudrate = 115200);
    Q_INVOKABLE QString urlHost(const QString & u) {return QUrl(u).host();}
    Q_INVOKABLE int urlPort(const QString & u) {return QUrl(u).port();}

signals:
    /* It is simple to work with QML text states:
     *
     * DISCONNECTED - VESC is disconnected or connected but state not detected yet
     * UNITIALIZED - Skypuff app is waiting for correct settings
     * BRAKING .. and all skypuff states
     */
    void stateChanged(const QString& newState); // Clear state
    void stateTextChanged(const QString& newStateText);
    void settingsChanged(const QMLable_skypuff_config & cfg);
    void statusChanged(const QString &newStatus, bool isWarning = false);
    void brakingExtensionRangeChanged(const bool isBrakingExtensionRange);
    void positiveTachometerChanged(const bool isPositiveTachometer);
    void brakingRangeChanged(const bool isBrakingRange);
    void posChanged(const float meters);
    void speedChanged(const float ms);
    void motorKgChanged(const float kg);
    void powerChanged(const float batteryAmps);
    void tempFetsChanged(const float tempFets);
    void tempMotorChanged(const float tempMotor);
    void tempBatChanged(const float tempBat);
    void whInChanged(const float whIn);
    void whOutChanged(const float whOut);

    // Battery
    void batteryScalesChanged(const bool isValid);
    void batteryBlinkingChanged(const bool isBlinking);
    void batteryWarningChanged(const bool isWarning);
    void batteryChanged(const float percents);

    void faultChanged(const QString& newFault);

    // Connection
    void minResponseMillisChanged(const int millis);
    void maxResponseMillisChanged(const int millis);
    void avgResponseMillisChanged(const float millis);

protected slots:
    //void printReceived(QString str);
    void customAppDataReceived(QByteArray data);
    void portConnectedChanged();
    void firmwareVersionReceived(bool rx, bool limited);
    void logVescDialog(const QString & title, const QString & text);
protected:
    VescInterface *vesc;

    QMediaPlayer *player;
    QMediaPlaylist *playlist;

    int aliveTimerId;
    int aliveTimeoutTimerId;
    int getConfTimeoutTimerId;

    // Alive responce stats to monitor channel quality
    QElapsedTimer aliveResponseDelay;
    QContiguousCache<int> statsResponseTimes;
    int sumResponceTime;
    int minResponceTime;
    int maxResponceTime;

    QMLable_skypuff_config cfg;

    int aliveStep;
    int curTac;
    float erpm, motorAmps, batteryAmps;
    float tempFets, tempMotor;
    float vBat;
    mc_fault_code fault, playingFault;

    skypuff_state state;
    QString stateText;
    QString status;

    // Getters
    bool isBrakingRange() const {return abs(curTac) <= cfg.braking_length;}
    bool isBrakingExtensionRange() const {return abs(curTac) <= cfg.braking_length + cfg.braking_extension_length;}
    bool isPositiveTachometer() const {return curTac > 0;}
    float getRopeMeters() {return cfg.rope_length_to_meters();}
    float getDrawnMeters() {return cfg.tac_steps_to_meters(-curTac);}
    float getLeftMeters() {return cfg.tac_steps_to_meters(cfg.rope_length + curTac);}
    float getSpeedMs() {return cfg.erpm_to_ms(erpm);}
    float getMotorKg() {return motorAmps / cfg.amps_per_kg;}
    float getPower() {return batteryAmps * vBat;}
    float getTempFets() {return tempFets;}
    float getTempMotor() {return tempMotor;}

    // Battery
    bool isBatteryScaleValid() {return cfg.v_in_max && vBat;}
    bool isBatteryTooHigh();
    bool isBatteryTooLow();
    bool isBatteryWarning() {return isBatteryTooHigh() || isBatteryTooLow();}
    bool isBatteryBlinking();
    int getBatteryPercents() {return vBat ? (vBat - cfg.v_in_min) / (cfg.v_in_max - cfg.v_in_min) * (float)100 : 0;}
    float getBatteryVolts() {return vBat;}
    float getBatteryCellVolts() {return vBat ? vBat / cfg.battery_cells : 0;}

    // Connection stats
    int getMinResponseMillis() const {return minResponceTime == INT_MAX ? 0 : minResponceTime;}
    int getMaxResponseMillis() const {return maxResponceTime == INT_MIN ? 0 : maxResponceTime;}
    float getAvgResponseMillis() const {return sumResponceTime == 0 ? 0 : (float)sumResponceTime / (float)statsResponseTimes.count();}

    QString getState() {return state_str(state);}
    QString getStateText() {return stateText;}
    QString getStatusText() {return status;}
    QString getFaultTranslation();
    void playAudio();
    void playWarning(const char * resPath);

    // emit scales signals
    void scalesUpdated();
    void clearStats();

    // Setters
    void setState(const skypuff_state newState);
    void setStatus(const QString& mcuStatus);
    void setPos(const int new_pos);
    void setSpeed(const float new_erpm);
    void setPower(const float newMotorAmps, const float newBatteryAmps);
    void setTempFets(const float newTempFets);
    void setTempMotor(const float newTempMotor);
    void setFault(const mc_fault_code newFault);
    void setVBat(const float newVBat);

    void timerEvent(QTimerEvent *event) override;

    // Protocol
    void sendGetConf();
    void requestStats();
    void processStats(VByteArray &vb, bool temps);
    void processFault(VByteArray &vb);
    void processPullingTooHigh(VByteArray &vb);
    void processOutOfLimits(VByteArray &vb);
    void processUnwindedToOpposite(VByteArray &vb);
    void processUnwindedFromSlowing(VByteArray &vb);
    void processDetectingMotion(VByteArray &vb);
    void processTooSlowSpeedUp(VByteArray &vb);
    void processSettingsApplied(VByteArray &vb);
    void processZeroIsSet(VByteArray &vb);
    void processForceIsSet(VByteArray &vb);
    void processSettingsV1(VByteArray &vb);
    void processMsg(VByteArray &vb);
    void updateStatsResponse(const int millis);
};

#endif // SKYPUFF_H
