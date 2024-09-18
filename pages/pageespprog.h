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

#ifndef PAGEESPPROG_H
#define PAGEESPPROG_H

#include <QWidget>
#include <QTimer>
#include "vescinterface.h"
#include "esp32/esp32flash.h"

namespace Ui {
class PageEspProg;
}

class PageEspProg : public QWidget
{
    Q_OBJECT

public:
    explicit PageEspProg(QWidget *parent = nullptr);
    ~PageEspProg();

    void saveStateToSettings();
    VescInterface *vesc() const;
    void setVesc(VescInterface *vesc);

private slots:
    void timerSlot();
    void on_serialRefreshButton_clicked();
    void on_serialDisconnectButton_clicked();
    void on_serialConnectButton_clicked();
    void on_flashButton_clicked();
    void on_blChooseButton_clicked();
    void on_partChooseButton_clicked();
    void on_appChooseButton_clicked();
    void on_flashBlButton_clicked();
    void on_cancelButton_clicked();
    void on_eraseLispButton_clicked();
    void on_eraseQmlButton_clicked();

private:
    QTimer *mTimer;
    Ui::PageEspProg *ui;
    VescInterface *mVesc;
    Esp32Flash mEspFlash;
    bool mVescUploadOngoing;

    void listAllFw();
    void addFwToList(QString name, QString path);
};

#endif // PAGEESPPROG_H
