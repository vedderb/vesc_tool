/*
    Copyright 2021 Benjamin Vedder	benjamin@vedder.se

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

#include "customguitool.h"
#include "ui_customguitool.h"

#include <QFileDialog>
#include <QSettings>

CustomGuiTool::CustomGuiTool(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::CustomGuiTool)
{
    ui->setupUi(this);
    mVesc = nullptr;

    QSettings set;
    ui->pathEdit->setText(set.value("pagecustomgui/lastqml", "").toString());
}

CustomGuiTool::~CustomGuiTool()
{
    QSettings set;
    set.setValue("pagecustomgui/lastqml", ui->pathEdit->text());

    delete ui;
}

void CustomGuiTool::setVesc(VescInterface *vesc)
{
    mVesc = vesc;
}

void CustomGuiTool::on_chooseButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Load QML File"), "",
                                                    tr("QML files (*.qml)"));

    if (!fileName.isEmpty()) {
        ui->pathEdit->setText(fileName);
    }
}

void CustomGuiTool::on_reloadButton_clicked()
{
    if (mVesc && !ui->pathEdit->text().isEmpty()) {
        mQmlUi.reloadCustomGui("file://" + ui->pathEdit->text());
    }
}

void CustomGuiTool::on_closeButton_clicked()
{
    mQmlUi.stopCustomGui();
}

void CustomGuiTool::on_loadButton_clicked()
{
    if (mVesc && !ui->pathEdit->text().isEmpty()) {
        mQmlUi.startCustomGui(mVesc);
        mQmlUi.reloadCustomGui("file://" + ui->pathEdit->text());
    }
}
