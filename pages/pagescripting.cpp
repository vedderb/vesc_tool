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
#include <QDir>
#include <QVescCompleter>

PageScripting::PageScripting(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PageScripting)
{
    ui->setupUi(this);
    mVesc = nullptr;
    ui->qmlWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    ui->qmlWidget->setClearColor(Utility::getAppQColor("normalBackground"));

    ui->mainEdit->setModeQml();
    makeEditorConnections(ui->mainEdit);

    QPushButton *plusButton = new QPushButton();
    plusButton->setIcon(Utility::getIcon("icons/Plus Math-96.png"));
    ui->runButton->setIcon(Utility::getIcon("icons/Circled Play-96.png"));
    ui->runWindowButton->setIcon(Utility::getIcon("icons/Circled Play-96.png"));
    ui->fullscreenButton->setIcon(Utility::getIcon("icons/size_off.png"));
    ui->stopButton->setIcon(Utility::getIcon("icons/Shutdown-96.png"));
    ui->helpButton->setIcon(Utility::getIcon("icons/Help-96.png"));
    ui->clearConsoleButton->setIcon(Utility::getIcon("icons/Delete-96.png"));
    ui->openRecentButton->setIcon(Utility::getIcon("icons/Open Folder-96.png"));
    ui->removeSelectedButton->setIcon(Utility::getIcon("icons/Delete-96.png"));
    ui->clearRecentButton->setIcon(Utility::getIcon("icons/Delete-96.png"));
    ui->openExampleButton->setIcon(Utility::getIcon("icons/Open Folder-96.png"));
    ui->exportCArrayAppButton->setIcon(Utility::getIcon("icons/Save as-96.png"));
    ui->exportCArrayHwButton->setIcon(Utility::getIcon("icons/Save as-96.png"));
    ui->calcSizeButton->setIcon(Utility::getIcon("icons/Calculator-96.png"));
    ui->openQmluiAppButton->setIcon(Utility::getIcon("icons/Open Folder-96.png"));
    ui->openQmluiHwButton->setIcon(Utility::getIcon("icons/Open Folder-96.png"));
    ui->uploadButton->setIcon(Utility::getIcon("icons/Download-96.png"));
    ui->eraseOnlyButton->setIcon(Utility::getIcon("icons/Delete-96.png"));
    ui->clearUploadTextButton->setIcon(Utility::getIcon("icons/Delete-96.png"));
    ui->recentFilterClearButton->setIcon(Utility::getIcon("icons/Cancel-96.png"));
    ui->exampleFilterClearButton->setIcon(Utility::getIcon("icons/Cancel-96.png"));

    plusButton->setFlat(true);
    plusButton->setText("New Tab");
    ui->fileTabs->setCornerWidget(plusButton);
    connect(plusButton, &QPushButton::clicked, [this]() {
        createEditorTab("", "");
    });

    connect(ui->mainEdit, &ScriptEditor::fileNameChanged, [this](QString fileName) {
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
                    if (ui->mainEdit->codeEditor()->toPlainText().isEmpty()) {
                        ui->mainEdit->codeEditor()->setPlainText(file.readAll());
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
    for (auto fi: QDir("://res/qml/Examples/").entryInfoList(QDir::NoFilter, QDir::Name)) {
        QListWidgetItem *item = new QListWidgetItem;
        item->setText(fi.fileName());
        item->setData(Qt::UserRole, fi.filePath());
        ui->exampleList->addItem(item);
    }

    // Add close button that clears the main editor
    QPushButton *closeButton = new QPushButton();
    closeButton->setIcon(Utility::getIcon("icons/Delete-96.png"));
    closeButton->setFlat(true);
    ui->fileTabs->tabBar()->setTabButton(0, QTabBar::RightSide, closeButton);

    ScriptEditor *mainQmlEditor = qobject_cast<ScriptEditor*>(ui->fileTabs->widget(0));
    connect(closeButton, &QPushButton::clicked, [this, mainQmlEditor]() {
        // Clear main tab
        removeEditor(mainQmlEditor);
    });

    ui->splitter_2->setSizes(QList<int>({1000, 600}));

    // Clear debug edit from messages that appeared from loading other qml-components
    QTimer::singleShot(2000, [this]() {
        ui->debugEdit->clear();
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
            auto e = qobject_cast<ScriptEditor*>(ui->fileTabs->widget(i));
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
    mLoader.setVesc(vesc);

    ui->qmlWidget->engine()->rootContext()->setContextProperty("VescIf", mVesc);
    ui->qmlWidget->engine()->rootContext()->setContextProperty("QmlUi", this);
    ui->qmlWidget->engine()->rootContext()->setContextProperty("Utility", &mUtil);
}

void PageScripting::reloadParams()
{

}

bool PageScripting::hasUnsavedTabs()
{
    for (int i = 0; i < ui->fileTabs->count(); i++) {
        auto e = qobject_cast<ScriptEditor*>(ui->fileTabs->widget(i));
        if (e->hasUnsavedContent()) {
            return true;
        }
    }

    return false;
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
    ui->debugEdit->insertHtml("<font color=\"#4d7fc4\">" +
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

void PageScripting::openRecentList()
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

        if (ui->mainEdit->codeEditor()->toPlainText().isEmpty()) {
            ui->mainEdit->codeEditor()->setPlainText(file.readAll());
            ui->mainEdit->setFileNow(fileName);
        } else {
            createEditorTab(fileName, file.readAll());
        }

        mRecentFiles.removeAll(fileName);
        mRecentFiles.append(fileName);
        updateRecentList();
        ui->recentList->setCurrentRow(ui->recentList->count() - 1);

        file.close();
    } else {
        QMessageBox::critical(this, "Open Recent",
                              "Please select a file.");
    }
}

void PageScripting::on_openRecentButton_clicked()
{
   openRecentList();
}

// Recent add list
void PageScripting::on_recentList_doubleClicked()
{
   openRecentList();
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
    auto res = QMessageBox::question(this, "Clear Recent Files",
                                     "This will clear the list with recent files. Are you sure?");

    if (res == QMessageBox::Yes) {
        mRecentFiles.clear();
        updateRecentList();
    }
}



void PageScripting::openExample()
{
    QListWidgetItem *item = ui->exampleList->currentItem();

    if (item) {
        QFile file(item->data(Qt::UserRole).toString());

        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::critical(this, "Open QML File",
                                  "Could not open example for reading");
            return;
        }

        if (ui->mainEdit->codeEditor()->toPlainText().isEmpty()) {
            ui->mainEdit->codeEditor()->setPlainText(file.readAll());
            ui->mainEdit->setFileNow("");

            setEditorClean(ui->mainEdit);
        } else {
            createEditorTab("", file.readAll());
        }

        file.close();
    } else {
        QMessageBox::critical(this, "Open Example",
                              "Please select one example.");
    }
}

void PageScripting::on_openExampleButton_clicked()
{
    openExample();
}
void PageScripting::on_exampleList_doubleClicked()
{
    openExample();
}

void PageScripting::updateRecentList()
{
    ui->recentList->clear();
    for (auto f: mRecentFiles) {
        ui->recentList->addItem(f);
    }

    on_recentFilterEdit_textChanged(ui->recentFilterEdit->text());
}

void PageScripting::makeEditorConnections(ScriptEditor *editor)
{
    connect(editor->codeEditor(), &QCodeEditor::textChanged, [editor, this]() {
       setEditorDirty(editor);
    });
    connect(editor->codeEditor(), &QCodeEditor::runEmbeddedTriggered, [this]() {
        on_runButton_clicked();
    });
    connect(editor->codeEditor(), &QCodeEditor::runWindowTriggered, [this]() {
        on_runWindowButton_clicked();
    });
    connect(editor->codeEditor(), &QCodeEditor::stopTriggered, [this]() {
        on_stopButton_clicked();
    });
    connect(editor->codeEditor(), &QCodeEditor::clearConsoleTriggered, [this]() {
        ui->debugEdit->clear();
    });
    connect(editor, &ScriptEditor::fileOpened, [this](QString fileName) {
        mRecentFiles.removeAll(fileName);
        mRecentFiles.append(fileName);
        updateRecentList();
    });
    connect(editor, &ScriptEditor::fileSaved, [editor, this](QString fileName) {
        if (mVesc) {
            mVesc->emitStatusMessage("Saved " + fileName, true);
        }

        mRecentFiles.removeAll(fileName);
        mRecentFiles.append(fileName);
        updateRecentList();

        setEditorClean(editor);
    });
}

void PageScripting::createEditorTab(QString fileName, QString content)
{
    ScriptEditor *editor = new ScriptEditor();
    editor->setModeQml();
    int tabIndex = ui->fileTabs->addTab(editor, "");
    ui->fileTabs->setCurrentIndex(tabIndex);

    makeEditorConnections(editor);

    connect(editor, &ScriptEditor::fileNameChanged, [this, editor](QString fileName) {
        QFileInfo f(fileName);
        QString txt = "Unnamed";

        if (f.exists()) {
            txt = f.fileName();
        } else if (!fileName.isEmpty()) {
            txt = fileName;
        }

        ui->fileTabs->setTabText(ui->fileTabs->indexOf(editor), txt);
    });

    editor->setFileNow(fileName);
    editor->codeEditor()->setPlainText(content);

    QPushButton *closeButton = new QPushButton();
    closeButton->setIcon(Utility::getIcon("icons/Cancel-96.png"));
    closeButton->setFlat(true);
    ui->fileTabs->tabBar()->setTabButton(tabIndex, QTabBar::RightSide, closeButton);

    connect(closeButton, &QPushButton::clicked, [this, editor]() {
       removeEditor(editor);
    });

    // Do this at the end to make sure to account for the changes from loading the initial text
    setEditorClean(editor);
}


/**
 * @brief PageScripting::removeEditor Removes the editor (unless it is the 0th, in which case it simply clears it)
 * @param qmlEditor
 */
void PageScripting::removeEditor(ScriptEditor *editor)
{
   bool shouldCloseTab = false;

   // Check if tab is dirty
   if (editor->hasUnsavedContent()) {
       // Ask user for confirmation
       QMessageBox::StandardButton answer = QMessageBox::question(
            this,
            tr("Delete the tab"),
            tr("Do you want to delete the tab contents?"),
            QMessageBox::Yes | QMessageBox::No
       );

       if (answer == QMessageBox::Yes) {
          shouldCloseTab = true;
       } else {
          shouldCloseTab = false;
       }

   } else {
       shouldCloseTab = true;
   }

   // Only close if appropriate
   if (shouldCloseTab) {
       // Get index for this tab
       int tabIdx = ui->fileTabs->indexOf(editor);

       // Special handling of tabIdx == 0
       if (tabIdx == 0) {
            editor->codeEditor()->clear();
            editor->setFileNow("");
       } else {
           ui->fileTabs->removeTab(tabIdx);
       }
   }
}


/**
 * @brief PageScripting::setEditorDirtySet the editor as dirty, i.e. having text modifications
 * @param qmlEditor
 */
void PageScripting::setEditorDirty(ScriptEditor *editor)
{
    // Check if the editor is not already dirty
    if (editor->isDirty == false) {
        // Set editor as dirty
        editor->isDirty = true;

        // Get editor index
        int tabIdx = ui->fileTabs->indexOf(editor);

        // Append a `*` to signify a dirty editor
        QString tabText = ui->fileTabs->tabText(tabIdx);
        ui->fileTabs->setTabText(tabIdx, tabText + "*");
    }
}


/**
 * @brief PageScripting::setEditorClean Set the editor as clean, i.e. having no text modifications
 * @param qmlEditor
 */
void PageScripting::setEditorClean(ScriptEditor *editor)
{
    // Check if the editor is not already clean
    if (editor->isDirty == true) {
        // Set editor as clean
        editor->isDirty = false;

        // Get editor index
        int tabIdx = ui->fileTabs->indexOf(editor);

        // Get the tab's label
        QString tabText = ui->fileTabs->tabText(tabIdx);

        // Check if the final character is a `*`, which indicated it was a dirty file
        if (tabText.back() == "*") {
            // Remove the terminal `*`
            tabText.chop(1);
            ui->fileTabs->setTabText(tabIdx, tabText);
        }
    }
}


QString PageScripting::qmlToRun(bool importDir, bool prependImports)
{
    QString res = ui->mainEdit->contentAsText();

    if (prependImports) {
        res.prepend("import \"qrc:/mobile\";");
        res.prepend("import Vedder.vesc.vescinterface 1.0;");
    }

    if (importDir) {
        QFileInfo f(mDirNow);
        if (f.exists() && f.isDir()) {
            res.prepend("import \"file:/" + mDirNow + "\";");
        }
    }

    return res;
}

bool PageScripting::exportCArray(QString name)
{
    QString filename;
    QString dir = ".";

    auto editor = qobject_cast<ScriptEditor*>(ui->fileTabs->widget(0));
    QFileInfo fileNow = QFileInfo(editor->fileNow());
    if (fileNow.exists()) {
        dir = fileNow.path();
    }

    filename = QFileDialog::getSaveFileName(this,
                                        tr("Choose the .c or .h file name. Both files will be created."),
                                        dir,
                                        tr("C Source/Header files (*.c *.h)"));

    if (filename.isNull()) {
        return false;
    }

    if (filename.toLower().endsWith(".c") || filename.toLower().endsWith(".h")) {
        filename.chop(2);
    }

    QString sourceFileName = filename + ".c";
    QString headerFileName = filename + ".h";

    QFile sourceFile(sourceFileName);
    if (!sourceFile.open(QIODevice::WriteOnly)) {
        qWarning() << tr("Could not open %1 for writing").arg(sourceFileName);
        return false;
    }

    QFile headerFile(headerFileName);
    if (!headerFile.open(QIODevice::WriteOnly)) {
        qWarning() << tr("Could not open %1 for writing").arg(headerFileName);
        return false;
    }

    QTextStream outSource(&sourceFile);
    QTextStream outHeader(&headerFile);
    QFileInfo headerInfo(headerFile);
    QString headerNameStr = headerInfo.fileName().toUpper().replace(".", "_") + "_";
    QString prefix = headerInfo.fileName();
    prefix.chop(2);

    QByteArray compressed = qCompress(qmlToRun(false).toUtf8(), 9);

    outHeader << "// This file is autogenerated by VESC Tool\n\n";

    outHeader << "#ifndef " + headerNameStr + "\n";
    outHeader << "#define " + headerNameStr + "\n\n";

    outHeader << "#include \"datatypes.h\"\n";
    outHeader << "#include <stdint.h>\n";
    outHeader << "#include <stdbool.h>\n\n";

    outHeader << "// Constants\n";
    outHeader << "#define DATA_" << name.toUpper() << "_SIZE\t\t" << compressed.size() << "\n\n";

    outHeader << "// Variables\n";
    outHeader << "extern uint8_t data_" << name.toLower() << "[];\n\n";

    outHeader << "// " + headerNameStr + "\n";
    outHeader << "#endif\n";

    outSource << "// This file is autogenerated by VESC Tool\n\n";
    outSource << "#include \"" << headerInfo.fileName() << "\"\n\n";
    outSource << "uint8_t data_" << name.toLower() << "[" << compressed.size() << "] = {\n\t";

    int posCnt = 0;
    for (auto b: compressed) {
        if (posCnt >= 16) {
            posCnt = 0;
            outSource << "\n\t";
        }

        outSource << QString("0x%1, ").arg(quint8(b), 2, 16, QLatin1Char('0'));
        posCnt++;
    }

    outSource << "\n};\n";

    outHeader.flush();
    outSource.flush();
    headerFile.close();
    sourceFile.close();

    return true;
}

bool PageScripting::eraseQml(int size, bool reload)
{
    if (!mVesc) {
        return false;
    }

    if (!mVesc->isPortConnected()) {
        ui->uploadTextEdit->appendPlainText("Not connected");
        return false;
    }

    ui->uploadTextEdit->appendPlainText("Erasing QMLUI...");
    bool res = mLoader.qmlErase(size);

    if (res) {
        if (reload) {
            mVesc->reloadFirmware();
        }
        ui->uploadTextEdit->appendPlainText("Erase OK!");
    } else {
        ui->uploadTextEdit->appendPlainText("Erasing QMLUI failed");
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

void PageScripting::on_exportCArrayHwButton_clicked()
{
    exportCArray("qml_hw");
}

void PageScripting::on_exportCArrayAppButton_clicked()
{
    exportCArray("qml_app");
}

void PageScripting::on_openQmluiHwButton_clicked()
{
    if (mVesc && mVesc->isPortConnected() && mVesc->qmlHwLoaded()) {
        if (ui->mainEdit->codeEditor()->toPlainText().isEmpty()) {
            ui->mainEdit->codeEditor()->setPlainText(mVesc->qmlHw());
            ui->fileTabs->setTabText(ui->fileTabs->indexOf(ui->mainEdit), "VESC Hw");
        } else {
            createEditorTab("VESC Hw", mVesc->qmlHw());
        }
    } else {
        QMessageBox::critical(this, "Open Qmlui HW",
                              "No HW qmlui loaded.");
    }
}

void PageScripting::on_openQmluiAppButton_clicked()
{
    if (mVesc && mVesc->isPortConnected() && mVesc->qmlAppLoaded()) {
        if (ui->mainEdit->codeEditor()->toPlainText().isEmpty()) {
            ui->mainEdit->codeEditor()->setPlainText(mVesc->qmlApp());
            ui->fileTabs->setTabText(ui->fileTabs->indexOf(ui->mainEdit), "VESC App");
        } else {
            createEditorTab("VESC App", mVesc->qmlApp());
        }
    } else {
        QMessageBox::critical(this, "Open Qmlui App",
                              "No App qmlui loaded.");
    }
}

void PageScripting::on_uploadButton_clicked()
{
    ui->uploadButton->setEnabled(false);
    ui->eraseOnlyButton->setEnabled(false);

    auto script = mLoader.qmlCompress(qmlToRun(false, false));

    if (!eraseQml(script.size() + 100, false)) {
        ui->uploadButton->setEnabled(true);
        ui->eraseOnlyButton->setEnabled(true);
        return;
    }

    ui->uploadTextEdit->appendPlainText("Writing data...");
    bool res = mLoader.qmlUpload(script, ui->uploadFullscreenBox->isChecked());

    if (res) {
        ui->uploadTextEdit->appendPlainText("Write OK!");
    } else {
        ui->uploadTextEdit->appendPlainText("Write failed");
    }

    mVesc->reloadFirmware();

    ui->uploadButton->setEnabled(true);
    ui->eraseOnlyButton->setEnabled(true);
}

void PageScripting::on_eraseOnlyButton_clicked()
{
    ui->eraseOnlyButton->setEnabled(false);
    eraseQml(16);
    ui->eraseOnlyButton->setEnabled(true);
}

void PageScripting::on_calcSizeButton_clicked()
{
    QMessageBox::information(this, "QML Size",
                             QString("Compressed QML size: %1").
                             arg(qCompress(qmlToRun(false).toUtf8(), 9).size()));
}

void PageScripting::on_recentFilterEdit_textChanged(const QString &filter)
{
    for (int row = 0; row < ui->recentList->count(); ++row) {
        if (filter.isEmpty()) {
            ui->recentList->item(row)->setHidden(false);
        } else {
            ui->recentList->item(row)->setHidden(!ui->recentList->item(row)->text().
                                                 contains(filter, Qt::CaseInsensitive));
        }
    }
}

void PageScripting::on_exampleFilterEdit_textChanged(const QString &filter)
{
    for (int row = 0; row < ui->exampleList->count(); ++row) {
        if (filter.isEmpty()) {
            ui->exampleList->item(row)->setHidden(false);
        } else {
            ui->exampleList->item(row)->setHidden(!ui->exampleList->item(row)->text().
                                                 contains(filter, Qt::CaseInsensitive));
        }
    }
}
