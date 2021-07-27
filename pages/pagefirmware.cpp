/*
    Copyright 2016 - 2021 Benjamin Vedder	benjamin@vedder.se

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

#include "pagefirmware.h"
#include "ui_pagefirmware.h"
#include "widgets/helpdialog.h"
#include "utility.h"
#include "hexfile.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QDirIterator>

PageFirmware::PageFirmware(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PageFirmware)
{
    ui->setupUi(this);
    layout()->setContentsMargins(0, 0, 0, 0);
    ui->cancelButton->setEnabled(false);
    mVesc = nullptr;

    QString theme = Utility::getThemePath();
    ui->changelogButton->setIcon(QPixmap(theme + "icons/About-96.png"));
    ui->chooseButton->setIcon(QPixmap(theme + "icons/Open Folder-96.png"));
    ui->choose2Button->setIcon(QPixmap(theme + "icons/Open Folder-96.png"));
    ui->choose3Button->setIcon(QPixmap(theme + "icons/Open Folder-96.png"));
    ui->choose4Button->setIcon(QPixmap(theme + "icons/Open Folder-96.png"));
    ui->cancelButton->setIcon(QPixmap(theme + "icons/Cancel-96.png"));
    ui->uploadButton->setIcon(QPixmap(theme + "icons/Download-96.png"));
    ui->uploadAllButton->setIcon(QPixmap(theme + "icons/Download-96.png"));
    ui->readVersionButton->setIcon(QPixmap(theme + "icons/Upload-96.png"));

    updateHwList(FW_RX_PARAMS());
    updateBlList(FW_RX_PARAMS());

    mTimer = new QTimer(this);
    mTimer->start(500);

    connect(ui->hwList, SIGNAL(currentRowChanged(int)),
            this, SLOT(updateFwList()));
    connect(ui->showNonDefaultBox, SIGNAL(toggled(bool)),
            this, SLOT(updateFwList()));
    connect(mTimer, SIGNAL(timeout()), this, SLOT(timerSlot()));

    QSettings set;
    ui->fwEdit->setText(set.value("pagefirmware/lastcustomfile", "").toString());
    ui->fw2Edit->setText(set.value("pagefirmware/lastcustomfile2", "").toString());
    ui->fw3Edit->setText(set.value("pagefirmware/lastcustomfile3", "").toString());
    ui->fw4Edit->setText(set.value("pagefirmware/lastcustomfile4", "").toString());
}

PageFirmware::~PageFirmware()
{
    QSettings set;
    set.setValue("pagefirmware/lastcustomfile", ui->fwEdit->text());
    set.setValue("pagefirmware/lastcustomfile2", ui->fw2Edit->text());
    set.setValue("pagefirmware/lastcustomfile3", ui->fw3Edit->text());
    set.setValue("pagefirmware/lastcustomfile4", ui->fw4Edit->text());
    delete ui;
}

VescInterface *PageFirmware::vesc() const
{
    return mVesc;
}

void PageFirmware::setVesc(VescInterface *vesc)
{
    mVesc = vesc;

    if (mVesc) {
        ui->display->setText(mVesc->getFwUploadStatus());

        reloadParams();

        connect(mVesc, SIGNAL(fwUploadStatus(QString,double,bool)),
                this, SLOT(fwUploadStatus(QString,double,bool)));
        connect(mVesc, SIGNAL(fwRxChanged(bool,bool)),
                this, SLOT(fwRxChanged(bool,bool)));
    }
}

void PageFirmware::reloadParams()
{
    if (mVesc) {
        QStringList fws = mVesc->getSupportedFirmwares();
        QString str;
        for (int i = 0;i < fws.size();i++) {
            str.append(fws.at(i));
            if (i < (fws.size() - 1)) {
                str.append(", ");
            }
        }
        ui->supportedLabel->setText(str);
    }
}

void PageFirmware::timerSlot()
{
    if (mVesc) {
        if (mVesc->getFwUploadProgress() >= 0.0) {
            ui->uploadAllButton->setEnabled(mVesc->commands()->getLimitedSupportsFwdAllCan() &&
                                            !mVesc->commands()->getSendCan() && mVesc->getFwUploadProgress() < 0.0);
        }

        if (!mVesc->isPortConnected()) {
            ui->currentLabel->clear();
        }
    }
}

void PageFirmware::fwUploadStatus(const QString &status, double progress, bool isOngoing)
{
    if (isOngoing) {
        ui->display->setText(tr("%1 (%2 %)").
                             arg(status).
                             arg(progress * 100, 0, 'f', 1));
    } else {
        ui->display->setText(status);
    }

    ui->display->setValue(progress * 100.0);
    ui->uploadButton->setEnabled(!isOngoing);
    ui->uploadAllButton->setEnabled(!isOngoing);
    ui->cancelButton->setEnabled(isOngoing);
}

void PageFirmware::fwRxChanged(bool rx, bool limited)
{
    (void)limited;

    if (!rx) {
        return;
    }

    FW_RX_PARAMS params = mVesc->getLastFwRxParams();

    QString fwStr;
    QString strUuid = Utility::uuid2Str(params.uuid, true);

    if (!strUuid.isEmpty()) {
        fwStr += ", UUID: " + strUuid;
    }

    if (params.major >= 0) {
        fwStr = QString("Fw: %1.%2").arg(params.major).arg(params.minor);
        if (!params.hw.isEmpty()) {
            fwStr += ", Hw: " + params.hw;
        }

        if (!strUuid.isEmpty()) {
            fwStr += "\n" + strUuid;
        }
    }

    fwStr += "\n" + QString("Paired: %1, Status: ").
            arg(params.isPaired ? "true" : "false");
    if (params.isTestFw > 0) {
        fwStr += QString("BETA %1").arg(params.isTestFw);
    } else {
        fwStr += "STABLE";
    }

    fwStr += "\nHW Type: " + params.hwTypeStr();

    if (params.hwType == HW_TYPE_VESC) {
        fwStr += ", Phase Filters: ";
        fwStr += params.hasPhaseFilters ? "Yes" : "No";
    }

    ui->currentLabel->setText(fwStr);
    updateHwList(params);
    updateBlList(params);
    update();
}

void PageFirmware::updateHwList(FW_RX_PARAMS params)
{
    ui->hwList->clear();

    QString fwDir = "://res/firmwares";

    if (params.hwType == HW_TYPE_VESC_BMS) {
        fwDir = "://res/firmwares_bms";
    }

    QDirIterator it(fwDir);
    while (it.hasNext()) {
        QFileInfo fi(it.next());
        QStringList names = fi.fileName().split("_o_");

        if (fi.isDir() && (params.hw.isEmpty() || names.contains(params.hw, Qt::CaseInsensitive))) {
            QListWidgetItem *item = new QListWidgetItem;

            QString name = names.at(0);
            for(int i = 1;i < names.size();i++) {
                name += " & " + names.at(i);
            }

            item->setText(name);
            item->setData(Qt::UserRole, fi.absoluteFilePath());
            ui->hwList->insertItem(ui->hwList->count(), item);
        }
    }

    if (ui->hwList->count() > 0) {
        ui->hwList->setCurrentRow(0);
    }

    updateFwList();
}

void PageFirmware::updateFwList()
{
    ui->fwList->clear();
    QListWidgetItem *item = ui->hwList->currentItem();
    if (item != nullptr) {
        QString hw = item->data(Qt::UserRole).toString();

        QDirIterator it(hw);
        while (it.hasNext()) {
            QFileInfo fi(it.next());
            if (ui->showNonDefaultBox->isChecked() ||
                    fi.fileName().toLower() == "vesc_default.bin") {
                QListWidgetItem *item = new QListWidgetItem;
                item->setText(fi.fileName());
                item->setData(Qt::UserRole, fi.absoluteFilePath());
                ui->fwList->insertItem(ui->fwList->count(), item);
            }
        }
    }

    if (ui->fwList->count() > 0) {
        ui->fwList->setCurrentRow(0);
    }
}

void PageFirmware::updateBlList(FW_RX_PARAMS params)
{
    ui->blList->clear();

    QString blDir = "://res/bootloaders";

    if (params.hwType != HW_TYPE_VESC) {
        blDir = "://res/bootloaders_bms";
    }

    QDirIterator it(blDir);
    while (it.hasNext()) {
        QFileInfo fi(it.next());
        QStringList names = fi.fileName().replace(".bin", "").split("_o_");

        if (!fi.isDir() && (params.hw.isEmpty() || names.contains(params.hw, Qt::CaseInsensitive))) {
            QListWidgetItem *item = new QListWidgetItem;

            QString name = names.at(0);
            for(int i = 1;i < names.size();i++) {
                name += " & " + names.at(i);
            }

            item->setText(name);
            item->setData(Qt::UserRole, fi.absoluteFilePath());
            ui->blList->insertItem(ui->blList->count(), item);
        }
    }

    if (ui->blList->count() == 0) {
        QFileInfo generic(blDir + "/generic.bin");
        if (generic.exists()) {
            QListWidgetItem *item = new QListWidgetItem;
            item->setText("generic");
            item->setData(Qt::UserRole, generic.absoluteFilePath());
            ui->blList->insertItem(ui->blList->count(), item);
        }
    }

    if (ui->blList->count() > 0) {
        ui->blList->setCurrentRow(0);
    }
}

void PageFirmware::on_chooseButton_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Choose Firmware File"), ".",
                                                    tr("Firmware files (*.bin *.hex)"));
    if (!filename.isNull()) {
        ui->fwEdit->setText(filename);
    }
}

void PageFirmware::on_choose2Button_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Choose Firmware File"), ".",
                                                    tr("Firmware files (*.bin *.hex)"));
    if (!filename.isNull()) {
        ui->fw2Edit->setText(filename);
    }
}

void PageFirmware::on_choose3Button_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Choose Firmware File"), ".",
                                                    tr("Firmware files (*.bin *.hex)"));
    if (!filename.isNull()) {
        ui->fw3Edit->setText(filename);
    }
}

void PageFirmware::on_choose4Button_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Choose Firmware File"), ".",
                                                    tr("Firmware files (*.bin *.hex)"));
    if (!filename.isNull()) {
        ui->fw4Edit->setText(filename);
    }
}

void PageFirmware::on_uploadButton_clicked()
{
    uploadFw(false);
}

void PageFirmware::on_readVersionButton_clicked()
{
    if (mVesc) {
        mVesc->commands()->getFwVersion();
    }
}

void PageFirmware::on_cancelButton_clicked()
{
    if (mVesc) {
        mVesc->fwUploadCancel();
    }
}

void PageFirmware::on_changelogButton_clicked()
{
    HelpDialog::showHelp(this, "Firmware Changelog", Utility::fwChangeLog());
}

void PageFirmware::on_uploadAllButton_clicked()
{
    uploadFw(true);
}

void PageFirmware::uploadFw(bool allOverCan)
{
    if (mVesc) {
        if (!mVesc->isPortConnected()) {
            QMessageBox::critical(this,
                                  tr("Connection Error"),
                                  tr("The VESC is not connected. Please connect it."));
            return;
        }

        QFile file;

        if (ui->fwTabWidget->currentIndex() == 0) {
            QListWidgetItem *item = ui->fwList->currentItem();
            if (item) {
                file.setFileName(item->data(Qt::UserRole).toString());
            } else {
                if (ui->hwList->count() == 0) {
                    QMessageBox::warning(this,
                                         tr("Upload Error"),
                                         tr("This version of VESC Tool does not include any firmware "
                                            "for your hardware version. You can either "
                                            "upload a custom file or look for a later version of VESC "
                                            "Tool that might support your hardware."));
                } else {
                    QMessageBox::warning(this,
                                         tr("Upload Error"),
                                         tr("No firmware is selected."));
                }

                return;
            }
        } else if (ui->fwTabWidget->currentIndex() == 1) {
            if (ui->useFw1Button->isChecked()) {
                file.setFileName(ui->fwEdit->text());
            } else if (ui->useFw2Button->isChecked()) {
                file.setFileName(ui->fw2Edit->text());
            } else if (ui->useFw3Button->isChecked()) {
                file.setFileName(ui->fw3Edit->text());
            } else if (ui->useFw4Button->isChecked()) {
                file.setFileName(ui->fw4Edit->text());
            }

            QFileInfo fileInfo(file.fileName());
            if (!fileInfo.fileName().toLower().endsWith(".bin") &&
                    !fileInfo.fileName().toLower().endsWith(".hex")) {
                QMessageBox::critical(this,
                                      tr("Upload Error"),
                                      tr("The selected file name seems to be invalid."));
                return;
            }
        } else {
            QListWidgetItem *item = ui->blList->currentItem();

            if (item) {
                file.setFileName(item->data(Qt::UserRole).toString());
            } else {
                if (ui->blList->count() == 0) {
                    QMessageBox::warning(this,
                                         tr("Upload Error"),
                                         tr("This version of VESC Tool does not include any bootloader "
                                            "for your hardware version."));
                } else {
                    QMessageBox::warning(this,
                                         tr("Upload Error"),
                                         tr("No bootloader is selected."));
                }

                return;
            }
        }

        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::critical(this,
                                  tr("Upload Error"),
                                  tr("Could not open file. Make sure that the path is valid."));
            return;
        }

        bool isHex = false;
        if (file.fileName().toLower().endsWith(".hex")) {
            isHex = true;
        }

        if (file.size() > 400000 && !isHex) {
            QMessageBox::critical(this,
                                  tr("Upload Error"),
                                  tr("The selected file is too large to be a firmware."));
            return;
        }

        QMessageBox::StandardButton reply;
        bool isBootloader = false;

        if (ui->fwTabWidget->currentIndex() == 0 && ui->hwList->count() == 1) {
            reply = QMessageBox::warning(this,
                                         tr("Warning"),
                                         tr("Uploading new firmware will clear all settings on your VESC "
                                            "and you have to do the configuration again. Do you want to "
                                            "continue?"),
                                         QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        } else if ((ui->fwTabWidget->currentIndex() == 0 && ui->hwList->count() > 1) || ui->fwTabWidget->currentIndex() == 1) {
            reply = QMessageBox::warning(this,
                                         tr("Warning"),
                                         tr("Uploading firmware for the wrong hardware version "
                                            "WILL damage the VESC for sure. Are you sure that you have "
                                            "chosen the correct hardware version?"),
                                         QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        } else if (ui->fwTabWidget->currentIndex() == 2) {
            if (mVesc->commands()->getLimitedSupportsEraseBootloader()) {
                reply = QMessageBox::warning(this,
                                             tr("Warning"),
                                             tr("This will attempt to upload a bootloader to the connected VESC. "
                                                "Do you want to continue?"),
                                             QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            } else {
                reply = QMessageBox::warning(this,
                                             tr("Warning"),
                                             tr("This will attempt to upload a bootloader to the connected VESC. "
                                                "If the connected VESC already has a bootloader this will destroy "
                                                "the bootloader and firmware updates cannot be done anymore. Do "
                                                "you want to continue?"),
                                             QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            }
            isBootloader = true;
        } else {
            reply = QMessageBox::No;
        }

        if (reply == QMessageBox::Yes) {
            bool fwRes = false;

            if (isHex) {
                QMap<quint32, QByteArray> fwData;
                fwRes = HexFile::parseFile(file.fileName(), fwData);

                if (fwRes) {
                    QMapIterator<quint32, QByteArray> i(fwData);

                    QByteArray data;
                    bool startSet = false;
                    unsigned int startOffset = 0;

                    while (i.hasNext()) {
                        i.next();
                        if (!startSet) {
                            startSet = true;
                            startOffset = i.key();
                        }

                        while ((data.size() + startOffset) < i.key()) {
                            data.append(0xFF);
                        }

                        data.append(i.value());
                    }

                    fwRes = mVesc->fwUpload(data, isBootloader, allOverCan);
                }
            } else {
                QByteArray data = file.readAll();
                fwRes = mVesc->fwUpload(data, isBootloader, allOverCan);
            }

            if (!isBootloader && fwRes) {
                QMessageBox::warning(this,
                                     tr("Warning"),
                                     tr("The firmware upload is done. You must wait at least "
                                        "10 seconds before unplugging power. Otherwise the firmware will get corrupted and your "
                                        "VESC will become bricked. If that happens you need a SWD programmer to recover it."));
            }
        }
    }
}
