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

#ifndef LOGREADER_H
#define LOGREADER_H

#include <QObject>
#include <QFile>

class LogReader : public QObject
{
    Q_OBJECT
public:
    explicit LogReader(QObject *parent = nullptr);
    ~LogReader();

    Q_INVOKABLE bool openLogFile(QString fileName);
    Q_INVOKABLE QString readLine();
    Q_INVOKABLE QString readAll();
    Q_INVOKABLE bool atEnd();
    Q_INVOKABLE void closeLogFile();
    Q_INVOKABLE bool isLogOpen();

signals:

private:
    QFile mLogFile;

};

#endif // LOGREADER_H
