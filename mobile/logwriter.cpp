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

#include "logwriter.h"
#include <QSettings>
#include <QDir>
#include <QtDebug>
#include <QTextStream>

LogWriter::LogWriter(QObject *parent) : QObject(parent)
{

}

LogWriter::~LogWriter()
{
    closeLogFile();
}

bool LogWriter::openLogFile(QString fileName)
{
    QSettings set;
    QString logDir = set.value("path_script_output", "./log").toString();

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
     return mLogFile.open(QIODevice::WriteOnly | QIODevice::Text);
}

bool LogWriter::writeToLogFile(QString text)
{
    if (!isLogOpen()) {
        return false;
    }

    QTextStream os(&mLogFile);
    os << text;
    os.flush();

    return true;
}

void LogWriter::closeLogFile()
{
    if (mLogFile.isOpen()) {
        mLogFile.close();
    }
}

bool LogWriter::isLogOpen()
{
    return mLogFile.isOpen();
}
