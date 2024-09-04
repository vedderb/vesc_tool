/*
    Copyright 2016 - 2020 Benjamin Vedder	benjamin@vedder.se

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

#ifndef PAGEFIRMWARE_H
#define PAGEFIRMWARE_H

#include <QWidget>
#include <QTimer>
#include "vescinterface.h"

namespace Ui {
class PageFirmware;
}

class PageFirmware : public QWidget
{
    Q_OBJECT

public:
    explicit PageFirmware(QWidget *parent = nullptr);
    ~PageFirmware();

    void saveStateToSettings();
    VescInterface *vesc() const;
    void setVesc(VescInterface *vesc);
    void reloadParams();

private slots:
    void timerSlot();

    void fwUploadStatus(const QString &status, double progress, bool isOngoing);
    void fwRxChanged(bool rx, bool limited);
    void updateHwList(FW_RX_PARAMS params);
    void updateFwList();
    void updateArchFwList();
    void updateBlList(FW_RX_PARAMS params);

    void on_chooseButton_clicked();
    void on_choose2Button_clicked();
    void on_choose3Button_clicked();
    void on_choose4Button_clicked();
    void on_uploadButton_clicked();
    void on_readVersionButton_clicked();
    void on_cancelButton_clicked();
    void on_changelogButton_clicked();
    void on_uploadAllButton_clicked();
    void on_dlArchiveButton_clicked();

private:
    Ui::PageFirmware *ui;
    VescInterface *mVesc;
    QTimer *mTimer;

    void uploadFw(bool allOverCan);
    void reloadArchive();

};

#endif // PAGEFIRMWARE_H
