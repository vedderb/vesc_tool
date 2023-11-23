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

    ui->changelogButton->setIcon(Utility::getIcon("icons/About-96.png"));
    ui->chooseButton->setIcon(Utility::getIcon("icons/Open Folder-96.png"));
    ui->choose2Button->setIcon(Utility::getIcon("icons/Open Folder-96.png"));
    ui->choose3Button->setIcon(Utility::getIcon("icons/Open Folder-96.png"));
    ui->choose4Button->setIcon(Utility::getIcon("icons/Open Folder-96.png"));
    ui->cancelButton->setIcon(Utility::getIcon("icons/Cancel-96.png"));
    ui->uploadButton->setIcon(Utility::getIcon("icons/Download-96.png"));
    ui->uploadAllButton->setIcon(Utility::getIcon("icons/Download-96.png"));
    ui->readVersionButton->setIcon(Utility::getIcon("icons/Upload-96.png"));
    ui->dlArchiveButton->setIcon(Utility::getIcon("icons/Download-96.png"));

    updateHwList(FW_RX_PARAMS());
    updateBlList(FW_RX_PARAMS());

    mTimer = new QTimer(this);
    mTimer->start(500);

    connect(ui->hwList, SIGNAL(currentRowChanged(int)),
            this, SLOT(updateFwList()));
    connect(ui->showNonDefaultBox, SIGNAL(toggled(bool)),
            this, SLOT(updateFwList()));
    connect(mTimer, SIGNAL(timeout()), this, SLOT(timerSlot()));
    connect(ui->archVersionList, SIGNAL(currentRowChanged(int)),
            this, SLOT(updateArchFwList()));
    connect(ui->showNonDefaultArchBox, SIGNAL(toggled(bool)),
            this, SLOT(updateArchFwList()));

    QSettings set;
    ui->fwEdit->setText(set.value("pagefirmware/lastcustomfile", "").toString());
    ui->fw2Edit->setText(set.value("pagefirmware/lastcustomfile2", "").toString());
    ui->fw3Edit->setText(set.value("pagefirmware/lastcustomfile3", "").toString());
    ui->fw4Edit->setText(set.value("pagefirmware/lastcustomfile4", "").toString());

    reloadArchive();
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

        connect(mVesc, &VescInterface::fwArchiveDlProgress, [this](QString msg, double prog) {
            ui->displayDl->setText(msg);
            ui->displayDl->setValue(100.0 * prog);
        });
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
        fwStr = QString("Fw: v%1.%2").arg(params.major).arg(params.minor, 2, 10, QLatin1Char('0'));
        if (!params.fwName.isEmpty()) {
            fwStr += " (" + params.fwName + ")";
        }

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

    fwStr += "\nNRF Name: ";
    fwStr += (params.nrfNameSupported ? "Yes" : "No");
    fwStr += ", Pin: ";
    fwStr += (params.nrfPinSupported ? "Yes" : "No");

    ui->currentLabel->setText(fwStr);
    updateHwList(params);
    updateBlList(params);
    updateArchFwList();
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

    // Manually added entries. TODO: Come up with a system for them
    QString extraPath;
    if (params.hw == "VESC Express T") {
        extraPath = "://res/firmwares_esp/ESP32-C3/VESC Express";
    } else if (params.hw == "Devkit C3") {
        extraPath = "://res/firmwares_esp/ESP32-C3/DevKitM-1";
    } else if (params.hw == "STR-DCDC") {
        extraPath = "://res/firmwares_custom_module/str-dcdc";
    }

    if (!extraPath.isEmpty()) {
        QListWidgetItem *item = new QListWidgetItem;
        item->setText(params.hw);
        item->setData(Qt::UserRole, extraPath);
        ui->hwList->insertItem(ui->hwList->count(), item);
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
                    fi.fileName().toLower() == "vesc_default.bin" ||
                    fi.fileName().toLower() == "vesc_express.bin") {
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

void PageFirmware::updateArchFwList()
{
    ui->archFwList->clear();
    QListWidgetItem *item = ui->archVersionList->currentItem();
    if (item != nullptr) {
        QString hw = item->data(Qt::UserRole).toString();

        QDirIterator it(hw);
        while (it.hasNext()) {
            QFileInfo fi(it.next());

            QDirIterator it2(fi.absoluteFilePath());
            while (it2.hasNext()) {
                QFileInfo fi2(it2.next());

                QStringList names = fi.fileName().split("_o_");
                auto params = mVesc->getLastFwRxParams();

                if (!mVesc->isPortConnected() || params.hw.isEmpty() ||
                        names.contains(params.hw, Qt::CaseInsensitive)) {

                    if (ui->showNonDefaultArchBox->isChecked() ||
                            fi2.fileName().toLower() == "vesc_default.bin") {
                        QString name = names.at(0);
                        for(int i = 1;i < names.size();i++) {
                            name += " & " + names.at(i);
                        }

                        QListWidgetItem *item = new QListWidgetItem;
                        item->setText("HW " + name + ": " + fi2.fileName());
                        item->setData(Qt::UserRole, fi2.absoluteFilePath());
                        ui->archFwList->insertItem(ui->archFwList->count(), item);
                    }
                }
            }
        }
    }

    if (ui->archFwList->count() > 0) {
        ui->archFwList->setCurrentRow(0);
    }
}

void PageFirmware::updateBlList(FW_RX_PARAMS params)
{
    ui->blList->clear();

    QString blDir = "";
    switch (params.hwType) {
    case HW_TYPE_VESC:
        blDir = "://res/bootloaders";
        break;

    case HW_TYPE_VESC_BMS:
        blDir = "://res/bootloaders_bms";
        break;

    case HW_TYPE_CUSTOM_MODULE:
        QByteArray endEsp;
        endEsp.append('\0');
        endEsp.append('\0');
        endEsp.append('\0');
        endEsp.append('\0');

        if (!params.uuid.endsWith(endEsp)) {
            if (params.hw == "hm1") {
                blDir = "://res/bootloaders_bms";
            } else {
                blDir = "://res/bootloaders_custom_module/stm32g431";
            }
        }
        break;
    }

    if (blDir.isEmpty()) {
        return;
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
        {
            QFileInfo generic(blDir + "/generic.bin");
            if (generic.exists()) {
                QListWidgetItem *item = new QListWidgetItem;
                item->setText("generic");
                item->setData(Qt::UserRole, generic.absoluteFilePath());
                ui->blList->insertItem(ui->blList->count(), item);
            }
        }

        {
            QFileInfo stm32g431(blDir + "/stm32g431.bin");
            if (stm32g431.exists()) {
                QListWidgetItem *item = new QListWidgetItem;
                item->setText("stm32g431");
                item->setData(Qt::UserRole, stm32g431.absoluteFilePath());
                ui->blList->insertItem(ui->blList->count(), item);
            }
        }
    }

    if (ui->blList->count() > 0) {
        ui->blList->setCurrentRow(0);
    }
}

void PageFirmware::on_chooseButton_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Choose Firmware File"), ui->fwEdit->text(),
                                                    tr("Firmware files (*.bin *.hex)"));
    if (!filename.isNull()) {
        ui->fwEdit->setText(filename);
    }
}

void PageFirmware::on_choose2Button_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Choose Firmware File"), ui->fw2Edit->text(),
                                                    tr("Firmware files (*.bin *.hex)"));
    if (!filename.isNull()) {
        ui->fw2Edit->setText(filename);
    }
}

void PageFirmware::on_choose3Button_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Choose Firmware File"), ui->fw3Edit->text(),
                                                    tr("Firmware files (*.bin *.hex)"));
    if (!filename.isNull()) {
        ui->fw3Edit->setText(filename);
    }
}

void PageFirmware::on_choose4Button_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Choose Firmware File"), ui->fw4Edit->text(),
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
                                  tr("Not connected to device. Please connect first."));
            return;
        }

        QFile file, fileBl;

        if (ui->fwTabWidget->currentIndex() == 0 || ui->fwTabWidget->currentIndex() == 3) {
            QListWidgetItem *item = ui->fwList->currentItem();

            if (ui->fwTabWidget->currentIndex() == 3) {
                item = ui->archFwList->currentItem();
            }

            if (item) {
                file.setFileName(item->data(Qt::UserRole).toString());

                if (mVesc->commands()->getLimitedSupportsEraseBootloader()) {
                    item = ui->blList->currentItem();

                    if (item) {
                        fileBl.setFileName(item->data(Qt::UserRole).toString());
                    }
                }
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
                fileBl.setFileName(item->data(Qt::UserRole).toString());
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

        if (!fileBl.fileName().isEmpty()) {
            if (!fileBl.open(QIODevice::ReadOnly)) {
                QMessageBox::critical(this,
                                      tr("Upload Error"),
                                      tr("Could not open bootloader file. Make sure that the path is valid."));
                return;
            }
        }

        if (!file.fileName().isEmpty()) {
            if (!file.open(QIODevice::ReadOnly)) {
                QMessageBox::critical(this,
                                      tr("Upload Error"),
                                      tr("Could not open file. Make sure that the path is valid."));
                return;
            }
        }

        if (file.size() > 1500000 && !(file.fileName().toLower().endsWith(".hex"))) {
            QMessageBox::critical(this,
                                  tr("Upload Error"),
                                  tr("The selected file is too large to be a firmware."));
            return;
        }

        QMessageBox::StandardButton reply;

        if ((ui->fwTabWidget->currentIndex() == 0 && ui->hwList->count() == 1) || ui->fwTabWidget->currentIndex() == 3) {
            reply = QMessageBox::warning(this,
                                         tr("Warning"),
                                         tr("Uploading new firmware will clear all settings in the VESC firmware "
                                            "and you have to do the configuration again. Do you want to "
                                            "continue?"),
                                         QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        } else if ((ui->fwTabWidget->currentIndex() == 0 && ui->hwList->count() > 1) || ui->fwTabWidget->currentIndex() == 1) {
            reply = QMessageBox::warning(this,
                                         tr("Warning"),
                                         tr("Uploading firmware for the wrong hardware version "
                                            "WILL damage the hardware. Are you sure that you have "
                                            "chosen the correct hardware version?"),
                                         QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        } else if (ui->fwTabWidget->currentIndex() == 2) {
            if (mVesc->commands()->getLimitedSupportsEraseBootloader()) {
                reply = QMessageBox::warning(this,
                                             tr("Warning"),
                                             tr("This will attempt to upload a bootloader to the connected device. "
                                                "Do you want to continue?"),
                                             QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            } else {
                reply = QMessageBox::warning(this,
                                             tr("Warning"),
                                             tr("This will attempt to upload a bootloader to the connected device. "
                                                "If the connected device already has a bootloader this will destroy "
                                                "the bootloader and firmware updates cannot be done anymore. Do "
                                                "you want to continue?"),
                                             QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            }
        } else {
            reply = QMessageBox::No;
        }

        auto uploadFw = [this](QFile *file, bool isBootloader, bool allOverCan) {
            bool fwRes = false;

            if (file->fileName().toLower().endsWith(".hex")) {
                QMap<quint32, QByteArray> fwData;
                fwRes = HexFile::parseFile(file->fileName(), fwData);

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
                QByteArray data = file->readAll();
                fwRes = mVesc->fwUpload(data, isBootloader, allOverCan);
            }

            return fwRes;
        };

        if (reply == QMessageBox::Yes) {
            if (!fileBl.fileName().isEmpty()) {
                uploadFw(&fileBl, true, allOverCan);
            }

            if (!file.fileName().isEmpty()) {
                QString blMsg = tr("");

                // No bootloader was uploaded prior to the firmware
                if (fileBl.fileName().isEmpty()) {
                    blMsg = tr("\n\n"
                               "NOTE: If the old firmware is loaded again after the reboot a bootloader is probably missing. You "
                               "can try uploading one from the bootloader tab if that is the case.");
                }

                if (uploadFw(&file, false, allOverCan)) {
                    QMessageBox::warning(this,
                                         tr("Warning"),
                                         tr("The firmware upload is done. The device should reboot automatically within 10 seconds. Do "
                                            "NOT remove power before the reboot is done as that can brick the CPU and requires a programmer "
                                            "to fix.") + blMsg);
                }
            }
        }
    }
}

void PageFirmware::reloadArchive()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/res_fw.rcc";
    QFile file(path);
    if (file.exists()) {
        QResource::unregisterResource(path);
        QResource::registerResource(path);

        QString fwDir = "://fw_archive";

        ui->archVersionList->clear();

        QDirIterator it(fwDir);
        while (it.hasNext()) {
            QFileInfo fi(it.next());
            QListWidgetItem *item = new QListWidgetItem;

            item->setText(fi.fileName());
            item->setData(Qt::UserRole, fi.absoluteFilePath());
            ui->archVersionList->insertItem(ui->archVersionList->count(), item);
        }
    }
}

void PageFirmware::on_dlArchiveButton_clicked()
{
    ui->dlArchiveButton->setEnabled(false);
    ui->displayDl->setText("Preparing download...");

    if (mVesc) {
        if (mVesc->downloadFwArchive()) {
            reloadArchive();
        }
    }

    ui->dlArchiveButton->setEnabled(true);
}
