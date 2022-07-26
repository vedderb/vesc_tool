/*
    Copyright 2022 Benjamin Vedder	benjamin@vedder.se

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

#include "pagevescpackage.h"
#include "ui_pagevescpackage.h"
#include "utility.h"

PageVescPackage::PageVescPackage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PageVescPackage)
{
    ui->setupUi(this);
    mVesc = nullptr;
    layout()->setContentsMargins(0, 0, 0, 0);

    QString theme = Utility::getThemePath();
    ui->chooseLoadButton->setIcon(QIcon(theme +"icons/Open Folder-96.png"));
    ui->chooseLispButton->setIcon(QIcon(theme +"icons/Open Folder-96.png"));
    ui->chooseOutputButton->setIcon(QIcon(theme +"icons/Open Folder-96.png"));
    ui->chooseQmlButton->setIcon(QIcon(theme +"icons/Open Folder-96.png"));
    ui->writeButton->setIcon(QIcon(theme +"icons/Download-96.png"));
    ui->loadRefreshButton->setIcon(QIcon(theme +"icons/Refresh-96.png"));
    ui->outputRefreshButton->setIcon(QIcon(theme +"icons/Refresh-96.png"));
    ui->saveButton->setIcon(QIcon(theme +"icons/Save-96.png"));

    QSettings set;
    ui->loadEdit->setText(set.value("pagevescpackage/lastpkgload", "").toString());
    ui->lispEdit->setText(set.value("pagevescpackage/lastlisp", "").toString());
    ui->qmlEdit->setText(set.value("pagevescpackage/lastqml", "").toString());
    ui->outputEdit->setText(set.value("pagevescpackage/lastoutput", "").toString());

    on_loadRefreshButton_clicked();
    on_outputRefreshButton_clicked();
}

PageVescPackage::~PageVescPackage()
{
    QSettings set;
    set.setValue("pagevescpackage/lastpkgload", ui->loadEdit->text());
    set.setValue("pagevescpackage/lastlisp", ui->lispEdit->text());
    set.setValue("pagevescpackage/lastqml", ui->qmlEdit->text());
    set.setValue("pagevescpackage/lastoutput", ui->outputEdit->text());
    delete ui;
}

VescInterface *PageVescPackage::vesc() const
{
    return mVesc;
}

void PageVescPackage::setVesc(VescInterface *vesc)
{
    mVesc = vesc;
    mLoader.setVesc(vesc);
}

void PageVescPackage::reloadParams()
{

}

void PageVescPackage::on_chooseLoadButton_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Choose Package File"), ui->loadEdit->text(),
                                                    tr("VESC Package Files (*.vescpkg)"));
    if (!filename.isNull()) {
        ui->loadEdit->setText(filename);
        on_loadRefreshButton_clicked();
    }
}

void PageVescPackage::on_chooseLispButton_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Choose Lisp File"), ui->lispEdit->text(),
                                                    tr("Lisp files (*.lisp)"));
    if (!filename.isNull()) {
        ui->lispEdit->setText(filename);
    }
}

void PageVescPackage::on_chooseQmlButton_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Choose Qml File"), ui->qmlEdit->text(),
                                                    tr("Qml files (*.qml)"));
    if (!filename.isNull()) {
        ui->qmlEdit->setText(filename);
    }
}

void PageVescPackage::on_chooseOutputButton_clicked()
{
    QString filename = QFileDialog::getSaveFileName(this,
                                                    tr("Choose Package Output File"), ui->outputEdit->text(),
                                                    tr("VESC Package Files (*.vescpkg)"));

    if (!filename.isNull()) {
        if (!filename.endsWith(".vescpkg", Qt::CaseInsensitive)) {
            filename += ".vescpkg";
        }

        ui->outputEdit->setText(filename);
        on_outputRefreshButton_clicked();
    }
}

void PageVescPackage::on_saveButton_clicked()
{
    if (!mVesc) {
        return;
    }

    if (ui->outputEdit->text().isEmpty()) {
        on_chooseOutputButton_clicked();
    }

    QFile file(ui->outputEdit->text());

    if (!file.open(QIODevice::WriteOnly)) {
        mVesc->emitMessageDialog(tr("Save Package"),
                                 tr("Could not open %1 for writing.").arg(ui->outputEdit->text()),
                                 false, false);
        return;
    }

    VescPackage pkg;

    if (ui->lispBox->isChecked()) {
        QFile f(ui->lispEdit->text());
        if (!f.open(QIODevice::ReadOnly)) {
            mVesc->emitMessageDialog(tr("Save Package"),
                                     tr("Could not open lisp file for reading."),
                                     false, false);
            return;
        }

        QFileInfo fi(f);
        pkg.lispData = mLoader.lispPackImports(f.readAll(), fi.canonicalPath());
        f.close();
    }

    if (ui->qmlBox->isChecked()) {
        QFile f(ui->qmlEdit->text());
        if (!f.open(QIODevice::ReadOnly)) {
            mVesc->emitMessageDialog(tr("Save Package"),
                                     tr("Could not open qml file for reading."),
                                     false, false);
            return;
        }

        pkg.qmlFile = f.readAll();
        pkg.qmlIsFullscreen = ui->qmlFullscreenBox->isChecked();
        f.close();
    }

    pkg.description = ui->descriptionEdit->document()->toHtml();

    file.write(mLoader.packVescPackage(pkg));
    file.close();

    mVesc->emitMessageDialog(tr("Save Package"),
                             tr("Package Saved"),
                             true, false);
}

void PageVescPackage::on_loadRefreshButton_clicked()
{
    QFile f(ui->loadEdit->text());
    if (!f.open(QIODevice::ReadOnly)) {
        return;
    }

    auto pkg = mLoader.unpackVescPackage(f.readAll());
    ui->loadBrowser->document()->setHtml(pkg.description);
}

void PageVescPackage::on_writeButton_clicked()
{
    if (!mVesc) {
        return;
    }

    if (!mVesc->isPortConnected()) {
        mVesc->emitMessageDialog(tr("Write Package"), tr("Not Connected"), false);
        return;
    }

    QFile f(ui->loadEdit->text());
    if (!f.open(QIODevice::ReadOnly)) {
        mVesc->emitMessageDialog(tr("Write Package"),
                                 tr("Could not open package file for reading."),
                                 false, false);
        return;
    }

    QProgressDialog dialog(tr("Writing..."), QString(), 0, 0, this);
    dialog.setWindowModality(Qt::WindowModal);
    dialog.show();

    mLoader.installVescPackage(f.readAll());
}

void PageVescPackage::on_outputRefreshButton_clicked()
{
    QFile f(ui->outputEdit->text());
    if (!f.open(QIODevice::ReadOnly)) {
        return;
    }

    auto pkg = mLoader.unpackVescPackage(f.readAll());
    ui->descriptionEdit->document()->setHtml(pkg.description);
}
