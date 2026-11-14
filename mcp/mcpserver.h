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

#ifndef MCPSERVER_H
#define MCPSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHash>
#include <QSet>
#include <QJsonArray>
#include <QJsonObject>
#include <functional>

// Minimal, stateless Streamable HTTP transport for a native local MCP client.
// No nested event loops, worker threads, SSE, or controller dependencies.
class McpServer : public QObject
{
    Q_OBJECT
  public:
    typedef std::function<QJsonObject(const QString &, const QJsonObject &)> ToolHandler;

    explicit McpServer(QObject *parent = nullptr);
    ~McpServer();
    bool start(quint16 port, const QByteArray &token);
    void stop();
    quint16 port() const;
    QString errorString() const;
    void setTools(const QJsonArray &tools, ToolHandler handler);

  signals:
    void toolCalled(const QString &name);

  private:
    void acceptConnections();
    void readRequest(QTcpSocket *socket);
    void respond(QTcpSocket *socket, int status, const QByteArray &body = QByteArray());
    QJsonObject dispatch(const QJsonObject &request);
    static QJsonObject rpcError(const QJsonValue &id, int code, const QString &message);

    QTcpServer *mServer;
    QHash<QTcpSocket *, QByteArray> mBuffers;
    QSet<QTcpSocket *> mSockets;
    QByteArray mToken;
    QString mError;
    QJsonArray mTools;
    ToolHandler mHandler;
};

#endif // MCPSERVER_H
