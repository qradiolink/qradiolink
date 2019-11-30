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

#include "telnetclient.h"


static QString CRLF ="\r\n";

TelnetClient::TelnetClient(QObject *parent) :
    QObject(parent)
{
    _connection_tries=0;
    _status=0;
    _hostname = "127.0.0.1";
    _port= CONTROL_PORT;
    _socket = new QTcpSocket;
    QObject::connect(_socket,SIGNAL(error(QAbstractSocket::SocketError )),
                     this,SLOT(connectionFailed(QAbstractSocket::SocketError)));
    QObject::connect(_socket,SIGNAL(connected()),this,SLOT(connectionSuccess()));
    QObject::connect(_socket,SIGNAL(readyRead()),this,SLOT(processData()));

}


TelnetClient::~TelnetClient()
{
    _socket->disconnectFromHost();
    delete _socket;
}

int TelnetClient::connectionStatus()
{
    return _status;
}


void TelnetClient::connectionSuccess()
{
    qDebug() << "Successfull outgoing connection";
    _status=1;
    _connection_tries=0;
    emit connectedToHost();
}


void TelnetClient::connectionFailed(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    _status=0;
    qDebug() << "Outgoing connection failed" << _socket->errorString();
    _connection_tries++;
    if(_connection_tries < 2)
    {
        this->connectHost(_hostname,_port);
    }
    else
    {
        emit connectionFailure();
    }

}


void TelnetClient::connectHost(const QString &host, const unsigned &port)
{
    qDebug() << "trying " << host;
    if(_status==1) return;
    _socket->connectToHost(host, port);
    _hostname = host;
    _port = port;

}

void TelnetClient::disconnectHost()
{
    if(_status==0)
        return;
    _socket->disconnectFromHost();
    _status=0;
    _connection_tries=0;
    emit disconnectedFromHost();
}


void TelnetClient::send(QString prop_name, QString value)
{
    QString command = prop_name + ";" + value + CRLF;
    _socket->write(command.toUtf8());
    _socket->flush();

}

void TelnetClient::sendBin(const char *payload, int size)
{
    _socket->write(payload, size);
    _socket->flush();
}

void TelnetClient::processData()
{
    if (_status !=1) return;
    QByteArray data;

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
                    if (_socket->bytesAvailable()==0)
                    {
                        endOfLine = true;
                    }
                    data.append( ch );
                }
            }
            else
            {
                break;
            }
        }
    }
    qDebug() << "Good message from " << _socket->peerAddress().toString();
    emit haveMessage(data);
}


