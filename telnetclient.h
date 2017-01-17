// Written by Adrian Musceac YO8RZZ at gmail dot com, started October 2013.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
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

#ifndef TELNETCLIENT_H
#define TELNETCLIENT_H

#include <QTcpSocket>
#include <QAbstractSocket>
#include <QHostAddress>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTime>
#include <QCoreApplication>
#include <QLatin1String>
#include "config_defines.h"

class TelnetClient : public QObject
{
    Q_OBJECT
public:
    explicit TelnetClient(QObject *parent = 0);
    ~TelnetClient();

    void send(QString prop_name, QString value);
    void sendBin(const char *payload, int size);
    int connectionStatus();
    void disconnectHost();
    unsigned inline status() {return _status;}

public slots:
    void processData();
    void connectHost(const QString &host, const unsigned &port);

signals:
    void connectionFailure();
    void connectedToHost();
    void haveMessage(QByteArray data);
    void disconnectedFromHost();

private:
    QTcpSocket *_socket;
    unsigned _connection_tries;
    unsigned _status;
    QString _hostname;
    unsigned _port;


private slots:
    void connectionSuccess();
    void connectionFailed(QAbstractSocket::SocketError error);

};

#endif // TELNETCLIENT_H
