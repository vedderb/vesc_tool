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
#include <QMessageBox>
#include <QFileDialog>
#include <QProgressDialog>
#include <QDirIterator>

PageVescPackage::PageVescPackage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PageVescPackage)
{
    ui->setupUi(this);
    mVesc = nullptr;
    layout()->setContentsMargins(0, 0, 0, 0);

    ui->chooseLoadButton->setIcon(Utility::getIcon("icons/Open Folder-96.png"));
    ui->chooseLispButton->setIcon(Utility::getIcon("icons/Open Folder-96.png"));
    ui->chooseOutputButton->setIcon(Utility::getIcon("icons/Open Folder-96.png"));
    ui->chooseQmlButton->setIcon(Utility::getIcon("icons/Open Folder-96.png"));
    ui->writeButton->setIcon(Utility::getIcon("icons/Download-96.png"));
    ui->loadRefreshButton->setIcon(Utility::getIcon("icons/Refresh-96.png"));
    ui->outputRefreshButton->setIcon(Utility::getIcon("icons/Refresh-96.png"));
    ui->saveButton->setIcon(Utility::getIcon("icons/Save-96.png"));
    ui->dlArchiveButton->setIcon(Utility::getIcon("icons/Refresh-96.png"));
    ui->installButton->setIcon(Utility::getIcon("icons/Download-96.png"));
    ui->uninstallButton->setIcon(Utility::getIcon("icons/Delete-96.png"));

    QSettings set;
    ui->loadEdit->setText(set.value("pagevescpackage/lastpkgload", "").toString());
    ui->lispEdit->setText(set.value("pagevescpackage/lastlisp", "").toString());
    ui->qmlEdit->setText(set.value("pagevescpackage/lastqml", "").toString());
    ui->outputEdit->setText(set.value("pagevescpackage/lastoutput", "").toString());

    ui->descriptionEdit->document()->setHtml("Package Description");

    on_loadRefreshButton_clicked();
    on_outputRefreshButton_clicked();

    reloadArchive();

    mDescriptionUpdated = true;
    connect(ui->descriptionEdit, &QMarkdownTextEdit::textChanged, [this]() {
        mDescriptionUpdated = true;
    });

    mPreviewTimer = new QTimer(this);
    mPreviewTimer->start(500);
    connect(mPreviewTimer, &QTimer::timeout, [this]() {
        if (mDescriptionUpdated) {
            mDescriptionUpdated = false;
            auto posOld = ui->descriptionBrowser->verticalScrollBar()->value();
            ui->descriptionBrowser->setHtml(
                        Utility::md2html(ui->descriptionEdit->document()->toPlainText()));
            ui->descriptionBrowser->verticalScrollBar()->setValue(posOld);
        }
    });
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

        if (QFile::exists(filename)) {
            if (ui->descriptionEdit->toPlainText().size() > 10) {
                QMessageBox::StandardButton reply =
                        QMessageBox::warning(this,
                                             tr("Replace Content"),
                                             tr("Opening an existing package will replace the content "
                                                "in the editor. Do you want to continue?"),
                                             QMessageBox::Ok | QMessageBox::Cancel);

                if (reply == QMessageBox::Ok) {
                    ui->outputEdit->setText(filename);
                    on_outputRefreshButton_clicked();
                }
            } else {
                ui->outputEdit->setText(filename);
                on_outputRefreshButton_clicked();
            }
        } else {
            ui->outputEdit->setText(filename);
            on_saveButton_clicked();
        }
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

    pkg.name = ui->nameEdit->text();
    pkg.description = ui->descriptionEdit->document()->toPlainText();

    file.write(mLoader.packVescPackage(pkg));
    file.close();

    mVesc->emitStatusMessage(tr("Package Saved"), true);
}

void PageVescPackage::on_loadRefreshButton_clicked()
{
    QFile f(ui->loadEdit->text());
    if (!f.open(QIODevice::ReadOnly)) {
        return;
    }

    auto pkg = mLoader.unpackVescPackage(f.readAll());

    QString line1 = QTextStream(&pkg.description).readLine();
    if (line1.contains("<!DOCTYPE HTML PUBLIC", Qt::CaseInsensitive)) {
        ui->loadBrowser->document()->setHtml(pkg.description);
    } else {
        ui->loadBrowser->document()->setHtml(Utility::md2html(pkg.description));
    }
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

    QProgressDialog dialog(tr("Writing..."), QString(), 0, 0, this);
    dialog.setWindowModality(Qt::WindowModal);
    dialog.show();

    mLoader.installVescPackageFromPath(ui->loadEdit->text());
}

void PageVescPackage::on_outputRefreshButton_clicked()
{
    QFile f(ui->outputEdit->text());
    if (!f.open(QIODevice::ReadOnly)) {
        return;
    }

    auto pkg = mLoader.unpackVescPackage(f.readAll());

    QString line1 = QTextStream(&pkg.description).readLine();
    if (line1.contains("<!DOCTYPE HTML PUBLIC", Qt::CaseInsensitive)) {
        ui->descriptionEdit->document()->setHtml(pkg.description);
        QString md = ui->descriptionEdit->document()->toMarkdown();
        ui->descriptionEdit->document()->setPlainText(md);
    } else {
        ui->descriptionEdit->document()->setPlainText(pkg.description);
    }

    mDescriptionUpdated = true;

    ui->nameEdit->setText(pkg.name);
}

void PageVescPackage::on_dlArchiveButton_clicked()
{
    ui->dlArchiveButton->setEnabled(false);
    ui->displayDl->setText("Preparing download...");

    connect(&mLoader, &CodeLoader::downloadProgress, [this](qint64 bytesReceived, qint64 bytesTotal) {
        ui->displayDl->setText("Downloading...");
        ui->displayDl->setValue(100.0 * (double)bytesReceived / (double)bytesTotal);
    });

    bool ok = mLoader.downloadPackageArchive();

    if (ok) {
        ui->displayDl->setText("Download Finished");
        mVesc->emitStatusMessage("Downloads OK", true);
    } else {
        ui->displayDl->setText("Download Failed");
        mVesc->emitStatusMessage("Downloads Failed", false);
    }

    ui->dlArchiveButton->setEnabled(true);

    reloadArchive();
}

void PageVescPackage::on_uninstallButton_clicked()
{
    if (!mVesc) {
        return;
    }

    if (!mVesc->isPortConnected()) {
        mVesc->emitMessageDialog(tr("Uninstall Package"), tr("Not Connected"), false);
        return;
    }

    QProgressDialog dialog(tr("Erasing..."), QString(), 0, 0, this);
    dialog.setWindowModality(Qt::WindowModal);
    dialog.show();

    mLoader.qmlErase(16);
    mLoader.lispErase(16);

    Utility::sleepWithEventLoop(500);
    mVesc->reloadFirmware();

    mVesc->emitMessageDialog(tr("Uninstall Package"),
                             tr("Uninstallation Done!"),
                             true);
}

void PageVescPackage::on_installButton_clicked()
{
    if (!mVesc) {
        return;
    }

    if (!mVesc->isPortConnected()) {
        mVesc->emitMessageDialog(tr("Install Package"), tr("Not Connected"), false);
        return;
    }

    if (mCurrentPkg.loadOk) {
        QProgressDialog dialog(tr("Writing..."), QString(), 0, 0, this);
        dialog.setWindowModality(Qt::WindowModal);
        dialog.show();

        mLoader.installVescPackage(mCurrentPkg);

        mVesc->emitMessageDialog(tr("Install Package"),
                                 tr("Installation Done!"),
                                 true);
    } else {
        mVesc->emitMessageDialog(tr("Install Package"),
                                 tr("No package selected."),
                                 false, false);
    }
}

void PageVescPackage::reloadArchive()
{
    auto pList = mLoader.reloadPackageArchive();

    ui->applicationList->clear();
    ui->libraryList->clear();

    foreach (auto p, pList) {
        auto pVal = p.value<VescPackage>();
        QListWidgetItem *item = new QListWidgetItem;
        item->setText(pVal.name);
        item->setData(Qt::UserRole, p);

        if (pVal.isLibrary) {
            ui->libraryList->insertItem(ui->libraryList->count(), item);
        } else {
            ui->applicationList->insertItem(ui->applicationList->count(), item);
        }
    }
}

void PageVescPackage::packageSelected(VescPackage pkg)
{
    mCurrentPkg = pkg;
    ui->storeBrowser->document()->setHtml(Utility::md2html(pkg.description));
    ui->installButton->setEnabled(!pkg.isLibrary);
    if (ui->installButton->isEnabled()) {
        ui->installButton->setToolTip("");
    } else {
        ui->installButton->setToolTip("This is a library, so it is not supposed to be installed. You can use "
                                      "it from your own LispBM-scripts without installing it.");
    }
}

void PageVescPackage::on_applicationList_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    (void)previous;
    if (current != nullptr) {
        packageSelected(current->data(Qt::UserRole).value<VescPackage>());
        ui->libraryList->setCurrentItem(nullptr);
    }
}

void PageVescPackage::on_libraryList_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    (void)previous;
    if (current != nullptr) {
        packageSelected(current->data(Qt::UserRole).value<VescPackage>());
        ui->applicationList->setCurrentItem(nullptr);
    }
}
