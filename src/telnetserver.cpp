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

TelnetServer::TelnetServer(const Settings *settings, Logger *logger, RadioChannels *radio_channels,
                           QObject *parent) :
    QObject(parent)
{
    _settings = settings;
    _logger = logger;
    command_processor = new CommandProcessor(settings, logger, radio_channels);
    _server = new QTcpServer;
    _hostaddr = QHostAddress::LocalHost;
    _stop = false;
}

TelnetServer::~TelnetServer()
{
    _logger->log(Logger::LogLevelInfo, QString("Shutting down remote control interface"));
    delete _server;
    delete command_processor;
}

void TelnetServer::start()
{
    if(_server->isListening())
        return;
    bool status;
    if(_settings->control_port != 0)
        status = _server->listen(_hostaddr,_settings->control_port);
    else
        status = _server->listen(_hostaddr,CONTROL_PORT);
    if(!status)
        _logger->log(Logger::LogLevelWarning, QString(
            "Server could not bind to port %1, another instance is probably listening already"
            ).arg(_settings->control_port));
    QObject::connect(_server,SIGNAL(newConnection()),this,SLOT(getConnection()));
}


void TelnetServer::stop()
{
    if(!_server->isListening())
        return;
    _stop = true;
    for(int i =0;i<_connected_clients.size();i++)
    {
        QTcpSocket *s = _connected_clients[i];
        s->write("Server is stopping now.\n");
        s->flush();
        s->disconnectFromHost();
        delete s;
    }
     _connected_clients.clear();
     QObject::disconnect(_server,SIGNAL(newConnection()),this,SLOT(getConnection()));
     _server->close();
}

void TelnetServer::getConnection()
{
    // ok
    QTcpSocket *socket = _server->nextPendingConnection();
    _logger->log(Logger::LogLevelInfo, "Incoming connection from: "
              + socket->peerAddress().toString()
              + QString(" port: %1").arg(socket->peerPort()));
    if(socket->state() == QTcpSocket::ConnectedState)
    {
        _logger->log(Logger::LogLevelInfo, "Connection established");
        if(!_settings->gpredict_control)
        {
            QByteArray response;
            response.append("Welcome! ");
            getCommandList(response);
            response.append("qradiolink> ");
            socket->write(response);
            socket->flush();
        }
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
    _logger->log(Logger::LogLevelInfo, "Connection status: " + socket->errorString());
    int i = _connected_clients.indexOf(socket);
    _connected_clients.remove(i);
    if(_settings->gpredict_control)
    {
        command_processor->endGPredictControl();
    }

}

void TelnetServer::connectionSuccess()
{
    QTcpSocket *socket = dynamic_cast<QTcpSocket*>(QObject::sender());
    _logger->log(Logger::LogLevelInfo, "Connection established with: "
              + socket->peerAddress().toString()
              + QString(": %1").arg(socket->peerPort()));
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
           _logger->log(Logger::LogLevelWarning,
                        "Receiving packet too large... Dropping.");
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
                _logger->log(Logger::LogLevelCritical,"Telnet server socket read error");
                break;
            }
        }
        else
        {
            break;
        }
    }
    if(!_settings->gpredict_control)
    {
        _logger->log(Logger::LogLevelDebug, QString("Command message from %1 - %2").arg(
                     socket->peerAddress().toString()).arg(QString(data)));
    }

    QByteArray response = processCommand(data, socket);

    if(_settings->gpredict_control)
    {
        socket->write(response.data(),response.size());
        socket->flush();
        return;
    }

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
        response.append(available_commands.at(i).toLocal8Bit());
    }
    response.append("\n\n");
}

QByteArray TelnetServer::processCommand(QByteArray data, QTcpSocket *socket)
{

    /// sanity checks:
    if(data.length() > 1080) // not expecting novels
    {
        QByteArray response("");
        _logger->log(Logger::LogLevelWarning,
               QString("Received message to large (dropping) from: %1").arg(
                  socket->peerAddress().toString()));
        _connected_clients.remove(_connected_clients.indexOf(socket));
        socket->close();
        return response;
    }
    if(data.length() < 1)
    {
        QByteArray response("\n");
        return response;
    }

    QString message = QString::fromLocal8Bit(data);

    /// GPredict control logic:
    if(_settings->gpredict_control)
    {
        QString gpredict_result = command_processor->processGPredictMessages(message);
        QByteArray response;
        response.append(gpredict_result.toStdString().c_str());
        return response;
    }

    if(message == "\r\n")
    {
        QByteArray response("\n");
        return response;
    }
    if((message == "exit\r\n") || (message == "quit\r\n") || (message == "\u0004"))
    {
        QByteArray response("Bye!\n");
        socket->write(response.data(),response.size());
        socket->flush();
        _connected_clients.remove(_connected_clients.indexOf(socket));
        socket->close();
        QByteArray eof("EOF");
        return eof;
    }
    if((message == "help\r\n") || (message == "?\r\n"))
    {
        QByteArray response("Available commands:\n");
        getCommandList(response);
        return response;
    }



    /// poked processor logic:

    if(!command_processor->validateCommand(message))
    {
        QByteArray response("\e[31mCommand not recognized\e[0m\n");
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
