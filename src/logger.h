// Written by Adrian Musceac YO8RZZ , started March 2016.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QDebug>
#include <QMutex>
#include <QFileInfo>
#include <QDateTime>
#include <iostream>


#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
void logMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg, QFile *log_file);
#else
void logMessage(QtMsgType type, const char *msg, QFile *log_file);
#endif


class Logger  : public QObject
{
    Q_OBJECT
public:
    enum
    {
        LogLevelInfo,
        LogLevelDebug,
        LogLevelWarning,
        LogLevelCritical,
        LogLevelFatal
    };
    explicit Logger(QObject *parent = 0);
    ~Logger();
    void log(int type, QString msg);
    void set_console_log(bool value);

signals:
    void applicationLog(QString msg);

private:
    QFile *_log_file;
    bool _console_log;
    QTextStream *_stream;
    QMutex _mutex;
};

#endif // LOGGER_H
