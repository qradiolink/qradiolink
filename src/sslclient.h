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

#ifndef SSLCLIENT_H
#define SSLCLIENT_H

#include <QSslSocket>
#include <QByteArray>
#include <QSslCipher>
#include <QSslCertificate>
#include <QAbstractSocket>
#include <QUdpSocket>
#include <QHostAddress>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTime>
#include <QTimer>
#include <QCoreApplication>
#include <unistd.h>
#include "config_defines.h"

class SSLClient : public QObject
{
    Q_OBJECT
public:
    explicit SSLClient(QObject *parent = 0);
    ~SSLClient();

    void sendBin(quint8 *payload, quint64 size);
    void sendUDP(quint8 *payload, quint64 size);
    int connectionStatus();
    void disconnectHost();
    QSslCipher getCipher();
    unsigned inline status() {return _status;}

public slots:
    void processData();
    void connectHost(const QString &host, const unsigned &port);
    void readPendingDatagrams();

signals:
    void connectionFailure();
    void connectedToHost();
    void haveMessage(QByteArray buf);
    void haveUDPData(QByteArray buf);
    void disconnectedFromHost();
    void logMessage(QString log_msg);

private:
    QSslSocket *_socket;
    QUdpSocket *_udp_socket;
    unsigned _connection_tries;
    unsigned _status;
    QString _hostname;
    unsigned _port;
    bool _reconnect;


private slots:
    void connectionSuccess();
    void connectionFailed(QAbstractSocket::SocketError);
    void sslError(QList<QSslError> errors);
    void tryReconnect();

};

#endif // SSLCLIENT_H
