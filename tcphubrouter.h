#ifndef TCPHUBROUTER_H
#define TCPHUBROUTER_H

#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>
#include <QByteArray>

#include <packet.h>
#include <commands.h>
#include <datatypes.h>

class TcpHubRouter : public QObject
{
    Q_OBJECT
public:
    explicit TcpHubRouter(QObject *parent = nullptr);
    QTcpSocket *mTcpSocket;
    Packet *mPacket;
    Commands *mCommands;
signals:
    void disconnected(QString);
    void vescConnection(QTcpSocket *socket);
    void vescToolConnection(QTcpSocket *socket);

public slots:
    void socketDisconnected();
    void socketReadyRead();

    void packetData(QByteArray& packet);
    void packetReceived(QByteArray& packet);

public:
    void connect_client(QTcpSocket *socket);


};

#endif // TCPHUBROUTER_H
