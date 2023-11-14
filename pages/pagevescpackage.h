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

#ifndef PAGEVESCPACKAGE_H
#define PAGEVESCPACKAGE_H

#include <QWidget>
#include <QListWidgetItem>
#include <QTimer>
#include "vescinterface.h"
#include "codeloader.h"

namespace Ui {
class PageVescPackage;
}

class PageVescPackage : public QWidget
{
    Q_OBJECT

public:
    explicit PageVescPackage(QWidget *parent = nullptr);
    ~PageVescPackage();

    VescInterface *vesc() const;
    void setVesc(VescInterface *vesc);
    void reloadParams();

private slots:
    void on_chooseLoadButton_clicked();
    void on_chooseLispButton_clicked();
    void on_chooseQmlButton_clicked();
    void on_chooseOutputButton_clicked();
    void on_saveButton_clicked();
    void on_loadRefreshButton_clicked();
    void on_writeButton_clicked();
    void on_outputRefreshButton_clicked();
    void on_dlArchiveButton_clicked();
    void on_uninstallButton_clicked();
    void on_installButton_clicked();
    void on_applicationList_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);
    void on_libraryList_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);

private:
    Ui::PageVescPackage *ui;
    VescInterface *mVesc;
    CodeLoader mLoader;
    VescPackage mCurrentPkg;
    bool mDescriptionUpdated;
    QTimer *mPreviewTimer;

    void reloadArchive();
    void packageSelected(VescPackage pkg);

};

#endif // PAGEVESCPACKAGE_H
