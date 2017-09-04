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

#ifndef PARAMDIALOG_H
#define PARAMDIALOG_H

#include <QDialog>
#include "configparams.h"

namespace Ui {
class ParamDialog;
}

class ParamDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ParamDialog(QString title,
                         QString text,
                         ConfigParams *params,
                         QStringList names,
                         QWidget *parent = 0);
    ~ParamDialog();

    static void showParams(QString title,
                           QString text,
                           ConfigParams *params,
                           QStringList names,
                           QWidget *parent = 0);

private slots:
    void on_closeButton_clicked();

private:
    Ui::ParamDialog *ui;
    ConfigParams mConfig;

};

#endif // PARAMDIALOG_H
