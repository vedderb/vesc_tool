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

#ifndef PAGELISP_H
#define PAGELISP_H

#include <QWidget>
#include "vescinterface.h"
#include "widgets/scripteditor.h"
#include "codeloader.h"

namespace Ui {
class PageLisp;
}

class PageLisp : public QWidget
{
    Q_OBJECT

public:
    explicit PageLisp(QWidget *parent = nullptr);
    ~PageLisp();

    VescInterface *vesc() const;
    void setVesc(VescInterface *vesc);
    void reloadParams();
    bool hasUnsavedTabs();

private slots:
    void on_openRecentButton_clicked();
    void on_removeSelectedButton_clicked();
    void on_clearRecentButton_clicked();
    void on_recentList_doubleClicked();
    void on_openExampleButton_clicked();
    void on_exampleList_doubleClicked();
    void on_runButton_clicked();
    void on_stopButton_clicked();
    void on_uploadButton_clicked();
    void on_readExistingButton_clicked();
    void on_eraseButton_clicked();
    void on_rescaleButton_clicked();
    void on_helpButton_clicked();
    void on_replEdit_returnPressed();
    void on_replHelpButton_clicked();
    void on_streamButton_clicked();
    void on_recentFilterEdit_textChanged(const QString &arg1);
    void on_exampleFilterEdit_textChanged(const QString &arg1);

private:
    Ui::PageLisp *ui;
    VescInterface *mVesc;
    QStringList mRecentFiles;
    QString mDirNow;
    QTimer mPollTimer;
    QMap<QString, QVector<QPair<qint64, double> > > mBindingData;
    CodeLoader mLoader;

    void updateRecentList();
    void makeEditorConnections(ScriptEditor *editor);
    void createEditorTab(QString fileName, QString content);
    void removeEditor(ScriptEditor *editor);
    void setEditorDirty(ScriptEditor * editor);
    void setEditorClean(ScriptEditor * editor);
    void openExample();
    void openRecentList();
    bool eraseCode(int size);

};

#endif // PAGELISP_H
