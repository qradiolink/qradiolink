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

#include "sslclient.h"


SSLClient::SSLClient(QObject *parent) :
    QObject(parent)
{
    _connection_tries=0;
    _status=0;
    _reconnect = false;
    _hostname = "127.0.0.1";
    _port= MUMBLE_PORT;
    QSslSocket::addDefaultCaCertificates(QSslSocket::systemCaCertificates());
    {
        QList<QSslCipher> pref;
        foreach(QSslCipher c, QSslSocket::defaultCiphers()) {
            if (c.usedBits() < 128)
                continue;
            pref << c;
        }
        if (pref.isEmpty())
            qFatal("No ciphers of at least 128 bit found");
        QSslSocket::setDefaultCiphers(pref);
    }
    _socket = new QSslSocket;
    _socket->setPeerVerifyMode(QSslSocket::QueryPeer);
    _socket->ignoreSslErrors();
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    _socket->setProtocol(QSsl::TlsV1_2);
#else
    _socket->setProtocol(QSsl::TlsV1);
#endif
    QObject::connect(_socket,SIGNAL(error(QAbstractSocket::SocketError )),
                     this,SLOT(connectionFailed(QAbstractSocket::SocketError)));
    QObject::connect(_socket,SIGNAL(disconnected()),this,SLOT(tryReconnect()));
    QObject::connect(_socket,SIGNAL(sslErrors(QList<QSslError>)),
                     this,SLOT(sslError(QList<QSslError>)));
    QObject::connect(_socket,SIGNAL(encrypted()),this,SLOT(connectionSuccess()));
    QObject::connect(_socket,SIGNAL(readyRead()),this,SLOT(processData()));

    _udp_socket = new QUdpSocket;

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    _udp_socket->bind(QHostAddress(QHostAddress::AnyIPv4),UDP_PORT);
#else
    _udp_socket->bind(QHostAddress(QHostAddress::Any),UDP_PORT);
#endif
    QObject::connect(_udp_socket, SIGNAL(readyRead()),
                     this, SLOT(readPendingDatagrams()));

}

SSLClient::~SSLClient()
{
    _reconnect = false;
    _socket->disconnectFromHost();
    delete _socket;
    delete _udp_socket;
}

int SSLClient::connectionStatus()
{
    return _status;
}

void SSLClient::connectionSuccess()
{
    emit logMessage("Successfull server connection");
    _status=1;
    _connection_tries=0;
    _reconnect = true;
    emit connectedToHost();
}

QSslCipher SSLClient::getCipher()
{
    return _socket->sessionCipher();
}

void SSLClient::connectionFailed(QAbstractSocket::SocketError)
{

    emit connectionFailure();
    emit logMessage(QString("Outgoing connection failed %1").arg(_socket->errorString()));
    if(_status==1)
    {
        _socket->close();
        _status=0;
        return;
    }
    else
    {
        QMetaObject::invokeMethod(this, "tryReconnect", Qt::QueuedConnection);
    }
}

void SSLClient::tryReconnect()
{
    if(!_reconnect)
        return;
    emit logMessage("Disconnected from server");
    _connection_tries++;
    // FIXME: blocks the main thread
    QTimer timer;
    timer.setSingleShot(true);
    timer.start(1000);
    while(timer.isActive())
    {
        QCoreApplication::processEvents();
    }
    if(_connection_tries < 2000000)
    {
        QMetaObject::invokeMethod(this, "connectHost", Qt:: QueuedConnection,
        Q_ARG(QString, _hostname), Q_ARG(const unsigned, _port));
    }
    else
    {
        return;
    }
}

void SSLClient::sslError(QList<QSslError> errors)
{
    emit logMessage("SSL errors occured");
    for(int i =0;i<errors.size();i++)
    {
        emit logMessage(errors.at(i).errorString());
    }
    const QSslCertificate cert = _socket->peerCertificate();
    _socket->peerCertificateChain() << cert;
}

void SSLClient::connectHost(const QString &host, const unsigned &port)
{
    if(_status==1)
    {
        _socket->close();
        _status=0;
    }
    emit logMessage("Trying connection to" + host);
    _socket->connectToHostEncrypted(host, port);
    _hostname = host;
    _port = port;
}

void SSLClient::disconnectHost()
{
    if(_status==0)
        return;
    QObject::disconnect(_socket,SIGNAL(disconnected()),this,SLOT(tryReconnect()));
    _socket->disconnectFromHost();
    _reconnect = false;
    _status=0;
    _connection_tries=0;
    emit disconnectedFromHost();
}

void SSLClient::sendBin(quint8 *payload, quint64 size)
{
    char *message = reinterpret_cast<char*>(payload);
    _socket->write(message,size);
    _socket->flush();
}

void SSLClient::processData()
{
    if (_status !=1) return;
    QByteArray buf;
    buf.append(_socket->readAll());
    bool endOfLine = false;
    while ((!endOfLine))
    {
        if(_status==1)
        {
            char ch;
            if(_socket->bytesAvailable()>0)
            {
                int bytesRead = _socket->read(&ch, sizeof(ch));
                if (bytesRead == sizeof(ch))
                {
                    buf.append( ch );
                    if(_socket->bytesAvailable()==0)
                    {
                        endOfLine = true;
                    }
                }
            }
            else
            {
                break;
            }
        }
        else
        {
            break;
        }

    }
    emit haveMessage(buf);
}

void SSLClient::readPendingDatagrams()
{
    while (_udp_socket->hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(_udp_socket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;
        _udp_socket->readDatagram(datagram.data(), datagram.size(),
                             &sender, &senderPort);
        emit haveUDPData(datagram);
    }
}

void SSLClient::sendUDP(quint8 *payload, quint64 size)
{
    char *message = reinterpret_cast<char*>(payload);

    quint64 sent = _udp_socket->writeDatagram(
                message,size,QHostAddress(_hostname),_port);
    _udp_socket->flush();
    emit logMessage(QString("SSL socket write failed, sent %1 bytes").arg(sent));
}
