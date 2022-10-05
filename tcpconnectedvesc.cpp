#include "tcpconnectedvesc.h"

TcpConnectedVesc::TcpConnectedVesc(QObject *parent) : QObject{parent}
{

}

void TcpConnectedVesc::setTcpSocket(QTcpSocket *tcpSocket)
{
    mTcpSocket = tcpSocket;
}

QTcpSocket *TcpConnectedVesc::tcpSocket() const
{
    return mTcpSocket;
}
