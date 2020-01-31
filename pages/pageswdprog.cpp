/*
    Copyright 2019 Benjamin Vedder	benjamin@vedder.se

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

#include <QFileDialog>
#include <QMessageBox>
#include "pageswdprog.h"
#include "ui_pageswdprog.h"
#include "utility.h"

PageSwdProg::PageSwdProg(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PageSwdProg)
{
    ui->setupUi(this);
    layout()->setContentsMargins(0, 0, 0, 0);
    mVesc = 0;

    mTimer = new QTimer(this);
    mTimer->start(500);
    mFlashOffset = 0;

    QSettings set;
    if (set.contains("pageswdprog/lastcustomfile")) {
        ui->fwEdit->setText(set.value("pageswdprog/lastcustomfile").toString());
    }

    connect(mTimer, SIGNAL(timeout()), this, SLOT(timerSlot()));
}

PageSwdProg::~PageSwdProg()
{
    QSettings set;
    set.setValue("pageswdprog/lastcustomfile", ui->fwEdit->text());
    delete ui;
}

void PageSwdProg::on_chooseButton_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Choose Firmware File"), ".",
                                                    tr("Binary files (*.bin)"));

    if (filename.isNull()) {
        return;
    }

    ui->fwEdit->setText(filename);
}

void PageSwdProg::on_connectButton_clicked()
{
    if (mVesc) {
        mVesc->commands()->bmMapPinsDefault();
        ui->connectButton->setEnabled(false);
        Utility::waitSignal(mVesc->commands(), SIGNAL(bmMapPinsDefaultRes(bool)), 100);
        ui->connectButton->setEnabled(true);
        mVesc->commands()->bmConnect();
    }
}

void PageSwdProg::on_uploadButton_clicked()
{
    if (mVesc) {
        if (!mVesc->isPortConnected()) {
            QMessageBox::critical(this,
                                  tr("Connection Error"),
                                  tr("The VESC is not connected."));
            return;
        }

        if (ui->tabWidget->currentIndex() == 0) {
            auto current = ui->fwList->currentItem();

            if (current) {
                SwdFw fw = current->data(Qt::UserRole).value<SwdFw>();

                if (!mVesc->swdEraseFlash()) {
                    return;
                }

                QFile file(fw.path);
                if (!file.exists()) {
                    QMessageBox::critical(this,
                                          tr("File Error"),
                                          tr("The FW file does not exist."));
                    return;
                }

                if (!file.open(QIODevice::ReadOnly)) {
                    QMessageBox::critical(this,
                                          tr("File Error"),
                                          tr("Could not open firmware file."));
                    return;
                }

                mVesc->swdUploadFw(file.readAll(), mFlashOffset + fw.addr, ui->verifyBox->isChecked());

                if (!fw.bootloaderPath.isEmpty()) {
                    QFile file2(fw.bootloaderPath);
                    if (!file2.exists()) {
                        QMessageBox::critical(this,
                                              tr("File Error"),
                                              tr("The bootloader file does not exist."));
                        return;
                    }

                    if (!file2.open(QIODevice::ReadOnly)) {
                        QMessageBox::critical(this,
                                              tr("File Error"),
                                              tr("Could not open bootloader file."));
                        return;
                    }

                    mVesc->swdUploadFw(file2.readAll(), mFlashOffset + fw.bootloaderAddr, ui->verifyBox->isChecked());
                }
            } else {
                QMessageBox::critical(this,
                                      tr("Upload Error"),
                                      tr("No firmware selected."));
                return;
            }
        } else if (ui->tabWidget->currentIndex() == 1) {
            QFile file(ui->fwEdit->text());
            if (!file.exists()) {
                QMessageBox::critical(this,
                                      tr("File Error"),
                                      tr("The FW file does not exist."));
                return;
            }
            if (!file.open(QIODevice::ReadOnly)) {
                QMessageBox::critical(this,
                                      tr("File Error"),
                                      tr("Could not open file. Make sure that the path is valid."));
                return;
            }
            if (file.size() > (1024 * 1024 * 5)) {
                QMessageBox::critical(this,
                                      tr("Upload Error"),
                                      tr("The selected file is too large to be a firmware."));
                return;
            }
            if (!mVesc->swdEraseFlash()) {
                return;
            }
            mVesc->swdUploadFw(file.readAll(), mFlashOffset, ui->verifyBox->isChecked());
        }

        mVesc->swdReboot();
    }
}

VescInterface *PageSwdProg::vesc() const
{
    return mVesc;
}

void PageSwdProg::setVesc(VescInterface *vesc)
{
    mVesc = vesc;

    connect(mVesc, SIGNAL(fwUploadStatus(QString,double,bool)),
            this, SLOT(fwUploadStatus(QString,double,bool)));
    connect(mVesc->commands(), SIGNAL(bmConnRes(int)),
            this, SLOT(bmConnRes(int)));

    connect(mVesc->commands(), &Commands::bmMapPinsNrf5xRes, [this](bool res) {
        if (!res) {
            mVesc->emitMessageDialog("Connect NRF5X",
                                     "This hardware version does not have a SWD connection to the "
                                     "NRF5X module.",
                                     false, false);
        }
    });
}

void PageSwdProg::timerSlot()
{
    if (mVesc && !mVesc->isPortConnected()) {
        ui->targetLabel->clear();
        ui->fwList->clear();
    }
}

void PageSwdProg::fwUploadStatus(const QString &status, double progress, bool isOngoing)
{
    if (isOngoing) {
        ui->display->setText(tr("%1 (%2 %)").
                             arg(status).
                             arg(progress * 100, 0, 'f', 1));
    } else {
        ui->display->setText(status);
    }

    ui->display->setValue(progress * 100.0);

    ui->connectButton->setEnabled(!isOngoing);
    ui->connectNrf5xButton->setEnabled(!isOngoing);
    ui->eraseFlashButton->setEnabled(!isOngoing);
    ui->disconnectButton->setEnabled(!isOngoing);
    ui->uploadButton->setEnabled(!isOngoing);
}

void PageSwdProg::bmConnRes(int res)
{
    mFlashOffset = 0;
    ui->fwList->clear();

    if (res == -2) {
        ui->targetLabel->clear();
        mVesc->emitMessageDialog("Connect to SWD Target", "Could not recognize target", false, false);
    } else if (res == -1) {
        ui->targetLabel->clear();
        mVesc->emitMessageDialog("Connect to SWD Target", "Could not connect to target", false, false);
    } else if (res == 1) {
        ui->targetLabel->setText("STM32F40x");
        mFlashOffset = 0x08000000;
    } else if (res == 2) {
        ui->targetLabel->setText("NRF51822 128K/16K");
    } else if (res == 3) {
        ui->targetLabel->setText("NRF51822 256K/16K");
    } else if (res == 4) {
        ui->targetLabel->setText("NRF51822 256K/32K");
    } else if (res == 5) {
        ui->targetLabel->setText("NRF52832 256K/32K");
    } else if (res == 6) {
        ui->targetLabel->setText("NRF52832 256K/64K");
    } else if (res == 7) {
        ui->targetLabel->setText("NRF52832 512K/64K");
    } else if (res == 8) {
        ui->targetLabel->setText("NRF52840 1M/256K");
    }

    switch (res) {
    case 1:
        // TODO: This can be auto-generated by parsing the file names.
        addSwdFw("VESC 4.6 & 4.7", "://res/firmwares/46_o_47/VESC_default.bin",
                 0, "://res/bootloaders/40_o_47_o_48_o_410_o_411_o_412_o_DAS_RS.bin");
        addSwdFw("VESC 4.8", "://res/firmwares/48/VESC_default.bin",
                 0, "://res/bootloaders/40_o_47_o_48_o_410_o_411_o_412_o_DAS_RS.bin");
        addSwdFw("VESC 4.10 - 4.12", "://res/firmwares/410_o_411_o_412/VESC_default.bin",
                 0, "://res/bootloaders/40_o_47_o_48_o_410_o_411_o_412_o_DAS_RS.bin");
        addSwdFw("VESC SIX", "://res/firmwares/60/VESC_default.bin",
                 0, "://res/bootloaders/60_o_75_300_o_HD_o_UAVC_OMEGA_o_75_300_R2_o_60_MK3_o_100_250.bin");
        addSwdFw("VESC 75/300 R1", "://res/firmwares/75_300/VESC_default.bin",
                 0, "://res/bootloaders/60_o_75_300_o_HD_o_UAVC_OMEGA_o_75_300_R2_o_60_MK3_o_100_250.bin");
        addSwdFw("VESC 75/300 R2", "://res/firmwares/75_300_R2/VESC_default.bin",
                 0, "://res/bootloaders/60_o_75_300_o_HD_o_UAVC_OMEGA_o_75_300_R2_o_60_MK3_o_100_250.bin");
        addSwdFw("VESC HD", "://res/firmwares/HD/VESC_default.bin",
                 0, "://res/bootloaders/60_o_75_300_o_HD_o_UAVC_OMEGA_o_75_300_R2_o_60_MK3_o_100_250.bin");
        addSwdFw("VESC SIX MK3", "://res/firmwares/60_MK3/VESC_default.bin",
                 0, "://res/bootloaders/60_o_75_300_o_HD_o_UAVC_OMEGA_o_75_300_R2_o_60_MK3_o_100_250.bin");
        addSwdFw("VESC 100/250", "://res/firmwares/100_250/VESC_default.bin",
                 0, "://res/bootloaders/60_o_75_300_o_HD_o_UAVC_OMEGA_o_75_300_R2_o_60_MK3_o_100_250.bin");
        break;

    case 2:
    case 3:
        addSwdFw("BLE - Xtal: 16M RX: 1 TX: 2 LED: 3",
                 "://res/other_fw/nrf51_vesc_ble_16k_16m_rx1_tx2_led3.bin");
        addSwdFw("BLE - Xtal: 16M RX: 11 TX: 9 LED: 3",
                 "://res/other_fw/nrf51_vesc_ble_16k_16m_rx11_tx9_led3.bin");
        break;

    case 4:
        addSwdFw("BLE - Xtal: 16M RX: 11 TX: 9 LED: 3",
                 "://res/other_fw/nrf51_vesc_ble_32k_16m_rx11_tx9_led3.bin");
        addSwdFw("BLE TRAMPA - Xtal: 32M RX: 2 TX: 1 LED: 3",
                 "://res/other_fw/nrf51_vesc_ble_32k_32m_rx2_tx1_led3.bin");
        addSwdFw("BLE VESC Builtin - Xtal: 32M RX: 1 TX: 2 LED: 3",
                 "://res/other_fw/nrf51_vesc_ble_32k_32m_rx1_tx2_led3.bin");
        addSwdFw("Remote Trampa MT - Xtal: 32M",
                 "://res/other_fw/nrf51_remote_mt_32k_32m.bin");
        break;

    case 5:
    case 6:
    case 7:
        addSwdFw("BLE TRAMPA - RX: 7 TX: 6 LED: 8",
                 "://res/other_fw/nrf52832_vesc_ble_rx7_tx6_led8.bin");
        addSwdFw("BLE VESC Builtin - RX: 6 TX: 7 LED: 8",
                 "://res/other_fw/nrf52832_vesc_ble_rx6_tx7_led8.bin");
        break;

    case 8:
        addSwdFw("BLE Sparkfun Mini - RX: 11 TX: 8 LED: 7",
                 "://res/other_fw/nrf52840_vesc_ble_rx11_tx8_led7.bin");
        addSwdFw("VESC HD Builtin - RX: 26 TX: 25 LED: 27",
                 "://res/other_fw/nrf52840_vesc_ble_rx26_tx25_led27.bin");
        addSwdFw("Wand Remote",
                 "://res/other_fw/nrf52840_stick_remote.bin");
        break;

    default:
        break;
    }
}

void PageSwdProg::on_disconnectButton_clicked()
{
    if (mVesc) {
        mVesc->commands()->bmDisconnect();
        ui->targetLabel->clear();
        ui->fwList->clear();
    }
}

void PageSwdProg::on_cancelButton_clicked()
{
    if (mVesc) {
        mVesc->swdCancel();
    }
}

void PageSwdProg::addSwdFw(QString name, QString path, uint32_t addr, QString blPath, uint32_t blAddr)
{
    QListWidgetItem *item = new QListWidgetItem;
    item->setText(name);
    SwdFw fw;
    fw.path = path;
    fw.addr = addr;
    fw.bootloaderPath = blPath;
    fw.bootloaderAddr = blAddr;
    item->setData(Qt::UserRole, QVariant::fromValue(fw));
    ui->fwList->insertItem(ui->fwList->count(), item);
}

void PageSwdProg::on_eraseFlashButton_clicked()
{
    if (mVesc) {
        if (!mVesc->isPortConnected()) {
            QMessageBox::critical(this,
                                  tr("Connection Error"),
                                  tr("The VESC is not connected."));
            return;
        }

        if (mVesc->swdEraseFlash()) {
            QMessageBox::information(this,
                                  tr("Erase Flash"),
                                  tr("The flash memory on the target was erased successfully!"));
        }
    }
}

void PageSwdProg::on_connectNrf5xButton_clicked()
{
    if (mVesc) {
        mVesc->commands()->bmMapPinsNrf5x();
        ui->connectNrf5xButton->setEnabled(false);
        Utility::waitSignal(mVesc->commands(), SIGNAL(bmMapPinsNrf5xRes(bool)), 100);
        ui->connectNrf5xButton->setEnabled(true);
        mVesc->commands()->bmConnect();
    }
}
