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

#ifndef CONFIGPARAMS_H
#define CONFIGPARAMS_H

#include <QObject>
#include <QHash>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include "configparam.h"
#include "vbytearray.h"

class ConfigParams : public QObject
{
    Q_OBJECT
public:
    explicit ConfigParams(QObject *parent = 0);
    void addParam(const QString &name, ConfigParam param);
    void deleteParam(const QString &name);
    Q_INVOKABLE void setUpdateOnly(const QString &name);
    Q_INVOKABLE QString getUpdateOnly();
    Q_INVOKABLE bool getUpdatesEnabled() const;
    Q_INVOKABLE void setUpdatesEnabled(bool updatesEnabled);
    void clearParams();
    void clearAll();

    Q_INVOKABLE bool hasParam(const QString &name);
    ConfigParam *getParam(const QString &name);
    ConfigParam getParamCopy(const QString &name) const;

    Q_INVOKABLE bool isParamDouble(const QString &name);
    Q_INVOKABLE bool isParamInt(const QString &name);
    Q_INVOKABLE bool isParamEnum(const QString &name);
    Q_INVOKABLE bool isParamQString(const QString &name);
    Q_INVOKABLE bool isParamBool(const QString &name);

    Q_INVOKABLE double getParamDouble(const QString &name);
    Q_INVOKABLE int getParamInt(const QString &name);
    Q_INVOKABLE int getParamEnum(const QString &name);
    Q_INVOKABLE QString getParamQString(const QString &name);
    Q_INVOKABLE bool getParamBool(const QString &name);
    Q_INVOKABLE QString getLongName(const QString &name);
    Q_INVOKABLE QString getDescription(const QString &name);

    Q_INVOKABLE double getParamMaxDouble(const QString &name);
    Q_INVOKABLE double getParamMinDouble(const QString &name);
    Q_INVOKABLE double getParamStepDouble(const QString &name);
    Q_INVOKABLE int getParamDecimalsDouble(const QString &name);
    Q_INVOKABLE int getParamMaxInt(const QString &name);
    Q_INVOKABLE int getParamMinInt(const QString &name);
    Q_INVOKABLE int getParamStepInt(const QString &name);
    Q_INVOKABLE QStringList getParamEnumNames(const QString &name);
    Q_INVOKABLE double getParamEditorScale(const QString &name);
    Q_INVOKABLE QString getParamSuffix(const QString &name);
    Q_INVOKABLE bool getParamEditAsPercentage(const QString &name);
    Q_INVOKABLE bool getParamShowDisplay(const QString &name);
    Q_INVOKABLE bool getParamTransmittable(const QString &name);

    QStringList getParamOrder() const;
    void setParamOrder(const QStringList &order);

    QWidget *getEditor(const QString &name, QWidget *parent = 0);

    void getParamSerial(VByteArray &vb, const QString &name);
    void setParamSerial(VByteArray &vb, const QString &name, QObject *src = 0);

    QStringList getSerializeOrder() const;
    void setSerializeOrder(const QStringList &serializeOrder);
    void clearSerializeOrder();

    void serialize(VByteArray &vb);
    void deSerialize(VByteArray &vb);

    void getXML(QXmlStreamWriter &stream, QString configName);
    bool setXML(QXmlStreamReader &stream, QString configName);
    bool saveXml(QString fileName, QString configName);
    bool loadXml(QString fileName, QString configName);
    QString xmlStatus();

    void getParamsXML(QXmlStreamWriter &stream);
    bool setParamsXML(QXmlStreamReader &stream);
    bool saveParamsXml(QString fileName);
    bool loadParamsXml(QString fileName);

    bool saveCDefines(const QString &fileName, bool wrapIfdef = false);

    QStringList checkDifference(ConfigParams *config);

    quint32 getSignature();

    // Operators
    ConfigParams& operator=(const ConfigParams &other);

signals:
    void paramChangedDouble(QObject *src, QString name, double newParam);
    void paramChangedInt(QObject *src, QString name, int newParam);
    void paramChangedEnum(QObject *src, QString name, int newParam);
    void paramChangedQString(QObject *src, QString name, QString newParam);
    void paramChangedBool(QObject *src, QString name, bool newParam);
    void updateRequested();
    void updateRequestDefault();
    void updated();
    void savingXml();

public slots:
    void updateParamDouble(QString name, double param, QObject *src = 0);
    void updateParamInt(QString name, int param, QObject *src = 0);
    void updateParamEnum(QString name, int param, QObject *src = 0);
    void updateParamString(QString name, QString param, QObject *src = 0);
    void updateParamBool(QString name, bool param, QObject *src = 0);
    void requestUpdate();
    void requestUpdateDefault();
    void updateDone();

private:
    QHash<QString, ConfigParam> mParams;
    QStringList mParamList;
    QString mUpdateOnlyName;
    bool mUpdatesEnabled;
    QStringList mSerializeOrder;
    QString mXmlStatus;

    bool almostEqual(float A, float B, float eps);

};

#endif // CONFIGPARAMS_H
