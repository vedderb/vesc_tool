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

#ifndef HELPDIALOG_H
#define HELPDIALOG_H

#include <QDialog>
#include "configparams.h"

namespace Ui {
class HelpDialog;
}

class HelpDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HelpDialog(QString title, QString text, QWidget *parent = 0);
    ~HelpDialog();

    void showEvent(QShowEvent *event) override;

    static void showHelp(QWidget *parent, ConfigParams *params, QString name, bool modal = true);
    static void showHelp(QWidget *parent, QString title, QString text);

private slots:
    void on_okButton_clicked();

private:
    Ui::HelpDialog *ui;
};

#endif // HELPDIALOG_H
