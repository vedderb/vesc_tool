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

#include "logreader.h"
#include <QSettings>
#include <QDir>
#include <QtDebug>
#include <QTextStream>

LogReader::LogReader(QObject *parent) : QObject(parent)
{

}

LogReader::~LogReader()
{
    closeLogFile();
}

bool LogReader::openLogFile(QString fileName)
{
    QSettings set;
    QString logDir = set.value("path_script_input", "./log").toString();

    if (logDir.startsWith("file:/")) {
        logDir.remove(0, 6);
    }

    if (!QDir(logDir).exists()) {
        QDir().mkpath(logDir);
    }

    if (!QDir(logDir).exists()) {
        qWarning() << "Output directory does not exist";
        return false;
    }

     mLogFile.setFileName(QString("%1/%2").arg(logDir).arg(fileName));
     return mLogFile.open(QIODevice::ReadOnly | QIODevice::Text);
}

QString LogReader::readLine()
{
    QString res;

    if (isLogOpen()) {
        res = mLogFile.readLine();
    }

    return res;
}

QString LogReader::readAll()
{
    QString res;

    if (isLogOpen()) {
        res = mLogFile.readAll();
    }

    return res;
}

bool LogReader::atEnd()
{
    bool res = true;

    if (isLogOpen()) {
        res = mLogFile.atEnd();
    }

    return res;
}

void LogReader::closeLogFile()
{
    if (mLogFile.isOpen()) {
        mLogFile.close();
    }
}

bool LogReader::isLogOpen()
{
    return mLogFile.isOpen();
}
