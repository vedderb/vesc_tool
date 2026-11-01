/*
    Copyright 2026 Benjamin Vedder	benjamin@vedder.se

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

#ifndef QMLUICACHE_H
#define QMLUICACHE_H

#include <QQmlComponent>
#include <QQmlEngine>
#include <QSaveFile>

namespace QmlUiCache
{

// Compile without creating the object: validation must not run device UI code.
inline QString decode(const QByteArray &data, const QUrl &baseUrl)
{
    const QByteArray source = qUncompress(data);
    const QString qml = QString::fromUtf8(source);
    if (qml.trimmed().isEmpty() || qml.contains(QChar::Null) || qml.toUtf8() != source) {
        return QString();
    }

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(source, baseUrl);
    if (!component.isReady()) {
        qWarning().noquote() << "Invalid device QML:" << component.errorString();
        return QString();
    }
    return qml;
}

inline bool store(const QString &path, const QByteArray &data)
{
    QSaveFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(data) == data.size() && file.commit();
}

} // namespace QmlUiCache

#endif // QMLUICACHE_H
