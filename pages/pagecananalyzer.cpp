/*
    Copyright 2020 Benjamin Vedder	benjamin@vedder.se

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

#include "pagecananalyzer.h"
#include "ui_pagecananalyzer.h"

PageCanAnalyzer::PageCanAnalyzer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PageCanAnalyzer)
{
    ui->setupUi(this);
    layout()->setContentsMargins(0, 0, 0, 0);
    ui->msgTable->setColumnWidth(1, 120);
    mVesc = nullptr;
}

PageCanAnalyzer::~PageCanAnalyzer()
{
    delete ui;
}

VescInterface *PageCanAnalyzer::vesc() const
{
    return mVesc;
}

void PageCanAnalyzer::setVesc(VescInterface *vesc)
{
    mVesc = vesc;

    if (mVesc) {
        reloadParams();
        connect(mVesc->commands(), SIGNAL(canFrameRx(QByteArray,quint32,bool)),
                this, SLOT(canFrameRx(QByteArray,quint32,bool)));
    }
}

void PageCanAnalyzer::reloadParams()
{
    if (mVesc) {
        ui->paramTable->clearParams();
        ui->paramTable->addParamRow(mVesc->appConfig(), "can_mode");
        ui->paramTable->addParamRow(mVesc->appConfig(), "can_baud_rate");
    }
}

void PageCanAnalyzer::canFrameRx(QByteArray data, quint32 id, bool isExtended)
{
    ui->msgTable->setRowCount(ui->msgTable->rowCount() + 1);

    ui->msgTable->setItem(ui->msgTable->rowCount() - 1, 0,
                          new QTableWidgetItem(QString(isExtended ? "Yes" : "No")));
    ui->msgTable->setItem(ui->msgTable->rowCount() - 1, 1,
                          new QTableWidgetItem(QString("0x%1").
                                               arg(id, 8, 16, QLatin1Char('0'))));
    ui->msgTable->setItem(ui->msgTable->rowCount() - 1, 2,
                          new QTableWidgetItem(QString("%1").arg(data.size())));

    for (int i = 0;i < data.size();i++) {
        ui->msgTable->setItem(ui->msgTable->rowCount() - 1, i + 3,
                              new QTableWidgetItem(QString("0x%1").
                                                   arg(quint8(data.at(i)),
                                                       2, 16, QLatin1Char('0'))));
    }
}

void PageCanAnalyzer::on_sendButton_clicked()
{
    if (mVesc) {
        VByteArray vb;
        QVector<int> bytes;

        bytes.append(ui->sendD0Box->value());
        bytes.append(ui->sendD1Box->value());
        bytes.append(ui->sendD2Box->value());
        bytes.append(ui->sendD3Box->value());
        bytes.append(ui->sendD4Box->value());
        bytes.append(ui->sendD5Box->value());
        bytes.append(ui->sendD6Box->value());
        bytes.append(ui->sendD7Box->value());

        for (auto i: bytes) {
            if (i >= 0) {
                vb.vbAppendUint8(quint8(i));
            } else {
                break;
            }
        }

        QString idTxt = ui->sendIdEdit->text().toLower().replace(" ", "");
        quint32 id = 0;
        bool ok = false;

        if (idTxt.startsWith("0x")) {
            idTxt.remove(0, 2);
            id = idTxt.toUInt(&ok, 16);
        } else {
            id = idTxt.toUInt(&ok, 10);
        }

        if (ok) {
            mVesc->commands()->forwardCanFrame(vb, id, ui->sendExtBox->currentIndex() == 1);
        } else {
            mVesc->emitMessageDialog("Send CAN",
                                     "Unable to parse ID. ID must be a decimal number, or "
                                     "a hexadecimal number starting with 0x",
                                     false);
        }
    }
}

void PageCanAnalyzer::on_clearRxButton_clicked()
{
    ui->msgTable->setRowCount(0);
}
