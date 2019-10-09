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

CommandProcessor::CommandProcessor(Settings *settings, QObject *parent) : QObject(parent)
{
    _settings = settings;
    _command_list = new QVector<command*>;
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
        list.append(_command_list->at(i)->action);
    }
    return list;
}

bool CommandProcessor::validateCommand(QString message)
{
    return false;
}

QString CommandProcessor::runCommand(QString message)
{
    return "Command not recognized";
}
