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

#include "telnetserver.h"

static QString CRLF ="\r\n";

TelnetServer::TelnetServer(Settings *settings, DatabaseApi *db, QObject *parent) :
    QObject(parent)
{
    _hostname = QHostAddress::Any;
    _listen_port = CONTROL_PORT;
    _stop=false;
    _server = new QTcpServer;
    _db = db;
    if(settings->_control_port != 0)
        _server->listen(_hostname,settings->_control_port);
    else
        _server->listen(_hostname,_listen_port);
    QObject::connect(_server,SIGNAL(newConnection()),this,SLOT(getConnection()));
    qDebug() << "Telnet server init completed";
}

TelnetServer::~TelnetServer()
{
    _server->close();
    delete _server;
    qDebug() << "Telnet server exiting";
}

void TelnetServer::stop()
{
    _stop = true;
    for(int i =0;i<_connected_clients.size();i++)
    {
        QTcpSocket *s = _connected_clients.at(i);
        s->write("Server is stopping now.");
        s->disconnectFromHost();
    }
    _connected_clients.clear();
}

void TelnetServer::getConnection()
{
    // ok
    QTcpSocket *socket = _server->nextPendingConnection();
    qDebug() << "Incoming connection" << socket->peerAddress().toString();
    if(socket->state() == QTcpSocket::ConnectedState)
        qDebug() << "Connection established";

    QObject::connect(socket,SIGNAL(error(QAbstractSocket::SocketError )),this,SLOT(connectionFailed(QAbstractSocket::SocketError)));
    QObject::connect(socket,SIGNAL(connected()),this,SLOT(connectionSuccess()));
    QObject::connect(socket,SIGNAL(readyRead()),this,SLOT(processData()));
    QObject::connect(socket, SIGNAL(disconnected()), socket, SLOT(deleteLater()));

    _connected_clients.append(socket);

}

void TelnetServer::connectionFailed(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    // ok
    QTcpSocket *socket = dynamic_cast<QTcpSocket*>(QObject::sender());
    qDebug() << "Connection error: " << socket->errorString();
    int i = _connected_clients.indexOf(socket);
    _connected_clients.remove(i);

}

void TelnetServer::connectionSuccess()
{
    QTcpSocket *socket = dynamic_cast<QTcpSocket*>(QObject::sender());
    qDebug() << "Connection established: " << socket->peerAddress().toString() << ":" << socket->peerPort();
}


void TelnetServer::processData()
{

    QTcpSocket *socket = dynamic_cast<QTcpSocket*>(QObject::sender());
    QByteArray data;

    bool endOfLine = false;

    while ((!endOfLine))
    {
        char ch;
        if(socket->bytesAvailable()>0)
        {
            int bytesRead = socket->read(&ch, sizeof(ch));
            if (bytesRead == sizeof(ch))
            {
                data.append(ch);
                if (socket->bytesAvailable()==0)
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
    qDebug() << "Good message from: " << socket->peerAddress().toString();
    QByteArray response = processCommand(data);
    socket->write(response.data(),response.size());

}

QByteArray TelnetServer::processCommand(QByteArray data)
{
    quint8 type = static_cast<quint8>(data.at(0));
    quint8 size = static_cast<quint8>(data.at(1));
    QByteArray command = data.remove(0,2);
    if(size < 2)
    {
        qDebug() << "Invalid command " << type;
        return NULL;
    }
    if(type == static_cast<quint8>(Parameters))
    {
        QRadioLink::Parameters param;
        param.ParseFromArray(command.data(),command.size());
        Station* s=_db->get_station_by_id(param.station_id());
        param.set_caller_id(s->called_by);
        param.set_in_call(s->in_call);
        param.set_channel_id(s->channel_id);
        quint8 size = param.ByteSize();
        char bin_data[size+2];
        param.SerializeToArray(bin_data+2, size);
        bin_data[0] = static_cast<char>(Parameters);
        bin_data[1] = static_cast<char>(size);
        return QByteArray(bin_data, size+2);
    }
    else if(type == static_cast<quint8>(JoinConference))
    {
        QRadioLink::JoinConference join;
        join.ParseFromArray(command.data(),command.size());
        emit joinConference(join.channel_id(),join.caller_id(),join.server_id());
        return QByteArray(0);
    }
    else if(type == static_cast<quint8>(LeaveConference))
    {
        QRadioLink::LeaveConference leave;
        leave.ParseFromArray(command.data(),command.size());
        if(leave.leave())
            emit leaveConference(0,0,0);
        return QByteArray(0);
    }
    else
    {
        return QByteArray(0);
    }
}
