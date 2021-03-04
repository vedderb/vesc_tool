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

#include "pagescripting.h"
#include "ui_pagescripting.h"
#include "widgets/helpdialog.h"

#include <QQmlEngine>
#include <QQmlContext>
#include <QSettings>
#include <QmlHighlighter>
#include <QDateTime>
#include <QFileDialog>
#include <QMessageBox>
#include <QDirIterator>
#include <QVescCompleter>

PageScripting::PageScripting(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PageScripting)
{
    ui->setupUi(this);
    mVesc = nullptr;
    ui->qmlWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);

    makeEditorConnections(ui->mainEdit);

    QPushButton *plusButton = new QPushButton();
    plusButton->setIcon(QIcon("://res/icons/Plus Math-96.png"));
    plusButton->setFlat(true);
    plusButton->setText("New Tab");
    ui->fileTabs->setCornerWidget(plusButton);
    connect(plusButton, &QPushButton::clicked, [this]() {
        createEditorTab("", "");
    });

    connect(ui->mainEdit, &QmlEditor::fileNameChanged, [this](QString fileName) {
        QFileInfo f(fileName);
        QString txt = "main";

        if (f.exists()) {
            txt = f.fileName() + " (main)";
            mDirNow = f.path();
        } else {
            mDirNow = "";
        }

        ui->fileTabs->setTabText(0, txt);
    });

    QSettings set;
    {
        int size = set.beginReadArray("pagescripting/recentfiles");
        for (int i = 0; i < size; ++i) {
            set.setArrayIndex(i);
            QString path = set.value("path").toString();
            QFileInfo f(path);
            if (f.exists()) {
                mRecentFiles.append(path);
            }
        }
        set.endArray();
    }
    {
        int size = set.beginReadArray("pagescripting/recentopenfiles");
        for (int i = 0; i < size; ++i) {
            set.setArrayIndex(i);
            QString path = set.value("path").toString();
            QFileInfo f(path);
            if (f.exists()) {
                QFile file(path);
                if (file.open(QIODevice::ReadOnly)) {
                    if (ui->mainEdit->editor()->toPlainText().isEmpty()) {
                        ui->mainEdit->editor()->setPlainText(file.readAll());
                        ui->mainEdit->setFileNow(path);
                    } else {
                        createEditorTab(path, file.readAll());
                    }
                }
            }
        }
        set.endArray();
        ui->fileTabs->setCurrentIndex(0);
    }

    updateRecentList();

    // Load examples
    QDirIterator it("://res/qml/Examples/");
    while (it.hasNext()) {
        QFileInfo fi(it.next());
        QListWidgetItem *item = new QListWidgetItem;
        item->setText(it.fileName());
        item->setData(Qt::UserRole, it.filePath());
        ui->exampleList->addItem(item);
    }

    // Add close button that clears the main editor
    QPushButton *closeButton = new QPushButton();
    closeButton->setIcon(QIcon("://res/icons/Delete-96.png"));
    closeButton->setFlat(true);
    ui->fileTabs->tabBar()->setTabButton(0, QTabBar::RightSide, closeButton);

    auto editor = qobject_cast<QmlEditor*>(ui->fileTabs->widget(0));
    connect(closeButton, &QPushButton::clicked, [editor]() {
        editor->editor()->clear();
        editor->setFileNow("");
    });
}

PageScripting::~PageScripting()
{
    QSettings set;
    {
        set.remove("pagescripting/recentfiles");
        set.beginWriteArray("pagescripting/recentfiles");
        int ind = 0;
        for (auto f: mRecentFiles) {
            set.setArrayIndex(ind);
            set.setValue("path", f);
            ind++;
        }
        set.endArray();
    }
    {
        set.remove("pagescripting/recentopenfiles");
        set.beginWriteArray("pagescripting/recentopenfiles");
        for (int i = 0;i < ui->fileTabs->count();i++) {
            auto e = qobject_cast<QmlEditor*>(ui->fileTabs->widget(i));
            set.setArrayIndex(i);
            set.setValue("path", e->fileNow());
        }
        set.endArray();
    }

    delete ui;
}

VescInterface *PageScripting::vesc() const
{
    return mVesc;
}

void PageScripting::setVesc(VescInterface *vesc)
{
    mVesc = vesc;

    ui->qmlWidget->engine()->rootContext()->setContextProperty("VescIf", mVesc);
    ui->qmlWidget->engine()->rootContext()->setContextProperty("QmlUi", this);
}

void PageScripting::reloadParams()
{

}

void PageScripting::debugMsgRx(QtMsgType type, const QString msg)
{
    QString str;

    if (type == QtWarningMsg || type == QtCriticalMsg || type == QtFatalMsg) {
        str = "<font color=\"red\">" + msg + "</font><br>";
    } else {
        str = msg + "<br>";
    }

    ui->debugEdit->moveCursor(QTextCursor::End);
    ui->debugEdit->insertHtml("<font color=\"blue\">" +
                              QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss: ") +
                              "</font>" + str);
    ui->debugEdit->moveCursor(QTextCursor::End);
}

void PageScripting::on_runButton_clicked()
{
    ui->qmlWidget->setSource(QUrl(QLatin1String("qrc:/res/qml/DynamicLoader.qml")));
    ui->qmlWidget->engine()->clearComponentCache();
    emit reloadQml(qmlToRun());
}

void PageScripting::on_stopButton_clicked()
{
    ui->qmlWidget->setSource(QUrl(QLatin1String("")));
    mQmlUi.stopCustomGui();
}

void PageScripting::on_runWindowButton_clicked()
{
    ui->runWindowButton->setEnabled(false);

    if (!mQmlUi.isCustomGuiRunning()) {
        mQmlUi.startCustomGui(mVesc);
    } else {
        mQmlUi.clearQmlCache();
    }

    QTimer::singleShot(10, [this]() {
        mQmlUi.emitReloadCustomGui("qrc:/res/qml/DynamicLoader.qml");
    });

    QTimer::singleShot(1000, [this]() {
        mQmlUi.emitReloadQml(qmlToRun());
        ui->runWindowButton->setEnabled(true);
    });
}

void PageScripting::on_fullscreenButton_clicked()
{
    mQmlUi.emitToggleFullscreen();
}

void PageScripting::on_openRecentButton_clicked()
{
    auto *item = ui->recentList->currentItem();

    if (item) {
        QString fileName = item->text();
        QFile file(fileName);

        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::critical(this, "Open QML File",
                                  "Could not open\n " + fileName + " for reading");
            return;
        }

        if (ui->mainEdit->editor()->toPlainText().isEmpty()) {
            ui->mainEdit->editor()->setPlainText(file.readAll());
            ui->mainEdit->setFileNow(fileName);
        } else {
            createEditorTab(fileName, file.readAll());
        }

        file.close();
    } else {
        QMessageBox::critical(this, "Open Recent",
                              "Please select a file.");
    }
}

void PageScripting::on_removeSelectedButton_clicked()
{
    auto *item = ui->recentList->currentItem();

    if (item) {
        QString fileName = item->text();
        mRecentFiles.removeOne(fileName);
        updateRecentList();
    }
}

void PageScripting::on_clearRecentButton_clicked()
{
    mRecentFiles.clear();
    updateRecentList();
}

void PageScripting::on_openExampleButton_clicked()
{
    auto *item = ui->exampleList->currentItem();

    if (item) {
        QFile file(item->data(Qt::UserRole).toString());

        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::critical(this, "Open QML File",
                                  "Could not open example for reading");
            return;
        }

        if (ui->mainEdit->editor()->toPlainText().isEmpty()) {
            ui->mainEdit->editor()->setPlainText(file.readAll());
            ui->mainEdit->setFileNow("");
        } else {
            createEditorTab("", file.readAll());
        }

        file.close();
    } else {
        QMessageBox::critical(this, "Open Example",
                              "Please select one example.");
    }
}

void PageScripting::updateRecentList()
{
    ui->recentList->clear();
    for (auto f: mRecentFiles) {
        ui->recentList->addItem(f);
    }
}

void PageScripting::makeEditorConnections(QmlEditor *editor)
{
    connect(editor->editor(), &QCodeEditor::runEmbeddedTriggered, [this]() {
        on_runButton_clicked();
    });
    connect(editor->editor(), &QCodeEditor::runWindowTriggered, [this]() {
        on_runWindowButton_clicked();
    });
    connect(editor->editor(), &QCodeEditor::stopTriggered, [this]() {
        on_stopButton_clicked();
    });
    connect(editor->editor(), &QCodeEditor::clearConsoleTriggered, [this]() {
        ui->debugEdit->clear();
    });
    connect(editor, &QmlEditor::fileOpened, [this](QString fileName) {
        if (!mRecentFiles.contains(fileName)) {
            mRecentFiles.append(fileName);
            updateRecentList();
        }
    });
    connect(editor, &QmlEditor::fileSaved, [this](QString fileName) {
        if (mVesc) {
            mVesc->emitStatusMessage("Saved " + fileName, true);
        }

        if (!mRecentFiles.contains(fileName)) {
            mRecentFiles.append(fileName);
            updateRecentList();
        }
    });
}

void PageScripting::createEditorTab(QString fileName, QString content)
{
    QmlEditor *editor = new QmlEditor();
    int tabIndex = ui->fileTabs->addTab(editor, "");
    ui->fileTabs->setCurrentIndex(tabIndex);

    makeEditorConnections(editor);

    connect(editor, &QmlEditor::fileNameChanged, [this, editor](QString fileName) {
        QFileInfo f(fileName);
        QString txt = "Unnamed";

        if (f.exists()) {
            txt = f.fileName();
        }

        ui->fileTabs->setTabText(ui->fileTabs->indexOf(editor), txt);
    });

    editor->setFileNow(fileName);
    editor->editor()->setPlainText(content);

    QPushButton *closeButton = new QPushButton();
    closeButton->setIcon(QIcon("://res/icons/Cancel-96.png"));
    closeButton->setFlat(true);
    ui->fileTabs->tabBar()->setTabButton(tabIndex, QTabBar::RightSide, closeButton);

    connect(closeButton, &QPushButton::clicked, [this, editor]() {
        ui->fileTabs->removeTab(ui->fileTabs->indexOf(editor));
    });
}

QString PageScripting::qmlToRun()
{
    QString res = ui->mainEdit->editor()->toPlainText();
    res.prepend("import \"qrc:/mobile\";");

    QFileInfo f(mDirNow);
    if (f.exists() && f.isDir()) {
        res.prepend("import \"file:/" + mDirNow + "\";");
    }
    return res;
}

void PageScripting::on_helpButton_clicked()
{
    QString html = "<b>Keyboard Commands</b><br>"
                   "Ctrl + '+'   : Increase font size<br>"
                   "Ctrl + '-'   : Decrease font size<br>"
                   "Ctrl + space : Show auto-complete suggestions<br>"
                   "Ctrl + '/'   : Toggle auto-comment on line or block<br>"
                   "Ctrl + 'i'   : Auto-indent selected line or block<br>"
                   "Ctrl + 'f'   : Open search (and replace) bar<br>"
                   "Ctrl + 'e'   : Run or restart embedded<br>"
                   "Ctrl + 'w'   : Run or restart window<br>"
                   "Ctrl + 'q'   : Stop code<br>"
                   "Ctrl + 'd'   : Clear console<br>"
                   "Ctrl + 's'   : Save file<br>";

    HelpDialog::showHelpMonospace(this, "VESC Tool Script Editor", html.replace(" ","&nbsp;"));
}
