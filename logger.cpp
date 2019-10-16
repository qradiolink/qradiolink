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

#include "logger.h"


#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
void logMessage(QtMsgType type, const QMessageLogContext &context,
                const QString &msg)
{
    Q_UNUSED(context);
#else
void logMessage(QtMsgType type, const char *msg)
{
#endif
    QString time= QDateTime::currentDateTime().toString(
                "d/MMM/yyyy hh:mm:ss");
    QString txt;
    //log_file->open(QIODevice::WriteOnly | QIODevice::Append);
    switch (type) {
    case QtInfoMsg:
        txt = QString("[%1] Info: %2").arg(time).arg(msg);
        break;
    case QtDebugMsg:
        txt = QString("[%1] Debug: %2").arg(time).arg(msg);
        break;
    case QtWarningMsg:
        txt = QString("[%1] Warning: %2").arg(time).arg(msg);
    break;
    case QtCriticalMsg:
        txt = QString("[%1] Critical: %2").arg(time).arg(msg);
    break;
    case QtFatalMsg:
        txt = QString("[%1] Fatal: %2").arg(time).arg(msg);
    break;
    }

    QFile outFile("qradiolink.log");
    outFile.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream ts(&outFile);
    ts << txt << endl;
    outFile.close();
}

Logger::Logger()
{

    _log_file = new QFile("/var/log/qradiolink.log");

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    qInstallMessageHandler(logMessage);
#else
    qInstallMsgHandler(logMessage);
#endif


}
