/*
    Copyright 2025 Felix van der Donk

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

#ifndef VESCTASKS_H
#define VESCTASKS_H

#include <QObject>
#include <QTimer>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QTcpSocket>
#include <QHostInfo>
#include <functional>
#include <chrono>

#include <QtTaskTree/QTaskTree>

using namespace QtTaskTree;

// ============================================================================
// SignalWaitTask — waits for an arbitrary signal with a timeout.
//
// This is the core building block replacing every QEventLoop pattern in the
// codebase.  Use it through its QCustomTask alias `SignalWaitTaskItem`.
//
// The setup handler must:
//   1. Call task.setTimeout() if a non-default timeout is needed.
//   2. Call task.connectSignal() to tell it which signal to wait for.
//   3. Optionally send the command that will eventually produce the signal.
//
// When the awaited signal fires the captured data is stored inside the task
// (via a user-provided lambda) and the task reports DoneResult::Success.
// On timeout it reports DoneResult::Error.
// ============================================================================

class SignalWaitTask : public QObject
{
    Q_OBJECT

public:
    explicit SignalWaitTask(QObject *parent = nullptr)
        : QObject(parent)
    {}

    void setTimeout(int ms) { m_timeoutMs = ms; }
    int timeout() const { return m_timeoutMs; }

    /// Connect to an arbitrary signal. When that signal fires, `onSignal` is
    /// invoked (to capture data) and the task completes successfully.
    template<typename Sender, typename Signal, typename Slot>
    void connectSignal(Sender *sender, Signal signal, Slot &&onSignal) {
        m_conn = QObject::connect(sender, signal, this, [this, onSignal = std::forward<Slot>(onSignal)](auto&&... args) {
            std::invoke(onSignal, std::forward<decltype(args)>(args)...);
            m_succeeded = true;
            emit done(DoneResult::Success);
        });
    }

    /// Overload when no data capture is needed — just wait for the signal.
    template<typename Sender, typename Signal>
    void connectSignal(Sender *sender, Signal signal) {
        m_conn = QObject::connect(sender, signal, this, [this]() {
            m_succeeded = true;
            emit done(DoneResult::Success);
        });
    }

    /// String-based overload for QML callers where the signal is a runtime
    /// string (e.g. SIGNAL(updated())).  Uses old-style connect.
    void connectStringSignal(QObject *sender, const char *signal) {
        m_conn = QObject::connect(sender, signal,
                                  this, SLOT(onStringSignalFired()));
    }

    void start() {
        m_timer.setSingleShot(true);
        connect(&m_timer, &QTimer::timeout, this, [this]() {
            if (!m_succeeded) {
                emit done(DoneResult::Error);
            }
        });
        m_timer.start(m_timeoutMs);
    }

    ~SignalWaitTask() override {
        QObject::disconnect(m_conn);
    }

    bool succeeded() const { return m_succeeded; }

signals:
    void done(DoneResult result);

private slots:
    void onStringSignalFired() {
        m_succeeded = true;
        emit done(DoneResult::Success);
    }

private:
    QTimer m_timer;
    QMetaObject::Connection m_conn;
    int m_timeoutMs = 3000;
    bool m_succeeded = false;
};

/// The QCustomTask alias used in task trees:
///   SignalWaitTaskItem([](SignalWaitTask &task) { ... setup ... })
using SignalWaitTaskItem = QCustomTask<SignalWaitTask>;

// ============================================================================
// DelayTask — a pure delay (replaces Utility::sleepWithEventLoop).
//
// Just use QtTaskTree::timeoutTask(std::chrono::milliseconds{ms}) instead.
// This alias exists for documentation clarity.
// ============================================================================

// Use timeoutTask(ms, DoneResult::Success) for a sleep.

// ============================================================================
// NetworkReplyTask — downloads from a URL and provides the reply data.
//
// Setup handler sets URL (and optionally a progress callback).
// On success the reply data is available via replyData().
// ============================================================================

class NetworkReplyTask : public QObject
{
    Q_OBJECT

public:
    explicit NetworkReplyTask(QObject *parent = nullptr)
        : QObject(parent)
    {}

    void setUrl(const QUrl &url) { m_url = url; }
    void setProgressCallback(std::function<void(qint64, qint64)> cb) { m_progressCb = std::move(cb); }

    /// If set, incoming data is incrementally written to this device instead of
    /// being buffered in memory.
    void setOutputDevice(QIODevice *dev) { m_outputDevice = dev; }

    QByteArray replyData() const { return m_data; }
    QNetworkReply::NetworkError error() const { return m_error; }

    void start() {
        m_reply = m_manager.get(QNetworkRequest(m_url));

        connect(m_reply, &QNetworkReply::downloadProgress, this,
                [this](qint64 bytesReceived, qint64 bytesTotal) {
            if (m_outputDevice) {
                m_outputDevice->write(m_reply->read(m_reply->size()));
            }
            if (m_progressCb) {
                m_progressCb(bytesReceived, bytesTotal);
            }
        });

        connect(m_reply, &QNetworkReply::finished, this, [this]() {
            m_error = m_reply->error();
            if (m_outputDevice) {
                m_outputDevice->write(m_reply->readAll());
            } else {
                m_data = m_reply->readAll();
            }
            m_reply->deleteLater();
            m_reply = nullptr;
            emit done(m_error == QNetworkReply::NoError ? DoneResult::Success : DoneResult::Error);
        });
    }

    ~NetworkReplyTask() override {
        if (m_reply) {
            m_reply->abort();
            m_reply->deleteLater();
        }
    }

signals:
    void done(DoneResult result);

private:
    QNetworkAccessManager m_manager;
    QNetworkReply *m_reply = nullptr;
    QUrl m_url;
    QByteArray m_data;
    QNetworkReply::NetworkError m_error = QNetworkReply::NoError;
    std::function<void(qint64, qint64)> m_progressCb;
    QIODevice *m_outputDevice = nullptr;
};

using NetworkReplyTaskItem = QCustomTask<NetworkReplyTask>;

// ============================================================================
// ProcessTask — runs a QProcess and waits for it to finish.
//
// Replaces MainWindow::waitProcess and SystemCommandExecutor::executeCommand.
// ============================================================================

class ProcessTask : public QObject
{
    Q_OBJECT

public:
    explicit ProcessTask(QObject *parent = nullptr)
        : QObject(parent)
    {}

    void setProgram(const QString &program) { m_program = program; }
    void setArguments(const QStringList &args) { m_arguments = args; }
    void setTimeout(int ms) { m_timeoutMs = ms; }
    void setStdOutCallback(std::function<void(const QString &)> cb) { m_stdOutCb = std::move(cb); }
    void setStdErrCallback(std::function<void(const QString &)> cb) { m_stdErrCb = std::move(cb); }

    int exitCode() const { return m_exitCode; }
    bool wasKilled() const { return m_killed; }
    QByteArray standardOutput() const { return m_stdOut; }
    QByteArray standardError() const { return m_stdErr; }

    void start() {
        connect(&m_process, &QProcess::finished, this, [this](int exitCode, QProcess::ExitStatus) {
            m_exitCode = exitCode;
            emit done(DoneResult::Success);
        });

        connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
            if (!m_killed) {
                m_exitCode = -1;
                emit done(DoneResult::Error);
            }
        });

        connect(&m_process, &QProcess::readyReadStandardOutput, this, [this]() {
            auto data = m_process.readAllStandardOutput();
            m_stdOut.append(data);
            if (m_stdOutCb) m_stdOutCb(QString::fromUtf8(data));
        });

        connect(&m_process, &QProcess::readyReadStandardError, this, [this]() {
            auto data = m_process.readAllStandardError();
            m_stdErr.append(data);
            if (m_stdErrCb) m_stdErrCb(QString::fromUtf8(data));
        });

        if (m_timeoutMs > 0) {
            m_timer.setSingleShot(true);
            connect(&m_timer, &QTimer::timeout, this, [this]() {
                m_killed = true;
                m_process.kill();
                m_process.waitForFinished(1000);
                emit done(DoneResult::Error);
            });
            m_timer.start(m_timeoutMs);
        }

        m_process.start(m_program, m_arguments);
    }

    ~ProcessTask() override {
        if (m_process.state() == QProcess::Running) {
            m_process.kill();
            m_process.waitForFinished(1000);
        }
    }

signals:
    void done(DoneResult result);

private:
    QProcess m_process;
    QTimer m_timer;
    QString m_program;
    QStringList m_arguments;
    int m_timeoutMs = 0;
    int m_exitCode = -1;
    bool m_killed = false;
    QByteArray m_stdOut;
    QByteArray m_stdErr;
    std::function<void(const QString &)> m_stdOutCb;
    std::function<void(const QString &)> m_stdErrCb;
};

using ProcessTaskItem = QCustomTask<ProcessTask>;

// ============================================================================
// TcpConnectTask — connects a QTcpSocket with timeout.
// ============================================================================

class TcpConnectTask : public QObject
{
    Q_OBJECT

public:
    explicit TcpConnectTask(QObject *parent = nullptr)
        : QObject(parent)
    {}

    void setSocket(QTcpSocket *socket) { m_socket = socket; }
    void setHost(const QHostAddress &host) { m_host = host; }
    void setPort(quint16 port) { m_port = port; }
    void setTimeout(int ms) { m_timeoutMs = ms; }
    void setOnConnected(std::function<void()> cb) { m_onConnected = std::move(cb); }

    void start() {
        m_timer.setSingleShot(true);
        connect(&m_timer, &QTimer::timeout, this, [this]() {
            emit done(DoneResult::Error);
        });

        connect(m_socket, &QTcpSocket::connected, this, [this]() {
            m_timer.stop();
            if (m_onConnected) m_onConnected();
            emit done(DoneResult::Success);
        });

        m_timer.start(m_timeoutMs);
        m_socket->connectToHost(m_host, m_port);
    }

signals:
    void done(DoneResult result);

private:
    QTcpSocket *m_socket = nullptr;
    QHostAddress m_host;
    quint16 m_port = 0;
    int m_timeoutMs = 3000;
    QTimer m_timer;
    std::function<void()> m_onConnected;
};

using TcpConnectTaskItem = QCustomTask<TcpConnectTask>;

// ============================================================================
// SocketLineReaderTask — reads lines from a socket until newline with timeout.
// ============================================================================

class SocketLineReaderTask : public QObject
{
    Q_OBJECT

public:
    explicit SocketLineReaderTask(QObject *parent = nullptr)
        : QObject(parent)
    {}

    void setSocket(QTcpSocket *socket) { m_socket = socket; }
    void setTimeout(int ms) { m_timeoutMs = ms; }

    QString line() const { return m_line; }

    void start() {
        m_timer.setSingleShot(true);
        connect(&m_timer, &QTimer::timeout, this, [this]() {
            m_line.clear();
            emit done(DoneResult::Error);
        });
        m_timer.start(m_timeoutMs);

        m_readConn = connect(m_socket, &QTcpSocket::readyRead, this, [this]() {
            while (m_socket->bytesAvailable() > 0) {
                QByteArray rxb = m_socket->read(1);
                if (rxb.size() == 1) {
                    if (rxb[0] != '\n') {
                        m_buffer.append(rxb[0]);
                    } else {
                        m_buffer.append('\0');
                        m_line = QString::fromLocal8Bit(m_buffer);
                        m_timer.stop();
                        emit done(DoneResult::Success);
                        return;
                    }
                }
            }
        });
    }

    ~SocketLineReaderTask() override {
        QObject::disconnect(m_readConn);
    }

signals:
    void done(DoneResult result);

private:
    QTcpSocket *m_socket = nullptr;
    QTimer m_timer;
    int m_timeoutMs = 3000;
    QByteArray m_buffer;
    QString m_line;
    QMetaObject::Connection m_readConn;
};

using SocketLineReaderTaskItem = QCustomTask<SocketLineReaderTask>;

// ============================================================================
// PollTimerTask — runs a periodic timer callback until an external condition
// triggers completion.  Replaces patterns like testDirection() and
// scanCANbus() that use QTimer + QEventLoop.
// ============================================================================

class PollTimerTask : public QObject
{
    Q_OBJECT

public:
    explicit PollTimerTask(QObject *parent = nullptr)
        : QObject(parent)
    {}

    void setTimeout(int ms) { m_timeoutMs = ms; }
    void setInterval(int ms) { m_intervalMs = ms; }
    void setOnTick(std::function<void()> cb) { m_onTick = std::move(cb); }

    /// Call this from within the tick callback to end early with success.
    void finish() {
        m_timer.stop();
        m_pollTimer.stop();
        emit done(DoneResult::Success);
    }

    void start() {
        if (m_timeoutMs > 0) {
            m_timer.setSingleShot(true);
            connect(&m_timer, &QTimer::timeout, this, [this]() {
                m_pollTimer.stop();
                emit done(DoneResult::Success); // timeout = normal completion for poll tasks
            });
            m_timer.start(m_timeoutMs);
        }

        connect(&m_pollTimer, &QTimer::timeout, this, [this]() {
            if (m_onTick) m_onTick();
        });
        m_pollTimer.start(m_intervalMs);
    }

signals:
    void done(DoneResult result);

private:
    QTimer m_timer;
    QTimer m_pollTimer;
    int m_timeoutMs = 0;
    int m_intervalMs = 100;
    std::function<void()> m_onTick;
};

using PollTimerTaskItem = QCustomTask<PollTimerTask>;

// ============================================================================
// runTree — convenience to synchronously run a TaskTree.
//
// This is the ONLY place we still block the caller using a local event loop,
// but it's inside a well-defined QTaskTree so we get proper cancellation
// semantics and no re-entrancy issues.
//
// Usage:
//   bool ok = runTree({Group{...}});
//
// For truly async code, construct a QTaskTree yourself and connect to done().
// ============================================================================

inline bool runTree(const Group &root)
{
    QTaskTree tree(root);
    bool success = false;
    QEventLoop loop;
    QObject::connect(&tree, &QTaskTree::done, &loop, [&success, &loop](DoneWith result) {
        success = (result == DoneWith::Success);
        loop.quit();
    });
    tree.start();
    if (tree.isRunning())
        loop.exec();
    return success;
}

#endif // VESCTASKS_H
