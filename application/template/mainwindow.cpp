/*
    Copyright 2018 Benjamin Vedder	benjamin@vedder.se

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    */

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
	
	mVesc = new VescInterface(this);
    mVesc->mcConfig()->loadParamsXml("://res/parameters_mcconf.xml");
    mVesc->appConfig()->loadParamsXml("://res/parameters_appconf.xml");
    mVesc->infoConfig()->loadParamsXml("://res/info.xml");

    mTimer = new QTimer(this);
    mTimer->start(20);

    connect(mTimer, SIGNAL(timeout()), this, SLOT(timerSlot()));
    connect(mVesc->commands(), SIGNAL(valuesReceived(MC_VALUES,unsigned int)),
            this, SLOT(valuesReceived(MC_VALUES,unsigned int)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::timerSlot()
{
    if (ui->statusLabel->text() != mVesc->getConnectedPortName()) {
        ui->statusLabel->setText(mVesc->getConnectedPortName());
    }

    if (mVesc->isPortConnected()) {
        mVesc->commands()->getValues();
    }
}

void MainWindow::valuesReceived(MC_VALUES values, unsigned int mask)
{
    (void)mask;
    ui->textEdit->clear();
    ui->textEdit->appendPlainText(
                QString::asprintf(
                    "V_IN    : %.2f\n"
                    "Current : %.2f",
                    values.v_in,
                    values.current_motor));
}

void MainWindow::on_connectButton_clicked()
{
    mVesc->autoconnect();
}

void MainWindow::on_disconnectButton_clicked()
{
    mVesc->disconnectPort();
}
