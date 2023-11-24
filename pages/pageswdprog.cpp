/*
    Copyright 2019 - 2021 Benjamin Vedder	benjamin@vedder.se

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
#include <QTableWidgetItem>
#include <QLineEdit>
#include <QPushButton>
#include "pageswdprog.h"
#include "ui_pageswdprog.h"
#include "utility.h"
#include "hexfile.h"

PageSwdProg::PageSwdProg(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PageSwdProg)
{
    ui->setupUi(this);
    layout()->setContentsMargins(0, 0, 0, 0);
    mVesc = nullptr;

    ui->chooseButton->setIcon(Utility::getIcon("icons/Open Folder-96.png"));
    ui->choose2Button->setIcon(Utility::getIcon("icons/Open Folder-96.png"));
    ui->choose3Button->setIcon(Utility::getIcon("icons/Open Folder-96.png"));
    ui->choose4Button->setIcon(Utility::getIcon("icons/Open Folder-96.png"));
    ui->uicrReadButton->setIcon(Utility::getIcon("icons/Upload-96.png"));
    ui->uicrWriteButton->setIcon(Utility::getIcon("icons/Download-96.png"));
    ui->uicrEraseButton->setIcon(Utility::getIcon("icons/Delete-96.png"));
    ui->connectButton->setIcon(Utility::getIcon("icons/Connected-96.png"));
    ui->connectNrf5xButton->setIcon(Utility::getIcon("icons/Connected-96.png"));
    ui->disconnectButton->setIcon(Utility::getIcon("icons/Disconnected-96.png"));
    ui->resetButton->setIcon(Utility::getIcon("icons/Restart-96.png"));
    ui->eraseFlashButton->setIcon(Utility::getIcon("icons/Delete-96.png"));
    ui->uploadButton->setIcon(Utility::getIcon("icons/Download-96.png"));
    ui->cancelButton->setIcon(Utility::getIcon("icons/Cancel-96.png"));

    mTimer = new QTimer(this);
    mTimer->start(500);
    mFlashOffset = 0;

    QSettings set;
    if (set.contains("pageswdprog/lastcustomfile")) {
        ui->fwEdit->setText(set.value("pageswdprog/lastcustomfile").toString());
    }
    if (set.contains("pageswdprog/lastcustomfile2")) {
        ui->fw2Edit->setText(set.value("pageswdprog/lastcustomfile2").toString());
    }
    if (set.contains("pageswdprog/lastcustomfile3")) {
        ui->fw3Edit->setText(set.value("pageswdprog/lastcustomfile3").toString());
    }
    if (set.contains("pageswdprog/lastcustomfile4")) {
        ui->fw4Edit->setText(set.value("pageswdprog/lastcustomfile4").toString());
    }

    connect(mTimer, SIGNAL(timeout()), this, SLOT(timerSlot()));

    // UICR tab
    ui->uicrTable->setColumnWidth(0, 200);
    ui->uicrTable->setColumnWidth(2, 200);

    auto addDataItem = [this](QString name, QString offset, QString defaultValue) {
        ui->uicrTable->setRowCount(ui->uicrTable->rowCount() + 1);
        ui->uicrTable->setItem(ui->uicrTable->rowCount() - 1, 0, new QTableWidgetItem(name));
        ui->uicrTable->setItem(ui->uicrTable->rowCount() - 1, 1, new QTableWidgetItem(offset));

        QLineEdit *le = new QLineEdit;
        le->setText(defaultValue);
        QFont font;
        font.setFamily("DejaVu Sans Mono");
        le->setFont(font);
        ui->uicrTable->setCellWidget(ui->uicrTable->rowCount() - 1, 2, le);

        QPushButton *readButton = new QPushButton;
        readButton->setText("Read");
        readButton->setIcon(Utility::getIcon("icons/Upload-96.png"));
        ui->uicrTable->setCellWidget(ui->uicrTable->rowCount() - 1, 3, readButton);

        connect(readButton, &QAbstractButton::clicked, [this, offset, le]() {
            if (mVesc) {
                if (ui->targetLabel->text().isEmpty()) {
                    QMessageBox::information(this,
                                             tr("Read UICR"),
                                             tr("SWD must be connected for this command to work."));
                    return;
                }

                uint32_t ofs = offset.mid(2).toUInt(nullptr, 16);
                auto data = mVesc->commands()->bmReadMemWait(0x10001000 + ofs, 4);

                if (data.size() == 4) {
                    VByteArray vb(data);
                    le->setText("0x" + QString("%1").
                                arg(vb.vbPopFrontUint32(), 8, 16, QLatin1Char('0')).toUpper());
                } else {
                    QMessageBox::information(this,
                                             tr("Read UICR"),
                                             tr("Could not read UICR."));
                }
            }
        });

        QPushButton *writeButton = new QPushButton;
        writeButton->setText("Write");
        writeButton->setIcon(Utility::getIcon("icons/Download-96.png"));
        ui->uicrTable->setCellWidget(ui->uicrTable->rowCount() - 1, 4, writeButton);

        connect(writeButton, &QAbstractButton::clicked, [this, offset, le, name]() {
            if (mVesc) {
                if (ui->targetLabel->text().isEmpty()) {
                    QMessageBox::information(this,
                                             tr("Write UICR"),
                                             tr("SWD must be connected for this command to work."));
                    return;
                }

                QString txt = le->text();
                int base = 10;

                if (txt.toLower().startsWith("0x")) {
                    txt.remove(0, 2);
                    base = 16;
                }

                bool ok = false;
                quint32 val = txt.toUInt(&ok, base);
                if (ok) {
                    VByteArray vb;
                    vb.vbAppendUint32(val);
                    uint32_t ofs = offset.mid(2).toUInt(nullptr, 16);
                    int res = mVesc->commands()->bmWriteMemWait(0x10001000 + ofs, vb);

                    if (res != 1) {
                        QMessageBox::warning(this,
                                             tr("Write UICR"),
                                             QString("Unable to write UICR register. Res: %1").
                                             arg(res));
                    }
                } else {
                    QMessageBox::warning(this,
                                         tr("Write UICR"),
                                         QString("Unable to parse value for %1").
                                         arg(name));
                    return;
                }
            }
        });
    };

    for (int i = 0;i < 15;i++) {
        addDataItem(QString("NRFFW[%1]").arg(i),
                    "0x0" + QString("%1").arg(i * 4 + 0x14, 0, 16).toUpper(),
                    "0xFFFFFFFF");
    }

    for (int i = 0;i < 12;i++) {
        addDataItem(QString("NRFHW[%1]").arg(i),
                    "0x0" + QString("%1").arg(i * 4 + 0x50, 0, 16).toUpper(),
                    "0xFFFFFFFF");
    }

    for (int i = 0;i < 32;i++) {
        addDataItem(QString("CUSTOMER[%1]").arg(i),
                    "0x0" + QString("%1").arg(i * 4 + 0x80, 0, 16).toUpper(),
                    "0xFFFFFFFF");
    }

    addDataItem("PSELRESET[0]", "0x200", "0xFFFFFFFF");
    addDataItem("PSELRESET[1]", "0x204", "0xFFFFFFFF");
    addDataItem("APPROTECT", "0x208", "0xFFFFFFFF");
    addDataItem("NFCPINS", "0x20C", "0xFFFFFFFF");
    addDataItem("DEBUGCTRL", "0x210", "0xFFFFFFFF");
    addDataItem("REGOUT0", "0x304", "0xFFFFFFFF");
}

PageSwdProg::~PageSwdProg()
{
    QSettings set;
    set.setValue("pageswdprog/lastcustomfile", ui->fwEdit->text());
    set.setValue("pageswdprog/lastcustomfile2", ui->fw2Edit->text());
    set.setValue("pageswdprog/lastcustomfile3", ui->fw3Edit->text());
    set.setValue("pageswdprog/lastcustomfile4", ui->fw4Edit->text());
    delete ui;
}

void PageSwdProg::on_chooseButton_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Choose Firmware File"), ".",
                                                    tr("Firmware files (*.bin *.hex)"));

    if (filename.isNull()) {
        return;
    }

    ui->fwEdit->setText(filename);
}

void PageSwdProg::on_choose2Button_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Choose Firmware File 2"), ".",
                                                    tr("Firmware files (*.bin *.hex)"));

    if (filename.isNull()) {
        return;
    }

    ui->fw2Edit->setText(filename);
}

void PageSwdProg::on_choose3Button_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Choose Firmware File 3"), ".",
                                                    tr("Firmware files (*.bin *.hex)"));

    if (filename.isNull()) {
        return;
    }

    ui->fw3Edit->setText(filename);
}

void PageSwdProg::on_choose4Button_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Choose Firmware File 4"), ".",
                                                    tr("Firmware files (*.bin *.hex)"));

    if (filename.isNull()) {
        return;
    }

    ui->fw4Edit->setText(filename);
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

        auto uploadHex = [this](QString name) {
            QMap<quint32, QByteArray> fwData;
            bool fwRes = HexFile::parseFile(name, fwData);

            if (fwRes) {
                QMapIterator<quint32, QByteArray> i(fwData);

                while (i.hasNext()) {
                    i.next();
                    QByteArray data = i.value();
                    fwRes = mVesc->swdUploadFw(data, i.key(),
                                               ui->verifyBox->isChecked());
                    if (!fwRes) {
                        break;
                    }
                }
            }
        };

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

                bool isHex = false;
                if (file.fileName().toLower().endsWith(".hex")) {
                    isHex = true;
                }

                if (isHex) {
                    uploadHex(file.fileName());
                } else {
                    mVesc->swdUploadFw(file.readAll(), mFlashOffset + fw.addr, ui->verifyBox->isChecked());
                }

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

                    isHex = false;
                    if (file2.fileName().toLower().endsWith(".hex")) {
                        isHex = true;
                    }

                    if (isHex) {
                        uploadHex(file2.fileName());
                    } else {
                        mVesc->swdUploadFw(file2.readAll(), mFlashOffset + fw.bootloaderAddr, ui->verifyBox->isChecked());
                    }
                }
            } else {
                QMessageBox::critical(this,
                                      tr("Upload Error"),
                                      tr("No firmware selected."));
                return;
            }
        } else if (ui->tabWidget->currentIndex() == 1) {
            QFile file(ui->fwEdit->text());

            if (ui->useFw2Button->isChecked()) {
                file.setFileName(ui->fw2Edit->text());
            }

            if (ui->useFw3Button->isChecked()) {
                file.setFileName(ui->fw3Edit->text());
            }

            if (ui->useFw4Button->isChecked()) {
                file.setFileName(ui->fw4Edit->text());
            }

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

            bool isHex = false;
            if (file.fileName().toLower().endsWith(".hex")) {
                isHex = true;
            }

            if (!isHex && file.size() > (1024 * 1024 * 5)) {
                QMessageBox::critical(this,
                                      tr("Upload Error"),
                                      tr("The selected file is too large to be a firmware."));
                return;
            }
            if (!mVesc->swdEraseFlash()) {
                return;
            }

            if (isHex) {
                uploadHex(file.fileName());
            } else {
                mVesc->swdUploadFw(file.readAll(), mFlashOffset, ui->verifyBox->isChecked());
            }
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
    } else if (res == 9) {
        ui->targetLabel->setText("STM32F30x");
        mFlashOffset = 0x08000000;
    } else if (res == 10) {
        ui->targetLabel->setText("STM32L47x");
        mFlashOffset = 0x08000000;
    } else if (res == 11) {
        ui->targetLabel->setText("STM32G43");
        mFlashOffset = 0x08000000;
    } else if (res == 12) {
        ui->targetLabel->setText("STM32G47");
        mFlashOffset = 0x08000000;
    } else if (res == 13) {
        ui->targetLabel->setText("STM32G49");
        mFlashOffset = 0x08000000;
    }

    switch (res) {
    case 1: {
        QDir dir("://res/firmwares");
        dir.setSorting(QDir::Name);
        for (auto fi: dir.entryInfoList()) {
            QFileInfo fiDefault(fi.absoluteFilePath() + "/VESC_default.bin");

            if (fiDefault.exists()) {
                addSwdFw(fi.fileName().replace("_o_", " & "),
                         fiDefault.absoluteFilePath(),
                         0, ":/res/bootloaders/generic.bin");
            }
        }
    } break;

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
        addSwdFw("Wand Remote Magnetic Throttle",
                 "://res/other_fw/nrf52840_wand_mag.bin");
        addSwdFw("Stormcore Builtin - RX: 31 TX: 30 LED: 5",
                 "://res/other_fw/nrf52840_stormcore_ble_rx31_tx30_led5.bin");
        break;

    case 10:
        addSwdFw("Trampa 12s7p BMS",
                 "://res/firmwares_bms/12s7p/vesc_default.bin", 0,
                 "://res/bootloaders_bms/generic.bin", 0x3E000);
        addSwdFw("Trampa 18s Light BMS",
                 "://res/firmwares_bms/18s_light/vesc_default.bin", 0,
                 "://res/bootloaders_bms/generic.bin", 0x3E000);
        addSwdFw("Trampa 18s Light LMP BMS",
                 "://res/firmwares_bms/18s_light_lmp/vesc_default.bin", 0,
                 "://res/bootloaders_bms/generic.bin", 0x3E000);
        addSwdFw("Trampa 18s Light MK2 BMS",
                 "://res/firmwares_bms/18s_light_mk2/vesc_default.bin", 0,
                 "://res/bootloaders_bms/generic.bin", 0x3E000);
        addSwdFw("Power Switch 120V",
                 "://res/other_fw/vesc_power_switch_120.bin", 0,
                 "://res/bootloaders_bms/generic.bin", 0x3E000);
        break;

    case 11:
        addSwdFw("STR-DCDC",
                 "://res/firmwares_custom_module/str-dcdc/vesc_default.bin", 0,
                 "://res/bootloaders_custom_module/stm32g431/stm32g431.bin", 0x1E000);

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

void PageSwdProg::on_uicrReadButton_clicked()
{
    if (mVesc) {
        if (ui->targetLabel->text().isEmpty()) {
            QMessageBox::information(this,
                                     tr("Read UICR"),
                                     tr("SWD must be connected for this command to work."));
            return;
        }

        ui->uicrReadButton->setEnabled(false);
        auto data1 = mVesc->commands()->bmReadMemWait(0x10001000 + 0x14, 0xFC - 0x10);
        auto data2 = mVesc->commands()->bmReadMemWait(0x10001000 + 0x200, 16);
        ui->uicrReadButton->setEnabled(true);
        if (data1.size() == (0xFC - 0x10) && data2.size() == 16) {
            VByteArray vb1(data1);
            vb1.append(data2);
            int ind = 0;
            while(!vb1.isEmpty()) {
                auto reg = vb1.vbPopFrontUint32();
                if (QLineEdit *le = qobject_cast<QLineEdit*>(ui->uicrTable->cellWidget(ind, 2))) {
                    le->setText("0x" + QString("%1").arg(reg, 8, 16, QLatin1Char('0')).toUpper());
                }
                ind++;
            }
        } else {
            QMessageBox::warning(this,
                                 tr("Read UICR"),
                                 tr("Could not read UICR registers."));
        }
    }
}

void PageSwdProg::on_uicrWriteButton_clicked()
{
    if (mVesc) {
        if (ui->targetLabel->text().isEmpty()) {
            QMessageBox::information(this,
                                     tr("Write UICR"),
                                     tr("SWD must be connected for this command to work."));
            return;
        }

        VByteArray vb;
        for (int i = 0;i < (ui->uicrTable->rowCount() - 1);i++) {
            if (QLineEdit *le = qobject_cast<QLineEdit*>(ui->uicrTable->cellWidget(i, 2))) {
                QString txt = le->text();
                int base = 10;

                if (txt.toLower().startsWith("0x")) {
                    txt.remove(0, 2);
                    base = 16;
                }

                bool ok = false;
                quint32 val = txt.toUInt(&ok, base);
                if (ok) {
                    vb.vbAppendUint32(val);
                } else {
                    QMessageBox::warning(this,
                                         tr("Write UICR"),
                                         QString("Unable to parse value for %1").
                                         arg(ui->uicrTable->item(i, 0)->text()));
                    return;
                }
            }
        }

        QByteArray data1 = vb.left(0xFC - 0x10);
        QByteArray data2 = vb.right(16);

        ui->uicrWriteButton->setEnabled(false);
        int res1 = mVesc->commands()->bmWriteMemWait(0x10001000 + 0x14, data1);
        int res2 = mVesc->commands()->bmWriteMemWait(0x10001000 + 0x200, data2);
        ui->uicrWriteButton->setEnabled(true);

        if (res1 != 1 || res2 != 1) {
            QMessageBox::warning(this,
                                 tr("Write UICR"),
                                 QString("Unable to write UICR registers. Res1: %1, Res2: %2").
                                 arg(res1).arg(res2));
        }
    }
}

void PageSwdProg::on_uicrEraseButton_clicked()
{
    if (mVesc) {
        if (ui->targetLabel->text().isEmpty()) {
            QMessageBox::information(this,
                                     tr("Write UICR"),
                                     tr("SWD must be connected for this command to work."));
            return;
        }

        mVesc->commands()->sendTerminalCmd("bm_target_cmd erase_uicr");
    }
}

void PageSwdProg::on_resetButton_clicked()
{
    if (mVesc) {
        if (ui->targetLabel->text().isEmpty()) {
            QMessageBox::information(this,
                                     tr("Reset Target"),
                                     tr("SWD must be connected for this command to work."));
            return;
        }

        mVesc->commands()->sendTerminalCmd("bm_reset");
    }
}
