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

#include "paramdialog.h"
#include "ui_paramdialog.h"

ParamDialog::ParamDialog(QString title,
                         QString text,
                         ConfigParams *params,
                         QStringList names,
                         QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ParamDialog)
{
    ui->setupUi(this);
    setWindowTitle(title);
    ui->textLabel->setText(text);
    mConfig = *params;

    for (QString s: names) {
        ConfigParam *p = mConfig.getParam(s);

        if (p) {
            p->transmittable = false; // To hide the read buttons.
            ui->paramTable->addParamRow(&mConfig, s);
        }
    }

    setAttribute(Qt::WA_DeleteOnClose);
}

ParamDialog::~ParamDialog()
{
    delete ui;
}

void ParamDialog::showParams(QString title,
                             QString text,
                             ConfigParams *params,
                             QStringList names,
                             QWidget *parent)
{
    ParamDialog *p = new ParamDialog(title, text, params, names, parent);
    p->exec();
}

void ParamDialog::on_closeButton_clicked()
{
    close();
}
