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

    ui->CANbusScanButton->setIcon(Utility::getIcon("icons/Refresh-96.png"));
    ui->canRefreshButton->setIcon(Utility::getIcon("icons/Refresh-96.png"));
    ui->CANbusConnectButton->setIcon(Utility::getIcon("icons/Connected-96.png"));
    ui->CANbusDisconnectButton->setIcon(Utility::getIcon("icons/Disconnected-96.png"));
    ui->tcpConnectButton->setIcon(Utility::getIcon("icons/Connected-96.png"));
    ui->tcpDisconnectButton->setIcon(Utility::getIcon("icons/Disconnected-96.png"));
    ui->udpConnectButton->setIcon(Utility::getIcon("icons/Connected-96.png"));
    ui->udpDisconnectButton->setIcon(Utility::getIcon("icons/Disconnected-96.png"));
    ui->serialRefreshButton->setIcon(Utility::getIcon("icons/Refresh-96.png"));
    ui->serialConnectButton->setIcon(Utility::getIcon("icons/Connected-96.png"));
    ui->serialDisconnectButton->setIcon(Utility::getIcon("icons/Disconnected-96.png"));
    ui->bleConnectButton->setIcon(Utility::getIcon("icons/Connected-96.png"));
    ui->bleDisconnectButton->setIcon(Utility::getIcon("icons/Disconnected-96.png"));
    ui->bleScanButton->setIcon(Utility::getIcon("icons/Refresh-96.png"));
    ui->CANbusScanButton->setIcon(Utility::getIcon("icons/Refresh-96.png"));
    ui->addConnectedButton->setIcon(Utility::getIcon("icons/Plus Math-96.png"));
    ui->addUuidButton->setIcon(Utility::getIcon("icons/Plus Math-96.png"));
    ui->unpairButton->setIcon(Utility::getIcon("icons/Restart-96.png"));
    ui->deletePairedButton->setIcon(Utility::getIcon("icons/Delete-96.png"));
    ui->clearPairedButton->setIcon(Utility::getIcon("icons/Delete-96.png"));
    ui->canDefaultButton->setIcon(Utility::getIcon("icons/Bug-96.png"));
    ui->helpButton->setIcon(Utility::getIcon("icons/Help-96.png"));
    ui->autoConnectButton->setIcon(Utility::getIcon("icons/Wizard-96.png"));
    ui->bleSetNameButton->setIcon(Utility::getIcon("icons/Ok-96.png"));
    ui->pairConnectedButton->setIcon(Utility::getIcon("icons/Circled Play-96.png"));
    ui->tcpHubConnectButton->setIcon(Utility::getIcon("icons/Connected-96.png"));
    ui->tcpHubDisconnectButton->setIcon(Utility::getIcon("icons/Disconnected-96.png"));
    ui->hubDefaultButton->setIcon(Utility::getIcon("icons/Restart-96.png"));
    ui->tcpDetectConnectButton->setIcon(Utility::getIcon("icons/Connected-96.png"));
    ui->tcpDetectDisconnectButton->setIcon(Utility::getIcon("icons/Disconnected-96.png"));

    QIcon mycon = QIcon(Utility::getIcon("icons/can_off.png"));
    mycon.addPixmap(Utility::getIcon("icons/can_on.png"), QIcon::Normal, QIcon::On);
    ui->canFwdButton->setIcon(mycon);

    mUdpListen = new UdpServerSimple(this);
    mUdpListen->startServerBroadcast(65109);

    connect(mUdpListen, &UdpServerSimple::dataRx, [this](const QByteArray &data) {
        QString str(data);
        auto tokens = str.split("::");
        if (tokens.size() == 3) {
            auto name = tokens.at(0);
            auto ip = tokens.at(1);
            auto port = tokens.at(2);
            tokens.append(QString::number(QTime::currentTime().msecsSinceStartOfDay()));

            bool found = false;
            for (int i = 0;i < ui->tcpDetectBox->count();i++) {
                auto d = ui->tcpDetectBox->itemData(i).toStringList();
                if (d.at(1) == ip) {
                    ui->tcpDetectBox->setItemData(i, tokens);
                    found = true;
                    break;
                }
            }

            if (!found) {
                QString itemName = name + " - " + ip + ":" + port;
                ui->tcpDetectBox->addItem(itemName, tokens);
            }
        }
    });
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

    ui->udpServerEdit->setText(mVesc->getLastUdpServer());
    ui->udpPortBox->setValue(mVesc->getLastUdpPort());

    ui->tcpHubServerEdit->setText(mVesc->getLastTcpHubServer());
    ui->tcpHubPortBox->setValue(mVesc->getLastTcpHubPort());
    ui->tcpHubVescIdLineEdit->setText(mVesc->getLastTcpHubVescID());
    ui->tcpHubVescPasswordLineEdit->setText(mVesc->getLastTcpHubVescPass());

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
    for (int i = 0;i < ui->tcpDetectBox->count();i++) {
        auto d = ui->tcpDetectBox->itemData(i).toStringList();
        if ((QTime::currentTime().msecsSinceStartOfDay() - d.at(3).toInt()) > 3000) {
            ui->tcpDetectBox->removeItem(i);
            break;
        }
    }

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

    QString tcpIpTxt = "Server IPs\n";
    QString tcpClientTxt = "Connected Clients\n";
    if (mVesc->tcpServerIsRunning()) {
        for (auto adr: Utility::getNetworkAddresses()) {
            tcpIpTxt += adr.toString() + "\n";
        }

        if (mVesc->tcpServerIsClientConnected()) {
            tcpClientTxt += mVesc->tcpServerClientIp();
        }
    }
    else {
        ui->tcpServerPortBox->setEnabled(true);
    }

    if (ui->tcpServerAddressesEdit->toPlainText() != tcpIpTxt) {
        ui->tcpServerAddressesEdit->setPlainText(tcpIpTxt);
    }

    if (ui->tcpServerClientsEdit->toPlainText() != tcpClientTxt) {
        ui->tcpServerClientsEdit->setPlainText(tcpClientTxt);
    }

    QString udpIpTxt = "Server IPs\n";
    QString udpClientTxt = "Connected Clients\n";
    if(mVesc->udpServerIsRunning()) {
        for (auto adr: Utility::getNetworkAddresses()) {
            udpIpTxt += adr.toString() + "\n";
        }

        if (mVesc->udpServerIsClientConnected()) {
            udpClientTxt += mVesc->udpServerClientIp();
        }
    }
    else {
        ui->udpServerPortBox->setEnabled(true);
    }

    if (ui->udpServerAddressesEdit->toPlainText() != udpIpTxt) {
        ui->udpServerAddressesEdit->setPlainText(udpIpTxt);
    }

    if (ui->udpServerClientsEdit->toPlainText() != udpClientTxt) {
        ui->udpServerClientsEdit->setPlainText(udpClientTxt);
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
        item->setIcon(QIcon("://res/icon.svg"));
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
        auto ports = mVesc->listSerialPorts();
        foreach(auto &info, ports) {
            auto port = info.value<VSerialInfo_t>();
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

void PageConnection::on_udpDisconnectButton_clicked()
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

void PageConnection::on_udpConnectButton_clicked()
{
    if (mVesc) {
        QString udpServer = ui->udpServerEdit->text();
        int udpPort = ui->udpPortBox->value();
        mVesc->connectUdp(udpServer, udpPort);
    }
}

void PageConnection::on_helpButton_clicked()
{
    if (mVesc) {
        HelpDialog::showHelp(this, mVesc->infoConfig(), "help_can_forward");
    }
}

void PageConnection::on_canFwdButton_clicked()
{
    bool checked = ui->canFwdButton->isChecked();

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

void PageConnection::on_canFwdBox_activated(int)
{
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

void PageConnection::on_tcpServerEnableBox_toggled(bool isEnabled)
{
    if (mVesc) {
        if (isEnabled)
        {
            mVesc->tcpServerStart(ui->tcpServerPortBox->value());
            ui->tcpServerPortBox->setEnabled(false);
        } else {
            mVesc->tcpServerStop();
        }
    }
}

void PageConnection::on_udpServerEnableBox_toggled(bool isEnabled)
{
    if (mVesc) {
        if (isEnabled) {
            mVesc->udpServerStart(ui->udpServerPortBox->value());
            ui->udpServerPortBox->setEnabled(false);
        } else {
           mVesc->udpServerStop();
        }
    }
}

void PageConnection::on_tcpDetectConnectButton_clicked()
{
    if (mVesc && ui->tcpDetectBox->count() > 0) {
        auto d = ui->tcpDetectBox->currentData().toStringList();
        mVesc->connectTcp(d.at(1), d.at(2).toInt());
    }
}

void PageConnection::on_tcpDetectDisconnectButton_clicked()
{
    if (mVesc) {
        mVesc->disconnectPort();
    }
}

void PageConnection::on_tcpHubConnectButton_clicked()
{
    if (mVesc) {
        QString tcpServer = ui->tcpHubServerEdit->text();
        int tcpPort = ui->tcpHubPortBox->value();
        QString uuid = ui->tcpHubVescIdLineEdit->text().replace(" ", "").replace(":", "").toUpper();
        QString pass = ui->tcpHubVescPasswordLineEdit->text();

        if (ui->hubClientButton->isChecked()) {
            mVesc->connectTcpHub(tcpServer, tcpPort, uuid, pass);
        } else {
            mVesc->tcpServerConnectToHub(tcpServer, tcpPort, uuid, pass);
        }
    }
}

void PageConnection::on_tcpHubDisconnectButton_clicked()
{
    if (mVesc) {
        if (ui->hubClientButton->isChecked()) {
            mVesc->disconnectPort();
        } else {
            mVesc->tcpServerStop();
        }
    }
}

void PageConnection::on_hubDefaultButton_clicked()
{
    ui->tcpHubServerEdit->setText("veschub.vedder.se");
    ui->tcpHubPortBox->setValue(65101);
}
