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

#include "helpdialog.h"
#include "ui_helpdialog.h"
#include <QDebug>
#include <QMessageBox>

HelpDialog::HelpDialog(QString title, QString text, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HelpDialog)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(title);
    ui->textEdit->setText(text);
    ui->textEdit->viewport()->setAutoFillBackground(false);
}

HelpDialog::~HelpDialog()
{
    delete ui;
}

void HelpDialog::showHelp(QWidget *parent, ConfigParams *params, QString name, bool modal)
{
    ConfigParam *param = params->getParam(name);

    if (param) {
        HelpDialog *h = new HelpDialog(param->longName,
                                       param->description,
                                       parent);
        if (modal) {
            h->exec();
        } else {
            h->show();
        }
    } else {
        QMessageBox::warning(parent,
                             tr("Show help"),
                             tr("Help text for %1 not found.").arg(name));
    }
}

void HelpDialog::showHelp(QWidget *parent, QString title, QString text)
{
    HelpDialog *h = new HelpDialog(title, text, parent);
    h->exec();
}

void HelpDialog::showEvent(QShowEvent *event)
{
    QSize s = ui->textEdit->document()->size().toSize();
    int tot = (this->height() - ui->textEdit->height()) + s.height() + 5;

    if (tot < 140) {
        this->resize(this->width(), 140);
    } else if (tot > 450) {
        this->resize(this->width(), 450);
    } else {
        this->resize(this->width(), tot);
    }

    QDialog::showEvent(event);
}

void HelpDialog::on_okButton_clicked()
{
    close();
}
