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

#ifndef SWDPROG_H
#define SWDPROG_H

#include <QWidget>
#include <QTimer>
#include "vescinterface.h"

namespace Ui {
class PageSwdProg;
}

class PageSwdProg : public QWidget
{
    Q_OBJECT

public:
    explicit PageSwdProg(QWidget *parent = 0);
    ~PageSwdProg();

    VescInterface *vesc() const;
    void setVesc(VescInterface *vesc);

    class SwdFw {
    public:
        SwdFw() {
            addr = 0;
            bootloaderAddr = 0xE0000; // VESC Bootloader location
        }

        uint32_t addr;
        QString path;
        QString bootloaderPath;
        uint32_t bootloaderAddr;
    };

private slots:
    void timerSlot();
    void fwUploadStatus(const QString &status, double progress, bool isOngoing);
    void bmConnRes(int res);

    void on_chooseButton_clicked();
    void on_connectButton_clicked();
    void on_uploadButton_clicked();
    void on_disconnectButton_clicked();
    void on_cancelButton_clicked();
    void on_eraseFlashButton_clicked();
    void on_connectNrf5xButton_clicked();

private:
    Ui::PageSwdProg *ui;
    VescInterface *mVesc;
    QTimer *mTimer;
    uint32_t mFlashOffset;

    void addSwdFw(QString name, QString path, uint32_t addr = 0, QString blPath = "", uint32_t blAddr = 0xE0000);

};

Q_DECLARE_METATYPE(PageSwdProg::SwdFw)

#endif // SWDPROG_H
