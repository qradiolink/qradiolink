// Written by Adrian Musceac YO8RZZ , started August 2019.
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

#include "commandprocessor.h"

CommandProcessor::CommandProcessor(const Settings *settings, QObject *parent) : QObject(parent)
{
    _settings = settings;
    _command_list = new QVector<command*>;
    buildCommandList();
}

CommandProcessor::~CommandProcessor()
{
    for(int i=0;i<_command_list->length();i++)
    {
        delete _command_list->at(i);
    }
    _command_list->clear();
    delete _command_list;
}

void CommandProcessor::buildCommandList()
{
    _command_list->append(new command("rxstatus", 0));
    _command_list->append(new command("txstatus", 0));
    _command_list->append(new command("txactive", 0));
}

QStringList CommandProcessor::listAvailableCommands()
{
    QStringList list;
    for(int i=0;i<_command_list->size();i++)
    {
        list.append(_command_list->at(i)->action + "\n");
    }
    return list;
}

QStringList CommandProcessor::getCommand(QString message, int &command_index)
{
    command_index = -1;
    message = message.trimmed();
    QStringList tokens = message.split(" ");
    if((tokens.length() > 4) || (tokens.length() < 1))
    {
        QStringList none("");
        return none;
    }

    QString action = tokens.at(0);

    for(int i=0; i< _command_list->length();i++)
    {
        if(_command_list->at(i)->action == action)
        {
            command_index = i;
            break;
        }
    }
    return tokens;
}

bool CommandProcessor::validateCommand(QString message)
{
    QRegularExpression re(
                "[a-zA-Z]+\\s*[a-zA-Z0-9]*\\s*[a-zA-Z0-9]*\\s+[a-zA-Z0-9]*\\s*[a-zA-Z0-9]*\\s*");
    QRegularExpressionValidator validator(re, 0);

    int pos = 0;
    if(QValidator::Acceptable != validator.validate(message, pos))
    {
        return false;
    }
    int command_index = -1;
    getCommand(message, command_index);

    if(command_index < 0)
    {
        return false;
    }
    return true;
}

QString CommandProcessor::runCommand(QString message)
{
    int command_index;
    QString response;
    QStringList tokens = getCommand(message, command_index);
    QString param1, param2, param3;
    if(tokens.size() > 1)
    {
        param1 = tokens.at(1);
    }
    if(tokens.size() > 2)
    {
        param2 = tokens.at(2);
    }
    if(tokens.size() > 3)
    {
        param3 = tokens.at(3);
    }
    command *run = _command_list->at(command_index);


    /// Actual command processing
    ///
    switch (command_index) {

    case 0:
        if(_settings->_rx_status)
            response.append("RX status is active.\n");
        else
            response.append("RX status is inactive.\n");
        break;

    case 1:
        if(_settings->_tx_status)
            response.append("TX status is active.\n");
        else
            response.append("TX status is inactive.\n");
        break;

    case 2:
        if(_settings->_in_transmission)
            response.append("Currently transmitting.\n");
        else
            response.append("Not transmitting.\n");
        break;

    default:
        response.append("Command not found\n");
        break;
    }
    return response;
}
