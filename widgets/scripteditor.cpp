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

#include "scripteditor.h"
#include "ui_scripteditor.h"

#include <QmlHighlighter>
#include <LispHighlighter>
#include <QVescCompleter>
#include <QLispCompleter>
#include <QMessageBox>
#include <QFileDialog>
#include "utility.h"

ScriptEditor::ScriptEditor(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ScriptEditor)
{
    ui->setupUi(this);
    mIsModeLisp = false;

    QString theme = Utility::getThemePath();
    ui->searchHideButton->setIcon(QPixmap(theme + "icons/Cancel-96.png"));
    ui->openFileButton->setIcon(QPixmap(theme + "icons/Open Folder-96.png"));
    ui->saveButton->setIcon(QPixmap(theme + "icons/Save-96.png"));
    ui->saveAsButton->setIcon(QPixmap(theme + "icons/Save as-96.png"));
    ui->searchWidget->setVisible(false);
    ui->codeEdit->setTabReplaceSize(4);

    connect(ui->codeEdit, &QCodeEditor::saveTriggered, [this]() {
        on_saveButton_clicked();
    });
    connect(ui->codeEdit, &QCodeEditor::searchTriggered, [this]() {
        ui->searchWidget->setVisible(true);
        auto selected = ui->codeEdit->textCursor().selectedText();
        if (!selected.isEmpty()) {
            ui->searchEdit->setText(selected);
        }
        ui->codeEdit->searchForString(ui->searchEdit->text());
        ui->searchEdit->setFocus();
    });
}

ScriptEditor::~ScriptEditor()
{
    delete ui;
}

QCodeEditor *ScriptEditor::codeEditor()
{
    return ui->codeEdit;
}

QString ScriptEditor::fileNow()
{
    return ui->fileNowLabel->text();
}

void ScriptEditor::setFileNow(QString fileName)
{
    ui->fileNowLabel->setText(fileName);
    emit fileNameChanged(fileName);
}

void ScriptEditor::setModeQml()
{
    ui->codeEdit->setHighlighter(new QmlHighlighter);
    ui->codeEdit->setCompleter(new QVescCompleter);
    mIsModeLisp = false;
}

void ScriptEditor::setModeLisp()
{
    ui->codeEdit->setHighlighter(new LispHighlighter);
    ui->codeEdit->setCompleter(new QLispCompleter);
    ui->codeEdit->setCommentStr(";");
    ui->codeEdit->setIndentStrs("(", ")");
    ui->codeEdit->setAutoParentheses(false);
    ui->codeEdit->setSeparateMinus(false);
    mIsModeLisp = true;
}

void ScriptEditor::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        if (ui->searchEdit->hasFocus() || ui->replaceEdit->hasFocus()) {
            on_searchHideButton_clicked();
        }
    }
}

void ScriptEditor::on_openFileButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open %1 File").arg(mIsModeLisp ? "Lisp" : "Qml"), ui->fileNowLabel->text(),
                                                    mIsModeLisp ? tr("Lisp files (*.lisp)") : tr("QML files (*.qml)"));

    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::critical(this, tr("Open %1 File").arg(mIsModeLisp ? "Lisp" : "Qml"),
                                  "Could not open\n" + fileName + "\nfor reading");
            return;
        }

        ui->codeEdit->setPlainText(file.readAll());
        ui->fileNowLabel->setText(fileName);
        emit fileNameChanged(fileName);

        emit fileOpened(fileName);

        file.close();
    }
}

void ScriptEditor::on_saveButton_clicked()
{
    QString fileName = ui->fileNowLabel->text();

    QFileInfo fi(fileName);
    if (!fi.exists()) {
        QMessageBox::critical(this, "Save File",
                              "Current file path not valid. Use save as instead.");
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, tr("Save %1 File").arg(mIsModeLisp ? "Lisp" : "Qml"),
                              "Could not open\n" + fileName + "\nfor writing");
        return;
    }

    file.write(ui->codeEdit->toPlainText().toUtf8());
    file.close();

    emit fileSaved(fileName);
}

void ScriptEditor::on_saveAsButton_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("Save %1").arg(mIsModeLisp ? "Lisp" : "Qml"), ui->fileNowLabel->text(),
                                                    mIsModeLisp ? tr("Lisp files (*.lisp)") : tr("QML files (*.qml)"));

    QString ending = mIsModeLisp ? ".lisp" : ".qml";

    if (!fileName.isEmpty()) {
        if (!fileName.toLower().endsWith(ending)) {
            fileName.append(ending);
        }

        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly)) {
            QMessageBox::critical(this, tr("Save %1 File").arg(mIsModeLisp ? "Lisp" : "Qml"),
                                  "Could not open\n" + fileName + "\nfor writing");
            return;
        }

        file.write(ui->codeEdit->toPlainText().toUtf8());
        file.close();

        ui->fileNowLabel->setText(fileName);
        emit fileNameChanged(fileName);

        emit fileSaved(fileName);
    }
}

void ScriptEditor::on_searchEdit_textChanged(const QString &arg1)
{
    ui->codeEdit->searchForString(arg1);
}

void ScriptEditor::on_searchPrevButton_clicked()
{
    ui->codeEdit->searchPreviousResult();
    ui->codeEdit->setFocus();
}

void ScriptEditor::on_searchNextButton_clicked()
{
    ui->codeEdit->searchNextResult();
    ui->codeEdit->setFocus();
}

void ScriptEditor::on_replaceThisButton_clicked()
{
    if (ui->codeEdit->textCursor().selectedText() == ui->searchEdit->text()) {
        ui->codeEdit->textCursor().insertText(ui->replaceEdit->text());
        ui->codeEdit->searchNextResult();
    }
}

void ScriptEditor::on_replaceAllButton_clicked()
{
    ui->codeEdit->searchNextResult();
    while (!ui->codeEdit->textCursor().selectedText().isEmpty()) {
        ui->codeEdit->textCursor().insertText(ui->replaceEdit->text());
        ui->codeEdit->searchNextResult();
    }
}

void ScriptEditor::on_searchHideButton_clicked()
{
    ui->searchWidget->setVisible(false);
    ui->codeEdit->searchForString("");
    ui->codeEdit->setFocus();
}

void ScriptEditor::on_searchCaseSensitiveBox_toggled(bool checked)
{
    ui->codeEdit->searchSetCaseSensitive(checked);
}
