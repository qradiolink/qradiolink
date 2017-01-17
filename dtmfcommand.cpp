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

#include "dtmfcommand.h"

DtmfCommand::DtmfCommand(Settings *settings, DatabaseApi *db, MumbleClient *mumble, QObject *parent) :
    QObject(parent)
{
    _settings = settings;
    _db = db;

    _conference_stations = new QVector<Station*>;
    _dialing_number = "";
    _current_station = NULL;
    _mumble = mumble;
    _conference_id = -1;
}

DtmfCommand::~DtmfCommand()
{
    _mumble->disconnectFromServer();
    delete _conference_stations;
}

void DtmfCommand::haveCall(QVector<char> *dtmf)
{


    std::string number;
    for(int i=0;i<dtmf->size();i++)
    {
        if((dtmf->at(i)!='*') && (dtmf->at(i)!='C') && (dtmf->at(i)!='D'))
        {
            number.push_back(dtmf->at(i));
            emit speak(QString(dtmf->at(i)));
        }
    }

    dtmf->clear();

    emit readyInput();
    _dialing_number = number;
    if(_conference_id < 2)
    {
        QString channel_name = _mumble->createChannel();
    }
    else
    {
        channelReady(_conference_id);
    }

}

void DtmfCommand::channelReady(int chan_number)
{
    Q_UNUSED(chan_number);
    if(_dialing_number =="")
        return;
    _conference_id = _mumble->getChannelId();

    qDebug() << "Conference created: " << _conference_id;
    int res;
    res = _mumble->callStation(QString::fromStdString(_dialing_number));
    if(res < 0)
    {
        QString voice= "I cannot connect to host.";
        emit speak(voice);
        _conference_id = -1;
        _mumble->disconnectFromCall();
    }
    else if(res > 1)
    {
        _mumble->joinChannel(res);
    }
    else
    {
        QString voice= "Joining conference.";
        emit speak(voice);
    }
    _dialing_number ="";
}

void DtmfCommand::haveCommand(QVector<char> *dtmf)
{
    std::string number;
    for(int i=0;i<dtmf->size();i++)
    {
        if((dtmf->at(i)!='*') && (dtmf->at(i)!='D'))
        {
            number.push_back(dtmf->at(i));
        }
    }
    dtmf->clear();

    emit readyInput();

    if(number=="B")
    {
        QString voice= "Disconnecting from the conference.";
        emit speak(voice);
        _conference_id = -1;
        _mumble->disconnectFromCall();
        return;

    }

    if(number=="A")
    {
        QString voice= "Disconnecting all stations from the conference.";
        emit speak(voice);
        _conference_id = -1;
        _mumble->disconnectAllStations();
        _mumble->disconnectFromCall();
        return;
    }
    if(number=="#9")
    {
        emit tellStations();
        return;
    }
    int res;
    Q_UNUSED(res);
    res = _mumble->disconnectStation(QString::fromStdString(number));
    QString voice= "Station "+QString::fromStdString(number) +" was disconnected.";
    emit speak(voice);

}

void DtmfCommand::newStation(Station *s)
{
    QString voice = "Station connected:  " + s->_radio_id;
    emit speak(voice);
}

void DtmfCommand::leftStation(Station *s)
{
    QString voice = "Station " + s->_radio_id + " disconnected:  ";
    emit speak(voice);
}
