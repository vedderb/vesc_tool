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
#include <QDirIterator>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>

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
    ui->dlArchiveButton->setIcon(QPixmap(theme + "icons/Refresh-96.png"));
    ui->installButton->setIcon(QIcon(theme +"icons/Download-96.png"));
    ui->uninstallButton->setIcon(QIcon(theme +"icons/Delete-96.png"));

    QSettings set;
    ui->loadEdit->setText(set.value("pagevescpackage/lastpkgload", "").toString());
    ui->lispEdit->setText(set.value("pagevescpackage/lastlisp", "").toString());
    ui->qmlEdit->setText(set.value("pagevescpackage/lastqml", "").toString());
    ui->outputEdit->setText(set.value("pagevescpackage/lastoutput", "").toString());

    ui->descriptionEdit->document()->setHtml("Package Description");

    on_loadRefreshButton_clicked();
    on_outputRefreshButton_clicked();

    ui->splitter->setSizes(QList<int>({500, 1000}));

    reloadArchive();
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

    pkg.name = ui->nameEdit->text();
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
    ui->nameEdit->setText(pkg.name);
}

void PageVescPackage::on_dlArchiveButton_clicked()
{
    ui->dlArchiveButton->setEnabled(false);
    ui->displayDl->setText("Preparing download...");

    QUrl url("http://home.vedder.se/vesc_pkg/vesc_pkg_all.rcc");
    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    QNetworkReply *reply = manager.get(request);

    connect(reply, &QNetworkReply::downloadProgress, [this](qint64 bytesReceived, qint64 bytesTotal) {
        ui->displayDl->setText("Downloading...");
        ui->displayDl->setValue(100.0 * (double)bytesReceived / (double)bytesTotal);
    });

    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        ui->displayDl->setText("Download Finished");
        QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
                "/vesc_pkg_all.rcc";
        QFile file(path);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(reply->readAll());
            file.close();
            reloadArchive();
        }
    } else {
        ui->displayDl->setText("Download Failed");
    }

    reply->abort();
    reply->deleteLater();

    ui->dlArchiveButton->setEnabled(true);
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

    mLoader.qmlErase();
    mLoader.lispErase();

    mVesc->emitMessageDialog(tr("Uninstall Package"),
                             tr("Uninstallation Done! Please disconnect and reconnect to "
                                "revert possible VESC Tool GUI updates."),
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

    if (!mCurrentPath.isEmpty()) {
        QFile f(mCurrentPath);
        if (!f.open(QIODevice::ReadOnly)) {
            mVesc->emitMessageDialog(tr("Install Package"),
                                     tr("Could not open package file for reading."),
                                     false, false);
            return;
        }

        QProgressDialog dialog(tr("Writing..."), QString(), 0, 0, this);
        dialog.setWindowModality(Qt::WindowModal);
        dialog.show();

        mLoader.installVescPackage(f.readAll());

        mVesc->emitMessageDialog(tr("Install Package"),
                                 tr("Install Done! Please disconnect and reconnect to "
                                    "apply possible VESC Tool GUI updates from this package."),
                                 true);
    } else {
        mVesc->emitMessageDialog(tr("Install Package"),
                                 tr("No package selected."),
                                 false, false);
    }
}

void PageVescPackage::reloadArchive()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
            "/vesc_pkg_all.rcc";
    QFile file(path);
    if (file.exists()) {
        QResource::unregisterResource(path);
        QResource::registerResource(path);

        QString pkgDir = "://vesc_packages";

        ui->applicationList->clear();
        ui->libraryList->clear();

        QDirIterator it(pkgDir);
        while (it.hasNext()) {
            QFileInfo fi(it.next());

            QDirIterator it2(fi.absoluteFilePath());
            while (it2.hasNext()) {
                QFileInfo fi2(it2.next());

                if (fi2.absoluteFilePath().toLower().endsWith(".vescpkg")) {
                    QListWidgetItem *item = new QListWidgetItem;
                    QString name = fi2.fileName();

                    QFile f(fi2.absoluteFilePath());
                    if (f.open(QIODevice::ReadOnly)) {
                        auto pkg = mLoader.unpackVescPackage(f.readAll());
                        name = pkg.name;
                    }

                    item->setText(name);
                    item->setData(Qt::UserRole, fi2.absoluteFilePath());

                    if (fi2.absoluteFilePath().startsWith("://vesc_packages/lib_")) {
                        ui->libraryList->insertItem(ui->libraryList->count(), item);
                    } else {
                        ui->applicationList->insertItem(ui->applicationList->count(), item);
                    }
                }
            }
        }
    }
}

void PageVescPackage::packageSelected(QString path)
{
    mCurrentPath = path;
    QFile f(path);
    if (f.open(QIODevice::ReadOnly)) {
        auto pkg = mLoader.unpackVescPackage(f.readAll());
        ui->storeBrowser->document()->setHtml(pkg.description);
        ui->installButton->setEnabled(!path.startsWith("://vesc_packages/lib_"));
        if (ui->installButton->isEnabled()) {
            ui->installButton->setToolTip("");
        } else {
            ui->installButton->setToolTip("This is a library, so it is not supposed to be installed. You can use "
                                          "it from your own LispBM-scripts without installing it.");
        }
    }
}

void PageVescPackage::on_applicationList_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    (void)previous;
    if (current != nullptr) {
        packageSelected(current->data(Qt::UserRole).toString());
        ui->libraryList->setCurrentItem(nullptr);
    }
}

void PageVescPackage::on_libraryList_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    (void)previous;
    if (current != nullptr) {
        packageSelected(current->data(Qt::UserRole).toString());
        ui->applicationList->setCurrentItem(nullptr);
    }
}
