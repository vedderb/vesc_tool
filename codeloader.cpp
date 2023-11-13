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

#include "codeloader.h"
#include "utility.h"
#include <QEventLoop>
#include <QFileDialog>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>

CodeLoader::CodeLoader(QObject *parent) : QObject(parent)
{
    mVesc = nullptr;
}

VescInterface *CodeLoader::vesc() const
{
    return mVesc;
}

void CodeLoader::setVesc(VescInterface *vesc)
{
    mVesc = vesc;
}

bool CodeLoader::lispErase(int size)
{
    if (!mVesc) {
        return false;
    }

    if (!mVesc->isPortConnected()) {
        mVesc->emitMessageDialog(tr("Erase old code"), tr("Not Connected"), false);
        return false;
    }

    auto waitEraseRes = [this]() {
        int res = -10;

        QEventLoop loop;
        QTimer timeoutTimer;
        timeoutTimer.setSingleShot(true);
        timeoutTimer.start(8000);
        auto conn = connect(mVesc->commands(), &Commands::lispEraseCodeRx,
                            [&res,&loop](bool erRes) {
            res = erRes ? 1 : -1;
            loop.quit();
        });

        connect(&timeoutTimer, SIGNAL(timeout()), &loop, SLOT(quit()));
        loop.exec();

        disconnect(conn);
        return res;
    };

    mVesc->commands()->lispEraseCode(size);

    int erRes = waitEraseRes();
    if (erRes != 1) {
        QString msg = tr("Unknown failure");

        if (erRes == -10) {
            msg = tr("Erase timed out");
        } else if (erRes == -1) {
            msg = tr("Erasing Lisp Code failed");
        }

        mVesc->emitMessageDialog(tr("Erase Lisp"), msg, false);
        return false;
    }

    return true;
}

QByteArray CodeLoader::lispPackImports(QString codeStr, QString editorPath)
{
    VByteArray vb;
    vb.vbAppendUint16(0); // Flags: 0
    vb.append(codeStr.toLocal8Bit());

    if (vb.at(vb.size() - 1) != '\0') {
        vb.append('\0');
    }

    auto dbg = [this](QString title, QString msg, bool isGood) {
        if (mVesc) {
            mVesc->emitMessageDialog(title, msg, isGood);
        } else {
            if (isGood) {
                qDebug() << title << ":" << msg;
            } else {
                qWarning() << title << ":" << msg;
            }
        }
    };

    // Create and append data table
    {
        QList<QPair<QString, QByteArray> > files;
        auto lines = codeStr.split("\n");
        int line_num = 0;

        foreach (auto line, lines) {
            line_num++;

            QString path;
            QString tag;
            bool isInvalid;

            if (getImportFromLine(line, path, tag, isInvalid)) {
                if (isInvalid) {
                    dbg(tr("Append Imports"),
                        tr("Line: %1: Invalid import tag.").arg(line_num),
                        false);
                    return QByteArray();
                }

                auto pkgErrorMsg = "If you are importing from a package in the git repository you might "
                                   "need to update the package archive. That can be done from the the "
                                   "VESC Packages page.";

                bool isPkgImport = false;
                QString pkgImportName;
                if (path.startsWith("pkg::")) {
                    path.remove(0, 5);

                    auto atInd = path.indexOf("@");
                    if (atInd > 0) {
                        pkgImportName = path.mid(0, atInd);
                        path.remove(0, atInd + 1);
                    } else {
                        dbg(tr("Append Imports"),
                            tr("Line: %1: Invalid import tag.").arg(line_num),
                            false);
                        return QByteArray();
                    }

                    isPkgImport = true;
                } else if (path.startsWith("pkg@")) {
                    path.remove(0, 4);
                    isPkgImport = true;
                }

                QFileInfo fi(editorPath + "/" + path);
                if (!fi.exists()) {
                    fi = QFileInfo(path);
                }

                if (fi.exists()) {
                    QFile f(fi.absoluteFilePath());
                    if (f.open(QIODevice::ReadOnly)) {
                        auto fileData = f.readAll();

                        if (isPkgImport) {
                            auto pkg = unpackVescPackage(fileData);
                            auto imports = lispUnpackImports(pkg.lispData);

                            if (pkgImportName.isEmpty()) {
                                auto importData = imports.first.toLocal8Bit();
                                importData.append('\0'); // Pad with 0 in case it is a text file
                                files.append(qMakePair(tag, importData));
                            } else {
                                bool found = false;
                                for (auto i: imports.second) {
                                    if (i.first == pkgImportName) {
                                        auto importData = i.second;
                                        importData.append('\0'); // Pad with 0 in case it is a text file
                                        files.append(qMakePair(tag, importData));
                                        found = true;
                                        break;
                                    }
                                }

                                if (!found) {
                                    mVesc->emitMessageDialog(tr("Append Imports"),
                                                             tr("Tag %1 not found in package %2. %3").
                                                             arg(pkgImportName).arg(path).arg(pkgErrorMsg),
                                                             false);
                                    return QByteArray();
                                }
                            }
                        } else {
                            fileData.append('\0'); // Pad with 0 in case it is a text file
                            files.append(qMakePair(tag, fileData));
                        }
                    } else {
                        dbg(tr("Append Imports"),
                            tr("Line: %1: Imported file cannot be opened.").arg(line_num),
                            false);
                        return QByteArray();
                    }
                } else {
                    dbg(tr("Append Imports"),
                        tr("Line: %1: Imported file not found: %2. %3").
                        arg(line_num).arg(fi.absoluteFilePath()).arg(pkgErrorMsg),
                        false);
                    return QByteArray();
                }
            }
        }

        int file_table_size = 0;
        for (auto f: files) {
            file_table_size += f.first.length() + 9;
        }

        vb.vbAppendInt16(files.size());

        int file_offset = vb.size() + file_table_size - 2;

        for (auto f: files) {
            // Align on 4 bytes in case this is loaded as code
            while (file_offset % 4 != 0) {
                file_offset++;
            }

            vb.vbAppendString(f.first);
            vb.vbAppendInt32(file_offset);
            vb.vbAppendInt32(f.second.size());
            file_offset += f.second.size();
        }

        for (auto f: files) {
            while ((vb.size() - 2) % 4 != 0) {
                vb.append('\0');
            }
            vb.append(f.second);
        }
    }

    return std::move(vb);
}

QPair<QString, QList<QPair<QString, QByteArray> > > CodeLoader::lispUnpackImports(QByteArray data)
{
    // In case the import starts with a flag it is removed here. Note: The flag is a
    // bit confusing and there is a slightly more consistent way of handling it, but
    // I left it as is to not break compatibility.
    if (data.size() > 2 && data[0] == '\0' && data[1] == '\0') {
        data.remove(0, 2);
    }

    QList<QPair<QString, QByteArray> > imports;
    int end = data.indexOf(QByteArray("\0", 1));

    if (end > 0) {
        VByteArray vb = data.mid(end + 1);

        if (vb.size() > 3) {
            auto num_imports = vb.vbPopFrontInt16();
            if (num_imports > 0 && num_imports < 500) {
                for (int i = 0;i < num_imports;i++) {
                    auto name = vb.vbPopFrontString();
                    auto offset = vb.vbPopFrontInt32();
                    auto len = vb.vbPopFrontInt32() - 1; // Remove 1 as the files are padded with one 0
                    imports.append(qMakePair(name, data.mid(offset, len)));
                }
            }
        }

        data = data.left(end);
    }

    return qMakePair(QString::fromLocal8Bit(data), imports);
}

bool CodeLoader::lispUpload(VByteArray vb)
{
    quint16 crc = Packet::crc16((const unsigned char*)vb.constData(), uint32_t(vb.size()));
    VByteArray data;
    data.vbAppendUint32(vb.size() - 2);
    data.vbAppendUint16(crc);
    data.append(vb);

    // The ESP32 partition table has 512k space for lisp scripts. The STM32
    // has one 128k flash page.
    auto fwParams = mVesc->getLastFwRxParams();
    int max_size = 1024 * 500;
    if (fwParams.hwType == HW_TYPE_VESC) {
        max_size = 1024 * 120;
    }

    if (data.size() > max_size) {
        mVesc->emitMessageDialog(tr("Upload Code"), tr("Not enough space"), false);
        return false;
    }

    auto waitWriteRes = [this]() {
        int res = -10;

        QEventLoop loop;
        QTimer timeoutTimer;
        timeoutTimer.setSingleShot(true);
        timeoutTimer.start(1000);
        auto conn = connect(mVesc->commands(), &Commands::lispWriteCodeRx,
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

    auto writeChunk = [this, waitWriteRes](QByteArray data, quint32 offset) {
        int tries = 5;
        int res = -10;
        while (tries > 0) {
            mVesc->commands()->lispWriteCode(data, offset);
            if ((res = waitWriteRes()) >= 0) {
                return res;
            }
            tries--;
        }

        return res;
    };

    quint32 offset = 0;
    bool ok = true;
    while (data.size() > 0) {
        const int chunkSize = 384;
        int sz = data.size() > chunkSize ? chunkSize : data.size();

        if (writeChunk(data.mid(0, sz), offset) < 0) {
            mVesc->emitMessageDialog(tr("Upload Code"), tr("Write failed"), false);
            ok = false;
            break;
        }

        offset += sz;
        data.remove(0, sz);
    }

    return ok;
}

bool CodeLoader::lispUpload(QString codeStr, QString editorPath)
{
    VByteArray vb = lispPackImports(codeStr, editorPath);

    if (vb.isEmpty()) {
        return false;
    }

    return lispUpload(vb);
}

bool CodeLoader::lispStream(VByteArray vb, qint8 mode)
{
    if (!mVesc->isPortConnected()) {
        mVesc->emitMessageDialog(tr("Stream code"), tr("Not Connected"), false);
        return false;
    }

    auto waitWriteRes = [this]() {
        int res = -10;

        QEventLoop loop;
        QTimer timeoutTimer;
        timeoutTimer.setSingleShot(true);
        timeoutTimer.start(4000);
        auto conn = connect(mVesc->commands(), &Commands::lispStreamCodeRx,
                            [&res,&loop](qint32 offset, qint16 result) {
            (void)offset;
            res = result;
            loop.quit();
        });

        connect(&timeoutTimer, SIGNAL(timeout()), &loop, SLOT(quit()));
        loop.exec();

        disconnect(conn);
        return res;
    };

    qint32 offset = 0;
    qint32 size_tot = vb.size();
    bool ok = true;
    while (vb.size() > 0) {
        const int chunkSize = 384;
        int sz = vb.size() > chunkSize ? chunkSize : vb.size();

        mVesc->commands()->lispStreamCode(vb.mid(0, sz), offset, size_tot, mode);
        auto writeRes = waitWriteRes();
        if (writeRes != 0) {
            mVesc->emitMessageDialog(tr("Stream Code"), tr("Stream failed. Result: %1").arg(writeRes), false);
            ok = false;
            break;
        }

        offset += sz;
        vb.remove(0, sz);
    }

    return ok;
}

QString CodeLoader::lispRead(QWidget *parent, QString &lispPath)
{
    if (!mVesc->isPortConnected()) {
        mVesc->emitMessageDialog(tr("Read code"), tr("Not Connected"), false);
        return "";
    }

    QByteArray lispData;
    int lenLispLast = 0;
    auto conn = connect(mVesc->commands(), &Commands::lispReadCodeRx,
                        [&](int lenLisp, int ofsLisp, QByteArray data) {
        if (lispData.size() <= ofsLisp) {
            lispData.append(data);
        }
        lenLispLast = lenLisp;
    });

    auto getLispChunk = [&](int size, int offset, int tries, int timeout) {
        bool res = false;

        for (int j = 0;j < tries;j++) {
            mVesc->commands()->lispReadCode(size, offset);
            res = Utility::waitSignal(mVesc->commands(), SIGNAL(lispReadCodeRx(int,int,QByteArray)), timeout);
            if (res) {
                break;
            }
        }
        return res;
    };

    QString res = "";

    if (getLispChunk(10, 0, 5, 1500)) {
        while (lispData.size() < lenLispLast) {
            int dataLeft = lenLispLast - lispData.size();
            if (!getLispChunk(dataLeft > 400 ? 400 : dataLeft, lispData.size(), 5, 1500)) {
                break;
            }
        }

        if (lispData.size() == lenLispLast) {
            auto unpacked = lispUnpackImports(lispData);

            res = unpacked.first;
            auto num_imports = unpacked.second.length();

            if (num_imports > 0) {
                auto reply = QMessageBox::warning(parent,
                                                  tr("Imports"),
                                                  tr("%1 imports found. Do you want to save them as files?").arg(num_imports),
                                                  QMessageBox::Yes | QMessageBox::No);

                QMap<QString, QString> importPaths;
                foreach (auto line, res.split('\n')) {
                    QString path;
                    QString tag;
                    bool isInvalid;
                    if (getImportFromLine(line, path, tag, isInvalid)) {
                        if (!isInvalid) {
                            importPaths.insert(tag, path);
                        }
                    }
                }

                if (reply == QMessageBox::Yes) {
                    QString dirName = QFileDialog::getExistingDirectory(parent, tr("Choose Directory"));

                    if (!dirName.isEmpty()) {
                        QFile fileLisp(dirName + "/From VESC.lisp");
                        if (!fileLisp.exists()) {
                            if (fileLisp.open(QIODevice::WriteOnly)) {
                                fileLisp.write(res.toUtf8());
                                fileLisp.close();
                                lispPath = QFileInfo(fileLisp).canonicalFilePath();
                            }
                        }

                        foreach (auto i, unpacked.second) {
                            QString fileName = dirName + "/" + i.first + ".bin";

                            if (importPaths.contains(i.first)) {
                                const auto &path = importPaths[i.first];

                                if (!path.startsWith("/") &&
                                        !path.startsWith("\\") &&
                                        !path.startsWith("pkg::") &&
                                        !path.startsWith("pkg@")) {
                                    fileName = dirName + "/" + path;
                                }
                            }

                            QFile file(fileName);
                            QFileInfo fi(file);
                            QDir().mkpath(fi.path());

                            if (file.exists()) {
                                auto reply = QMessageBox::question(parent,
                                                                   tr("Replace File"),
                                                                   tr("File %1 exists. Do you want to replace it?").arg(i.first));

                                if (reply != QMessageBox::Yes) {
                                    continue;
                                }
                            }

                            if (!file.open(QIODevice::WriteOnly)) {
                                QMessageBox::critical(parent, tr("Save Import"),
                                                      "Could not open\n" + file.fileName() + "\nfor writing");
                                return QByteArray();
                            }

                            file.write(i.second);
                            file.close();
                        }
                    }
                }
            }

            return res;
        }
    }

    mVesc->emitMessageDialog(tr("Get Lisp"),
                             tr("Could not read Lisp code"),
                             false);

    disconnect(conn);

    return "";
}

bool CodeLoader::qmlErase(int size)
{
    if (!mVesc) {
        return false;
    }

    if (!mVesc->isPortConnected()) {
        mVesc->emitMessageDialog(tr("Erase Qml"), tr("Not Connected"), false);
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

    mVesc->commands()->qmlUiErase(size);

    int erRes = waitEraseRes();
    if (erRes != 1) {
        QString msg = "Unknown failure";

        if (erRes == -10) {
            msg = "Erase timed out";
        } else if (erRes == -1) {
            msg = "Erasing QMLUI failed";
        }

        mVesc->emitMessageDialog(tr("Erase Qml"), msg, false);
        return false;
    }

    return true;
}

QByteArray CodeLoader::qmlCompress(QString script)
{
    script.prepend("import \"qrc:/mobile\";");
    script.prepend("import Vedder.vesc.vescinterface 1.0;");
    return qCompress(script.toUtf8(), 9);
}

bool CodeLoader::qmlUpload(QByteArray script, bool isFullscreen)
{
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

    auto writeChunk = [this, waitWriteRes](QByteArray data, quint32 offset) {
        int tries = 5;
        int res = -10;
        while (tries > 0) {
            mVesc->commands()->qmlUiWrite(data, offset);
            if ((res = waitWriteRes()) >= 0) {
                return res;
            }
            tries--;
        }

        return res;
    };

    VByteArray vb;
    vb.vbAppendUint16(isFullscreen ? 2 : 1);
    vb.append(script);
    quint16 crc = Packet::crc16((const unsigned char*)vb.constData(),
                                uint32_t(vb.size()));
    VByteArray data;
    data.vbAppendUint32(vb.size() - 2);
    data.vbAppendUint16(crc);
    data.append(vb);

    if (data.size() > (1024 * 120)) {
        mVesc->emitMessageDialog(tr("Upload Qml"), tr("Not enough space"), false);
        return false;
    }

    quint32 offset = 0;
    bool ok = true;
    while (data.size() > 0) {
        const int chunkSize = 384;
        int sz = data.size() > chunkSize ? chunkSize : data.size();

        if (writeChunk(data.mid(0, sz), offset) < 0) {
            mVesc->emitMessageDialog(tr("Upload Qml"), tr("Qml write failed"), false);
            ok = false;
            break;
        }

        offset += sz;
        data.remove(0, sz);
    }

    return ok;
}

QByteArray CodeLoader::packVescPackage(VescPackage pkg)
{
    VByteArray data;

    data.vbAppendString("VESC Packet");

    if (!pkg.name.isEmpty()) {
        auto dataRaw = pkg.name.toUtf8();
        data.vbAppendString("name");
        data.vbAppendInt32(dataRaw.size());
        data.append(dataRaw);
    }

    if (!pkg.description.isEmpty()) {
        auto dataRaw = pkg.description.toUtf8();
        data.vbAppendString("description");
        data.vbAppendInt32(dataRaw.size());
        data.append(dataRaw);
    }

    if (!pkg.lispData.isEmpty()) {
        data.vbAppendString("lispData");
        data.vbAppendInt32(pkg.lispData.size());
        data.append(pkg.lispData);
    }

    if (!pkg.qmlFile.isEmpty()) {
        auto dataRaw = pkg.qmlFile.toUtf8();
        data.vbAppendString("qmlFile");
        data.vbAppendInt32(dataRaw.size());
        data.append(dataRaw);
    }

    data.vbAppendString("qmlIsFullscreen");
    data.vbAppendInt32(1);
    data.vbAppendInt8(pkg.qmlIsFullscreen);

    return qCompress(data, 9);
}

VescPackage CodeLoader::unpackVescPackage(QByteArray data)
{
    VescPackage pkg;
    VByteArray vb(qUncompress(data));

    // Yes, packet is a typo and should say package...
    // That does not matter in practice and changing that now breaks
    // compatibility.
    if (vb.vbPopFrontString() != "VESC Packet") {
        qWarning() << "Invalid VESC Packet";
        return pkg;
    }

    pkg.compressedData = data;

    while (!vb.isEmpty()) {
        QString name = vb.vbPopFrontString();

        if (name.isEmpty()) {
            qWarning() << "Empty name";
            break;
        }

        if (name == "name") {
            auto len = vb.vbPopFrontInt32();
            auto dataRaw = vb.left(len);
            vb.remove(0, len);
            pkg.name = QString::fromUtf8(dataRaw);
            pkg.loadOk = true;
        } else if (name == "description") {
            auto len = vb.vbPopFrontInt32();
            auto dataRaw = vb.left(len);
            vb.remove(0, len);
            pkg.description = QString::fromUtf8(dataRaw);
        } else if (name == "lispData") {
            auto len = vb.vbPopFrontInt32();
            auto dataRaw = vb.left(len);
            vb.remove(0, len);
            pkg.lispData = dataRaw;
        } else if (name == "qmlFile") {
            auto len = vb.vbPopFrontInt32();
            auto dataRaw = vb.left(len);
            vb.remove(0, len);
            pkg.qmlFile = QString::fromUtf8(dataRaw);
        } else if (name == "qmlIsFullscreen") {
            vb.vbPopFrontInt32(); // Discard length
            pkg.qmlIsFullscreen = vb.vbPopFrontInt8();
        } else {
            // Unknown identifier, skip
            auto len = vb.vbPopFrontInt32();
            vb.remove(0, len);
        }
    }

    return pkg;
}

bool CodeLoader::installVescPackage(VescPackage pkg)
{
    bool res = true;
    QByteArray qml;

    if (res && !pkg.qmlFile.isEmpty()) {
        qml = qmlCompress(pkg.qmlFile);
        res = qmlErase(qml.size() + 100);
    }

    if (res && !pkg.qmlFile.isEmpty()) {
        res = qmlUpload(qml, pkg.qmlIsFullscreen);
    }

    if (res && !pkg.lispData.isEmpty()) {
        res = lispErase(pkg.lispData.size() + 100);
    }

    if (res && !pkg.lispData.isEmpty()) {
        res = lispUpload(VByteArray(pkg.lispData));
    }

    if (res && !pkg.lispData.isEmpty()) {
        mVesc->commands()->lispSetRunning(1);
    }

    Utility::sleepWithEventLoop(500);
    mVesc->reloadFirmware();

    return res;
}

bool CodeLoader::installVescPackage(QByteArray data)
{
    return installVescPackage(unpackVescPackage(data));
}

bool CodeLoader::installVescPackageFromPath(QString path)
{
    if (path.startsWith("file:/")) {
        path.remove(0, 6);
    }

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        mVesc->emitMessageDialog(tr("Write Package"),
                                 tr("Could not open package file for reading."),
                                 false, false);
        return false;
    }

    return installVescPackage(f.readAll());
}

QVariantList CodeLoader::reloadPackageArchive()
{
    QVariantList res;
    QString appDataLoc = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if(!QDir(appDataLoc).exists()) {
            QDir().mkpath(appDataLoc);
    }
    QString path = appDataLoc + "/vesc_pkg_all.rcc";
    QFile file(path);
    if (file.exists()) {
        QResource::unregisterResource(path);
        QResource::registerResource(path);

        QString pkgDir = "://vesc_packages";

        QDirIterator it(pkgDir);
        while (it.hasNext()) {
            QFileInfo fi(it.next());

            QDirIterator it2(fi.absoluteFilePath());
            while (it2.hasNext()) {
                QFileInfo fi2(it2.next());

                if (fi2.absoluteFilePath().toLower().endsWith(".vescpkg")) {
                    QString name = fi2.fileName();
                    QFile f(fi2.absoluteFilePath());
                    if (f.open(QIODevice::ReadOnly)) {
                        auto pkg = unpackVescPackage(f.readAll());
                        name = pkg.name;
                        pkg.isLibrary = fi2.absoluteFilePath().startsWith("://vesc_packages/lib_");
                        res.append(QVariant::fromValue(pkg));
                    }
                }
            }
        }
    }

    return res;
}

bool CodeLoader::downloadPackageArchive()
{
    bool res = false;

    QUrl url("http://home.vedder.se/vesc_pkg/vesc_pkg_all.rcc");
    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    QNetworkReply *reply = manager.get(request);

    connect(reply, &QNetworkReply::downloadProgress, [this](qint64 bytesReceived, qint64 bytesTotal) {
        emit downloadProgress(bytesReceived, bytesTotal);
    });

    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        QString appDataLoc = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if(!QDir(appDataLoc).exists()) {
                QDir().mkpath(appDataLoc);
        }
        QString path = appDataLoc + "/vesc_pkg_all.rcc";
        QResource::unregisterResource(path);
        QFile file(path);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(reply->readAll());
            file.close();
            res = true;
        }

        // Remove image cache
        QString cacheLoc = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
        QDir(cacheLoc + "/img/").removeRecursively();
    }

    reply->abort();
    reply->deleteLater();

    return res;
}

bool CodeLoader::getImportFromLine(QString line, QString &path, QString &tag, bool &isInvalid)
{
    bool res = false;
    isInvalid = false;

    while (line.startsWith(" ")) {
        line.remove(0, 1);
    }

    while (line.startsWith("( ")) {
        line.remove(1, 1);
    }

    if (line.startsWith("(import ", Qt::CaseInsensitive)) {
        int start = line.indexOf("\"");
        int end = line.lastIndexOf("\"");

        if (start > 0 && end > start) {
            path = line.mid(start + 1, end - start - 1);
            tag = line.mid(end + 1).replace("\r", "").replace(" ", "").replace(")", "").replace("'", "");
            if (tag.indexOf(";") >= 0) {
                tag = tag.mid(0, tag.indexOf(";"));
            }
        } else {
            isInvalid = true;
        }

        res = true;
    }

    if (path.isEmpty() || tag.isEmpty()) {
        isInvalid = true;
    }

    return res;
}
