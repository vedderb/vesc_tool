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

#include "mcpserver.h"

#include <QJsonDocument>
#include <QJsonParseError>
#include <QRegularExpression>
#include <QTimer>

namespace
{
const int HeaderLimit = 8192;
const int BodyLimit = 65536;
const int ConnectionLimit = 8;
const char ProtocolVersion[] = "2025-11-25";

bool tokenMatches(const QByteArray &received, const QByteArray &expected)
{
    if (received.size() != expected.size()) {
        return false;
    }
    unsigned char difference = 0;
    for (int i = 0; i < received.size(); i++) {
        difference |= static_cast<unsigned char>(received.at(i) ^ expected.at(i));
    }
    return difference == 0;
}
} // namespace

McpServer::McpServer(QObject *parent) : QObject(parent)
{
    mServer = new QTcpServer(this);
    mServer->setMaxPendingConnections(ConnectionLimit);
    connect(mServer, &QTcpServer::newConnection, this, &McpServer::acceptConnections);
}

bool McpServer::start(quint16 port, const QByteArray &token)
{
    stop();
    // Require a caller-provided secret. Never persist or print it.
    if (!QRegularExpression("^[A-Za-z0-9_-]{32,256}$")
             .match(QString::fromLatin1(token))
             .hasMatch()) {
        mError = tr("MCP requires a token of 32 to 256 letters, digits, '_' or '-'.");
        return false;
    }
    if (!mServer->listen(QHostAddress::LocalHost, port)) {
        mError = mServer->errorString();
        return false;
    }
    mToken = token;
    mError.clear();
    return true;
}

McpServer::~McpServer()
{
    stop();
}

void McpServer::stop()
{
    mServer->close();
    const auto sockets = mSockets.values();
    for (auto socket : sockets) {
        socket->abort();
        socket->deleteLater();
    }
    mBuffers.clear();
    mSockets.clear();
    mToken.clear();
}

quint16 McpServer::port() const
{
    return mServer->serverPort();
}

QString McpServer::errorString() const
{
    return mError;
}

void McpServer::setTools(const QJsonArray &tools, ToolHandler handler)
{
    mTools = tools;
    mHandler = handler;
}

void McpServer::acceptConnections()
{
    while (mServer->hasPendingConnections()) {
        auto socket = mServer->nextPendingConnection();
        if (mSockets.size() >= ConnectionLimit) {
            socket->abort();
            socket->deleteLater();
            continue;
        }
        socket->setParent(this);
        socket->setReadBufferSize(HeaderLimit + BodyLimit + 1);
        mBuffers.insert(socket, QByteArray());
        mSockets.insert(socket);
        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() { readRequest(socket); });
        connect(socket, &QTcpSocket::disconnected, this, [this, socket]() {
            mBuffers.remove(socket);
            mSockets.remove(socket);
            socket->deleteLater();
        });
        QTimer::singleShot(5000, socket, [this, socket]() {
            if (mBuffers.contains(socket)) {
                respond(socket, 408);
            } else if (mSockets.contains(socket)) {
                // Response bytes must not keep a non-reading client alive indefinitely.
                socket->abort();
            }
        });
    }
}

void McpServer::respond(QTcpSocket *socket, int status, const QByteArray &body)
{
    mBuffers.remove(socket);
    QByteArray reason;
    switch (status) {
    case 200:
        reason = "OK";
        break;
    case 202:
        reason = "Accepted";
        break;
    case 400:
        reason = "Bad Request";
        break;
    case 401:
        reason = "Unauthorized";
        break;
    case 403:
        reason = "Forbidden";
        break;
    case 404:
        reason = "Not Found";
        break;
    case 405:
        reason = "Method Not Allowed";
        break;
    case 406:
        reason = "Not Acceptable";
        break;
    case 408:
        reason = "Request Timeout";
        break;
    case 413:
        reason = "Payload Too Large";
        break;
    case 415:
        reason = "Unsupported Media Type";
        break;
    default:
        reason = "Internal Server Error";
        break;
    }
    QByteArray headers = "HTTP/1.1 " + QByteArray::number(status) + " " + reason + "\r\n";
    headers += "Connection: close\r\nCache-Control: no-store\r\n";
    if (status == 405) {
        headers += "Allow: POST\r\n";
    }
    if (!body.isEmpty()) {
        headers += "Content-Type: application/json\r\n";
    }
    headers += "Content-Length: " + QByteArray::number(body.size()) + "\r\n\r\n";
    socket->write(headers + body);
    socket->disconnectFromHost();
}

void McpServer::readRequest(QTcpSocket *socket)
{
    if (!mBuffers.contains(socket)) {
        return;
    }
    QByteArray &buffer = mBuffers[socket];
    buffer += socket->readAll();
    const int end = buffer.indexOf("\r\n\r\n");
    if (end < 0) {
        if (buffer.size() > HeaderLimit) {
            respond(socket, 413);
        }
        return;
    }
    if (end + 4 > HeaderLimit || buffer.size() > HeaderLimit + BodyLimit) {
        respond(socket, 413);
        return;
    }

    auto lines = buffer.left(end + 2).split('\n');
    lines.removeLast();
    const auto requestLine = lines.takeFirst().trimmed().split(' ');
    if (requestLine.size() != 3 || requestLine.at(2) != "HTTP/1.1") {
        respond(socket, 400);
        return;
    }
    QHash<QByteArray, QByteArray> headers;
    for (auto line : lines) {
        if (!line.endsWith('\r')) {
            respond(socket, 400);
            return;
        }
        line.chop(1);
        const int colon = line.indexOf(':');
        const auto key = line.left(colon).toLower();
        if (colon < 1 || headers.contains(key) ||
            !QRegularExpression("^[a-z0-9!#$%&'*+.^_`|~-]+$")
                 .match(QString::fromLatin1(key))
                 .hasMatch()) {
            respond(socket, 400);
            return;
        }
        headers.insert(key, line.mid(colon + 1).trimmed());
    }

    // Consume a bounded, complete request before replying, including errors.
    // Closing while a native HTTP client is still sending its body can cause
    // a TCP reset instead of delivering the HTTP error (notably on Windows).
    if (headers.contains("transfer-encoding")) {
        respond(socket, 400);
        return;
    }
    int length = 0;
    if (headers.contains("content-length")) {
        const auto lengthText = headers.value("content-length");
        bool ok = false;
        length = lengthText.toInt(&ok);
        if (!ok || length < 0 ||
            !QRegularExpression("^[0-9]+$").match(QString::fromLatin1(lengthText)).hasMatch()) {
            respond(socket, 400);
            return;
        }
    } else if (requestLine.at(0) == "POST") {
        respond(socket, 400);
        return;
    }
    if (length > BodyLimit) {
        respond(socket, 413);
        return;
    }
    const int expected = end + 4 + length;
    if (buffer.size() < expected) {
        return;
    }
    if (buffer.size() != expected) {
        respond(socket, 400);
        return;
    }

    const auto host = headers.value("host").toLower();
    const auto suffix = ":" + QByteArray::number(port());
    // Native local clients only: reject every browser Origin and foreign Host.
    if (headers.contains("origin") ||
        (host != "127.0.0.1" + suffix && host != "localhost" + suffix)) {
        respond(socket, 403);
        return;
    }
    if (!tokenMatches(headers.value("authorization"), "Bearer " + mToken)) {
        respond(socket, 401);
        return;
    }
    if (requestLine.at(1) != "/mcp") {
        respond(socket, 404);
        return;
    }
    // No SSE stream or session deletion is offered by this stateless server.
    if (requestLine.at(0) != "POST") {
        respond(socket, 405);
        return;
    }
    const auto version = headers.value("mcp-protocol-version");
    if (!version.isEmpty() && version != ProtocolVersion && version != "2025-03-26") {
        respond(socket, 400);
        return;
    }
    if (headers.value("content-type").split(';').first().trimmed().toLower() !=
        "application/json") {
        respond(socket, 415);
        return;
    }
    const auto accept = headers.value("accept").toLower();
    if (!accept.contains("application/json") || !accept.contains("text/event-stream")) {
        respond(socket, 406);
        return;
    }
    const auto body = buffer.mid(end + 4);
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || QString::fromUtf8(body).toUtf8() != body) {
        respond(socket, 200,
                QJsonDocument(rpcError(QJsonValue(), -32700, "Parse error"))
                    .toJson(QJsonDocument::Compact));
        return;
    }
    if (!document.isObject()) {
        respond(socket, 200,
                QJsonDocument(rpcError(QJsonValue(), -32600, "Invalid request"))
                    .toJson(QJsonDocument::Compact));
        return;
    }
    const auto response = dispatch(document.object());
    respond(socket, response.isEmpty() ? 202 : 200,
            response.isEmpty() ? QByteArray()
                               : QJsonDocument(response).toJson(QJsonDocument::Compact));
}

QJsonObject McpServer::rpcError(const QJsonValue &id, int code, const QString &message)
{
    return {{"jsonrpc", "2.0"},
            {"id", id},
            {"error", QJsonObject{{"code", code}, {"message", message}}}};
}

QJsonObject McpServer::dispatch(const QJsonObject &request)
{
    const auto id = request.value("id");
    if (request.value("jsonrpc") != "2.0" || !request.value("method").isString() ||
        (request.contains("id") && !id.isString() && !id.isDouble()) ||
        (request.contains("params") && !request.value("params").isObject())) {
        return rpcError(QJsonValue(), -32600, "Invalid request");
    }
    // Notifications are never executed as tools and never receive RPC responses.
    if (!request.contains("id")) {
        return {};
    }
    const auto method = request.value("method").toString();
    const auto params = request.value("params").toObject();
    if (params.contains("_meta") && !params.value("_meta").isObject()) {
        return rpcError(id, -32602, "Invalid request metadata");
    }
    QJsonObject result;
    if (method == "initialize") {
        if (!params.value("protocolVersion").isString() ||
            !params.value("capabilities").isObject() || !params.value("clientInfo").isObject() ||
            !params.value("clientInfo").toObject().value("name").isString() ||
            !params.value("clientInfo").toObject().value("version").isString()) {
            return rpcError(id, -32602, "Invalid initialization parameters");
        }
        const auto requested = params.value("protocolVersion").toString();
        const auto version = requested == "2025-03-26" ? requested : QString(ProtocolVersion);
        result = {
            {"protocolVersion", version},
            {"capabilities", QJsonObject{{"tools", QJsonObject{{"listChanged", false}}}}},
            {"serverInfo", QJsonObject{{"name", "vesc-tool"}, {"version", "0.1.0"}}},
            {"instructions",
             "Use only the advertised tools. App-memory configuration and cached data "
             "do not prove controller persistence or freshness. Enabled uploads require approval "
             "in VESC Tool and successful readback. Poll returned job ids; do not infer success "
             "from sending."}};
    } else if (method == "ping") {
        result = {};
    } else if (method == "tools/list") {
        for (auto it = params.begin(); it != params.end(); ++it) {
            if (it.key() != "_meta" && (it.key() != "cursor" || !it.value().isNull())) {
                return rpcError(id, -32602, "Pagination is not supported");
            }
        }
        result = {{"tools", mTools}};
    } else if (method == "tools/call") {
        if (!params.value("name").isString() ||
            (params.contains("arguments") && !params.value("arguments").isObject())) {
            return rpcError(id, -32602, "Invalid tool parameters");
        }
        const auto name = params.value("name").toString();
        bool found = false;
        for (const auto tool : mTools) {
            found |= tool.toObject().value("name").toString() == name;
        }
        if (!found || !mHandler) {
            return rpcError(id, -32602, "Unknown tool");
        }
        result = mHandler(name, params.value("arguments").toObject());
        emit toolCalled(name);
    } else {
        return rpcError(id, -32601, "Method not found");
    }
    return {{"jsonrpc", "2.0"}, {"id", id}, {"result", result}};
}
