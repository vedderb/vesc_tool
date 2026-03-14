#ifndef SAMPLEDDATAHELPER_H
#define SAMPLEDDATAHELPER_H

#include <QObject>
#include <QVector>
#include <QVariantList>
#include <QTimer>
#include <QtQml/qqmlregistration.h>
#include "vescinterface.h"

class SampledDataHelper : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(bool hasData READ hasData NOTIFY dataReady)

public:
    explicit SampledDataHelper(QObject *parent = nullptr);

    Q_INVOKABLE void setVescInterface(VescInterface *vesc);

    // Sampling commands (mode values match debug_sampling_mode enum)
    Q_INVOKABLE void sampleNow(int samples, int decimation, bool raw);
    Q_INVOKABLE void sampleStart(int samples, int decimation, bool raw);
    Q_INVOKABLE void sampleTriggerStart(int samples, int decimation, bool raw);
    Q_INVOKABLE void sampleTriggerFault(int samples, int decimation, bool raw);
    Q_INVOKABLE void sampleTriggerStartNosend(int samples, int decimation, bool raw);
    Q_INVOKABLE void sampleTriggerFaultNosend(int samples, int decimation, bool raw);
    Q_INVOKABLE void sampleLast(int samples, int decimation, bool raw);
    Q_INVOKABLE void sampleStop(int samples, int decimation, bool raw);

    // Get processed plot data for current chart
    // Returns a list of objects: [{name, color, xData[], yData[]}]
    Q_INVOKABLE QVariantList getCurrentPlotData(
        int plotMode, // 0=time, 1=fft
        bool showI1, bool showI2, bool showI3, bool showMcTotal,
        bool showPosition, bool showPhase,
        int filterType, // 0=none, 1=fir, 2=mean
        double filterFreq, int filterTaps, bool compDelay, bool hamming,
        double fSampFreq, int decimation);

    // Get processed plot data for voltage (BEMF) chart
    Q_INVOKABLE QVariantList getVoltagePlotData(
        bool showPh1, bool showPh2, bool showPh3,
        bool showVGnd, bool showPosition, bool showPhaseV,
        bool truncate);

    // Get filter plot data: [{name, xData[], yData[]}]
    Q_INVOKABLE QVariantList getFilterPlotData(
        int filterType, double filterFreq, int filterTaps,
        bool useHamming, bool scatter, double fSampFreq);

    // Save/Load CSV
    Q_INVOKABLE bool saveCsv(const QString &path);
    Q_INVOKABLE bool loadCsv(const QString &path);

    double progress() const { return mProgress; }
    bool hasData() const { return !curr1Vector.isEmpty(); }

signals:
    void progressChanged();
    void dataReady();

private slots:
    void samplesReceived(QByteArray bytes);
    void sampleGetTimerSlot();

private:
    VescInterface *mVesc = nullptr;
    QTimer mSampleGetTimer;
    double mProgress = 0.0;

    // Final sample buffers
    QVector<double> curr1Vector;
    QVector<double> curr2Vector;
    QVector<double> curr3Vector;
    QVector<double> ph1Vector;
    QVector<double> ph2Vector;
    QVector<double> ph3Vector;
    QVector<double> vZeroVector;
    QVector<double> currTotVector;
    QVector<double> fSwVector;
    QByteArray statusArray;
    QByteArray phaseArray;

    // Temp buffers during reception
    QVector<int> tmpIndexVector;
    QVector<double> tmpCurr1Vector;
    QVector<double> tmpCurr2Vector;
    QVector<double> tmpCurr3Vector;
    QVector<double> tmpPh1Vector;
    QVector<double> tmpPh2Vector;
    QVector<double> tmpPh3Vector;
    QVector<double> tmpVZeroVector;
    QVector<double> tmpCurrTotVector;
    QVector<double> tmpFSwVector;
    QByteArray tmpStatusArray;
    QByteArray tmpPhaseArray;
    int tmpSampleCnt = 0;
    int tmpSampleRetryCnt = 0;
    int mSamplesToWait = 0;
    int mDecimation = 1;
    bool mRaw = false;

    void clearBuffers();

    QVariantList toJsArray(const QVector<double> &v);
    QVariantMap makeSeries(const QString &name, const QString &color,
                           const QVector<double> &x, const QVector<double> &y);
};

#endif // SAMPLEDDATAHELPER_H
