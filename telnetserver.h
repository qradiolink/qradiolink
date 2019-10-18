// Written by Adrian Musceac YO8RZZ , started October 2013.
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

#ifndef TELNETSERVER_H
#define TELNETSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QString>
#include <QStringList>
#include <QRegularExpressionValidator>
#include <QTime>
#include <QAbstractSocket>
#include <QElapsedTimer>
#include <QVector>
#include <QHostAddress>
#include <QDebug>
#include <QCoreApplication>
#include "config_defines.h"
#include "ext/dec.h"
#include "settings.h"
#include "station.h"
#include "commandprocessor.h"
#include "logger.h"

class TelnetServer : public QObject
{
    Q_OBJECT
public:
    explicit TelnetServer(const Settings *settings, Logger *logger, QObject *parent = 0);
    ~TelnetServer();

    CommandProcessor *command_processor; // public needed for signals and slots


signals:
    void finished();

public slots:
    void stop();
    void start();

private slots:
    void getConnection();
    void connectionFailed(QAbstractSocket::SocketError error);
    void connectionSuccess();
    void processData();

private:
    const Settings *_settings;
    Logger *_logger;
    QTcpServer *_server;
    QVector<QTcpSocket*> _connected_clients;
    QTcpSocket _socket;
    int _status;
    bool _stop;
    QHostAddress _hostaddr;


    QByteArray processCommand(QByteArray data, QTcpSocket *socket);
    void getCommandList(QByteArray &response);
};

#endif // TELNETSERVER_H
