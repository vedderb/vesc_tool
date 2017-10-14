/*
    Copyright 2016 - 2017 Benjamin Vedder	benjamin@vedder.se

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

#include "pageconnection.h"
#include "ui_pageconnection.h"
#include "widgets/helpdialog.h"
#include "util.h"
#include <QMessageBox>
#include <QSettings>

PageConnection::PageConnection(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PageConnection)
{
    ui->setupUi(this);
    layout()->setContentsMargins(0, 0, 0, 0);

    QString lastTcpServer =
        QSettings().value("tcp_server", ui->tcpServerEdit->text()).toString();
    ui->tcpServerEdit->setText(lastTcpServer);

    int lastTcpPort =
        QSettings().value("tcp_port", ui->tcpPortBox->value()).toInt();
    ui->tcpPortBox->setValue(lastTcpPort);

    mVesc = 0;
    mTimer = new QTimer(this);

    connect(mTimer, SIGNAL(timeout()),
            this, SLOT(timerSlot()));

    mTimer->start(20);
}

PageConnection::~PageConnection()
{
    delete ui;
}

VescInterface *PageConnection::vesc() const
{
    return mVesc;
}

void PageConnection::setVesc(VescInterface *vesc)
{
    mVesc = vesc;
    on_serialRefreshButton_clicked();
}

void PageConnection::timerSlot()
{
    if (mVesc) {
        QString str = mVesc->getConnectedPortName();
        if (str != ui->statusLabel->text()) {
            ui->statusLabel->setText(mVesc->getConnectedPortName());
        }

        // CAN fwd
        if (ui->canFwdButton->isChecked() != mVesc->commands()->getSendCan()) {
            ui->canFwdButton->setChecked(mVesc->commands()->getSendCan());
        }
    }
}

void PageConnection::on_serialRefreshButton_clicked()
{
    if (mVesc) {
        ui->serialPortBox->clear();
        QList<VSerialInfo_t> ports = mVesc->listSerialPorts();
        foreach(const VSerialInfo_t &port, ports) {
            ui->serialPortBox->addItem(port.name, port.systemPath);
        }
        ui->serialPortBox->setCurrentIndex(0);

        on_canFwdBox_valueChanged(ui->canFwdBox->value());
    }
}

void PageConnection::on_serialDisconnectButton_clicked()
{
    if (mVesc) {
        mVesc->disconnectPort();
    }
}

void PageConnection::on_serialConnectButton_clicked()
{
    if (mVesc) {
        mVesc->connectSerial(ui->serialPortBox->currentData().toString(),
                             ui->serialBaudBox->value());
    }
}

void PageConnection::on_tcpDisconnectButton_clicked()
{
    if (mVesc) {
        mVesc->disconnectPort();
    }
}

void PageConnection::on_tcpConnectButton_clicked()
{
    if (mVesc) {
        QString tcpServer = ui->tcpServerEdit->text();
        int tcpPort = ui->tcpPortBox->value();
        mVesc->connectTcp(tcpServer, tcpPort);
        QSettings().setValue("tcp_server", tcpServer);
        QSettings().setValue("tcp_port", tcpPort);
    }
}

void PageConnection::on_canFwdBox_valueChanged(int arg1)
{
    if (mVesc) {
        mVesc->commands()->setCanSendId(arg1);
    }
}

void PageConnection::on_helpButton_clicked()
{
    if (mVesc) {
        HelpDialog::showHelp(this, mVesc->infoConfig(), "help_can_forward");
    }
}

void PageConnection::on_canFwdButton_toggled(bool checked)
{
    if (mVesc) {
        mVesc->commands()->setSendCan(checked);
    }
}

void PageConnection::on_autoConnectButton_clicked()
{
    util::autoconnectBlockingWithProgress(mVesc, this);
}
