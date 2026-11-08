/*
    Copyright 2026 Marvin Holzwarth

    This file is part of VESC Tool.

    VESC Tool is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VESC Tool is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
    */

#ifndef MCPDEVICEOPERATIONS_H
#define MCPDEVICEOPERATIONS_H

#include "mcpdevice.h"
#include "mcpwritetransaction.h"
#include <QElapsedTimer>
#include <QJsonArray>
#include <QTimer>

class McpDeviceOperations : public QObject
{
    Q_OBJECT
  public:
    explicit McpDeviceOperations(McpDevice *device, const QString &directory,
                                 QObject *parent = nullptr);
    ~McpDeviceOperations();
    QJsonObject start(const QString &operation, const QJsonObject &arguments);
    QJsonObject job(const QString &id, bool includeSamples = false) const;
    void cancel(const QString &id);
    void approve(const QString &id, bool accepted);
    bool busy() const;
    QString currentId() const
    {
        return mId;
    }
  signals:
    void busyChanged(bool busy);
    void approvalRequested(const QString &id, const QString &title, const QString &details);

  private:
    void tick();
    bool send(int command);
    bool sendPacket(const QByteArray &packet);
    void phase(const QString &name, int timeoutMs = 8000);
    void finish(bool ok, const QString &error = QString());
    void recordJob();
    void onConfig(const QString &kind);
    void onLispCode(int total, int offset, const QByteArray &data);
    void readLisp(const QString &phase);
    void writeLispChunk();
    void processPending();
    McpWriteTransaction::Guard guard() const;
    QString save(const QString &suffix, const QByteArray &bytes);
    QString backupConfig(const QString &kind, const QString &source);
    QJsonObject mJob;
    QHash<QString, QJsonObject> mJobs;
    QStringList mHistory;
    McpDevice *mDevice;
    QString mDirectory;
    QString mId, mOperation, mPhase, mTarget, mKind;
    QTimer mPoll, mDeadline;
    QElapsedTimer mClock, mValuesAge, mInputAge;
    QJsonObject mValues, mInput, mDesired;
    QString mPendingAck, mPendingConfigKind;
    QJsonObject mPendingConfigValues;
    QByteArray mDesiredWire, mLispImage, mReadData;
    QJsonArray mSamples;
    int mReadTotal = -1, mReadRequested = 0, mWriteOffset = 0, mChunkSize = 0;
    int mDuration = 1000, mInterval = 50;
    qint64 mLastPoll = -1000;
    bool mWriteIssued = false, mApprovalSent = false;
    McpWriteTransaction mTransaction;
};
#endif
