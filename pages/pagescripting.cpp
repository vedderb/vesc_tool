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
#include "packet.h"

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

    makeEditorConnections(ui->mainEdit);

    QPushButton *plusButton = new QPushButton();
    QString theme = Utility::getThemePath();
    plusButton->setIcon(QIcon(theme +"icons/Plus Math-96.png"));
    ui->runButton->setIcon(QIcon(theme +"icons/Circled Play-96.png"));
    ui->runWindowButton->setIcon(QIcon(theme +"icons/Circled Play-96.png"));
    ui->fullscreenButton->setIcon(QIcon(theme +"icons/size_off.png"));
    ui->stopButton->setIcon(QIcon(theme +"icons/Shutdown-96.png"));
    ui->helpButton->setIcon(QIcon(theme +"icons/Help-96.png"));
    ui->clearConsoleButton->setIcon(QIcon(theme +"icons/Delete-96.png"));
    ui->openRecentButton->setIcon(QIcon(theme +"icons/Open Folder-96.png"));
    ui->removeSelectedButton->setIcon(QIcon(theme +"icons/Delete-96.png"));
    ui->clearRecentButton->setIcon(QIcon(theme +"icons/Delete-96.png"));
    ui->openExampleButton->setIcon(QIcon(theme +"icons/Open Folder-96.png"));
    ui->exportCArrayAppButton->setIcon(QIcon(theme +"icons/Save as-96.png"));
    ui->exportCArrayHwButton->setIcon(QIcon(theme +"icons/Save as-96.png"));
    ui->calcSizeButton->setIcon(QIcon(theme +"icons/Calculator-96.png"));
    ui->openQmluiAppButton->setIcon(QIcon(theme +"icons/Open Folder-96.png"));
    ui->openQmluiHwButton->setIcon(QIcon(theme +"icons/Open Folder-96.png"));
    ui->uploadButton->setIcon(QIcon(theme +"icons/Download-96.png"));
    ui->eraseOnlyButton->setIcon(QIcon(theme +"icons/Delete-96.png"));
    ui->clearUploadTextButton->setIcon(QIcon(theme +"icons/Delete-96.png"));

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
    for (auto fi: QDir("://res/qml/Examples/").entryInfoList(QDir::NoFilter, QDir::Name)) {
        QListWidgetItem *item = new QListWidgetItem;
        item->setText(fi.fileName());
        item->setData(Qt::UserRole, fi.filePath());
        ui->exampleList->addItem(item);
    }

    // Add close button that clears the main editor
    QPushButton *closeButton = new QPushButton();
    closeButton->setIcon(QIcon(theme +"icons/Delete-96.png"));
    closeButton->setFlat(true);
    ui->fileTabs->tabBar()->setTabButton(0, QTabBar::RightSide, closeButton);

    auto editor = qobject_cast<QmlEditor*>(ui->fileTabs->widget(0));
    connect(closeButton, &QPushButton::clicked, [editor]() {
        editor->editor()->clear();
        editor->setFileNow("");
    });

    ui->splitter_2->setSizes(QList<int>({1000, 600}));
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
    ui->qmlWidget->engine()->rootContext()->setContextProperty("Utility", &mUtil);
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
void PageScripting::on_recentList_doubleClicked()
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
    auto res = QMessageBox::question(this, "Clear Recent Files",
                                     "This will clear the list with recent files. Are you sure?");

    if (res == QMessageBox::Yes) {
        mRecentFiles.clear();
        updateRecentList();
    }
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

void PageScripting::on_exampleList_doubleClicked()
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
        } else if (!fileName.isEmpty()) {
            txt = fileName;
        }

        ui->fileTabs->setTabText(ui->fileTabs->indexOf(editor), txt);
    });

    editor->setFileNow(fileName);
    editor->editor()->setPlainText(content);
    QString theme = Utility::getThemePath();

    QPushButton *closeButton = new QPushButton();
    closeButton->setIcon(QIcon(theme +"icons/Cancel-96.png"));
    closeButton->setFlat(true);
    ui->fileTabs->tabBar()->setTabButton(tabIndex, QTabBar::RightSide, closeButton);

    connect(closeButton, &QPushButton::clicked, [this, editor]() {
        ui->fileTabs->removeTab(ui->fileTabs->indexOf(editor));
    });
}

QString PageScripting::qmlToRun(bool importDir)
{
    QString res = ui->mainEdit->editor()->toPlainText();
    res.prepend("import \"qrc:/mobile\";");
    res.prepend("import Vedder.vesc.vescinterface 1.0;");

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

    auto editor = qobject_cast<QmlEditor*>(ui->fileTabs->widget(0));
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

bool PageScripting::eraseQml()
{
    if (!mVesc) {
        return false;
    }

    if (!mVesc->isPortConnected()) {
        ui->uploadTextEdit->appendPlainText("Not connected");
        return false;
    }

    auto waitEraseRes = [this]() {
        int res = -10;

        QEventLoop loop;
        QTimer timeoutTimer;
        timeoutTimer.setSingleShot(true);
        timeoutTimer.start(6000);
        auto conn = connect(mVesc->commands(), &Commands::eraseQmluiResReceived,
                            [&res,&loop](bool erRes) {
            res = erRes ? 1 : -1;
            loop.quit();
        });

        connect(&timeoutTimer, SIGNAL(timeout()), &loop, SLOT(quit()));
        loop.exec();

        disconnect(conn);
        return res;
    };

    mVesc->commands()->qmlUiErase();

    ui->uploadTextEdit->appendPlainText("Erasing QMLUI...");

    int erRes = waitEraseRes();
    if (erRes != 1) {
        QString msg = "Unknown failure";

        if (erRes == -10) {
            msg = "Erase timed out";
        } else if (erRes == -1) {
            msg = "Erasing QMLUI failed";
        }

        ui->uploadTextEdit->appendPlainText(msg);
        return false;
    }

    ui->uploadTextEdit->appendPlainText("Erase OK!");
    return true;
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
        if (ui->mainEdit->editor()->toPlainText().isEmpty()) {
            ui->mainEdit->editor()->setPlainText(mVesc->qmlHw());
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
        if (ui->mainEdit->editor()->toPlainText().isEmpty()) {
            ui->mainEdit->editor()->setPlainText(mVesc->qmlApp());
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

    if (!eraseQml()) {
        ui->uploadButton->setEnabled(true);
        ui->eraseOnlyButton->setEnabled(true);
        return;
    }

    auto waitWriteRes = [this]() {
        int res = -10;

        QEventLoop loop;
        QTimer timeoutTimer;
        timeoutTimer.setSingleShot(true);
        timeoutTimer.start(1000);
        auto conn = connect(mVesc->commands(), &Commands::writeQmluiResReceived,
                            [&res,&loop](bool erRes, quint32 offset) {
            (void)offset;
            res = erRes ? 1 : -1;
            loop.quit();
        });

        connect(&timeoutTimer, SIGNAL(timeout()), &loop, SLOT(quit()));
        loop.exec();

        disconnect(conn);
        return res;
    };

    VByteArray vb;
    vb.vbAppendUint16(ui->uploadFullscreenBox->isChecked() ? 2 : 1);
    vb.append(qCompress(qmlToRun(false).toUtf8(), 9));
    quint16 crc = Packet::crc16((const unsigned char*)vb.constData(),
                                uint32_t(vb.size()));
    VByteArray data;
    data.vbAppendUint32(vb.size() - 2);
    data.vbAppendUint16(crc);
    data.append(vb);

    if (data.size() > (1024 * 120)) {
        ui->uploadTextEdit->appendPlainText("Not enough space");
        ui->uploadButton->setEnabled(true);
        ui->eraseOnlyButton->setEnabled(true);
        return;
    }

    ui->uploadTextEdit->appendPlainText("Writing data...");

    quint32 offset = 0;
    bool ok = true;
    while (data.size() > 0) {
        const int chunkSize = 384;
        int sz = data.size() > chunkSize ? chunkSize : data.size();

        mVesc->commands()->qmlUiWrite(data.mid(0, sz), offset);
        if (!waitWriteRes()) {
            ui->uploadTextEdit->appendPlainText("Write failed");
            ok = false;
            break;
        }

        offset += sz;
        data.remove(0, sz);
    }

    if (ok) {
        ui->uploadTextEdit->appendPlainText("Write OK!");
    }

    ui->uploadButton->setEnabled(true);
    ui->eraseOnlyButton->setEnabled(true);
}

void PageScripting::on_eraseOnlyButton_clicked()
{
    ui->eraseOnlyButton->setEnabled(false);
    eraseQml();
    ui->eraseOnlyButton->setEnabled(true);
}

void PageScripting::on_calcSizeButton_clicked()
{
    QMessageBox::information(this, "QML Size",
                             QString("Compressed QML size: %1").
                             arg(qCompress(qmlToRun(false).toUtf8(), 9).size()));
}
