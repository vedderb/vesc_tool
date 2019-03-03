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

#include "detectfochall.h"
#include "ui_detectfochall.h"
#include "helpdialog.h"
#include <QMessageBox>

DetectFocHall::DetectFocHall(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DetectFocHall)
{
    ui->setupUi(this);
    layout()->setContentsMargins(0, 0, 0, 0);
    mVesc = 0;
}

DetectFocHall::~DetectFocHall()
{
    delete ui;
}

void DetectFocHall::on_helpButton_clicked()
{
    if (mVesc) {
        HelpDialog::showHelp(this, mVesc->infoConfig(), "help_foc_hall_detect");
    }
}

void DetectFocHall::on_startButton_clicked()
{
    if (mVesc) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::warning(this,
                                     tr("Detect FOC Hall Sensor Parameters"),
                                     tr("This is going to turn the motor slowly. Make "
                                        "sure that nothing is in the way."),
                                     QMessageBox::Ok | QMessageBox::Cancel);

        if (reply == QMessageBox::Ok) {
            mVesc->commands()->measureHallFoc(ui->currentBox->value());
        }
    }
}

void DetectFocHall::on_applyButton_clicked()
{
    if (mVesc) {
        mVesc->mcConfig()->updateParamInt("foc_hall_table__0", ui->hall0Box->value());
        mVesc->mcConfig()->updateParamInt("foc_hall_table__1", ui->hall1Box->value());
        mVesc->mcConfig()->updateParamInt("foc_hall_table__2", ui->hall2Box->value());
        mVesc->mcConfig()->updateParamInt("foc_hall_table__3", ui->hall3Box->value());
        mVesc->mcConfig()->updateParamInt("foc_hall_table__4", ui->hall4Box->value());
        mVesc->mcConfig()->updateParamInt("foc_hall_table__5", ui->hall5Box->value());
        mVesc->mcConfig()->updateParamInt("foc_hall_table__6", ui->hall6Box->value());
        mVesc->mcConfig()->updateParamInt("foc_hall_table__7", ui->hall7Box->value());
        mVesc->emitStatusMessage(tr("Hall Sensor Parameters Applied"), true);
    }
}

VescInterface *DetectFocHall::vesc() const
{
    return mVesc;
}

void DetectFocHall::setVesc(VescInterface *vesc)
{
    mVesc = vesc;

    if (mVesc) {
        connect(mVesc->commands(), SIGNAL(focHallTableReceived(QVector<int>,int)),
                this, SLOT(focHallTableReceived(QVector<int>,int)));
    }
}

void DetectFocHall::focHallTableReceived(QVector<int> hall_table, int res)
{
    if (res != 0) {
        mVesc->emitStatusMessage(tr("Bad FOC Hall Detection Result Received"), false);
    } else {
        mVesc->emitStatusMessage(tr("FOC Hall Result Received"), true);
        ui->hall0Box->setValue(hall_table.at(0));
        ui->hall1Box->setValue(hall_table.at(1));
        ui->hall2Box->setValue(hall_table.at(2));
        ui->hall3Box->setValue(hall_table.at(3));
        ui->hall4Box->setValue(hall_table.at(4));
        ui->hall5Box->setValue(hall_table.at(5));
        ui->hall6Box->setValue(hall_table.at(6));
        ui->hall7Box->setValue(hall_table.at(7));
    }
}
