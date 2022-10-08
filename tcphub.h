#ifndef TCPHUB_H
#define TCPHUB_H

#include <QObject>
#include <QMap>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include "packet.h"

/*
 *  - VESC connects to Server (HUB) and gets registered.
 *  - Timeout if no valid data from connected thing.
 *  - VESC sends its FW-info and a password message that gets associated with the TCP connection
 *    - Break out the FW_info command (assemble info) make callable form VESC_C_IF extension.
 *  - Connected vesc sends IM-ALIVE messages periodically.
 *
 *  - VESC_TOOL connects to server and and can list connected VESCS
 *  - VESC_TOOL can connect to a VESC, via the HUB, if it provides the correct password.
 *
 *  - If VESC_TOOL drops, reconnect if made avaialable again
 *  - If the VESC drops, kill connection.
 */

class TcpConnectedVesc : public QObject
{
    Q_OBJECT

public:
    explicit TcpConnectedVesc(QObject *parent = nullptr);
    Packet *mPacket;
    QTcpSocket *mTcpSocket;

signals:
    void connectionChanged(bool state, QHostAddress addr);
public slots:
};

class TcpHub : public QObject
{
    Q_OBJECT
public:
    explicit TcpHub(QObject *parent = nullptr);
    bool start(int port, QHostAddress addr = QHostAddress::Any);

signals:

public slots:
    void newTcpHubConnection();
    void periodic();

private:
    QTimer *mPeriodic;

    // Not entirely happy with a QString key. But QHostAddress is not possible.
    QMap<QString, TcpConnectedVesc*> mTcpConnectedVescs;
    QTcpServer *mTcpHubServer;
};

#endif // TCPHUB_H
