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

CommandProcessor::CommandProcessor(const Settings *settings, QObject *parent)
    : QObject(parent)
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


QStringList CommandProcessor::listAvailableCommands()
{
    QStringList list;
    for(int i=0;i<_command_list->size();i++)
    {
        list.append("\e[33m" + _command_list->at(i)->action
                    + QString(" (%1 parameters)").arg(
                        _command_list->at(i)->params) + "\e[0m\n");
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
        if((_command_list->at(i)->action == action)
                && (_command_list->at(i)->params == (tokens.length() - 1)))
        {
            command_index = i;
            break;
        }
    }
    return tokens;
}

bool CommandProcessor::validateCommand(QString message)
{
    QRegularExpression re("^[a-zA-Z0-9_]+[\\sa-zA-Z0-9_]*\\r\\n$");
    QRegularExpressionValidator validator(re, 0);

    int pos = 0;
    if(QValidator::Acceptable != validator.validate(message, pos))
    {
        // regex match failed
        return false;
    }
    int command_index = -1;
    getCommand(message, command_index);

    if(command_index < 0)
    {
        // command not found
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


    /// Actual command processing
    ///
    processStatusCommands(command_index, response);
    processActionCommands(command_index, response);

    if(response.length() < 1)
        response = "Command not found";

    return "\e[32m" + response + "\e[0m\n";
}

void CommandProcessor::processStatusCommands(int command_index, QString &response)
{
    switch (command_index) {
    case 0:
        if(_settings->_rx_status)
            response.append("RX status is active.");
        else
            response.append("RX status is inactive.");
        break;
    case 1:
        if(_settings->_tx_status)
            response.append("TX status is active.");
        else
            response.append("TX status is inactive.");
        break;
    case 2:
        if(_settings->_in_transmission)
            response.append("Currently transmitting.");
        else
            response.append("Not transmitting.");
        break;
    case 3:
        response.append(QString("Current RX mode is %1.").arg(_settings->rx_mode));
        break;
    case 4:
        response.append(QString("Current TX mode is %1.").arg(_settings->tx_mode));
        break;
    case 5:
        response.append(QString("Current RX CTCSS tone is %1.").arg(_settings->rx_ctcss));
        break;
    case 6:
        response.append(QString("Current TX CTCSS tone is %1.").arg(_settings->tx_ctcss));
        break;
    case 7:
        response.append(QString("Current RX volume is %1.").arg(_settings->rx_volume));
        break;
    case 8:
        response.append(QString("Current TX volume is %1.").arg(_settings->tx_volume));
        break;
    case 9:
        response.append(QString("Current Squelch value is %1.").arg(_settings->squelch));
        break;
    case 10:
        response.append(QString("Current RX gain is %1.").arg(_settings->rx_sensitivity));
        break;
    case 11:
        response.append(QString("Current TX gain value is %1.").arg(_settings->tx_power));
        break;
    case 12:
        response.append(QString("Current RSSI value is %1.").arg(_settings->_rssi));
        break;
    case 13:
        if(_settings->_voip_connected)
        {
            response.append(
                QString("Connected to VOIP server: %1; Channel number: %2.").arg(
                        _settings->voip_server).arg(_settings->_current_voip_channel));
        }
        else
        {
            response.append("Not connected to VOIP network");
        }

        break;
    case 14:
        if(_settings->_voip_forwarding)
            response.append(QString("Radio is forwarded to VOIP network."));
        else
            response.append(QString("Radio is not forwarded to VOIP network."));
        break;
    case 15:
        if(_settings->_vox_enabled)
            response.append(QString("VOX is on."));
        else
            response.append(QString("VOX is off."));
        break;
    case 16:
        if(_settings->_repeater_enabled)
            response.append(QString("Repeater is enabled."));
        else
            response.append(QString("Repeater is disabled."));
        break;

    default:
        break;
    }
}

void CommandProcessor::processActionCommands(int command_index, QString &response)
{
    switch (command_index) {
    case 17:
        break;
    case 18:
        break;
    case 19:
        break;
    default:
        break;
    }
}

void CommandProcessor::buildCommandList()
{
    _command_list->append(new command("rxstatus", 0));
    _command_list->append(new command("txstatus", 0));
    _command_list->append(new command("txactive", 0));
    _command_list->append(new command("rxmode", 0));
    _command_list->append(new command("txmode", 0));
    _command_list->append(new command("rxctcss", 0));
    _command_list->append(new command("txctcss", 0));
    _command_list->append(new command("rxvolume", 0));
    _command_list->append(new command("txvolume", 0));
    _command_list->append(new command("squelch", 0));
    _command_list->append(new command("rxgain", 0));
    _command_list->append(new command("txgain", 0));
    _command_list->append(new command("rssi", 0));
    _command_list->append(new command("voipstatus", 0));
    _command_list->append(new command("forwardingstatus", 0));
    _command_list->append(new command("voxstatus", 0));
    _command_list->append(new command("repeaterstatus", 0));


    _command_list->append(new command("rxinit", 1));
    _command_list->append(new command("txinit", 1));
    _command_list->append(new command("rxtune", 1));

}
