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

#include "pageterminal.h"
#include "ui_pageterminal.h"
#include "utility.h"

PageTerminal::PageTerminal(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PageTerminal)
{
    ui->setupUi(this);

    ui->helpButton->setIcon(Utility::getIcon("icons/Help-96.png"));
    ui->sendButton->setIcon(Utility::getIcon("icons/Send File-96.png"));
    ui->clearButton->setIcon(Utility::getIcon("icons/Delete-96.png"));

    layout()->setContentsMargins(0, 0, 0, 0);
    mVesc = 0;
}

PageTerminal::~PageTerminal()
{
    delete ui;
}

void PageTerminal::clearTerminal()
{
    ui->terminalBrowser->clear();
    ui->terminalEdit->setFocus();
}

VescInterface *PageTerminal::vesc() const
{
    return mVesc;
}

void PageTerminal::setVesc(VescInterface *vesc)
{
    mVesc = vesc;

    if (mVesc) {
        connect(mVesc->commands(), SIGNAL(printReceived(QString)),
                this, SLOT(printReceived(QString)));
    }
}

void PageTerminal::printReceived(QString str)
{
    ui->terminalBrowser->moveCursor(QTextCursor::End);
    ui->terminalBrowser->insertPlainText(str + "\n");
    ui->terminalBrowser->moveCursor(QTextCursor::End);

    int maxLines = 5000;
    int removeLines = 1000;

    if (ui->terminalBrowser->document()->lineCount() > maxLines) {
        QString txt = ui->terminalBrowser->toPlainText();
        auto lines = txt.split("\n");
        if (lines.length() >= removeLines) {
            QString shorter;
            for (int i = removeLines;i < lines.length();i++) {
                shorter.append(lines.at(i));

                if (i != (lines.length() - 1)) {
                    shorter.append("\n");
                }
            }
            ui->terminalBrowser->setText(shorter);
            ui->terminalBrowser->moveCursor(QTextCursor::End);
        }
    }
}

void PageTerminal::on_sendButton_clicked()
{
    if (mVesc && mVesc->isPortConnected()) {
        mVesc->commands()->sendTerminalCmd(ui->terminalEdit->text());
        ui->terminalEdit->clear();
    }
    else {
        ui->terminalBrowser->append("VESC not connected");
    }

    ui->terminalEdit->setFocus();
}

void PageTerminal::on_helpButton_clicked()
{
    ui->terminalEdit->setText("help");
    on_sendButton_clicked();
}

void PageTerminal::showEvent(QShowEvent *event)
{
    QWidget::showEvent( event );
    ui->terminalEdit->setFocus();
}
