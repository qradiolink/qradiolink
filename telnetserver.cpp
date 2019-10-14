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

TelnetServer::TelnetServer(const Settings *settings, QObject *parent) :
    QObject(parent)
{
    _settings = settings;
    command_processor = new CommandProcessor(settings);
    _hostaddr = QHostAddress::Any;
    _stop=false;
    _server = new QTcpServer;
}

TelnetServer::~TelnetServer()
{
    delete _server;
    delete command_processor;
}

void TelnetServer::start()
{
    if(_settings->control_port != 0)
        _server->listen(_hostaddr,_settings->control_port);
    else
        _server->listen(_hostaddr,CONTROL_PORT);
    QObject::connect(_server,SIGNAL(newConnection()),this,SLOT(getConnection()));
}


void TelnetServer::stop()
{
    _stop = true;
    for(int i =0;i<_connected_clients.size();i++)
    {
        QTcpSocket *s = _connected_clients.at(i);
        s->write("Server is stopping now.\n");
        s->flush();
        s->disconnectFromHost();
    }
     _connected_clients.clear();
    _server->close();
}

void TelnetServer::getConnection()
{
    // ok
    QTcpSocket *socket = _server->nextPendingConnection();
    std::cout << "Incoming connection from: "
              << socket->peerAddress().toString().toStdString()
              << " port: " << socket->peerPort() << std::endl;
    if(socket->state() == QTcpSocket::ConnectedState)
    {
        std::cout << "Connection established" << std::endl;
        QByteArray response;
        response.append("Welcome! ");
        getCommandList(response);
        response.append("qradiolink> ");
        socket->write(response);
        socket->flush();
    }

    QObject::connect(socket,SIGNAL(error(QAbstractSocket::SocketError )),
                     this,SLOT(connectionFailed(QAbstractSocket::SocketError)));
    QObject::connect(socket,SIGNAL(connected()),this,SLOT(connectionSuccess()));
    QObject::connect(socket,SIGNAL(readyRead()),this,SLOT(processData()));
    QObject::connect(socket, SIGNAL(disconnected()), socket, SLOT(deleteLater()));

    _connected_clients.append(socket);
}

void TelnetServer::connectionFailed(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);

    QTcpSocket *socket = dynamic_cast<QTcpSocket*>(QObject::sender());
    std::cout << "Connection status: " << socket->errorString().toStdString() << std::endl;
    int i = _connected_clients.indexOf(socket);
    _connected_clients.remove(i);

}

void TelnetServer::connectionSuccess()
{
    QTcpSocket *socket = dynamic_cast<QTcpSocket*>(QObject::sender());
    std::cout << "Connection established with: "
              << socket->peerAddress().toString().toStdString()
              << ":" << socket->peerPort() << std::endl;
}


void TelnetServer::processData()
{

    QTcpSocket *socket = dynamic_cast<QTcpSocket*>(QObject::sender());
    QByteArray data;
    QElapsedTimer timer;
    qint64 msec;
    timer.start();

    // FIXME: why the sequential read??!!!
    while (true)
    {
        msec = (quint64)timer.nsecsElapsed() / 1000000;
        if(msec > 2)
        {
            std::cerr << "Receiving packet too large... Dropping." << std::endl;
            _connected_clients.remove(_connected_clients.indexOf(socket));
            socket->close(); // TODO: blacklist?
            return;
        }
        char ch;
        if(socket->bytesAvailable()>0)
        {
            int bytesRead = socket->read(&ch, sizeof(ch));
            if (bytesRead == sizeof(ch))
            {
                data.append(ch);
                if (socket->bytesAvailable()==0)
                {
                    break;
                }
            }
            else
            {
                std::cerr << "Server socket read error" << std::endl;
                break;
            }
        }
        else
        {
            break;
        }
    }
    //qDebug() << "Message from: " << socket->peerAddress().toString();
    QByteArray response = processCommand(data, socket);
    if(response == "EOF")
        return;
    response.append("qradiolink> ");
    if(response.length() > 0)
    {
        socket->write(response.data(),response.size());
        socket->flush();
    }

}

void TelnetServer::getCommandList(QByteArray &response)
{
    QStringList available_commands = command_processor->listAvailableCommands();
    response.append("Available commands are: \n");
    for(int i=0;i<available_commands.length();i++)
    {
        response.append(available_commands.at(i));
    }
    response.append("\n\n");
}

QByteArray TelnetServer::processCommand(QByteArray data, QTcpSocket *socket)
{
    /// sanity checks:
    if(data.length() > 1080) // not expecting novels
    {
        QByteArray response("");
        std::cerr << "Received message to large (dropping) from: "
                  << socket->peerAddress().toString().toStdString() << std::endl;
        _connected_clients.remove(_connected_clients.indexOf(socket));
        socket->close();
        return response;
    }
    if(data.length() < 3)
    {
        QByteArray response("\n");
        return response;
    }

    QString message = QString::fromLocal8Bit(data);
    if((message == "exit\r\n") || (message == "quit\r\n"))
    {
        QByteArray response("Bye!\n");
        socket->write(response.data(),response.size());
        socket->flush();
        _connected_clients.remove(_connected_clients.indexOf(socket));
        socket->close();
        QByteArray eof("EOF");
        return eof;
    }

    /// poked processor logic:

    if(!command_processor->validateCommand(message))
    {
        QByteArray response("Command not recognized\n");
        getCommandList(response);
        return response;
    }

    QString result = command_processor->runCommand(message);
    if(result != "")
    {
        QByteArray response;
        response.append(result.toStdString().c_str());
        response.append("\n");
        return response;
    }
    else
    {
        QByteArray response("Command not processed\n");
        return response;
    }

}
