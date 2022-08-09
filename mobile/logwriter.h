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

#ifndef LOGWRITER_H
#define LOGWRITER_H

#include <QObject>
#include <QFile>

class LogWriter : public QObject
{
    Q_OBJECT
public:
    explicit LogWriter(QObject *parent = nullptr);
    ~LogWriter();

    Q_INVOKABLE bool openLogFile(QString fileName);
    Q_INVOKABLE bool writeToLogFile(QString text);
    Q_INVOKABLE void closeLogFile();
    Q_INVOKABLE bool isLogOpen();

signals:

private:
    QFile mLogFile;

};

#endif // LOGWRITER_H
