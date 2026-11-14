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

#ifndef MCPWRITETRANSACTION_H
#define MCPWRITETRANSACTION_H

#include <QObject>
#include <QJsonObject>
#include <QTimer>
#include <limits>

// Transport-independent write coordinator. Signals are requests to a trusted
// app-side adapter, not controller commands. McpDeviceOperations provides binding.
class McpWriteTransaction : public QObject
{
    Q_OBJECT
  public:
    enum State {
        Idle,
        ReadingBefore,
        SavingBackup,
        AwaitingApproval,
        AwaitingAck,
        ReadingBack,
        Succeeded,
        Failed,
        Cancelled
    };
    Q_ENUM(State)

    struct Guard {
        QString target;
        bool connected = false;
        bool firmwareCompatible = false;
        bool exclusiveAccess = false;
        bool inputNeutral = false;
        qint64 valuesAgeMs = -1;
        qint64 inputAgeMs = -1;
        double erpm = std::numeric_limits<double>::quiet_NaN();
        double duty = std::numeric_limits<double>::quiet_NaN();
        double motorCurrent = std::numeric_limits<double>::quiet_NaN();
    };

    explicit McpWriteTransaction(QObject *parent = nullptr, int operationTimeoutMs = 5000,
                                 int approvalTimeoutMs = 60000);
    bool begin(const QString &config, const QJsonObject &desired, const Guard &guard);
    bool configReceived(const QString &id, const QString &target, const QJsonObject &values,
                        const Guard &guard);
    bool backupFinished(const QString &id, bool saved, const QString &reference);
    // Approval must come from the trusted app UI, never directly from an MCP
    // client. The adapter supplies the CURRENT app desired-values fingerprint.
    bool approve(const QString &id, const QString &beforeFingerprint,
                 const QString &currentDesiredFingerprint, const Guard &guard);
    bool ackReceived(const QString &id, const QString &target, const QString &ack,
                     const Guard &guard);
    void observe(const Guard &guard);
    void cancel(const QString &id);
    State state() const;
    QString id() const;
    QJsonObject status() const;
    static QString fingerprint(const QJsonObject &values);

  signals:
    void changed();
    void readRequested(const QString &id, bool verification);
    void backupRequested(const QString &id, const QJsonObject &before);
    void writeRequested(const QString &id, const QJsonObject &desired);

  private:
    bool active() const;
    bool checkGuard(const Guard &guard, QString *reason) const;
    void enter(State state);
    void fail(const QString &reason);
    static bool validValues(const QJsonObject &values);
    static QString stateName(State state);

    QTimer mDeadline;
    State mState = Idle;
    QString mId;
    QString mConfig;
    QString mTarget;
    QString mBackup;
    QString mError;
    QJsonObject mBefore;
    QJsonObject mDesired;
    bool mWriteIssued = false;
    int mOperationTimeoutMs;
    int mApprovalTimeoutMs;
};

#endif // MCPWRITETRANSACTION_H
