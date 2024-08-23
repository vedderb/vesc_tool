/*
    Copyright 2021 - 2022 Benjamin Vedder	benjamin@vedder.se

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

#ifndef QMLEDITOR_H
#define QMLEDITOR_H

#include <QWidget>
#include <QCodeEditor>
#include <QFileSystemWatcher>

namespace Ui {
class ScriptEditor;
}

class ScriptEditor : public QWidget
{
    Q_OBJECT

public:
    explicit ScriptEditor(QWidget *parent = nullptr);
    ~ScriptEditor();

    QCodeEditor *codeEditor();
    QString fileNow();
    void setFileNow(QString fileName);
    void setModeQml();
    void setModeLisp();
    QString contentAsText();
    bool hasUnsavedContent();

    bool isDirty = false;

protected:
    void keyPressEvent(QKeyEvent *event);

signals:
    void fileOpened(QString fileName);
    void fileSaved(QString fileName);
    void fileNameChanged(QString newName);

private slots:
    void on_openFileButton_clicked();
    void on_saveButton_clicked();
    void on_saveAsButton_clicked();
    void on_searchEdit_textChanged(const QString &arg1);
    void on_searchPrevButton_clicked();
    void on_searchNextButton_clicked();
    void on_replaceThisButton_clicked();
    void on_replaceAllButton_clicked();
    void on_searchHideButton_clicked();
    void on_searchCaseSensitiveBox_toggled(bool checked);
    void on_refreshButton_clicked();
    void on_searchEdit_returnPressed();
    void on_fileChangeIgnoreButton_clicked();
    void on_fileChangedReloadButton_clicked();

private:
    Ui::ScriptEditor *ui;
    bool mIsModeLisp;
    QFileSystemWatcher mFsWatcher;

};

#endif // QMLEDITOR_H
