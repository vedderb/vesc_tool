#include "sampleddatahelper.h"
#include "digitalfiltering.h"
#include "vbytearray.h"
#include "utility.h"
#include <QFile>
#include <QTextStream>
#include <QFileInfo>

SampledDataHelper::SampledDataHelper(QObject *parent)
    : QObject(parent)
{
    connect(&mSampleGetTimer, &QTimer::timeout, this, &SampledDataHelper::sampleGetTimerSlot);
}

void SampledDataHelper::setVescInterface(VescInterface *vesc)
{
    if (mVesc) {
        disconnect(mVesc->commands(), &Commands::samplesReceived,
                   this, &SampledDataHelper::samplesReceived);
    }
    mVesc = vesc;
    if (mVesc) {
        connect(mVesc->commands(), &Commands::samplesReceived,
                this, &SampledDataHelper::samplesReceived);
        connect(mVesc->commands(), &Commands::sampleDataQmlStarted, this, [this](int samples) {
            clearBuffers();
            mSamplesToWait = samples;
        });
    }
}

// --- Sampling commands ---

void SampledDataHelper::sampleNow(int samples, int decimation, bool raw)
{
    if (!mVesc) return;
    clearBuffers();
    mDecimation = decimation;
    mRaw = raw;
    mVesc->commands()->samplePrint(DEBUG_SAMPLING_NOW, samples, decimation, raw);
    mSamplesToWait = samples;
}

void SampledDataHelper::sampleStart(int samples, int decimation, bool raw)
{
    if (!mVesc) return;
    clearBuffers();
    mDecimation = decimation;
    mRaw = raw;
    mVesc->commands()->samplePrint(DEBUG_SAMPLING_START, samples, decimation, raw);
    mSamplesToWait = samples;
}

void SampledDataHelper::sampleTriggerStart(int samples, int decimation, bool raw)
{
    if (!mVesc) return;
    clearBuffers();
    mDecimation = decimation;
    mRaw = raw;
    mVesc->commands()->samplePrint(DEBUG_SAMPLING_TRIGGER_START, samples, decimation, raw);
    mSamplesToWait = 1000; // max
}

void SampledDataHelper::sampleTriggerFault(int samples, int decimation, bool raw)
{
    if (!mVesc) return;
    clearBuffers();
    mDecimation = decimation;
    mRaw = raw;
    mVesc->commands()->samplePrint(DEBUG_SAMPLING_TRIGGER_FAULT, samples, decimation, raw);
    mSamplesToWait = 1000;
}

void SampledDataHelper::sampleTriggerStartNosend(int samples, int decimation, bool raw)
{
    if (!mVesc) return;
    clearBuffers();
    mDecimation = decimation;
    mRaw = raw;
    mVesc->commands()->samplePrint(DEBUG_SAMPLING_TRIGGER_START_NOSEND, samples, decimation, raw);
    mSamplesToWait = 1000;
}

void SampledDataHelper::sampleTriggerFaultNosend(int samples, int decimation, bool raw)
{
    if (!mVesc) return;
    clearBuffers();
    mDecimation = decimation;
    mRaw = raw;
    mVesc->commands()->samplePrint(DEBUG_SAMPLING_TRIGGER_FAULT_NOSEND, samples, decimation, raw);
    mSamplesToWait = 1000;
}

void SampledDataHelper::sampleLast(int samples, int decimation, bool raw)
{
    if (!mVesc) return;
    clearBuffers();
    mDecimation = decimation;
    mRaw = raw;
    mVesc->commands()->samplePrint(DEBUG_SAMPLING_SEND_LAST_SAMPLES, samples, decimation, raw);
    mSamplesToWait = 1000;
}

void SampledDataHelper::sampleStop(int samples, int decimation, bool raw)
{
    mSampleGetTimer.stop();
    if (!mVesc) return;
    mVesc->commands()->samplePrint(DEBUG_SAMPLING_OFF, samples, decimation, raw);
}

// --- Sample reception (binary protocol parsing) ---

void SampledDataHelper::samplesReceived(QByteArray bytes)
{
    VByteArray vb(bytes);

    bool supportsSendSingleSample = false;
    int sampleIndex = tmpIndexVector.size();
    if (vb.size() >= 44) {
        sampleIndex = vb.vbPopFrontInt16();
        supportsSendSingleSample = true;
    }

    // Pad vectors with zeroes for missing samples
    while (tmpIndexVector.size() <= sampleIndex) {
        tmpIndexVector.append(-1);
        tmpCurr1Vector.append(0.0);
        tmpCurr2Vector.append(0.0);
        tmpCurr3Vector.append(0.0);
        tmpPh1Vector.append(0.0);
        tmpPh2Vector.append(0.0);
        tmpPh3Vector.append(0.0);
        tmpVZeroVector.append(0.0);
        tmpCurrTotVector.append(0.0);
        if (!tmpFSwVector.isEmpty()) {
            tmpFSwVector.append(tmpFSwVector.last());
        } else {
            tmpFSwVector.append(0.0);
        }
        tmpStatusArray.append(char(0));
        tmpPhaseArray.append(char(0));
    }

    if (tmpIndexVector[sampleIndex] == -1) {
        tmpSampleCnt++;
    }

    tmpIndexVector[sampleIndex] = sampleIndex;
    tmpCurr1Vector[sampleIndex] = vb.vbPopFrontDouble32Auto();
    tmpCurr2Vector[sampleIndex] = vb.vbPopFrontDouble32Auto();

    if (vb.size() >= 30) {
        tmpCurr3Vector[sampleIndex] = vb.vbPopFrontDouble32Auto();
    } else {
        tmpCurr3Vector[sampleIndex] = -(tmpCurr1Vector[sampleIndex] + tmpCurr2Vector[sampleIndex]);
    }

    tmpPh1Vector[sampleIndex] = vb.vbPopFrontDouble32Auto();
    tmpPh2Vector[sampleIndex] = vb.vbPopFrontDouble32Auto();
    tmpPh3Vector[sampleIndex] = vb.vbPopFrontDouble32Auto();
    tmpVZeroVector[sampleIndex] = vb.vbPopFrontDouble32Auto();
    tmpCurrTotVector[sampleIndex] = vb.vbPopFrontDouble32Auto();
    tmpFSwVector[sampleIndex] = vb.vbPopFrontDouble32Auto();
    tmpStatusArray[sampleIndex] = vb.vbPopFrontInt8();
    tmpPhaseArray[sampleIndex] = vb.vbPopFrontInt8();

    double prog = double(tmpCurr1Vector.size()) / double(qMax(mSamplesToWait, 1));
    mProgress = qMin(prog, 1.0);
    emit progressChanged();

    if (supportsSendSingleSample) {
        mSampleGetTimer.start(1000);
        tmpSampleRetryCnt = 0;
    } else {
        tmpSampleCnt = tmpIndexVector.size();
    }

    if (tmpSampleCnt >= mSamplesToWait && mSamplesToWait > 0) {
        mSampleGetTimer.stop();

        curr1Vector = tmpCurr1Vector;
        curr2Vector = tmpCurr2Vector;
        curr3Vector = tmpCurr3Vector;
        ph1Vector = tmpPh1Vector;
        ph2Vector = tmpPh2Vector;
        ph3Vector = tmpPh3Vector;
        vZeroVector = tmpVZeroVector;
        currTotVector = tmpCurrTotVector;
        fSwVector = tmpFSwVector;
        statusArray = tmpStatusArray;
        phaseArray = tmpPhaseArray;

        mProgress = 1.0;
        emit progressChanged();
        emit dataReady();
    }
}

void SampledDataHelper::sampleGetTimerSlot()
{
    tmpSampleRetryCnt++;
    if (tmpSampleRetryCnt == 3) {
        if (mVesc) {
            mVesc->emitMessageDialog(tr("Sampled Data"),
                                     tr("Getting samples timed out"), false, false);
        }
        mSampleGetTimer.stop();
        return;
    }

    for (int i = 0; i < mSamplesToWait; i++) {
        if (tmpIndexVector.size() <= i || tmpIndexVector.at(i) == -1) {
            mVesc->commands()->samplePrint(DEBUG_SAMPLING_SEND_SINGLE_SAMPLE, i,
                                           mDecimation, mRaw);
        }
    }
}

// --- Plot data generation ---

QVariantList SampledDataHelper::getCurrentPlotData(
    int plotMode, bool showI1, bool showI2, bool showI3, bool showMcTotal,
    bool showPosition, bool showPhase,
    int filterType, double filterFreq, int filterTaps, bool compDelay, bool useHamming,
    double fSampFreq, int decimation)
{
    QVariantList result;
    int size = curr1Vector.size();
    if (size == 0) return result;

    const double f_samp = fSampFreq / decimation;

    QVector<double> position(size);
    for (int i = 0; i < size; i++) {
        position[i] = double((quint8)statusArray.at(i) & 7);
    }

    QVector<double> phase(size);
    for (int i = 0; i < size; i++) {
        phase[i] = double((quint8)phaseArray.at(i)) / 250.0 * 360.0;
    }

    QVector<double> curr1 = curr1Vector;
    QVector<double> curr2 = curr2Vector;
    QVector<double> curr3 = curr3Vector;
    QVector<double> totCurrentMc = currTotVector;
    QVector<double> fSw = fSwVector;

    // Generate filter
    QVector<double> filter;
    if (filterType == 2) { // Mean
        int fLen = (1 << filterTaps);
        for (int i = 0; i < fLen; i++) filter.append(1.0 / double(fLen));
    } else if (filterType == 1) { // FIR
        filter = DigitalFiltering::generateFirFilter(filterFreq, filterTaps, useHamming);
    }

    // Apply filter
    if (filterType > 0 && !filter.isEmpty()) {
        curr1 = DigitalFiltering::filterSignal(curr1, filter, compDelay);
        curr2 = DigitalFiltering::filterSignal(curr2, filter, compDelay);
        curr3 = DigitalFiltering::filterSignal(curr3, filter, compDelay);
    }

    bool showFft = (plotMode == 1);

    QVector<double> xAxisCurrDec;
    QVector<double> xAxisCurr;

    if (showFft) {
        int fftBits = 16;
        curr1 = DigitalFiltering::fftWithShift(curr1, fftBits, true);
        curr2 = DigitalFiltering::fftWithShift(curr2, fftBits, true);
        curr3 = DigitalFiltering::fftWithShift(curr3, fftBits, true);
        totCurrentMc = DigitalFiltering::fftWithShift(totCurrentMc, fftBits, true);

        curr1.resize(curr1.size() / 2);
        curr2.resize(curr2.size() / 2);
        curr3.resize(curr3.size() / 2);
        totCurrentMc.resize(totCurrentMc.size() / 2);

        xAxisCurrDec.resize(curr1.size());
        xAxisCurr.resize(totCurrentMc.size());

        for (int i = 0; i < xAxisCurrDec.size(); i++)
            xAxisCurrDec[i] = (double(i) / double(xAxisCurrDec.size())) * (f_samp / 2.0);
        for (int i = 0; i < xAxisCurr.size(); i++)
            xAxisCurr[i] = (double(i) / double(xAxisCurr.size())) * (f_samp / 2.0);
    } else {
        xAxisCurrDec.resize(curr1.size());
        xAxisCurr.resize(totCurrentMc.size());

        double prev_x = 0.0;
        double rat = double(fSw.size()) / double(qMax(xAxisCurrDec.size(), 1));
        for (int i = 0; i < xAxisCurrDec.size(); i++) {
            xAxisCurrDec[i] = prev_x;
            prev_x += 1.0 / fSw[int(double(i) * rat)];
        }

        prev_x = 0.0;
        rat = double(fSw.size()) / double(qMax(xAxisCurr.size(), 1));
        for (int i = 0; i < xAxisCurr.size(); i++) {
            xAxisCurr[i] = prev_x;
            prev_x += 1.0 / fSw[int(double(i) * rat)];
        }
    }

    QVector<double> xAxisVolt(ph1Vector.size());
    double prev_x = 0.0;
    for (int i = 0; i < xAxisVolt.size(); i++) {
        xAxisVolt[i] = prev_x;
        prev_x += 1.0 / fSw[i];
    }

    if (showI1)
        result.append(makeSeries("Phase 1 Current",
                                 Utility::getAppHexColor("plot_graph3"), xAxisCurrDec, curr1));
    if (showI2)
        result.append(makeSeries("Phase 2 Current",
                                 Utility::getAppHexColor("plot_graph4"), xAxisCurrDec, curr2));
    if (showI3)
        result.append(makeSeries("Phase 3 Current",
                                 Utility::getAppHexColor("plot_graph5"), xAxisCurrDec, curr3));
    if (showMcTotal)
        result.append(makeSeries("Total current filtered by MC",
                                 Utility::getAppHexColor("plot_graph6"), xAxisCurr, totCurrentMc));
    if (showPosition && !showFft)
        result.append(makeSeries("Current position",
                                 Utility::getAppHexColor("plot_graph1"), xAxisCurr, position));
    if (showPhase)
        result.append(makeSeries("FOC motor phase",
                                 Utility::getAppHexColor("plot_graph7"), xAxisVolt, phase));

    return result;
}

QVariantList SampledDataHelper::getVoltagePlotData(
    bool showPh1, bool showPh2, bool showPh3,
    bool showVGnd, bool showPosition, bool showPhaseV, bool truncate)
{
    QVariantList result;
    int size = ph1Vector.size();
    if (size == 0) return result;

    QVector<double> position(size);
    QVector<double> position_hall(size);
    for (int i = 0; i < size; i++) {
        position[i] = double((quint8)statusArray.at(i) & 7);
        position_hall[i] = double(((quint8)(statusArray.at(i) >> 3)) & 7) / 1.0;
    }

    QVector<double> phase(size);
    for (int i = 0; i < size; i++) {
        phase[i] = double((quint8)phaseArray.at(i)) / 250.0 * 360.0;
    }

    QVector<double> ph1 = ph1Vector;
    QVector<double> ph2 = ph2Vector;
    QVector<double> ph3 = ph3Vector;
    QVector<double> vZero = vZeroVector;
    QVector<double> fSw = fSwVector;

    if (truncate) {
        for (int i = 0; i < ph1.size(); i++) {
            if (!(position[i] == 1 || position[i] == 4)) ph1[i] = 0;
            if (!(position[i] == 2 || position[i] == 5)) ph2[i] = 0;
            if (!(position[i] == 3 || position[i] == 6)) ph3[i] = 0;
        }
    }

    QVector<double> xAxis(size);
    double prev_x = 0.0;
    for (int i = 0; i < size; i++) {
        xAxis[i] = prev_x;
        prev_x += 1.0 / fSw[i];
    }

    if (showPh1)
        result.append(makeSeries("Phase 1 voltage",
                                 Utility::getAppHexColor("plot_graph3"), xAxis, ph1));
    if (showPh2)
        result.append(makeSeries("Phase 2 voltage",
                                 Utility::getAppHexColor("plot_graph4"), xAxis, ph2));
    if (showPh3)
        result.append(makeSeries("Phase 3 voltage",
                                 Utility::getAppHexColor("plot_graph5"), xAxis, ph3));
    if (showVGnd)
        result.append(makeSeries("Virtual ground",
                                 Utility::getAppHexColor("plot_graph6"), xAxis, vZero));
    if (showPosition) {
        result.append(makeSeries("Current position",
                                 Utility::getAppHexColor("plot_graph1"), xAxis, position));
        result.append(makeSeries("Hall position",
                                 Utility::getAppHexColor("plot_graph2"), xAxis, position_hall));
    }
    if (showPhaseV)
        result.append(makeSeries("FOC motor phase",
                                 Utility::getAppHexColor("plot_graph7"), xAxis, phase));

    return result;
}

QVariantList SampledDataHelper::getFilterPlotData(
    int filterType, double filterFreq, int filterTaps,
    bool useHamming, bool scatter, double fSampFreq)
{
    Q_UNUSED(scatter)
    QVariantList result;

    QVector<double> filter;
    if (filterType == 2) {
        int fLen = (1 << filterTaps);
        for (int i = 0; i < fLen; i++) filter.append(1.0 / double(fLen));
    } else {
        filter = DigitalFiltering::generateFirFilter(filterFreq, filterTaps, useHamming);
    }

    // Filter coefficients
    QVector<double> filterIndex(filter.size());
    for (int i = 0; i < filter.size(); i++) filterIndex[i] = double(i);

    QVariantMap filterSeries;
    filterSeries["name"] = "Filter";
    filterSeries["color"] = Utility::getAppHexColor("plot_graph1");
    filterSeries["xData"] = toJsArray(filterIndex);
    filterSeries["yData"] = toJsArray(filter);
    result.append(filterSeries);

    // Frequency response
    QVector<double> response = DigitalFiltering::fftWithShift(filter, filterTaps + 4);
    response.resize(response.size() / 2);

    QVector<double> freqIndex(response.size());
    for (int i = 0; i < response.size(); i++) {
        freqIndex[i] = (double(i) / double(response.size())) * fSampFreq / 2.0;
    }

    QVariantMap responseSeries;
    responseSeries["name"] = "Filter Response";
    responseSeries["color"] = Utility::getAppHexColor("plot_graph1");
    responseSeries["xData"] = toJsArray(freqIndex);
    responseSeries["yData"] = toJsArray(response);
    result.append(responseSeries);

    return result;
}

// --- CSV Save/Load ---

bool SampledDataHelper::saveCsv(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return false;

    QTextStream stream(&file);

    // Generate time axis from fSw
    QVector<double> timeVec(fSwVector.size());
    double prev_t = 0.0;
    for (int i = 0; i < timeVec.size(); i++) {
        timeVec[i] = prev_t;
        prev_t += 1.0 / fSwVector[i];
    }

    stream << "T;I1;I2;I3;V1;V2;V3;I_tot;V_zero;Phase\n";

    for (int i = 0; i < curr1Vector.size(); i++) {
        stream << timeVec.at(i) << ";";
        stream << curr1Vector.at(i) << ";";
        stream << curr2Vector.at(i) << ";";
        stream << curr3Vector.at(i) << ";";
        stream << ph1Vector.at(i) << ";";
        stream << ph2Vector.at(i) << ";";
        stream << ph3Vector.at(i) << ";";
        stream << currTotVector.at(i) << ";";
        stream << vZeroVector.at(i) << ";";
        stream << double((quint8)phaseArray.at(i)) / 250.0 * 360.0;
        if (i < curr1Vector.size() - 1) stream << "\n";
    }

    file.close();
    return true;
}

bool SampledDataHelper::loadCsv(const QString &path)
{
    QFile inFile(path);
    if (!inFile.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

    QByteArray data = inFile.readAll();
    inFile.close();
    QTextStream in(&data);

    auto tokensLine1 = in.readLine().split(";");
    if (tokensLine1.size() < 1) return false;

    fSwVector.clear(); curr1Vector.clear(); curr2Vector.clear(); curr3Vector.clear();
    ph1Vector.clear(); ph2Vector.clear(); ph3Vector.clear();
    currTotVector.clear(); vZeroVector.clear(); phaseArray.clear(); statusArray.clear();

    int indT = -1, indI1 = -1, indI2 = -1, indI3 = -1;
    int indV1 = -1, indV2 = -1, indV3 = -1;
    int indI_tot = -1, indV_zero = -1, indPhase = -1;

    for (int i = 0; i < tokensLine1.size(); i++) {
        QString token = tokensLine1.at(i).toLower().replace(" ", "");
        if (token == "t") indT = i;
        else if (token == "i1") indI1 = i;
        else if (token == "i2") indI2 = i;
        else if (token == "i3") indI3 = i;
        else if (token == "v1") indV1 = i;
        else if (token == "v2") indV2 = i;
        else if (token == "v3") indV3 = i;
        else if (token == "i_tot") indI_tot = i;
        else if (token == "v_zero") indV_zero = i;
        else if (token == "phase") indPhase = i;
    }

    double tLast = -1.0;
    bool tLastSet = false;

    while (!in.atEnd()) {
        QStringList tokens = in.readLine().split(";");

        if (indT >= 0 && tokens.size() > indT) {
            double tNow = tokens.at(indT).toDouble();
            if (tLastSet) fSwVector.append(1.0 / (tNow - tLast));
            tLast = tNow;
            tLastSet = true;
        } else {
            fSwVector.append(15000.0);
        }

        curr1Vector.append((indI1 >= 0 && tokens.size() > indI1) ? tokens.at(indI1).toDouble() : 0.0);
        curr2Vector.append((indI2 >= 0 && tokens.size() > indI2) ? tokens.at(indI2).toDouble() : 0.0);
        curr3Vector.append((indI3 >= 0 && tokens.size() > indI3) ? tokens.at(indI3).toDouble() : 0.0);
        ph1Vector.append((indV1 >= 0 && tokens.size() > indV1) ? tokens.at(indV1).toDouble() : 0.0);
        ph2Vector.append((indV2 >= 0 && tokens.size() > indV2) ? tokens.at(indV2).toDouble() : 0.0);
        ph3Vector.append((indV3 >= 0 && tokens.size() > indV3) ? tokens.at(indV3).toDouble() : 0.0);
        currTotVector.append((indI_tot >= 0 && tokens.size() > indI_tot) ? tokens.at(indI_tot).toDouble() : 0.0);
        vZeroVector.append((indV_zero >= 0 && tokens.size() > indV_zero) ? tokens.at(indV_zero).toDouble() : 0.0);
        phaseArray.append((indPhase >= 0 && tokens.size() > indPhase)
                              ? quint8(tokens.at(indPhase).toDouble() / 360.0 * 250.0) : 0);
        statusArray.append(char(0));
    }

    if (fSwVector.size() < curr1Vector.size() && !fSwVector.isEmpty()) {
        fSwVector.append(fSwVector.last());
    }

    mProgress = 1.0;
    emit progressChanged();
    emit dataReady();
    return true;
}

// --- Helpers ---

void SampledDataHelper::clearBuffers()
{
    mSampleGetTimer.stop();
    tmpSampleCnt = 0;
    tmpSampleRetryCnt = 0;
    tmpIndexVector.clear();
    tmpCurr1Vector.clear(); tmpCurr2Vector.clear(); tmpCurr3Vector.clear();
    tmpPh1Vector.clear(); tmpPh2Vector.clear(); tmpPh3Vector.clear();
    tmpVZeroVector.clear(); tmpCurrTotVector.clear(); tmpFSwVector.clear();
    tmpStatusArray.clear(); tmpPhaseArray.clear();

    mProgress = 0.0;
    emit progressChanged();
}

QVariantList SampledDataHelper::toJsArray(const QVector<double> &v)
{
    QVariantList list;
    list.reserve(v.size());
    for (double d : v) list.append(d);
    return list;
}

QVariantMap SampledDataHelper::makeSeries(const QString &name, const QString &color,
                                          const QVector<double> &x, const QVector<double> &y)
{
    QVariantMap m;
    m["name"] = name;
    m["color"] = color;
    m["xData"] = toJsArray(x);
    m["yData"] = toJsArray(y);
    return m;
}
