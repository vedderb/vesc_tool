#include "tcphubrouter.h"

TcpHubRouter::TcpHubRouter(QObject *parent) : QObject(parent)
{
    mCommands = new Commands(this);
    mPacket = new Packet(this);
    connect(mPacket, SIGNAL(dataToSend(QByteArray&)),
            this, SLOT(packetData(QByteArray&)));
    connect(mPacket, SIGNAL(packetReceived(QByteArray&)),
            this, SLOT(packetReceived(QByteArray&)));
}

void TcpHubRouter::socketDisconnected()
{
    emit(disconnected(mTcpSocket->peerAddress().toString()));
}

void TcpHubRouter::socketReadyRead()
{
    QByteArray arr = mTcpSocket->readAll();
    qDebug() << "peer sent something: " << arr;
    mPacket->processData(arr);
}

void TcpHubRouter::packetData(QByteArray &packet)
{
    qDebug() << "packet is: " << packet;
}

void TcpHubRouter::packetReceived(QByteArray &packet)
{
    if (packet.size() != 2) {
        // Non desired packet.
        return;
    }
    if ((uint8_t)packet.at(0) == COMM_TCP_HUB_CONNECT) {
        if ((uint8_t)packet.at(1) == TCP_HUB_VESC_TOOL_CONNECTING) {
            emit vescToolConnection(mTcpSocket);
        } else if ((uint8_t)packet.at(1) == TCP_HUB_VESC_CONNECTING) {
            emit vescConnection(mTcpSocket);
        }
    } else {
        qDebug() << "packet received: " << packet;
    }


}

void TcpHubRouter::connect_client(QTcpSocket *socket)
{

}
