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
#include "utility.h"
#include <QMessageBox>
#include <QListWidgetItem>
#include <QInputDialog>

PageConnection::PageConnection(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PageConnection)
{
    ui->setupUi(this);
    layout()->setContentsMargins(0, 0, 0, 0);

    mVesc = nullptr;
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

    ui->tcpServerEdit->setText(mVesc->getLastTcpServer());
    ui->tcpPortBox->setValue(mVesc->getLastTcpPort());

#ifdef HAS_BLUETOOTH
    connect(mVesc->bleDevice(), SIGNAL(scanDone(QVariantMap,bool)),
            this, SLOT(bleScanDone(QVariantMap,bool)));

    QString lastBleAddr = mVesc->getLastBleAddr();
    if (lastBleAddr != "") {
        QString setName = mVesc->getBleName(lastBleAddr);

        QString name;
        if (!setName.isEmpty()) {
            name += setName;
            name += " [";
            name += lastBleAddr;
            name += "]";
        } else {
            name = lastBleAddr;
        }
        ui->bleDevBox->insertItem(0, name, lastBleAddr);
    }
#endif

#ifdef HAS_SERIALPORT
    ui->serialBaudBox->setValue(mVesc->getLastSerialBaud());
#endif

#ifdef HAS_CANBUS
    ui->CANbusBitrateBox->setValue(mVesc->getLastCANbusBitrate());

    ui->CANbusInterfaceBox->clear();
    QList<QString> interfaces = mVesc->listCANbusInterfaces();

    for(int i = 0;i < interfaces.size();i++) {
        ui->CANbusInterfaceBox->addItem(interfaces.at(i), interfaces.at(i));
    }

    ui->CANbusInterfaceBox->setCurrentIndex(0);
#endif

    connect(mVesc->commands(), SIGNAL(pingCanRx(QVector<int>,bool)),
            this, SLOT(pingCanRx(QVector<int>,bool)));
    connect(mVesc, SIGNAL(CANbusNewNode(int)),
            this, SLOT(CANbusNewNode(int)));
    connect(mVesc, SIGNAL(CANbusInterfaceListUpdated()),
            this, SLOT(CANbusInterfaceListUpdated()));
    connect(mVesc, SIGNAL(pairingListUpdated()),
            this, SLOT(pairingListUpdated()));

    pairingListUpdated();
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

        if (ui->canFwdBox->count() > 0) {
            int canVal = ui->canFwdBox->currentData().toInt();
            if (canVal != mVesc->commands()->getCanSendId()) {
                for (int i = 0;i < ui->canFwdBox->count();i++) {
                    if (ui->canFwdBox->itemData(i).toInt() == canVal) {
                        ui->canFwdBox->setCurrentIndex(i);
                        break;
                    }
                }
            }
        }
    }

    QString ipTxt = "Server IPs\n";
    QString clientTxt = "Connected Clients\n";
    if (mVesc->tcpServerIsRunning()) {
        for (auto adr: Utility::getNetworkAddresses()) {
            ipTxt += adr.toString() + "\n";
        }

        if (mVesc->tcpServerIsClientConnected()) {
            clientTxt += mVesc->tcpServerClientIp();
        }
    } else {
        ui->tcpServerPortBox->setEnabled(true);
    }

    if (ui->tcpServerAddressesEdit->toPlainText() != ipTxt) {
        ui->tcpServerAddressesEdit->setPlainText(ipTxt);
    }

    if (ui->tcpServerClientsEdit->toPlainText() != clientTxt) {
        ui->tcpServerClientsEdit->setPlainText(clientTxt);
    }
}

void PageConnection::bleScanDone(QVariantMap devs, bool done)
{
#ifdef HAS_BLUETOOTH
    if (done) {
        ui->bleScanButton->setEnabled(true);
    }

    ui->bleDevBox->clear();
    for (auto d: devs.keys()) {
        QString devName = devs.value(d).toString();
        QString addr = d;
        QString setName = mVesc->getBleName(addr);

        if (!setName.isEmpty()) {
            QString name;
            name += setName;
            name += " [";
            name += addr;
            name += "]";
            ui->bleDevBox->insertItem(0, name, addr);
        } else if (devName.contains("VESC")) {
            QString name;
            name += devName;
            name += " [";
            name += addr;
            name += "]";
            ui->bleDevBox->insertItem(0, name, addr);
        } else {
            QString name;
            name += devName;
            name += " [";
            name += addr;
            name += "]";
            ui->bleDevBox->addItem(name, addr);
        }
    }
    ui->bleDevBox->setCurrentIndex(0);
#else
    (void)devs;
    (void)done;
#endif
}

void PageConnection::pingCanRx(QVector<int> devs, bool isTimeout)
{
    (void)isTimeout;
    ui->canRefreshButton->setEnabled(true);

    ui->canFwdBox->clear();
    for (int dev: devs) {
        ui->canFwdBox->addItem(QString("VESC %1").arg(dev), dev);
    }
}

void PageConnection::CANbusNewNode(int node)
{
    ui->CANbusTargetIdBox->addItem(QString::number(node), QString::number(node));
}

void PageConnection::CANbusInterfaceListUpdated()
{
    ui->CANbusInterfaceBox->clear();
    QList<QString> interfaces = mVesc->listCANbusInterfaces();

    for(int i = 0; i<interfaces.size(); i++) {
        ui->CANbusInterfaceBox->addItem(interfaces.at(i), interfaces.at(i));
    }

    ui->CANbusInterfaceBox->setCurrentIndex(0);
}

void PageConnection::pairingListUpdated()
{
    ui->pairedListWidget->clear();

    for (QString uuid: mVesc->getPairedUuids()) {
        QListWidgetItem *item = new QListWidgetItem;
        item->setText("UUID: " + uuid);
        item->setIcon(QIcon("://res/icon.png"));
        item->setData(Qt::UserRole, uuid);
        ui->pairedListWidget->addItem(item);
    }

    if (ui->pairedListWidget->count() > 0) {
        ui->pairedListWidget->setCurrentRow(0);
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

void PageConnection::on_CANbusScanButton_clicked()
{
    if (mVesc) {
        ui->CANbusScanButton->setEnabled(false);
        mVesc->connectCANbus("socketcan", ui->CANbusInterfaceBox->currentData().toString(),
                             ui->CANbusBitrateBox->value());

        ui->CANbusTargetIdBox->clear();
        mVesc->scanCANbus();
        ui->CANbusScanButton->setEnabled(true);
    }
}

void PageConnection::on_CANbusDisconnectButton_clicked()
{
    if (mVesc) {
        mVesc->disconnectPort();
    }
}

void PageConnection::on_CANbusConnectButton_clicked()
{
    if (mVesc) {
        mVesc->setCANbusReceiverID(ui->CANbusTargetIdBox->currentData().toInt());
        mVesc->connectCANbus("socketcan", ui->CANbusInterfaceBox->currentData().toString(),
                             ui->CANbusBitrateBox->value());
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
        if (mVesc->commands()->getCanSendId() >= 0 || !checked) {
            mVesc->commands()->setSendCan(checked);
        } else {
            mVesc->emitMessageDialog("CAN Forward",
                                     "No CAN device is selected. Click on the refresh button "
                                     "if the selection box is empty.",
                                     false, false);
        }
    }
}

void PageConnection::on_autoConnectButton_clicked()
{
    Utility::autoconnectBlockingWithProgress(mVesc, this);
}

void PageConnection::on_bleScanButton_clicked()
{
#ifdef HAS_BLUETOOTH
    if (mVesc) {
        mVesc->bleDevice()->startScan();
        ui->bleScanButton->setEnabled(false);
    }
#endif
}

void PageConnection::on_bleDisconnectButton_clicked()
{
    if (mVesc) {
        mVesc->disconnectPort();
    }
}

void PageConnection::on_bleConnectButton_clicked()
{
    if (mVesc) {
        if (ui->bleDevBox->count() > 0) {
            mVesc->connectBle(ui->bleDevBox->currentData().toString());
        }
    }
}

void PageConnection::on_bleSetNameButton_clicked()
{
#ifdef HAS_BLUETOOTH
    if (mVesc) {
        QString name = ui->bleNameEdit->text();
        QString addr = ui->bleDevBox->currentData().toString();

        if (!name.isEmpty()) {
            mVesc->storeBleName(addr, name);
            name += " [";
            name += addr;
            name += "]";
            ui->bleDevBox->removeItem(0);
            ui->bleDevBox->insertItem(0, name, addr);
            ui->bleDevBox->setCurrentIndex(0);
        }
    }
#endif
}

void PageConnection::on_canFwdBox_currentIndexChanged(const QString &arg1)
{
    (void)arg1;
    if (mVesc && ui->canFwdBox->count() > 0) {
        mVesc->commands()->setCanSendId(quint32(ui->canFwdBox->currentData().toInt()));
    }
}

void PageConnection::on_canRefreshButton_clicked()
{
    if (mVesc) {
        ui->canRefreshButton->setEnabled(false);
        mVesc->commands()->pingCan();
    }
}

void PageConnection::on_canDefaultButton_clicked()
{
    ui->canFwdBox->clear();
    for (int dev = 0;dev < 255;dev++) {
        ui->canFwdBox->addItem(QString("VESC %1").arg(dev), dev);
    }
}

void PageConnection::on_pairConnectedButton_clicked()
{
    if (mVesc) {
        if (mVesc->isPortConnected()) {
            if (mVesc->commands()->isLimitedMode()) {
                mVesc->emitMessageDialog("Pair VESC",
                                         "The fiwmare must be updated to pair this VESC.",
                                         false, false);
            } else {
                QMessageBox::StandardButton reply;
                reply = QMessageBox::warning(this,
                                             tr("Pair connected VESC"),
                                             tr("This is going to pair the connected VESC with this instance of VESC Tool. VESC Tool instances "
                                                "that are not paired with this VESC will not be able to connect over bluetooth any more. Continue?"),
                                             QMessageBox::Ok | QMessageBox::Cancel);
                if (reply == QMessageBox::Ok) {
                    mVesc->addPairedUuid(mVesc->getConnectedUuid());
                    mVesc->storeSettings();
                    ConfigParams *ap = mVesc->appConfig();
                    mVesc->commands()->getAppConf();
                    bool res = Utility::waitSignal(ap, SIGNAL(updated()), 1500);

                    if (res) {
                        mVesc->appConfig()->updateParamBool("pairing_done", true, nullptr);
                        mVesc->commands()->setAppConf();
                    }
                }
            }
        } else {
            mVesc->emitMessageDialog("Pair VESC",
                                     "You are not connected to the VESC. Connect in order to pair it.",
                                     false, false);
        }
    }
}

void PageConnection::on_addConnectedButton_clicked()
{
    if (mVesc) {
        if (mVesc->isPortConnected()) {
            mVesc->addPairedUuid(mVesc->getConnectedUuid());
            mVesc->storeSettings();
        } else {
            mVesc->emitMessageDialog("Add UUID",
                                     "You are not connected to the VESC. Connect in order to add it.",
                                     false, false);
        }
    }
}

void PageConnection::on_deletePairedButton_clicked()
{
    if (mVesc) {
        QListWidgetItem *item = ui->pairedListWidget->currentItem();

        if (item) {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::warning(this,
                                         tr("Delete paired VESC"),
                                         tr("This is going to delete this VESC from the paired list. If that VESC "
                                            "has the pairing flag set you won't be able to connect to it over BLE "
                                            "any more. Are you sure?"),
                                         QMessageBox::Ok | QMessageBox::Cancel);
            if (reply == QMessageBox::Ok) {
                mVesc->deletePairedUuid(item->data(Qt::UserRole).toString());
                mVesc->storeSettings();
            }
        }
    }
}

void PageConnection::on_clearPairedButton_clicked()
{
    if (mVesc) {
        QListWidgetItem *item = ui->pairedListWidget->currentItem();

        if (item) {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::warning(this,
                                         tr("Clear paired VESCs"),
                                         tr("This is going to clear the pairing list of this instance of VESC Tool. Are you sure?"),
                                         QMessageBox::Ok | QMessageBox::Cancel);
            if (reply == QMessageBox::Ok) {
                mVesc->clearPairedUuids();
                mVesc->storeSettings();
            }
        }
    }
}

void PageConnection::on_addUuidButton_clicked()
{
    if (mVesc) {
        bool ok;
        QString text = QInputDialog::getText(this, "Add UUID",
                                             "UUID:", QLineEdit::Normal,
                                             "", &ok);
        if (ok) {
            mVesc->addPairedUuid(text);
            mVesc->storeSettings();
        }
    }
}

void PageConnection::on_unpairButton_clicked()
{
    if (mVesc) {
        if (mVesc->isPortConnected()) {
            if (mVesc->commands()->isLimitedMode()) {
                mVesc->emitMessageDialog("Unpair VESC",
                                         "The fiwmare must be updated on this VESC first.",
                                         false, false);
            } else {
                QListWidgetItem *item = ui->pairedListWidget->currentItem();

                if (item) {
                    QMessageBox::StandardButton reply;
                    reply = QMessageBox::warning(this,
                                                 tr("Unpair connected VESC"),
                                                 tr("This is going to unpair the connected VESC. Continue?"),
                                                 QMessageBox::Ok | QMessageBox::Cancel);
                    if (reply == QMessageBox::Ok) {
                        ConfigParams *ap = mVesc->appConfig();
                        mVesc->commands()->getAppConf();
                        bool res = Utility::waitSignal(ap, SIGNAL(updated()), 1500);

                        if (res) {
                            mVesc->appConfig()->updateParamBool("pairing_done", false, nullptr);
                            mVesc->commands()->setAppConf();
                            mVesc->deletePairedUuid(mVesc->getConnectedUuid());
                            mVesc->storeSettings();
                        }
                    }
                }
            }
        } else {
            mVesc->emitMessageDialog("Unpair VESC",
                                     "You are not connected to the VESC. Connect in order to unpair it.",
                                     false, false);
        }
    }
}

void PageConnection::on_tcpServerEnableBox_toggled(bool arg1)
{
    if (mVesc) {
        if (arg1) {
            mVesc->tcpServerStart(ui->tcpServerPortBox->value());
            ui->tcpServerPortBox->setEnabled(false);
        } else {
            mVesc->tcpServerStop();
        }
    }
}
