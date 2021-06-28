#include "qmleditor.h"
#include "ui_qmleditor.h"

#include <QmlHighlighter>
#include <QVescCompleter>
#include <QMessageBox>
#include <QFileDialog>
#include "utility.h"

QmlEditor::QmlEditor(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QmlEditor)
{
    ui->setupUi(this);

    QString theme = Utility::getThemePath();
    ui->searchHideButton->setIcon(QPixmap(theme + "icons/Cancel-96.png"));
    ui->openFileButton->setIcon(QPixmap(theme + "icons/Open Folder-96.png"));
    ui->saveButton->setIcon(QPixmap(theme + "icons/Save-96.png"));
    ui->saveAsButton->setIcon(QPixmap(theme + "icons/Save as-96.png"));
    ui->searchWidget->setVisible(false);

    ui->qmlEdit->setHighlighter(new QmlHighlighter);
    ui->qmlEdit->setCompleter(new QVescCompleter);
    ui->qmlEdit->setTabReplaceSize(4);

    connect(ui->qmlEdit, &QCodeEditor::saveTriggered, [this]() {
        on_saveButton_clicked();
    });
    connect(ui->qmlEdit, &QCodeEditor::searchTriggered, [this]() {
        ui->searchWidget->setVisible(true);
        auto selected = ui->qmlEdit->textCursor().selectedText();
        if (!selected.isEmpty()) {
            ui->searchEdit->setText(selected);
        }
        ui->qmlEdit->searchForString(ui->searchEdit->text());
        ui->searchEdit->setFocus();
    });
}

QmlEditor::~QmlEditor()
{
    delete ui;
}

QCodeEditor *QmlEditor::editor()
{
    return ui->qmlEdit;
}

QString QmlEditor::fileNow()
{
    return ui->fileNowLabel->text();
}

void QmlEditor::setFileNow(QString fileName)
{
    ui->fileNowLabel->setText(fileName);
    emit fileNameChanged(fileName);
}

void QmlEditor::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        if (ui->searchEdit->hasFocus() || ui->replaceEdit->hasFocus()) {
            on_searchHideButton_clicked();
        }
    }
}

void QmlEditor::on_openFileButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open QML File"), "",
                                                    tr("QML files (*.qml)"));

    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::critical(this, "Open QML File",
                                  "Could not open\n" + fileName + "\nfor reading");
            return;
        }

        ui->qmlEdit->setPlainText(file.readAll());
        ui->fileNowLabel->setText(fileName);
        emit fileNameChanged(fileName);

        emit fileOpened(fileName);

        file.close();
    }
}

void QmlEditor::on_saveButton_clicked()
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
        QMessageBox::critical(this, "Save QML File",
                              "Could not open\n" + fileName + "\nfor writing");
        return;
    }

    file.write(ui->qmlEdit->toPlainText().toUtf8());
    file.close();

    emit fileSaved(fileName);
}

void QmlEditor::on_saveAsButton_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("Save QML"), "",
                                                    tr("QML Files (*.qml)"));

    if (!fileName.isEmpty()) {
        if (!fileName.toLower().endsWith(".qml")) {
            fileName.append(".qml");
        }

        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly)) {
            QMessageBox::critical(this, "Save QML File",
                                  "Could not open\n" + fileName + "\nfor writing");
            return;
        }

        file.write(ui->qmlEdit->toPlainText().toUtf8());
        file.close();

        ui->fileNowLabel->setText(fileName);
        emit fileNameChanged(fileName);

        emit fileSaved(fileName);
    }
}

void QmlEditor::on_searchEdit_textChanged(const QString &arg1)
{
    ui->qmlEdit->searchForString(arg1);
}

void QmlEditor::on_searchPrevButton_clicked()
{
    ui->qmlEdit->searchPreviousResult();
    ui->qmlEdit->setFocus();
}

void QmlEditor::on_searchNextButton_clicked()
{
    ui->qmlEdit->searchNextResult();
    ui->qmlEdit->setFocus();
}

void QmlEditor::on_replaceThisButton_clicked()
{
    if (ui->qmlEdit->textCursor().selectedText() == ui->searchEdit->text()) {
        ui->qmlEdit->textCursor().insertText(ui->replaceEdit->text());
        ui->qmlEdit->searchNextResult();
    }
}

void QmlEditor::on_replaceAllButton_clicked()
{
    ui->qmlEdit->searchNextResult();
    while (!ui->qmlEdit->textCursor().selectedText().isEmpty()) {
        ui->qmlEdit->textCursor().insertText(ui->replaceEdit->text());
        ui->qmlEdit->searchNextResult();
    }
}

void QmlEditor::on_searchHideButton_clicked()
{
    ui->searchWidget->setVisible(false);
    ui->qmlEdit->searchForString("");
    ui->qmlEdit->setFocus();
}

void QmlEditor::on_searchCaseSensitiveBox_toggled(bool checked)
{
    ui->qmlEdit->searchSetCaseSensitive(checked);
}
