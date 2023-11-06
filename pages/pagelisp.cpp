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

#include <QDateTime>
#include <QMessageBox>
#include <QProgressDialog>
#include "pagelisp.h"
#include "ui_pagelisp.h"
#include "utility.h"
#include "widgets/helpdialog.h"

PageLisp::PageLisp(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PageLisp)
{
    ui->setupUi(this);
    mVesc = nullptr;

    Utility::setPlotColors(ui->bindingPlot);

    ui->mainEdit->setModeLisp();
    makeEditorConnections(ui->mainEdit);

    QPushButton *plusButton = new QPushButton();
    plusButton->setIcon(Utility::getIcon("icons/Plus Math-96.png"));
    ui->runButton->setIcon(Utility::getIcon("icons/Circled Play-96.png"));
    ui->stopButton->setIcon(Utility::getIcon("icons/Shutdown-96.png"));
    ui->helpButton->setIcon(Utility::getIcon("icons/Help-96.png"));
    ui->clearConsoleButton->setIcon(Utility::getIcon("icons/Delete-96.png"));
    ui->openRecentButton->setIcon(Utility::getIcon("icons/Open Folder-96.png"));
    ui->removeSelectedButton->setIcon(Utility::getIcon("icons/Delete-96.png"));
    ui->clearRecentButton->setIcon(Utility::getIcon("icons/Delete-96.png"));
    ui->openExampleButton->setIcon(Utility::getIcon("icons/Open Folder-96.png"));
    ui->uploadButton->setIcon(Utility::getIcon("icons/Download-96.png"));
    ui->readExistingButton->setIcon(Utility::getIcon("icons/Upload-96.png"));
    ui->eraseButton->setIcon(Utility::getIcon("icons/Delete-96.png"));
    ui->replHelpButton->setIcon(Utility::getIcon("icons/Help-96.png"));
    ui->streamButton->setIcon(Utility::getIcon("icons/Download-96.png"));
    ui->recentFilterClearButton->setIcon(Utility::getIcon("icons/Cancel-96.png"));
    ui->exampleFilterClearButton->setIcon(Utility::getIcon("icons/Cancel-96.png"));

    QIcon mycon = Utility::getIcon( "icons/expand_off.png");
    mycon.addPixmap(Utility::getIcon("icons/expand_on.png"), QIcon::Normal, QIcon::On);
    mycon.addPixmap(Utility::getIcon("icons/expand_off.png"), QIcon::Normal, QIcon::Off);
    ui->zoomHButton->setIcon(mycon);

    mycon = Utility::getIcon( "icons/expand_v_off.png");
    mycon.addPixmap(Utility::getIcon("icons/expand_v_on.png"), QIcon::Normal, QIcon::On);
    mycon.addPixmap(Utility::getIcon("icons/expand_v_off.png"), QIcon::Normal, QIcon::Off);
    ui->zoomVButton->setIcon(mycon);

    mycon = Utility::getIcon( "icons/size_off.png");
    mycon.addPixmap(Utility::getIcon("icons/size_on.png"), QIcon::Normal, QIcon::On);
    mycon.addPixmap(Utility::getIcon("icons/size_off.png"), QIcon::Normal, QIcon::Off);
    ui->autoscaleButton->setIcon(mycon);

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
        int size = set.beginReadArray("pagelisp/recentfiles");
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
        int size = set.beginReadArray("pagelisp/recentopenfiles");
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
    for (auto fi: QDir("://res/Lisp/Examples/").entryInfoList(QDir::NoFilter, QDir::Name)) {
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

    ScriptEditor *mainEditor = qobject_cast<ScriptEditor*>(ui->fileTabs->widget(0));
    connect(closeButton, &QPushButton::clicked, [this, mainEditor]() {
        // Clear main tab
        removeEditor(mainEditor);
    });

    ui->splitter->setSizes(QList<int>({1000, 100}));
    ui->splitter_2->setSizes(QList<int>({1000, 600}));

    mPollTimer.start(1000 / ui->pollRateBox->value());
    connect(&mPollTimer, &QTimer::timeout, [this]() {
        if (!ui->pollOffButton->isChecked()) {
            if (mVesc) {
                mVesc->commands()->lispGetStats(ui->pollAllButton->isChecked());
            }
        }
    });

    connect(ui->pollRateBox, QOverload<int>::of(&QSpinBox::valueChanged), [this](int val) {
        mPollTimer.setInterval(1000 / val);
    });

    auto updateZoom = [this]() {
        Qt::Orientations plotOrientations = Qt::Orientations(
                    ((ui->zoomHButton->isChecked() ? Qt::Horizontal : 0) |
                     (ui->zoomVButton->isChecked() ? Qt::Vertical : 0)));

        ui->bindingPlot->axisRect()->setRangeZoom(plotOrientations);
    };

    connect(ui->zoomHButton, &QPushButton::clicked, updateZoom);
    connect(ui->zoomVButton, &QPushButton::clicked, updateZoom);
    updateZoom();

    QFont legendFont = font();
    legendFont.setPointSize(9);
    ui->bindingPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    ui->bindingPlot->legend->setVisible(true);
    ui->bindingPlot->legend->setFont(legendFont);
    ui->bindingPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignRight|Qt::AlignBottom);
    ui->bindingPlot->xAxis->setLabel("Age (s)");
    ui->bindingPlot->yAxis->setLabel("Value");
    ui->bindingPlot->xAxis->setRangeReversed(true);
}

PageLisp::~PageLisp()
{
    QSettings set;
    {
        set.remove("pagelisp/recentfiles");
        set.beginWriteArray("pagelisp/recentfiles");
        int ind = 0;
        foreach (auto f, mRecentFiles) {
            set.setArrayIndex(ind);
            set.setValue("path", f);
            ind++;
        }
        set.endArray();
    }
    {
        set.remove("pagelisp/recentopenfiles");
        set.beginWriteArray("pagelisp/recentopenfiles");
        for (int i = 0;i < ui->fileTabs->count();i++) {
            auto e = qobject_cast<ScriptEditor*>(ui->fileTabs->widget(i));
            set.setArrayIndex(i);
            set.setValue("path", e->fileNow());
        }
        set.endArray();
    }

    delete ui;
}

VescInterface *PageLisp::vesc() const
{
    return mVesc;
}

void PageLisp::setVesc(VescInterface *vesc)
{
    mVesc = vesc;
    mLoader.setVesc(vesc);
    ui->experimentPlot->setVesc(vesc);

    connect(mVesc->commands(), &Commands::lispPrintReceived, [this](QString str) {
        ui->debugEdit->moveCursor(QTextCursor::End);
        ui->debugEdit->insertPlainText(str + "\n");
        ui->debugEdit->moveCursor(QTextCursor::End);

        int maxLines = 5000;
        int removeLines = 1000;

        if (ui->debugEdit->document()->lineCount() > maxLines) {
            QString txt = ui->debugEdit->toPlainText();
            auto lines = txt.split("\n");
            if (lines.length() >= removeLines) {
                QString shorter;
                for (int i = removeLines;i < lines.length();i++) {
                    shorter.append(lines.at(i));

                    if (i != (lines.length() - 1)) {
                        shorter.append("\n");
                    }
                }
                ui->debugEdit->setText(shorter);
                ui->debugEdit->moveCursor(QTextCursor::End);
            }
        }
    });

    connect(mVesc->commands(), &Commands::lispRunningResRx, [this](bool ok) {
        if (!ok) {
            mVesc->emitMessageDialog(tr("Start/Stop Lisp"),
                                     tr("Start/Stop Lisp failed. Did you forget to upload the code?"), false);
        }
    });

    connect(mVesc->commands(), &Commands::lispStatsRx, [this](LISP_STATS stats) {
        ui->cpuBar->setValue(stats.cpu_use * 10);
        ui->cpuBar->setFormat(QString("%1%").arg(stats.cpu_use, 0, 'f', 1));
        ui->heapBar->setValue(stats.heap_use);
        ui->memBar->setValue(stats.mem_use);

        ui->bindingTable->setColumnCount(2);
        ui->bindingTable->setRowCount(stats.number_bindings.size());

        QStringList selectedBindings;
        qint64 maxAgeMs = 5000;
        auto timeNow = QDateTime::currentMSecsSinceEpoch();

        int ind = 0;
        for (auto b: stats.number_bindings) {
            ui->bindingTable->setItem(ind, 0, new QTableWidgetItem(b.first));
            auto data = new QTableWidgetItem(QString::number(b.second, 'f'));
            data->setData(Qt::UserRole, b.second);
            ui->bindingTable->setItem(ind, 1, data);

            if (ui->bindingTable->item(ind, 1)->isSelected()) {
                selectedBindings.append(b.first);
                if (!mBindingData.contains(b.first)) {
                    mBindingData.insert(b.first, QVector<QPair<qint64, double>>());
                }

                mBindingData[b.first].append(qMakePair(timeNow, b.second));

                // Remove old data points
                while (!mBindingData[b.first].isEmpty() && mBindingData[b.first].at(0).first < (timeNow - maxAgeMs)) {
                    mBindingData[b.first].removeFirst();
                }
            }

            ind++;
        }

        // Remove bindings not selected
        for (auto it = mBindingData.begin(); it != mBindingData.end();) {
            if (!selectedBindings.contains(it.key())) {
                it = mBindingData.erase(it);
            } else {
                ++it;
            }
        }

        // Add graphs
        ui->bindingPlot->clearGraphs();
        int graphInd = 0;
        for (auto it = mBindingData.begin(); it != mBindingData.end();++it) {
            auto name = it.key();
            QVector<double> xAxis, yAxis;
            for (auto p: it.value()) {
                xAxis.append(double(timeNow - p.first) / 1000.0);
                yAxis.append(p.second);
            }

            ui->bindingPlot->addGraph();
            ui->bindingPlot->graph()->setPen(QPen(Utility::getAppQColor(QString("plot_graph%1").arg(graphInd + 1))));
            ui->bindingPlot->graph()->setName(name);
            ui->bindingPlot->graph()->setData(xAxis, yAxis);
            graphInd++;
        }

        if (ui->autoscaleButton->isChecked()) {
            ui->bindingPlot->rescaleAxes();
        }

        ui->bindingPlot->replotWhenVisible();
    });
}

void PageLisp::reloadParams()
{

}

bool PageLisp::hasUnsavedTabs()
{
    for (int i = 0; i < ui->fileTabs->count(); i++) {
        auto e = qobject_cast<ScriptEditor*>(ui->fileTabs->widget(i));
        if (e->hasUnsavedContent()) {
            return true;
        }
    }

    return false;
}

void PageLisp::updateRecentList()
{
    ui->recentList->clear();
    for (auto f: mRecentFiles) {
        ui->recentList->addItem(f);
    }

    on_recentFilterEdit_textChanged(ui->recentFilterEdit->text());
}

void PageLisp::makeEditorConnections(ScriptEditor *editor)
{
    connect(editor->codeEditor(), &QCodeEditor::textChanged, [editor, this]() {
       setEditorDirty(editor);
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
    connect(editor->codeEditor(), &QCodeEditor::runEmbeddedTriggered, [this]() {
        on_uploadButton_clicked();
    });
    connect(editor->codeEditor(), &QCodeEditor::runWindowTriggered, [this]() {
        on_streamButton_clicked();
    });
    connect(editor->codeEditor(), &QCodeEditor::stopTriggered, [this]() {
        on_stopButton_clicked();
    });
    connect(editor->codeEditor(), &QCodeEditor::runBlockTriggered, [this](QString text) {
        if (text.length() > 400) {
            mVesc->emitMessageDialog("Run Block",
                                     "Too much code selected, please select a smaller block.",
                                     false, false);
        } else {
            mVesc->commands()->lispSendReplCmd(text);
        }
    });
}

void PageLisp::createEditorTab(QString fileName, QString content)
{
    ScriptEditor *editor = new ScriptEditor();
    editor->setModeLisp();
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

void PageLisp::removeEditor(ScriptEditor *editor)
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

void PageLisp::setEditorDirty(ScriptEditor *editor)
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

void PageLisp::setEditorClean(ScriptEditor *editor)
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

void PageLisp::openExample()
{
    QListWidgetItem *item = ui->exampleList->currentItem();

    if (item) {
        QFile file(item->data(Qt::UserRole).toString());

        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::critical(this, "Open Lisp File",
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

void PageLisp::openRecentList()
{
    auto *item = ui->recentList->currentItem();

    if (item) {
        QString fileName = item->text();
        QFile file(fileName);

        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::critical(this, "Open Lisp File",
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

bool PageLisp::eraseCode(int size)
{
    QProgressDialog dialog("Erasing old script...", QString(), 0, 0, this);
    dialog.setWindowModality(Qt::WindowModal);
    dialog.show();
    auto res = mLoader.lispErase(size);
    dialog.close();
    return res;
}

void PageLisp::on_openRecentButton_clicked()
{
    openRecentList();
}

void PageLisp::on_removeSelectedButton_clicked()
{
    auto *item = ui->recentList->currentItem();

    if (item) {
        QString fileName = item->text();
        mRecentFiles.removeOne(fileName);
        updateRecentList();
    }
}

void PageLisp::on_clearRecentButton_clicked()
{
    auto res = QMessageBox::question(this, "Clear Recent Files",
                                     "This will clear the list with recent files. Are you sure?");

    if (res == QMessageBox::Yes) {
        mRecentFiles.clear();
        updateRecentList();
    }
}

void PageLisp::on_recentList_doubleClicked()
{
    openRecentList();
}

void PageLisp::on_openExampleButton_clicked()
{
    openExample();
}

void PageLisp::on_exampleList_doubleClicked()
{
    openExample();
}

void PageLisp::on_runButton_clicked()
{
    mVesc->commands()->lispSetRunning(1);
}

void PageLisp::on_stopButton_clicked()
{
    mVesc->commands()->lispSetRunning(0);
}

void PageLisp::on_uploadButton_clicked()
{
    QProgressDialog dialog(tr("Uploading..."), QString(), 0, 0, this);
    dialog.setWindowModality(Qt::WindowModal);
    dialog.show();

    auto e = qobject_cast<ScriptEditor*>(ui->fileTabs->widget(ui->fileTabs->currentIndex()));

    QString codeStr = "";
    QString editorPath = QDir::currentPath();

    if (e != nullptr) {
        codeStr = e->contentAsText();
        QFileInfo fi(e->fileNow());
        if (fi.exists()) {
            editorPath = fi.canonicalPath();
        }
    } else {
        codeStr = ui->mainEdit->contentAsText();
        QFileInfo fi(ui->mainEdit->fileNow());
        if (fi.exists()) {
            editorPath = fi.canonicalPath();
        }
    }

    VByteArray vb = mLoader.lispPackImports(codeStr, editorPath);
    if (vb.isEmpty()) {
        return;
    }

    if (!eraseCode(vb.size() + 100)) {
        return;
    }

    bool ok = mLoader.lispUpload(vb);

    if (ok && ui->autoRunBox->isChecked()) {
        on_runButton_clicked();
    }
}

void PageLisp::on_readExistingButton_clicked()
{
    QProgressDialog dialog(tr("Reading Code..."), QString(), 0, 0, this);
    dialog.setWindowModality(Qt::WindowModal);
    dialog.show();

    QString lispPath = "From VESC";
    auto code = mLoader.lispRead(this, lispPath);

    if (!code.isEmpty()) {
        if (ui->mainEdit->codeEditor()->toPlainText().isEmpty()) {
            ui->mainEdit->codeEditor()->setPlainText(code);
            ui->fileTabs->setTabText(ui->fileTabs->indexOf(ui->mainEdit), "From VESC");
        } else {
            createEditorTab(lispPath, code);
        }
    }
}

void PageLisp::on_eraseButton_clicked()
{
    eraseCode(16);
}

void PageLisp::on_rescaleButton_clicked()
{
    ui->bindingPlot->rescaleAxes();
    ui->bindingPlot->replotWhenVisible();
}

void PageLisp::on_helpButton_clicked()
{
    QString html = "<b>VESC Lisp Editor</b><br><br>"
                   ""
                   "For documentation, go to<br>"
                   "<a href=\"https://github.com/vedderb/bldc/blob/master/lispBM/README.md\">https://github.com/vedderb/bldc/blob/master/lispBM/README.md</a><br><br>"
                   ""
                   "You can also join the unofficial VESC Discord server and ask questions "
                   "in the lisp-scripting chat at<br>"
                   "<a href=\"https://discord.gg/JgvV5NwYts\">https://discord.gg/JgvV5NwYts</a><br><br>"
                   "<b>Keyboard Commands</b><br>"
                   "Ctrl + '+'   : Increase font size<br>"
                   "Ctrl + '-'   : Decrease font size<br>"
                   "Ctrl + space : Show auto-complete suggestions<br>"
                   "Ctrl + '/'   : Toggle auto-comment on line or block<br>"
                   "Ctrl + 'i'   : Remove trailing whitespaces from selected lines<br>"
                   "Ctrl + 'f'   : Open search (and replace) bar<br>"
                   "Ctrl + 'e'   : Upload (and run if set) application<br>"
                   "Ctrl + 'w'   : Stream application<br>"
                   "Ctrl + 'q'   : Stop application<br>"
                   "Ctrl + 'd'   : Clear console<br>"
                   "Ctrl + 's'   : Save file<br>"
                   "Ctrl + 'r'   : Run selected block in REPL<br>";

    HelpDialog::showHelpMonospace(this, "VESC Tool Script Editor", html);
}

void PageLisp::on_replEdit_returnPressed()
{
    mVesc->commands()->lispSendReplCmd(ui->replEdit->text());
    ui->replEdit->clear();
}

void PageLisp::on_replHelpButton_clicked()
{
    mVesc->commands()->lispSendReplCmd(":help");
}

void PageLisp::on_streamButton_clicked()
{
    QProgressDialog dialog(tr("Streaming..."), QString(), 0, 0, this);
    dialog.setWindowModality(Qt::WindowModal);
    dialog.show();

    auto e = qobject_cast<ScriptEditor*>(ui->fileTabs->widget(ui->fileTabs->currentIndex()));

    QString codeStr = "";

    if (e != nullptr) {
        codeStr = e->contentAsText();
    } else {
        ui->mainEdit->contentAsText();
    }

    mLoader.lispStream(codeStr.toLocal8Bit(), ui->streamModeBox->currentIndex());
}

void PageLisp::on_recentFilterEdit_textChanged(const QString &filter)
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

void PageLisp::on_exampleFilterEdit_textChanged(const QString &filter)
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
