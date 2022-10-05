#ifndef TCPCONNECTEDTOOL_H
#define TCPCONNECTEDTOOL_H

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>

#include <packet.h>
#include <commands.h>
#include <datatypes.h>

class TcpConnectedTool : public QObject
{
    Q_OBJECT


private:
    QTcpSocket *mTcpSocket = nullptr;
    QTcpSocket *mTcpVescSocket = nullptr;

signals:

public slots:
    void toolReadyRead();
    void vescReadyRead();

    void toolSendData(QByteArray& data);
    void vescSendData(QByteArray& data);

public:
    TcpConnectedTool(QObject *parent = nullptr);
    QTcpSocket *tcpSocket() const;
    void setTcpSocket(QTcpSocket *tcpSocket);
    QTcpSocket *tcpVescSocket() const;
    void setTcpVescSocket(QTcpSocket *tcpVescSocket);

    Packet *mPacket = nullptr;
    Packet *mVescPacket = nullptr;
};

#endif // TCPCONNECTEDTOOL_H
