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

#include <QtTest>
#include <QEventLoop>
#include <QTimer>
#include <QJsonDocument>
#include "mcpserver.h"
#include "mcpparamvalidator.h"

class TestMcpServer : public QObject
{
    Q_OBJECT
  private:
    McpServer mServer;
    int mCalls = 0;
    bool mEarlyResponse = false;
    const QByteArray mToken = QByteArray(64, 'a');

    QByteArray request(const QByteArray &body, QByteArray extra = QByteArray(),
                       QByteArray auth = QByteArray())
    {
        if (auth.isNull()) {
            auth = "Bearer " + mToken;
        }
        return "POST /mcp HTTP/1.1\r\nHost: 127.0.0.1:" + QByteArray::number(mServer.port()) +
               "\r\nAuthorization: " + auth +
               "\r\nAccept: application/json, text/event-stream\r\nContent-Type: "
               "application/json\r\n" +
               extra + "Content-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body;
    }

    QByteArray exchange(const QByteArray &data, int split = -1)
    {
        QTcpSocket socket;
        QByteArray response;
        bool bodySent = split < 0;
        mEarlyResponse = false;
        QEventLoop loop;
        QTimer timeout;
        timeout.setSingleShot(true);
        connect(&socket, &QTcpSocket::connected, &loop, [&]() {
            if (split < 0) {
                socket.write(data);
            } else {
                socket.write(data.left(split));
                QTimer::singleShot(10, &socket, [&]() {
                    bodySent = true;
                    socket.write(data.mid(split));
                });
            }
        });
        connect(&socket, &QTcpSocket::readyRead, &loop, [&]() {
            mEarlyResponse |= !bodySent;
            response += socket.readAll();
        });
        connect(&socket, &QTcpSocket::disconnected, &loop, &QEventLoop::quit);
        connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
        socket.connectToHost(QHostAddress::LocalHost, mServer.port());
        timeout.start(7000);
        loop.exec();
        response += socket.readAll();
        return response;
    }

    QJsonObject rpc(const QByteArray &response)
    {
        return QJsonDocument::fromJson(response.mid(response.indexOf("\r\n\r\n") + 4)).object();
    }

  private slots:
    void init()
    {
        mCalls = 0;
        mServer.setTools(
            {QJsonObject{{"name", "read"}, {"inputSchema", QJsonObject{{"type", "object"}}}}},
            [this](const QString &, const QJsonObject &) {
                mCalls++;
                return QJsonObject{
                    {"content", QJsonArray{QJsonObject{{"type", "text"}, {"text", "ok"}}}}};
            });
        QVERIFY(mServer.start(0, mToken));
    }

    void cleanup()
    {
        mServer.stop();
    }

    void initialize()
    {
        auto response = exchange(request(
            R"({"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-11-25","capabilities":{},"clientInfo":{"name":"test","version":"1"}}})"));
        QVERIFY(response.startsWith("HTTP/1.1 200 "));
        auto result = rpc(response).value("result").toObject();
        QCOMPARE(result.value("protocolVersion").toString(), QString("2025-11-25"));
        QVERIFY(result.value("capabilities").toObject().contains("tools"));
        QVERIFY(!response.contains("MCP-Session-Id"));
    }

    void negotiation()
    {
        auto response = exchange(request(
            R"({"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"future","capabilities":{},"clientInfo":{"name":"test","version":"1"}}})"));
        QCOMPARE(rpc(response).value("result").toObject().value("protocolVersion").toString(),
                 QString("2025-11-25"));
        response = exchange(request(
            R"({"jsonrpc":"2.0","id":2,"method":"initialize","params":{"protocolVersion":"2025-03-26","capabilities":{},"clientInfo":{"name":"test","version":"1"}}})"));
        QCOMPARE(rpc(response).value("result").toObject().value("protocolVersion").toString(),
                 QString("2025-03-26"));
    }

    void listAndCall()
    {
        auto response = exchange(request(R"({"jsonrpc":"2.0","id":"list","method":"tools/list"})"));
        QCOMPARE(rpc(response).value("id").toString(), QString("list"));
        QCOMPARE(rpc(response).value("result").toObject().value("tools").toArray().size(), 1);
        response = exchange(request(
            R"({"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":"read","arguments":{}}})"));
        QVERIFY(rpc(response).contains("result"));
        QCOMPARE(mCalls, 1);
    }

    void notificationsNeverCallTools()
    {
        auto response =
            exchange(request(R"({"jsonrpc":"2.0","method":"notifications/initialized"})"));
        QVERIFY(response.startsWith("HTTP/1.1 202 "));
        QVERIFY(rpc(response).isEmpty());
        response = exchange(
            request(R"({"jsonrpc":"2.0","method":"tools/call","params":{"name":"read"}})"));
        QVERIFY(response.startsWith("HTTP/1.1 202 "));
        QCOMPARE(mCalls, 0);
    }

    void requestMetadata()
    {
        const auto response = exchange(request(
            R"({"jsonrpc":"2.0","id":1,"method":"tools/list","params":{"cursor":null,"_meta":{"progressToken":"test"}}})"));
        QCOMPARE(rpc(response).value("result").toObject().value("tools").toArray().size(), 1);
    }

    void rpcErrors_data()
    {
        QTest::addColumn<QByteArray>("body");
        QTest::addColumn<int>("code");
        QTest::newRow("parse") << QByteArray("{") << -32700;
        QTest::newRow("batch") << QByteArray("[]") << -32600;
        QTest::newRow("version") << QByteArray(R"({"jsonrpc":"1.0","id":1,"method":"ping"})")
                                 << -32600;
        QTest::newRow("null-id") << QByteArray(R"({"jsonrpc":"2.0","id":null,"method":"ping"})")
                                 << -32600;
        QTest::newRow("method") << QByteArray(R"({"jsonrpc":"2.0","id":1,"method":"missing"})")
                                << -32601;
        QTest::newRow("unknown-tool")
            << QByteArray(
                   R"({"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"write"}})")
            << -32602;
        QTest::newRow("bad-arguments")
            << QByteArray(
                   R"({"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"read","arguments":[]}})")
            << -32602;
        QTest::newRow("bad-initialize")
            << QByteArray(R"({"jsonrpc":"2.0","id":1,"method":"initialize","params":{}})")
            << -32602;
        QTest::newRow("bad-metadata")
            << QByteArray(R"({"jsonrpc":"2.0","id":1,"method":"tools/list","params":{"_meta":[]}})")
            << -32602;
    }

    void rpcErrors()
    {
        QFETCH(QByteArray, body);
        QFETCH(int, code);
        const auto response = exchange(request(body));
        QCOMPARE(rpc(response).value("error").toObject().value("code").toInt(), code);
        QCOMPARE(mCalls, 0);
    }

    void authentication()
    {
        auto response = exchange(request("{}", QByteArray(), "Bearer wrong"));
        QVERIFY(response.startsWith("HTTP/1.1 401 "));
        QCOMPARE(mCalls, 0);
    }

    void browserOrigin()
    {
        const auto response = exchange(request("{}", "Origin: https://example.org\r\n"));
        QVERIFY(response.startsWith("HTTP/1.1 403 "));
    }

    void foreignHost()
    {
        auto data = request("{}");
        data.replace("Host: 127.0.0.1:", "Host: attacker.example:");
        QVERIFY(exchange(data).startsWith("HTTP/1.1 403 "));
    }

    void rejectsAmbiguousFraming()
    {
        QVERIFY(exchange(request("{}", "Content-Length: 2\r\n")).startsWith("HTTP/1.1 400 "));
        QVERIFY(
            exchange(request("{}", "Transfer-Encoding: chunked\r\n")).startsWith("HTTP/1.1 400 "));
        auto data = request("{}");
        data.replace("Content-Length: 2", "Content-Length: 65537");
        QVERIFY(exchange(data).startsWith("HTTP/1.1 413 "));
        QVERIFY(exchange(request("{}") + "extra").startsWith("HTTP/1.1 400 "));
    }

    void fragmentedRequest()
    {
        const auto data = request(R"({"jsonrpc":"2.0","id":7,"method":"ping"})");
        auto response = exchange(data, data.size() - 5);
        QVERIFY(response.startsWith("HTTP/1.1 200 "));
        QCOMPARE(rpc(response).value("id").toInt(), 7);
    }

    void repeatedConnections()
    {
        for (int i = 0; i < 100; i++) {
            const auto response = exchange(request(R"({"jsonrpc":"2.0","id":1,"method":"ping"})"));
            QVERIFY2(response.startsWith("HTTP/1.1 200 "), qPrintable(QString::number(i)));
        }
    }

    void receiveDeadline()
    {
        const auto response = exchange("POST /mcp HTTP/1.1\r\n");
        QVERIFY(response.startsWith("HTTP/1.1 408 "));
    }

    void headerLimit()
    {
        QVERIFY(exchange(QByteArray(8193, 'x')).startsWith("HTTP/1.1 413 "));
    }

    void noSseOrDelete()
    {
        auto data = request("{}");
        data.replace("POST /mcp", "GET /mcp");
        QVERIFY(exchange(data).startsWith("HTTP/1.1 405 "));
        data.replace("GET /mcp", "DELETE /mcp");
        QVERIFY(exchange(data).startsWith("HTTP/1.1 405 "));
    }

    void versionHeader()
    {
        QVERIFY(exchange(request("{}", "MCP-Protocol-Version: future\r\n"))
                    .startsWith("HTTP/1.1 400 "));
    }

    void fragmentedErrorRequests_data()
    {
        QTest::addColumn<QByteArray>("extra");
        QTest::addColumn<QByteArray>("auth");
        QTest::addColumn<int>("status");
        QTest::newRow("version") << QByteArray("MCP-Protocol-Version: future\r\n") << QByteArray()
                                 << 400;
        QTest::newRow("authentication") << QByteArray() << QByteArray("Bearer wrong") << 401;
        QTest::newRow("origin") << QByteArray("Origin: https://example.org\r\n") << QByteArray()
                                << 403;
    }

    void fragmentedErrorRequests()
    {
        QFETCH(QByteArray, extra);
        QFETCH(QByteArray, auth);
        QFETCH(int, status);
        const auto data = request("{}", extra, auth);
        // Header and body arrive in different event-loop iterations.
        const auto response = exchange(data, data.indexOf("\r\n\r\n") + 4);
        QVERIFY(response.startsWith("HTTP/1.1 " + QByteArray::number(status) + " "));
        QVERIFY(!mEarlyResponse);
    }

    void stopClosesClients()
    {
        QTcpSocket socket;
        socket.connectToHost(QHostAddress::LocalHost, mServer.port());
        QTRY_VERIFY(socket.state() == QAbstractSocket::ConnectedState);
        QTest::qWait(10);
        mServer.stop();
        QTRY_VERIFY(socket.state() == QAbstractSocket::UnconnectedState);
    }
    void connectionBudget()
    {
        QList<QTcpSocket *> clients;
        for (int i = 0; i < 8; ++i) {
            auto socket = new QTcpSocket(this);
            clients.append(socket);
            socket->connectToHost(QHostAddress::LocalHost, mServer.port());
        }
        for (auto socket : clients) {
            QTRY_COMPARE(socket->state(), QAbstractSocket::ConnectedState);
        }
        QVERIFY(exchange(request(R"({"jsonrpc":"2.0","id":1,"method":"ping"})")).isEmpty());
        mServer.stop();
        qDeleteAll(clients);
    }

    void invalidToken()
    {
        QVERIFY(!mServer.start(0, "short"));
        QVERIFY(!mServer.errorString().isEmpty());
        QVERIFY(!mServer.start(0, QByteArray(40, '\n')));
    }

    void parameterDoubleValidation()
    {
        ConfigParam param;
        param.setDouble(0.0, 5.0, -5.0);
        QString error;
        QVERIFY(McpParamValidator::validate(param, -5.0, &error));
        QVERIFY(McpParamValidator::validate(param, 5.0, &error));
        QVERIFY(!McpParamValidator::validate(param, 5.1, &error));
        QVERIFY(!McpParamValidator::validate(param, "1", &error));
        QVERIFY(!McpParamValidator::validate(param, QJsonValue(), &error));
    }

    void parameterIntegerValidation()
    {
        ConfigParam param;
        param.setInt(0, 100, 0);
        QString error;
        QVERIFY(McpParamValidator::validate(param, 100, &error));
        QVERIFY(!McpParamValidator::validate(param, -1, &error));
        QVERIFY(!McpParamValidator::validate(param, 1.5, &error));
        QVERIFY(!McpParamValidator::validate(param, 2147483648.0, &error));
        param.setEnum(0, {"zero", "one"});
        QVERIFY(McpParamValidator::validate(param, 1, &error));
        QVERIFY(!McpParamValidator::validate(param, 2, &error));
        QVERIFY(!McpParamValidator::validate(param, true, &error));
    }

    void parameterBoolAndStringValidation()
    {
        ConfigParam param;
        param.setBool(false);
        QString error;
        QVERIFY(McpParamValidator::validate(param, true, &error));
        QVERIFY(!McpParamValidator::validate(param, 1, &error));
        param.setString("", 3);
        QVERIFY(McpParamValidator::validate(param, "abc", &error));
        QVERIFY(!McpParamValidator::validate(param, "abcd", &error));
        QVERIFY(!McpParamValidator::validate(param, QString(QChar(0)), &error));
        param.type = CFG_T_BITFIELD;
        QVERIFY(!McpParamValidator::validate(param, 1, &error));
    }
};

QTEST_GUILESS_MAIN(TestMcpServer)
#include "tst_mcpserver.moc"
