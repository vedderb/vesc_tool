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

#ifndef COMMANDS_H
#define COMMANDS_H

#include <QObject>
#include <QTimer>
#include <QMap>
#include <QVariant>
#include <QVariantList>
#include "datatypes.h"
#include "configparams.h"

class Commands : public QObject
{
    Q_OBJECT
public:
    explicit Commands(QObject *parent = nullptr);

    void setLimitedMode(bool is_limited);
    Q_INVOKABLE bool isLimitedMode();
    Q_INVOKABLE bool setSendCan(bool sendCan, int id = -1);
    Q_INVOKABLE bool getSendCan();
    Q_INVOKABLE void setCanSendId(unsigned int id);
    Q_INVOKABLE int getCanSendId();
    void setMcConfig(ConfigParams *mcConfig);
    void setAppConfig(ConfigParams *appConfig);
    void checkMcConfig();
    Q_INVOKABLE void emitEmptyValues();
    Q_INVOKABLE void emitEmptySetupValues();
    Q_INVOKABLE void emitEmptyStats();

    Q_INVOKABLE bool getLimitedSupportsFwdAllCan() const;
    void setLimitedSupportsFwdAllCan(bool limitedSupportsFwdAllCan);

    Q_INVOKABLE bool getLimitedSupportsEraseBootloader() const;
    void setLimitedSupportsEraseBootloader(bool limitedSupportsEraseBootloader);

    Q_INVOKABLE QVector<int> getLimitedCompatibilityCommands() const;
    void setLimitedCompatibilityCommands(QVector<int> compatibilityCommands);

    Q_INVOKABLE static QString faultToStr(mc_fault_code fault);

    Q_INVOKABLE QByteArray bmReadMemWait(uint32_t addr, quint16 size, int timeoutMs = 3000);
    Q_INVOKABLE int bmWriteMemWait(uint32_t addr, QByteArray data, int timeoutMs = 3000);

    Q_INVOKABLE void setOdometer(unsigned odometer_meters);

    Q_INVOKABLE int bmsGetCanDevNum();
    Q_INVOKABLE BMS_VALUES bmsGetCanValues(int can_id);
    Q_INVOKABLE bool bmsHasCanValues(int can_id);

    Q_INVOKABLE void emitPlotInit(QString xLabel, QString yLabel);
    Q_INVOKABLE void emitPlotData(double x, double y);
    Q_INVOKABLE void emitPlotAddGraph(QString name);
    Q_INVOKABLE void emitPlotSetGraph(int graph);

    Q_INVOKABLE bool getMaxPowerLossBug() const;
    void setMaxPowerLossBug(bool maxPowerLossBug);

    Q_INVOKABLE QVariantList fileBlockList(QString path);
    Q_INVOKABLE QByteArray fileBlockRead(QString path);
    Q_INVOKABLE bool fileBlockWrite(QString path, QByteArray data);
    Q_INVOKABLE bool fileBlockMkdir(QString path);
    Q_INVOKABLE bool fileBlockRemove(QString path);
    Q_INVOKABLE void fileBlockCancel();
    Q_INVOKABLE bool fileBlockDidCancel();

    Q_INVOKABLE double getFilePercentage() const;
    Q_INVOKABLE double getFileSpeed() const;

signals:
    void dataToSend(QByteArray &data);

    void fwVersionReceived(FW_RX_PARAMS params);
    void eraseNewAppResReceived(bool ok);
    void eraseBootloaderResReceived(bool ok);
    void writeNewAppDataResReceived(bool ok, bool hasOffset, quint32 offset);
    void ackReceived(QString ackType);
    void valuesReceived(MC_VALUES values, unsigned int mask);
    void printReceived(QString str);
    void samplesReceived(QByteArray bytes);
    void rotorPosReceived(double pos);
    void experimentSamplesReceived(QVector<double> samples);
    void bldcDetectReceived(bldc_detect param);
    void decodedPpmReceived(double value, double last_len);
    void decodedAdcReceived(double value, double voltage, double value2, double voltage2);
    void decodedChukReceived(double value);
    void motorRLReceived(double r, double l, double ld_lq_diff);
    void motorLinkageReceived(double flux_linkage);
    void encoderParamReceived(ENCODER_DETECT_RES res);
    void customAppDataReceived(QByteArray data);
    void customHwDataReceived(QByteArray data);
    void focHallTableReceived(QVector<int> hall_table, int res);
    void nrfPairingRes(int res);
    void mcConfigCheckResult(QStringList paramsNotSet);
    void mcConfigWriteSent(bool checkSet);
    void gpdBufferNotifyReceived();
    void gpdBufferSizeLeftReceived(int sizeLeft);
    void valuesSetupReceived(SETUP_VALUES values, unsigned int mask);
    void detectAllFocReceived(int result);
    void pingCanRx(QVector<int> devs, bool isTimeout);
    void valuesImuReceived(IMU_VALUES values, unsigned int mask);
    void imuCalibrationReceived(QVector<double> cal);
    void bmConnRes(int res);
    void bmEraseFlashAllRes(int res);
    void bmWriteFlashRes(int res);
    void bmRebootRes(int res);
    void bmMapPinsDefaultRes(bool ok);
    void bmMapPinsNrf5xRes(bool ok);
    void plotInitReceived(QString xLabel, QString yLabel);
    void plotDataReceived(double x, double y);
    void plotAddGraphReceived(QString name);
    void plotSetGraphReceived(int graph);
    void bmReadMemRes(int res, QByteArray data);
    void deserializeConfigFailed(bool isMc, bool isApp);
    void canFrameRx(QByteArray data, quint32 id, bool isExtended);
    void bmsValuesRx(BMS_VALUES val);
    void customConfigChunkRx(int confInd, int lenConf, int ofsConf, QByteArray data);
    void customConfigRx(int confInd, QByteArray data);
    void pswStatusRx(PSW_STATUS stat);
    void qmluiHwRx(int lenQml, int ofsQml, QByteArray data);
    void qmluiAppRx(int lenQml, int ofsQml, QByteArray data);
    void eraseQmluiResReceived(bool ok);
    void writeQmluiResReceived(bool ok, quint32 offset);
    void ioBoardValRx(IO_BOARD_VALUES val);
    void statsRx(STAT_VALUES val, unsigned int mask);
    void gnssRx(GNSS_DATA val, unsigned int mask);
    void lispReadCodeRx(int lenQml, int ofsQml, QByteArray data);
    void lispEraseCodeRx(bool ok);
    void lispWriteCodeRx(bool ok, quint32 offset);
    void lispPrintReceived(QString str);
    void lispStatsRx(LISP_STATS stats);
    void lispRunningResRx(bool ok);
    void lispStreamCodeRx(quint32 offset, qint16 res);

    void fileListRx(bool hasMore, QList<FILE_LIST_ENTRY> files);
    void fileReadRx(qint32 offset, qint32 size, QByteArray data);
    void fileWriteRx(qint32 offset, bool ok);
    void fileMkdirRx(bool ok);
    void fileRemoveRx(bool ok);
    void fileProgress(int32_t prog, int32_t tot, double percentage, double bytesPerSec);

    void logStart(int fieldNum, double rateHz, bool appendTime, bool appendGnss, bool appendGnssTime);
    void logStop();
    void logConfigField(int fieldInd, LOG_HEADER header);
    void logSamples(int fieldStart, QVector<double> samples);

public slots:
    void processPacket(QByteArray data);

    void getFwVersion();
    void eraseNewApp(bool fwdCan, quint32 fwSize, HW_TYPE hwType, QString hwName);
    void eraseBootloader(bool fwdCan, HW_TYPE hwType, QString hwName);
    void writeNewAppData(QByteArray data, quint32 offset, bool fwdCan, HW_TYPE hwType, QString hwName);
    void writeNewAppDataLzo(QByteArray data, quint32 offset, quint16 decompressedLen, bool fwdCan);
    void jumpToBootloader(bool fwdCan, HW_TYPE hwType, QString hwName);
    void getValues();
    void sendTerminalCmd(QString cmd);
    void sendTerminalCmdSync(QString cmd);
    void setDutyCycle(double dutyCycle);
    void setCurrent(double current);
    void setCurrentBrake(double current);
    void setRpm(int rpm);
    void setPos(double pos);
    void setHandbrake(double current);
    void setDetect(disp_pos_mode mode);
    void samplePrint(debug_sampling_mode mode, int sample_len, int dec, bool raw);
    void getMcconf();
    void getMcconfDefault();
    void setMcconf(bool check = true);
    void getAppConf();
    void getAppConfDefault();
    void setAppConf();
    void setAppConfNoStore();
    void detectMotorParam(double current, double min_rpm, double low_duty);
    void reboot();
    void sendAlive();
    void getDecodedPpm();
    void getDecodedAdc();
    void getDecodedChuk();
    void setServoPos(double pos);
    void measureRL();
    void measureLinkage(double current, double min_rpm, double low_duty, double resistance);
    void measureEncoder(double current);
    void measureHallFoc(double current);
    void sendCustomAppData(QByteArray data);
    void sendCustomAppData(unsigned char *data, unsigned int len);
    void sendCustomHwData(QByteArray data);
    void setChukData(chuck_data &data);
    void pairNrf(int ms);
    void gpdSetFsw(float fsw);
    void getGpdBufferSizeLeft();
    void gpdFillBuffer(QVector<float> samples);
    void gpdOutputSample(float sample);
    void gpdSetMode(gpd_output_mode mode);
    void gpdFillBufferInt8(QVector<qint8> samples);
    void gpdFillBufferInt16(QVector<qint16> samples);
    void gpdSetBufferIntScale(float scale);
    void getValuesSetup();
    void setMcconfTemp(const MCCONF_TEMP &conf, bool is_setup, bool store,
                       bool forward_can, bool divide_by_controllers, bool ack);
    void getValuesSelective(unsigned int mask);
    void getValuesSetupSelective(unsigned int mask);
    void measureLinkageOpenloop(double current, double erpm_per_sec, double low_duty,
                                double resistance, double inductance);
    void detectAllFoc(bool detect_can, double max_power_loss, double min_current_in,
                      double max_current_in, double openloop_rpm, double sl_erpm);
    void pingCan();
    void disableAppOutput(int time_ms, bool fwdCan);
    void getImuData(unsigned int mask);
    void getImuCalibration(double yaw);
    void bmConnect();
    void bmEraseFlashAll();
    void bmWriteFlash(uint32_t addr, QByteArray data);
    void bmWriteFlashLzo(uint32_t addr, quint16 decompressedLen, QByteArray data);
    void bmReboot();
    void bmDisconnect();
    void bmMapPinsDefault();
    void bmMapPinsNrf5x();
    void bmReadMem(uint32_t addr, quint16 size);
    void setCurrentRel(double current);
    void forwardCanFrame(QByteArray data, quint32 id, bool isExtended);
    void setBatteryCut(double start, double end, bool store, bool fwdCan);

    void bmsGetValues();
    void bmsSetChargeAllowed(bool allowed);
    void bmsSetBalanceOverride(uint8_t cell, uint8_t override);
    void bmsResetCounters(bool ah, bool wh);
    void bmsForceBalance(bool bal_en);
    void bmsZeroCurrentOffset();

    void customConfigGetChunk(int confInd, int len, int offset);
    void customConfigGet(int confInd, bool isDefault);
    void customConfigSet(int confInd, ConfigParams *conf);

    void pswGetStatus(bool by_id, int id_ind);
    void pswSwitch(int id, bool is_on, bool plot);

    void qmlUiHwGet(int len, int offset);
    void qmlUiAppGet(int len, int offset);
    void qmlUiErase(int size);
    void qmlUiWrite(QByteArray data, quint32 offset);

    void ioBoardGetAll(int id);
    void ioBoardSetPwm(int id, int channel, double duty);
    void ioBoardSetDigital(int id, int channel, bool on);

    void getStats(unsigned int mask);
    void resetStats(bool sendAck);

    void getGnss(unsigned int mask);

    void lispReadCode(int len, int offset);
    void lispWriteCode(QByteArray data, quint32 offset);
    void lispStreamCode(QByteArray data, quint32 offset, quint32 totLen, qint8 mode);
    void lispEraseCode(int size);
    void lispSetRunning(bool running);
    void lispGetStats(bool all);
    void lispSendReplCmd(QString str);

    void setBleName(QString name);
    void setBlePin(QString pin);

    void fileList(QString path, QString from);
    void fileRead(QString path, qint32 offset);
    void fileWrite(QString path, qint32 offset, qint32 size, QByteArray data);
    void fileMkdir(QString path);
    void fileRemove(QString path);

private slots:
    void timerSlot();

private:
    void emitData(QByteArray data);

    QTimer *mTimer;
    bool mSendCan;
    int mCanId;
    bool mIsLimitedMode;
    bool mLimitedSupportsFwdAllCan;
    bool mLimitedSupportsEraseBootloader;
    bool mMaxPowerLossBug;
    QVector<int> mCompatibilityCommands; // int to be QML-compatible
    QMap<int, BMS_VALUES> mBmsValues;

    ConfigParams *mMcConfig;
    ConfigParams *mAppConfig;
    ConfigParams mMcConfigLast;
    bool mCheckNextMcConfig;

    int mTimeoutCount;
    int mTimeoutFwVer;
    int mTimeoutMcconf;
    int mTimeoutAppconf;
    int mTimeoutValues;
    int mTimeoutValuesSetup;
    int mTimeoutImuData;
    int mTimeoutDecPpm;
    int mTimeoutDecAdc;
    int mTimeoutDecChuk;
    int mTimeoutPingCan;
    int mTimeoutCustomConf;
    int mTimeoutBmsVal;
    int mTimeoutStats;

    double mFilePercentage;
    double mFileSpeed;
    bool mFileShouldCancel;

};

#endif // COMMANDS_H
