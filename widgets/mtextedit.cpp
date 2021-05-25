/*
    Copyright 2016 - 2017 Benjamin Vedder	benjamin@vedder.se

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

#include "mtextedit.h"
#include <QTextDocument>
#include <QTextCursor>
#include <QImage>
#include <QByteArray>
#include <QBuffer>
#include <stdlib.h>
#include <QInputDialog>

MTextEdit::MTextEdit(QWidget *parent) : QTextEdit(parent) {
}

bool MTextEdit::canInsertFromMimeData(const QMimeData *source) const {
    return source->hasImage() || QTextEdit::canInsertFromMimeData(source);
}

void MTextEdit::insertFromMimeData(const QMimeData *source) {
    if (source->hasImage()) {
        QStringList formats = source->formats();
        QString format;
        for (int i=0; i<formats.size(); i++) {
            if (formats[i] == "image/bmp")  { format = "BMP";  break; }
            if (formats[i] == "image/jpeg") { format = "JPG";  break; }
            if (formats[i] == "image/jpg")  { format = "JPG";  break; }
            if (formats[i] == "image/gif")  { format = "GIF";  break; }
            if (formats[i] == "image/png")  { format = "PNG";  break; }
            if (formats[i] == "image/pbm")  { format = "PBM";  break; }
            if (formats[i] == "image/pgm")  { format = "PGM";  break; }
            if (formats[i] == "image/ppm")  { format = "PPM";  break; }
            if (formats[i] == "image/tiff") { format = "TIFF"; break; }
            if (formats[i] == "image/xbm")  { format = "XBM";  break; }
            if (formats[i] == "image/xpm")  { format = "XPM";  break; }
        }
        if (!format.isEmpty()) {
//          dropImage(qvariant_cast<QImage>(source->imageData()), format);
            dropImage(qvariant_cast<QImage>(source->imageData()), "JPG");
            return;
        }
    }
    QTextEdit::insertFromMimeData(source);
}

QMimeData *MTextEdit::createMimeDataFromSelection() const {
    return QTextEdit::createMimeDataFromSelection();
}

void MTextEdit::dropImage(const QImage& image, const QString& format) {
    bool ok;
    int w = QInputDialog::getInt(this, tr("Insert Image"),
                                 tr("Enter image width to use (source width: %1)").arg(image.width()),
                                 400, 5, 1920, 20, &ok);

    if (ok) {
        int h = (w * image.height()) / image.width();
        QImage img1 = image.scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

        QByteArray bytes;
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);

        img1.save(&buffer, format.toLocal8Bit().data());
        buffer.close();
        QByteArray base64 = bytes.toBase64();
        QByteArray base64l;
        for (int i=0; i<base64.size(); i++) {
            base64l.append(base64[i]);
            if (i%80 == 0) {
                base64l.append("\n");
            }
        }

        QTextCursor cursor = textCursor();
        QTextImageFormat imageFormat;
        imageFormat.setWidth  ( img1.width() );
        imageFormat.setHeight ( img1.height() );
        imageFormat.setName   ( QString("data:image/%1;base64,%2")
                                .arg(QString("%1.%2").arg(rand()).arg(format))
                                .arg(base64l.data())
                                );
        cursor.insertImage    ( imageFormat );
    }
}
