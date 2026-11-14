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

#ifndef VESCMCPDEVICE_H
#define VESCMCPDEVICE_H
#include "mcpdevice.h"
#include "vescinterface.h"

class VescMcpDevice : public McpDevice
{
    Q_OBJECT
  public:
    explicit VescMcpDevice(VescInterface *vesc, QObject *parent = nullptr);
    bool ready() const override;
    QString target() const override;
    bool acquire(QObject *owner) override;
    void release(QObject *owner) override;
    bool send(QObject *owner, const QByteArray &packet) override;
    QJsonObject configValues(const QString &kind) const override;
    QJsonObject canonicalValues(const QString &kind, QString *error) const override;
    QByteArray configWire(const QString &kind) const override;
    QByteArray configXml(const QString &kind) const override;
    bool validateChanges(const QString &kind, const QJsonObject &desired,
                         QString *error) const override;
    QString inputMode() const override;

  private:
    ConfigParams *params(const QString &kind) const;
    static QJsonObject values(ConfigParams *params);
    QPointer<VescInterface> mVesc;
};
#endif
