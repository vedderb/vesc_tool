#ifndef TCPCONNECTEDVESC_H
#define TCPCONNECTEDVESC_H

#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>

#include <packet.h>
#include <commands.h>
#include <datatypes.h>

class TcpConnectedVesc : public QObject
{
    Q_OBJECT

private:
    QTcpSocket *mTcpSocket;

public:
    explicit TcpConnectedVesc(QObject *parent = nullptr);
    Packet *mPacket;
    QTcpSocket *tcpSocket() const;
    void setTcpSocket(QTcpSocket *tcpSocket);

signals:
    void connectionChanged(bool state, QHostAddress addr);
public slots:


};

#endif // TCPCONNECTEDVESC_H
