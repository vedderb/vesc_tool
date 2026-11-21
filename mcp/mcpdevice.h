/*
    Copyright 2026 Marvin Holzwarth

    This file is part of VESC Tool.

    VESC Tool is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VESC Tool is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
    */

#ifndef MCPDEVICE_H
#define MCPDEVICE_H
#include <QObject>
#include <QJsonObject>
#include <QByteArray>

// Abstract app-owned device boundary, also used by the hardware-free tests.
class McpDevice : public QObject
{
    Q_OBJECT
  public:
    explicit McpDevice(QObject *parent = nullptr) : QObject(parent) {}
    virtual bool ready() const = 0;
    virtual QString target() const = 0;
    virtual bool acquire(QObject *owner) = 0;
    virtual void release(QObject *owner) = 0;
    virtual bool send(QObject *owner, const QByteArray &packet) = 0;
    virtual QJsonObject configValues(const QString &kind) const = 0;
    virtual QJsonObject canonicalValues(const QString &kind, QString *error) const = 0;
    virtual QByteArray configWire(const QString &kind) const = 0;
    virtual QByteArray configXml(const QString &kind) const = 0;
    virtual bool validateChanges(const QString &kind, const QJsonObject &desired,
                                 QString *error) const = 0;
    virtual QString inputMode() const = 0;
  signals:
    void configuration(const QString &kind);
    void telemetry(const QJsonObject &values);
    void input(const QJsonObject &values);
    void firmware();
    void acknowledgment(const QString &text);
    void lispCode(int total, int offset, const QByteArray &data);
    void lispErased(bool ok);
    void lispWritten(bool ok, quint32 offset);
    void fault(const QString &message);
};
#endif
