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

#ifndef VESCMCPTOOLS_H
#define VESCMCPTOOLS_H

#include <QObject>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonObject>
#include "vescinterface.h"
#include "mcpdeviceoperations.h"

// Native MCP tool surface. Hardware operations use an app-owned job channel.
class VescMcpTools : public QObject
{
    Q_OBJECT
  public:
    explicit VescMcpTools(VescInterface *vesc, QObject *parent = nullptr);
    void setAllowEdits(bool allow);
    void setAllowControllerWrites(bool allow);
    McpDeviceOperations *operations() const
    {
        return mOperations;
    }
    QJsonArray tools() const;
    QJsonObject call(const QString &name, const QJsonObject &arguments);

  private:
    static QJsonObject result(const QJsonObject &data, bool error = false);
    static QJsonObject failure(const QString &message);
    void invalidateValues();
    QJsonObject configValues(ConfigParams *params) const;
    QString snapshot(ConfigParams *params, const QString &config) const;
    QString saveBackup(ConfigParams *params, const QString &config, QString *error) const;
    QJsonObject editConfig(const QJsonObject &arguments, bool apply);
    QJsonObject deviceTool(const QString &name, const QJsonObject &arguments);
    QJsonObject scriptTool(const QString &name, const QJsonObject &arguments);

    VescInterface *mVesc;
    QJsonObject mValues;
    QElapsedTimer mValuesAge;
    bool mSendCan;
    int mCanId;
    bool mAllowEdits = false;
    bool mAllowControllerWrites = false;
    McpDeviceOperations *mOperations;
};

#endif // VESCMCPTOOLS_H
