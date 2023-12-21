/*
    Copyright 2022 Benjamin Vedder	benjamin@vedder.se

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

#include "pageespprog.h"
#include "ui_pageespprog.h"
#include "utility.h"
#include <QFileDialog>

PageEspProg::PageEspProg(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PageEspProg)
{
    ui->setupUi(this);
    layout()->setContentsMargins(0, 0, 0, 0);
    mVesc = nullptr;

    ui->blChooseButton->setIcon(Utility::getIcon("icons/Open Folder-96.png"));
    ui->partChooseButton->setIcon(Utility::getIcon("icons/Open Folder-96.png"));
    ui->appChooseButton->setIcon(Utility::getIcon("icons/Open Folder-96.png"));
    ui->flashButton->setIcon(Utility::getIcon("icons/Download-96.png"));
    ui->flashBlButton->setIcon(Utility::getIcon("icons/Download-96.png"));
    ui->serialConnectButton->setIcon(Utility::getIcon("icons/Connected-96.png"));
    ui->serialDisconnectButton->setIcon(Utility::getIcon("icons/Disconnected-96.png"));
    ui->serialRefreshButton->setIcon(Utility::getIcon("icons/Refresh-96.png"));

    QSettings set;
    if (set.contains("pageespprog/lastcustomblfile")) {
        ui->blEdit->setText(set.value("pageespprog/lastcustomblfile").toString());
    }
    if (set.contains("pageespprog/lastcustompartfile")) {
        ui->partEdit->setText(set.value("pageespprog/lastcustompartfile").toString());
    }
    if (set.contains("pageespprog/lastcustomappfile")) {
        ui->appEdit->setText(set.value("pageespprog/lastcustomappfile").toString());
    }

    mTimer = new QTimer(this);
    connect(mTimer, SIGNAL(timeout()), this, SLOT(timerSlot()));
    mTimer->start(50);

    connect(&mEspFlash, &Esp32Flash::flashProgress, [this](double progress) {
        ui->progWidget->setValue(progress * 100.0);
    });

    connect(&mEspFlash, &Esp32Flash::stateUpdate, [this](QString msg) {
        ui->progWidget->setText(msg);
    });

    mVescUploadOngoing = false;

    listAllFw();
}

PageEspProg::~PageEspProg()
{
    QSettings set;
    set.setValue("pageespprog/lastcustomblfile", ui->blEdit->text());
    set.setValue("pageespprog/lastcustompartfile", ui->partEdit->text());
    set.setValue("pageespprog/lastcustomappfile", ui->appEdit->text());
    delete ui;
}

VescInterface *PageEspProg::vesc() const
{
    return mVesc;
}

void PageEspProg::setVesc(VescInterface *vesc)
{
    mVesc = vesc;

    connect(mVesc, &VescInterface::fwUploadStatus, [this]
            (QString status, double progress, bool isOngoing) {
        mVescUploadOngoing = isOngoing;
        ui->progWidget->setValue(progress * 100.0);
        ui->progWidget->setText(status);
    });

    on_serialRefreshButton_clicked();
}

void PageEspProg::timerSlot()
{
    bool vescConn = false;
    if (mVesc) {
        vescConn = mVesc->isPortConnected();
    }

    ui->serialConnectButton->setEnabled(!mEspFlash.isEspConnected());
    ui->serialDisconnectButton->setEnabled(mEspFlash.isEspConnected());
    ui->flashBlButton->setEnabled(!mEspFlash.isEspConnected() && vescConn && !mVescUploadOngoing);

    if (!mEspFlash.isEspConnected() && ui->flashButton->isEnabled()) {
        ui->flashButton->setEnabled(false);
    }
}

void PageEspProg::on_serialRefreshButton_clicked()
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

void PageEspProg::on_serialDisconnectButton_clicked()
{
    mEspFlash.disconnectEsp();
    listAllFw();
}

void PageEspProg::on_serialConnectButton_clicked()
{
    if (mEspFlash.connectEsp(ui->serialPortBox->currentData().toString())) {
        switch (mEspFlash.getTarget()) {
        case ESP32C3_CHIP: {
            ui->fwList->clear();

            QDir dir("://res/firmwares_esp/ESP32-C3");
            dir.setSorting(QDir::Name);
            for (auto fi: dir.entryInfoList()) {
                QFileInfo fiApp(fi.absoluteFilePath() + "/vesc_express.bin");
                QFileInfo fiBl(fi.absoluteFilePath() + "/bootloader.bin");
                QFileInfo fiPart(fi.absoluteFilePath() + "/partition-table.bin");

                if (fiApp.exists() && fiBl.exists() && fiPart.exists()) {
                    addFwToList(fi.fileName(), fi.canonicalFilePath());
                }
            }
            ui->flashButton->setEnabled(true);
        } break;

        default:
            break;
        }
    }
}

void PageEspProg::on_flashButton_clicked()
{
    QString blPath;
    QString partPath;
    QString appPath;

    if (ui->tabWidget->currentIndex() == 0) {
        auto item = ui->fwList->currentItem();
        if (item != nullptr) {
            QString path = item->data(Qt::UserRole).toString();
            blPath = path + "/bootloader.bin";
            partPath = path + "/partition-table.bin";
            appPath = path + "/vesc_express.bin";
        } else {
            mVesc->emitMessageDialog("Flash Firmware",
                                     "No Firmware Selected",
                                     false, false);
            return;
        }
    } else {
        blPath = ui->blEdit->text();
        partPath = ui->partEdit->text();
        appPath = ui->appEdit->text();
    }

    ui->flashButton->setEnabled(false);

    QFile fBl(blPath);
    if (!fBl.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open bootloader";
        ui->flashButton->setEnabled(true);
        return;
    }

    QFile fPart(partPath);
    if (!fPart.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open partition table";
        ui->flashButton->setEnabled(true);
        return;
    }

    QFile fApp(appPath);
    if (!fApp.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open application";
        ui->flashButton->setEnabled(true);
        return;
    }

#if 0 // This suddenly started working??
    // The builtin USB disconnects after a while for some reason. This does not
    // happen in esptool.py, but even after looking through that source code I
    // have not figured out what they are doing differently. This hack erases
    // the bootloader, which will make the ESP reboot and not boot properly.
    // Connecting to it in that state seems to not disconnect.
    if (mEspFlash.isBuiltinUsb()) {
        mEspFlash.eraseFlash(fBl.size(), 0x0);

        auto port = mEspFlash.espPort();
        mEspFlash.disconnectEsp();
        ui->progWidget->setText("Waiting for reboot...");
        Utility::sleepWithEventLoop(9000);

        for (int i = 0; i < 5;i++) {
            if (mEspFlash.connectEsp(port)) {
                break;
            }
            Utility::sleepWithEventLoop(500);
        }
    }
#endif

    if (!mEspFlash.isEspConnected()) {
        ui->flashButton->setEnabled(true);
        ui->progWidget->setText("Could not reconnect");
        return;
    }

    ui->progWidget->setText("Flashing bootloader...");
    mEspFlash.flashFirmware(fBl.readAll(), 0x0);

    ui->progWidget->setText("Flashing partition table...");
    ui->progWidget->setValue(0.0);
    mEspFlash.flashFirmware(fPart.readAll(), ui->partOffsetBox->value());

    ui->progWidget->setText("Flashing firmware...");
    ui->progWidget->setValue(0.0);
    mEspFlash.flashFirmware(fApp.readAll(), ui->appOffsetBox->value());

    ui->flashButton->setEnabled(true);
}

void PageEspProg::on_blChooseButton_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Choose Bootloader"),
                                                    ui->blEdit->text(),
                                                    tr("Binary files (*.bin)"));
    if (!filename.isNull()) {
        ui->blEdit->setText(filename);
    }
}

void PageEspProg::on_partChooseButton_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Choose Partition Table"),
                                                    ui->partEdit->text(),
                                                    tr("Binary files (*.bin)"));
    if (!filename.isNull()) {
        ui->partEdit->setText(filename);
    }
}

void PageEspProg::on_appChooseButton_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Choose Application"),
                                                    ui->appEdit->text(),
                                                    tr("Binary files (*.bin)"));
    if (!filename.isNull()) {
        ui->appEdit->setText(filename);
    }
}

void PageEspProg::addFwToList(QString name, QString path)
{
    QListWidgetItem *item = new QListWidgetItem;
    item->setText(name);
    item->setData(Qt::UserRole, QVariant::fromValue(path));
    ui->fwList->insertItem(ui->fwList->count(), item);
}

void PageEspProg::on_flashBlButton_clicked()
{
    QString appPath;

    if (ui->tabWidget->currentIndex() == 0) {
        auto item = ui->fwList->currentItem();
        if (item != nullptr) {
            QString path = item->data(Qt::UserRole).toString();
            appPath = path + "/vesc_express.bin";
        } else {
            mVesc->emitMessageDialog("Flash Firmware",
                                     "No Firmware Selected",
                                     false, false);
            return;
        }
    } else {
        appPath = ui->appEdit->text();
    }

    ui->flashButton->setEnabled(false);

    QFile fApp(appPath);
    if (!fApp.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open application";
        ui->flashButton->setEnabled(true);
        return;
    }

    auto fwData = fApp.readAll();
    fApp.close();

    mVesc->fwUpload(fwData, false, false, false);

    ui->flashButton->setEnabled(true);
}

void PageEspProg::on_cancelButton_clicked()
{
    mVesc->fwUploadCancel();
}

void PageEspProg::listAllFw()
{
    ui->fwList->clear();
    QDir dir("://res/firmwares_esp/ESP32-C3");
    dir.setSorting(QDir::Name);
    for (auto fi: dir.entryInfoList()) {
        QFileInfo fiApp(fi.absoluteFilePath() + "/vesc_express.bin");
        QFileInfo fiBl(fi.absoluteFilePath() + "/bootloader.bin");
        QFileInfo fiPart(fi.absoluteFilePath() + "/partition-table.bin");

        if (fiApp.exists() && fiBl.exists() && fiPart.exists()) {
            addFwToList(fi.fileName(), fi.canonicalFilePath());
        }
    }
}

