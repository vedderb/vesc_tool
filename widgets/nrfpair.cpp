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

#include "nrfpair.h"
#include "ui_nrfpair.h"
#include "helpdialog.h"
#include <QMessageBox>

NrfPair::NrfPair(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::NrfPair)
{
    ui->setupUi(this);
    layout()->setContentsMargins(0, 0, 0, 0);
    mVesc = 0;
    mPairCnt = 0.0;
    mRunning = false;

    mTimer = new QTimer(this);
    mTimer->start(100);

    connect(mTimer, SIGNAL(timeout()),
            this, SLOT(timerSlot()));
}

NrfPair::~NrfPair()
{
    delete ui;
}

VescInterface *NrfPair::vesc() const
{
    return mVesc;
}

void NrfPair::setVesc(VescInterface *vesc)
{
    mVesc = vesc;

    if (mVesc) {
        connect(mVesc->commands(), SIGNAL(nrfPairingRes(int)),
                this, SLOT(nrfPairingRes(int)));
    }
}

void NrfPair::startPairing()
{
    if (mVesc) {
        mVesc->commands()->pairNrf((int)(ui->timeBox->value() * 1000.0));
        mRunning = true;
    }
}

void NrfPair::nrfPairingRes(int res)
{
    if (!mRunning) {
        return;
    }

    switch (res) {
    case NRF_PAIR_STARTED:
        mPairCnt = ui->timeBox->value();
        ui->countLabel->setText(tr("%1").arg(mPairCnt, 0, 'f', 1));
        ui->startButton->setEnabled(false);
        break;

    case NRF_PAIR_OK:
        ui->startButton->setEnabled(true);
        mPairCnt = 0.0;
        ui->countLabel->setText(tr("%1").arg(mPairCnt, 0, 'f', 1));
        mVesc->emitStatusMessage("Pairing NRF Sucessful", true);
        QMessageBox::information(this,
                                 tr("NRF Pairing"),
                                 tr("Pairing was successful."));
        mRunning = false;
        break;

    case NRF_PAIR_FAIL:
        ui->startButton->setEnabled(true);
        mPairCnt = 0.0;
        ui->countLabel->setText(tr("%1").arg(mPairCnt, 0, 'f', 1));
        mVesc->emitStatusMessage("Pairing NRF Timed Out", false);
        mRunning = false;

        QMessageBox::StandardButton reply;
        reply = QMessageBox::warning(this,
                             tr("NRF Pairing"),
                             tr("Pairing timed out. Make sure to put your device (e.g. NRF nunchuk) "
                                "in pairing mode before the time runs out."
                                "<br><br>"
                                "To put the NRF nunchuk in "
                                "pairing mode, just switch it on using any of the buttons. Then it "
                                "will enter pairing mode if it was switched off previously."
                                "<br><br>"
                                "Would you like to try again?"),
                             QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

        if (reply == QMessageBox::Yes) {
            startPairing();
        }
        break;

    default:
        break;
    }
}

void NrfPair::timerSlot()
{
    if (mPairCnt > 0.01) {
        mPairCnt -= 0.1;

        if (mPairCnt <= 0.01) {
            ui->startButton->setEnabled(true);
            mPairCnt = 0.0;
        }

        ui->countLabel->setText(tr("%1").arg(mPairCnt, 0, 'f', 1));
    }
}

void NrfPair::on_startButton_clicked()
{
    startPairing();
}

void NrfPair::on_helpButton_clicked()
{
    if (mVesc) {
        HelpDialog::showHelp(this, mVesc->infoConfig(), "help_nrf_pair");
    }
}
