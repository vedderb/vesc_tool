#include "tcpconnectedtool.h"

TcpConnectedTool::TcpConnectedTool(QObject *parent) : QObject{parent}
{
    mPacket = new Packet(this);
    mVescPacket = new Packet(this);

    //connect(mPacket, SIGNAL(packetReceived(QByteArray&)), this, SLOT(tcpPacketReceived(QByteArray&)));
    //connect(mPacket, SIGNAL(dataToSend(QByteArray&)), this, SLOT(tcpSendData(QByteArray&)));
}

QTcpSocket *TcpConnectedTool::tcpSocket() const
{
    return mTcpSocket;
}

void TcpConnectedTool::setTcpSocket(QTcpSocket *tcpSocket)
{
    mTcpSocket = tcpSocket;
    connect(mTcpSocket, SIGNAL(readyRead()), this, SLOT(toolReadyRead()));
    connect(mPacket, SIGNAL(dataToSend(QByteArray&)), this, SLOT(toolSendData(QByteArray&)));
    // Connection status should be handled by the hub.
    //connect(mTcpSocket, SIGNAL(disconnected()), this, SLOT(toolDisconnected()));
}

QTcpSocket *TcpConnectedTool::tcpVescSocket() const
{
    return mTcpVescSocket;
}

void TcpConnectedTool::setTcpVescSocket(QTcpSocket *tcpVescSocket)
{
    mTcpVescSocket = tcpVescSocket;

    connect(mTcpVescSocket, SIGNAL(readyRead()), this, SLOT(vescReadyRead()));
    connect(mVescPacket, SIGNAL(dataToSend(QByteArray&)),this, SLOT(vescSendData(QByteArray&)));
    // Connection status should be handled by the hub.
    //connect(mTcpVescSocket, SIGNAL(disconnected()), this, SLOT(vescDisconnected())
}

void TcpConnectedTool::toolReadyRead()
{
    while (mTcpSocket->bytesAvailable() > 0) {
        mPacket->processData(mTcpSocket->readAll());
    }
}

void TcpConnectedTool::vescReadyRead()
{
    while (mTcpVescSocket->bytesAvailable() > 0) {
        mVescPacket->processData(mTcpSocket->readAll());
    }
}

void TcpConnectedTool::toolSendData(QByteArray &data)
{
    // Possible to intercept and inspect the data here.
    // in case there are hub connection messages interspersed
    if (mTcpVescSocket) {
        mTcpVescSocket->write(data);
    }
}

void TcpConnectedTool::vescSendData(QByteArray &data)
{
    if (mTcpSocket) {
        mTcpSocket->write(data);
    }
}


