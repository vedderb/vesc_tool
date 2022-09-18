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

#ifndef CODELOADER_H
#define CODELOADER_H

#include <QObject>
#include <QDir>
#include "vescinterface.h"

struct VescPackage {

public:
    VescPackage () {
        name = "VESC Package Name";
        qmlIsFullscreen = false;
    }

    QString name;
    QString description;
    QByteArray lispData;
    QString qmlFile;
    bool qmlIsFullscreen;
};

class CodeLoader : public QObject
{
    Q_OBJECT
public:
    explicit CodeLoader(QObject *parent = nullptr);

    VescInterface *vesc() const;
    void setVesc(VescInterface *vesc);

    bool lispErase();
    QByteArray lispPackImports(QString codeStr, QString editorPath = QDir::currentPath());
    QPair<QString, QList<QPair<QString, QByteArray> > > lispUnpackImports(QByteArray data);
    bool lispUpload(VByteArray vb);
    bool lispUpload(QString codeStr, QString editorPath = QDir::currentPath());
    bool lispStream(VByteArray vb, qint8 mode);
    QString lispRead(QWidget *parent = nullptr);

    bool qmlErase();
    bool qmlUpload(QString script, bool isFullscreen);

    QByteArray packVescPackage(VescPackage pkg);
    VescPackage unpackVescPackage(QByteArray data);
    bool installVescPackage(VescPackage pkg);
    bool installVescPackage(QByteArray data);

signals:

private:
    VescInterface *mVesc;

};

#endif // CODELOADER_H
