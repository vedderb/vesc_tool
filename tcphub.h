/*
    Copyright 2022 Benjamin Vedder	benjamin@vedder.se
    Copyright 2022 Joel Svensson    svenssonjoel@yahoo.se

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

#ifndef TCPHUB_H
#define TCPHUB_H

#include <QObject>
#include <QMap>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

/*
 *  - VESC connects to Server (HUB) and gets registered.
 *  - Timeout if no valid data from connected thing.
 *  - VESC sends its FW-info and a password message that gets associated with the TCP connection
 *  - Connected vesc sends IM-ALIVE messages periodically.
 *
 *  - VESC_TOOL connects to server and asks if vesc id is online
 *  - VESC_TOOL can connect to a VESC, via the HUB, if it provides the correct password.
 *
 *  - If VESC_TOOL drops, reconnect if made avaialable again
 *  - If the VESC drops, kill connection.
 */

struct TcpConnectedVesc
{
    TcpConnectedVesc() {
        vescSocket = nullptr;
        vescToolSocket = nullptr;
    }

    ~TcpConnectedVesc() {
        if (vescSocket != nullptr) {
            vescSocket->close();
            vescSocket->deleteLater();
        }

        if (vescToolSocket != nullptr) {
            vescToolSocket->close();
            vescToolSocket->deleteLater();
        }
    }

    QString pass;
    QTcpSocket *vescSocket;
    QTcpSocket *vescToolSocket;
};

class TcpHub : public QObject
{
    Q_OBJECT
public:
    explicit TcpHub(QObject *parent = nullptr);
    ~TcpHub();

    bool start(int port, QHostAddress addr);
    Q_INVOKABLE bool start(int port) {return start(port, QHostAddress::Any);}
    Q_INVOKABLE static bool ping(QString server, int port, QString uuid);

signals:

private slots:
    void newTcpHubConnection();

private:
    QMap<QString, TcpConnectedVesc*> mConnectedVescs;
    QTcpServer *mTcpHubServer;

};

#endif // TCPHUB_H
