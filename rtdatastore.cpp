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

#include "rtdatastore.h"
#include "commands.h"
#include "utility.h"

#include <QtMath>

RtDataStore::RtDataStore(Commands *commands, QObject *parent)
    : QObject(parent)
{
    connect(commands, &Commands::valuesReceived,
            this, &RtDataStore::onValuesReceived);
    connect(commands, &Commands::valuesImuReceived,
            this, &RtDataStore::onValuesImuReceived);
    connect(commands, &Commands::plotInitReceived,
            this, &RtDataStore::onPlotInitReceived);
    connect(commands, &Commands::plotDataReceived,
            this, &RtDataStore::onPlotDataReceived);
    connect(commands, &Commands::plotAddGraphReceived,
            this, &RtDataStore::onPlotAddGraphReceived);
    connect(commands, &Commands::plotSetGraphReceived,
            this, &RtDataStore::onPlotSetGraphReceived);
}

// ---------------------------------------------------------------------------
// RT Data
// ---------------------------------------------------------------------------

void RtDataStore::setRtMaxSamples(int max)
{
    if (max != m_rtMaxSamples) {
        m_rtMaxSamples = max;
        emit rtMaxSamplesChanged();
    }
}

QVariantList RtDataStore::rtSeriesPoints(const QString &name) const
{
    if (name == "motorCurrent")   return toVariantList(m_motorCurrent);
    if (name == "batteryCurrent") return toVariantList(m_batteryCurrent);
    if (name == "duty")           return toVariantList(m_duty);
    if (name == "rpm")            return toVariantList(m_rpm);
    if (name == "tempMosfet")     return toVariantList(m_tempMosfet);
    if (name == "tempMotor")      return toVariantList(m_tempMotor);
    return {};
}

void RtDataStore::onValuesReceived(MC_VALUES values, unsigned int mask)
{
    Q_UNUSED(mask)

    double idx = static_cast<double>(m_rtSampleIndex);

    m_motorCurrent.append(QPointF(idx, values.current_motor));
    m_batteryCurrent.append(QPointF(idx, values.current_in));
    m_duty.append(QPointF(idx, values.duty_now * 100.0));
    m_rpm.append(QPointF(idx, values.rpm));
    m_tempMosfet.append(QPointF(idx, values.temp_mos));
    m_tempMotor.append(QPointF(idx, values.temp_motor));

    m_rtSampleIndex++;

    // Trim with some extra headroom to avoid trimming every sample
    trimVector(m_motorCurrent,   m_rtMaxSamples, m_rtTrimExtra);
    trimVector(m_batteryCurrent, m_rtMaxSamples, m_rtTrimExtra);
    trimVector(m_duty,           m_rtMaxSamples, m_rtTrimExtra);
    trimVector(m_rpm,            m_rtMaxSamples, m_rtTrimExtra);
    trimVector(m_tempMosfet,     m_rtMaxSamples, m_rtTrimExtra);
    trimVector(m_tempMotor,      m_rtMaxSamples, m_rtTrimExtra);

    emit rtDataAppended(m_rtSampleIndex - 1,
                        values.current_motor, values.current_in,
                        values.duty_now * 100.0, values.rpm,
                        values.temp_mos, values.temp_motor);
}

// ---------------------------------------------------------------------------
// IMU Data
// ---------------------------------------------------------------------------

void RtDataStore::setImuMaxSamples(int max)
{
    if (max != m_imuMaxSamples) {
        m_imuMaxSamples = max;
        emit imuMaxSamplesChanged();
    }
}

QVariantList RtDataStore::imuSeriesPoints(const QString &name) const
{
    if (name == "roll")   return toVariantList(m_imuRoll);
    if (name == "pitch")  return toVariantList(m_imuPitch);
    if (name == "yaw")    return toVariantList(m_imuYaw);
    if (name == "accX")   return toVariantList(m_imuAccX);
    if (name == "accY")   return toVariantList(m_imuAccY);
    if (name == "accZ")   return toVariantList(m_imuAccZ);
    if (name == "gyroX")  return toVariantList(m_imuGyroX);
    if (name == "gyroY")  return toVariantList(m_imuGyroY);
    if (name == "gyroZ")  return toVariantList(m_imuGyroZ);
    return {};
}

void RtDataStore::onValuesImuReceived(IMU_VALUES values, unsigned int mask)
{
    Q_UNUSED(mask)

    // Compute elapsed time
    if (!m_imuTimerStarted) {
        m_imuTimer.start();
        m_imuTimerStarted = true;
    } else {
        double elapsed = m_imuTimer.restart() / 1000.0;
        if (elapsed > 1.0) elapsed = 1.0;
        m_imuSecondCounter += elapsed;
    }

    double t = m_imuSecondCounter;
    double rollDeg  = values.roll  * 180.0 / M_PI;
    double pitchDeg = values.pitch * 180.0 / M_PI;
    double yawDeg   = values.yaw   * 180.0 / M_PI;

    m_imuRoll.append(QPointF(t, rollDeg));
    m_imuPitch.append(QPointF(t, pitchDeg));
    m_imuYaw.append(QPointF(t, yawDeg));
    m_imuAccX.append(QPointF(t, values.accX));
    m_imuAccY.append(QPointF(t, values.accY));
    m_imuAccZ.append(QPointF(t, values.accZ));
    m_imuGyroX.append(QPointF(t, values.gyroX));
    m_imuGyroY.append(QPointF(t, values.gyroY));
    m_imuGyroZ.append(QPointF(t, values.gyroZ));

    trimVector(m_imuRoll,  m_imuMaxSamples, 0);
    trimVector(m_imuPitch, m_imuMaxSamples, 0);
    trimVector(m_imuYaw,   m_imuMaxSamples, 0);
    trimVector(m_imuAccX,  m_imuMaxSamples, 0);
    trimVector(m_imuAccY,  m_imuMaxSamples, 0);
    trimVector(m_imuAccZ,  m_imuMaxSamples, 0);
    trimVector(m_imuGyroX, m_imuMaxSamples, 0);
    trimVector(m_imuGyroY, m_imuMaxSamples, 0);
    trimVector(m_imuGyroZ, m_imuMaxSamples, 0);

    emit imuDataAppended(t, rollDeg, pitchDeg, yawDeg,
                         values.accX, values.accY, values.accZ,
                         values.gyroX, values.gyroY, values.gyroZ);
}

// ---------------------------------------------------------------------------
// Experiment Plot Data
// ---------------------------------------------------------------------------

QVariantList RtDataStore::plotGraphPoints(int index) const
{
    if (index < 0 || index >= m_plotGraphs.size()) return {};
    return toVariantList(m_plotGraphs[index].points);
}

QString RtDataStore::plotGraphLabel(int index) const
{
    if (index < 0 || index >= m_plotGraphs.size()) return {};
    return m_plotGraphs[index].label;
}

QString RtDataStore::plotGraphColor(int index) const
{
    if (index < 0 || index >= m_plotGraphs.size()) return {};
    return m_plotGraphs[index].color;
}

void RtDataStore::setPlotHistoryMax(int max)
{
    m_plotHistoryMax = max;
}

void RtDataStore::onPlotInitReceived(QString xLabel, QString yLabel)
{
    m_plotGraphs.clear();
    m_plotCurrentGraph = 0;
    m_plotXLabel = xLabel;
    m_plotYLabel = yLabel;
    emit plotGraphCountChanged();
    emit plotInitialized(xLabel, yLabel);
}

void RtDataStore::onPlotDataReceived(double x, double y)
{
    // Ensure graph exists
    QStringList colors = graphColors();
    while (m_plotGraphs.size() <= m_plotCurrentGraph) {
        PlotGraph g;
        g.label = QString("Graph %1").arg(m_plotGraphs.size() + 1);
        g.color = colors.value(m_plotGraphs.size() % colors.size());
        m_plotGraphs.append(g);
        emit plotGraphAdded(m_plotGraphs.size() - 1, g.label, g.color);
        emit plotGraphCountChanged();
    }

    PlotGraph &g = m_plotGraphs[m_plotCurrentGraph];
    g.points.append(QPointF(x, y));

    // History cap
    if (g.points.size() > m_plotHistoryMax) {
        int excess = g.points.size() - m_plotHistoryMax;
        g.points.remove(0, excess);
    }

    emit plotPointAdded(m_plotCurrentGraph, x, y);
}

void RtDataStore::onPlotAddGraphReceived(QString name)
{
    QStringList colors = graphColors();
    PlotGraph g;
    g.label = name;
    g.color = colors.value(m_plotGraphs.size() % colors.size());
    m_plotGraphs.append(g);
    emit plotGraphAdded(m_plotGraphs.size() - 1, name, g.color);
    emit plotGraphCountChanged();
}

void RtDataStore::onPlotSetGraphReceived(int graph)
{
    m_plotCurrentGraph = graph;
    emit plotCurrentGraphChanged(graph);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

QStringList RtDataStore::graphColors()
{
    return {
        Utility::getAppHexColor("plot_graph1"),
        Utility::getAppHexColor("plot_graph2"),
        Utility::getAppHexColor("plot_graph3"),
        Utility::getAppHexColor("plot_graph4"),
        Utility::getAppHexColor("plot_graph5"),
        Utility::getAppHexColor("plot_graph6")
    };
}

void RtDataStore::trimVector(QVector<QPointF> &vec, int maxSize, int extra)
{
    if (vec.size() > maxSize + extra) {
        vec.remove(0, vec.size() - maxSize);
    }
}

QVariantList RtDataStore::toVariantList(const QVector<QPointF> &pts) const
{
    QVariantList list;
    list.reserve(pts.size());
    for (const QPointF &p : pts) {
        list.append(QVariant::fromValue(p));
    }
    return list;
}
