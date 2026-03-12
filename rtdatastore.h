/*
    Copyright 2026 Benjamin Vedder benjamin@vedder.se

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

#ifndef RTDATASTORE_H
#define RTDATASTORE_H

#include <QObject>
#include <QPointF>
#include <QVector>
#include <QVariantList>
#include <QElapsedTimer>
#include <QString>

#include "datatypes.h"

class Commands;

/**
 * @brief Persistent store for streaming plot data.
 *
 * Connects to Commands signals and maintains rolling buffers for:
 *   - Real-time values (motor current, battery current, duty, RPM, temps)
 *   - IMU data (roll, pitch, yaw, accel, gyro)
 *   - Experiment plot data (dynamic graph array from Lisp/scripting)
 *
 * QML pages read existing data on creation (bulk load) and connect to
 * incremental signals for live updates.  Data survives StackView page
 * destruction/recreation.
 */
class RtDataStore : public QObject
{
    Q_OBJECT

    // ---- RT Data properties ----
    Q_PROPERTY(int rtSampleIndex READ rtSampleIndex NOTIFY rtDataAppended)
    Q_PROPERTY(int rtMaxSamples  READ rtMaxSamples  WRITE setRtMaxSamples NOTIFY rtMaxSamplesChanged)

    // ---- IMU properties ----
    Q_PROPERTY(double imuSecondCounter READ imuSecondCounter NOTIFY imuDataAppended)
    Q_PROPERTY(int    imuMaxSamples    READ imuMaxSamples    WRITE setImuMaxSamples NOTIFY imuMaxSamplesChanged)

    // ---- Experiment Plot properties ----
    Q_PROPERTY(int     plotGraphCount   READ plotGraphCount   NOTIFY plotGraphCountChanged)
    Q_PROPERTY(int     plotCurrentGraph READ plotCurrentGraph NOTIFY plotCurrentGraphChanged)
    Q_PROPERTY(QString plotXLabel       READ plotXLabel       NOTIFY plotInitialized)
    Q_PROPERTY(QString plotYLabel       READ plotYLabel       NOTIFY plotInitialized)

public:
    explicit RtDataStore(Commands *commands, QObject *parent = nullptr);

    // ----------------------------------------------------------------
    // RT Data – 6 named series keyed by string
    // ----------------------------------------------------------------
    int rtSampleIndex() const { return m_rtSampleIndex; }
    int rtMaxSamples()  const { return m_rtMaxSamples; }
    void setRtMaxSamples(int max);

    /// Return all stored points for the given RT series.
    /// Valid names: "motorCurrent", "batteryCurrent", "duty", "rpm",
    ///              "tempMosfet", "tempMotor"
    Q_INVOKABLE QVariantList rtSeriesPoints(const QString &name) const;

    // ----------------------------------------------------------------
    // IMU Data – 9 named series
    // ----------------------------------------------------------------
    double imuSecondCounter() const { return m_imuSecondCounter; }
    int    imuMaxSamples()    const { return m_imuMaxSamples; }
    void   setImuMaxSamples(int max);

    /// Valid names: "roll", "pitch", "yaw",
    ///              "accX", "accY", "accZ",
    ///              "gyroX", "gyroY", "gyroZ"
    Q_INVOKABLE QVariantList imuSeriesPoints(const QString &name) const;

    // ----------------------------------------------------------------
    // Experiment Plot Data – dynamic graph array
    // ----------------------------------------------------------------
    int     plotGraphCount()   const { return m_plotGraphs.size(); }
    int     plotCurrentGraph() const { return m_plotCurrentGraph; }
    QString plotXLabel()       const { return m_plotXLabel; }
    QString plotYLabel()       const { return m_plotYLabel; }

    Q_INVOKABLE QVariantList plotGraphPoints(int index) const;
    Q_INVOKABLE QString      plotGraphLabel(int index)  const;
    Q_INVOKABLE QString      plotGraphColor(int index)  const;
    Q_INVOKABLE void         setPlotHistoryMax(int max);

signals:
    // ---- RT Data ----
    void rtDataAppended(int sampleIdx,
                        double motorI, double batteryI, double duty,
                        double rpm, double tempMos, double tempMotor);
    void rtMaxSamplesChanged();

    // ---- IMU ----
    void imuDataAppended(double time,
                         double roll, double pitch, double yaw,
                         double accX, double accY, double accZ,
                         double gyroX, double gyroY, double gyroZ);
    void imuMaxSamplesChanged();

    // ---- Experiment Plot ----
    void plotInitialized(QString xLabel, QString yLabel);
    void plotGraphAdded(int index, QString name, QString color);
    void plotCurrentGraphChanged(int graph);
    void plotPointAdded(int graphIndex, double x, double y);
    void plotGraphCountChanged();

private slots:
    void onValuesReceived(MC_VALUES values, unsigned int mask);
    void onValuesImuReceived(IMU_VALUES values, unsigned int mask);
    void onPlotInitReceived(QString xLabel, QString yLabel);
    void onPlotDataReceived(double x, double y);
    void onPlotAddGraphReceived(QString name);
    void onPlotSetGraphReceived(int graph);

private:
    // ---- RT rolling buffers ----
    QVector<QPointF> m_motorCurrent;
    QVector<QPointF> m_batteryCurrent;
    QVector<QPointF> m_duty;
    QVector<QPointF> m_rpm;
    QVector<QPointF> m_tempMosfet;
    QVector<QPointF> m_tempMotor;
    int m_rtSampleIndex  = 0;
    int m_rtMaxSamples   = 500;
    int m_rtTrimExtra    = 100; // keep N extra before trimming

    // ---- IMU rolling buffers ----
    QVector<QPointF> m_imuRoll;
    QVector<QPointF> m_imuPitch;
    QVector<QPointF> m_imuYaw;
    QVector<QPointF> m_imuAccX;
    QVector<QPointF> m_imuAccY;
    QVector<QPointF> m_imuAccZ;
    QVector<QPointF> m_imuGyroX;
    QVector<QPointF> m_imuGyroY;
    QVector<QPointF> m_imuGyroZ;
    double m_imuSecondCounter = 0.0;
    int    m_imuMaxSamples    = 500;
    QElapsedTimer m_imuTimer;
    bool m_imuTimerStarted = false;

    // ---- Experiment plot ----
    struct PlotGraph {
        QString label;
        QString color;
        QVector<QPointF> points;
    };
    QVector<PlotGraph> m_plotGraphs;
    int     m_plotCurrentGraph = 0;
    QString m_plotXLabel;
    QString m_plotYLabel;
    int     m_plotHistoryMax = 2000;

    // Graph colors (same order as original QML)
    static QStringList graphColors();

    // Helpers
    void trimVector(QVector<QPointF> &vec, int maxSize, int extra);
    QVariantList toVariantList(const QVector<QPointF> &pts) const;
};

#endif // RTDATASTORE_H
