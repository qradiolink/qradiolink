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

#include "controller.h"

Controller::Controller(Settings *settings, DatabaseApi *db, MumbleClient *mumble, QObject *parent) : QObject(parent)
{
    _settings = settings;
    _db = db;
#ifndef MUMBLE
    _client = new IaxClient;
#else

#endif
    _conference_stations = new QVector<Station*>;
    _dialing_number = "";
    _connectable = false;
    _telnet = NULL;
    _current_station = NULL;
    Station *s = _db->get_local_station();
    _id = s->_id;
    delete s;
    _telnet = new TelnetClient;

    _mumble = mumble;
    _conference_id = -1;
}

Controller::~Controller()
{
#ifndef MUMBLE
    _client->end();
#endif
    _mumble->disconnectFromServer();
    delete _telnet;
    delete _conference_stations;
#ifndef MUMBLE
    delete _client;
#endif
}

void Controller::haveCall(QVector<char> *dtmf)
{

    bool p2p = false;
    Q_UNUSED(p2p);
    std::string number;
    for(int i=0;i<dtmf->size();i++)
    {
        if((dtmf->at(i)!='*') && (dtmf->at(i)!='C') && (dtmf->at(i)!='D'))
        {
            number.push_back(dtmf->at(i));
            emit speak(QString(dtmf->at(i)));
        }
    }
    if(dtmf->last()=='D')
    {
        p2p =true;
    }
    dtmf->clear();
    emit readyInput();
    _dialing_number = number;
    Station *s  = _db->get_station_by_radio_id(QString::fromStdString(number));

    if(s->_id != 0)
    {
        if(s->_active != 1)
        {
            QString voice= "The station you have tried to call is not active.";
            emit speak(voice);
            return;
        }
        if(!testConnection(s->_ip))
            return;
        getStationParameters(s);
        while(_current_station->_waiting==1)
        {
            QCoreApplication::processEvents();
        }
#ifndef MUMBLE
        if(p2p && (s->_in_call == 0))
        {
            QString voice= "Trying a direct call.";
            emit speak(voice);
            _client->init();
            _client->setProperties("test","test",s->_ip);
            _client->makeCall("777");
            _in_conference =1;
            _conference_id = "777";
            _conference_stations->append(s);
            return;
        }
#endif

        QVector<Server*> servers = _db->get_servers();
        if(servers.size() < 1)
        {
            QString voice= "There are no active servers in the list.";
            emit speak(voice);
            return;
        }
        // TODO: how do we pick a server?
        Server *server = servers[0];
        if(_current_station->_in_call == 1)
        {
            // check if the station is already in conference;
            // if it is, just join it
            // if we're calling a third party, notify our peers and join it's conference,
            // or notify the third party of our conference number
            // FIXME: what to do if we have two peers in separate conferences

            QString voice= "Joining the station in the conference.";
            emit speak(voice);
#ifdef MUMBLE
            _mumble->connectToServer(server->_ip,_settings->_voice_server_port);
            _mumble->joinChannel(_current_station->_conference_id);
#else
            _client->init();
            _client->setProperties(server->_username,server->_password,server->_ip);
            _client->makeCall(s->_conference_id.toStdString());
#endif
            _in_conference =1;
            _conference_id = _current_station->_conference_id;
            _conference_stations->append(_current_station);
            Station *st = _db->get_local_station();
            st->_called_by=0;
            st->_conference_id=_conference_id;
            st->_in_call=1;
            _db->update_station_parameters(st);
            delete st;
        }
        else
        {
            // if it's not, get the number of the first free conference and make a new conference
#ifdef MUMBLE
            _mumble->connectToServer(server->_ip,_settings->_voice_server_port);
            QString channel = _mumble->createChannel();
            if(channel.length() > 1)
            {
                _conference_id = _mumble->getChannelId();
            }
#else
            _client->init();
            _client->setProperties(server->_username,server->_password,server->_ip);
            _client->makeCall(_conference_id.toStdString());
             QObject::connect(_client,SIGNAL(callEnded()),this,SLOT(disconnectedFromCall()));
#endif
             QRadioLink::JoinConference join;
             join.set_caller_id(_id);
             join.set_channel_id(_conference_id);
             join.set_server_id(server->_id);
             int size = join.ByteSize();
             char data[size+2];
             join.SerializeToArray(data+2, size);
             data[0]= JoinConference;
             data[1] = size;
             QByteArray bin_data(data,size+2);
            _telnet->sendBin(bin_data.constData(), bin_data.size());
            QString voice= "Calling the station into the conference.";
            emit speak(voice);


            _in_conference =1;
            _conference_stations->append(s);
            Station *st = _db->get_local_station();
            st->_called_by=0;
            st->_conference_id=_conference_id;
            st->_in_call=1;
            _db->update_station_parameters(st);
            delete st;
        }

    }
    else
    {
        QString voice= "I can't find the station with this number.";
        emit speak(voice);
        delete s;
    }
}


void Controller::haveCommand(QVector<char> *dtmf)
{
    std::string number;
    for(int i=0;i<dtmf->size();i++)
    {
        if((dtmf->at(i)!='*') && (dtmf->at(i)!='#'))
        {
            number.push_back(dtmf->at(i));
        }
    }
    dtmf->clear();
    emit readyInput();

    if(number=="9")
    {
        QString voice= "Disconnecting from the conference.";
        emit speak(voice);
        disconnectedFromCall();
#ifndef MUMBLE
        _client->disconnectCall();
#endif
        _mumble->disconnectFromServer();

    }
    if(number=="99")
    {
        QString voice= "Disconnecting from the conference.";
        emit speak(voice);
#ifndef MUMBLE
        _client->disconnectCall();
#endif
        _mumble->disconnectFromServer();
    }

}

bool Controller::testConnection(QString host)
{
    _telnet->disconnectHost();
    QObject::connect(_telnet,SIGNAL(connectedToHost()),this,SLOT(readyConnect()));
    QObject::connect(_telnet,SIGNAL(connectionFailure()),this,SLOT(noConnection()));
    QObject::connect(_telnet,SIGNAL(disconnectedFromHost()),this,SLOT(disconnectedLink()));
    _telnet->connectHost(host,_settings->_control_port);
    int time = QDateTime::currentDateTime().toTime_t();
    while ((QDateTime::currentDateTime().toTime_t() - time) < 5)
    {
        QCoreApplication::processEvents();
        if(_connectable)
        {

            _connectable = false;
            return true;
        }
    }

    QString voice= "I cannot connect to host.";
    emit speak(voice);
    _connectable = false;


    return false;
}

void Controller::noConnection()
{
    QString voice= "Network error.";
    emit speak(voice);
    _connectable = false;
    delete _telnet;
    _telnet = NULL;
}

void Controller::readyConnect()
{
    _connectable = true;
}

void Controller::getStationParameters(Station *s)
{
    _current_station = s;
    _current_station->_waiting=1;
    qDebug() << "Getting station information";
    QObject::connect(_telnet,SIGNAL(haveMessage(QByteArray)),this,SLOT(setStationParameters(QByteArray)));
    QRadioLink::Parameters params;
    params.set_station_id(s->_id);
    int size = params.ByteSize();
    char data[size+2];
    params.SerializeToArray(data+2, size);
    data[0] = static_cast<char>(Parameters);
    data[1] = static_cast<char>(size);
    QByteArray bin_data(data, size+2);
    _telnet->sendBin(bin_data.constData(), bin_data.size());
}

void Controller::setStationParameters(QByteArray data)
{
    qDebug() << "Configuring peer station";
    quint8 type = static_cast<quint8>(data.at(0));
    if(type!=static_cast<quint8>(Parameters))
    {
        qDebug() << "invalid message";
        return;
    }
    data.remove(0,2);
    QRadioLink::Parameters param;
    param.ParseFromArray(data.constData(),data.size());
    _current_station->_in_call=param.in_call();
    _current_station->_conference_id=param.channel_id();
    _current_station->_called_by = param.caller_id();
    _current_station->_waiting=0;
    _db->update_station_parameters(_current_station);
    QObject::disconnect(_telnet,SIGNAL(haveMessage(QByteArray)),this,SLOT(setStationParameters(QByteArray)));
}

int Controller::getChannel()
{
    return _conference_id; //TODO:
}

void Controller::joinConference(int number, int id, int server_id)
{
    qDebug() << server_id;
    Server *server = _db->get_server_by_id(server_id);

    if(server->_id < 1)
    {

        QString voice= "There are no active servers in the list.";
        emit speak(voice);
        /** FIXME: no valid server is returned
        return;
        */
    }
    qDebug() << server->_id;
    server->_ip="192.168.1.2";
    QString voice= "Joining conference.";
    emit speak(voice);
    Station *s = _db->get_local_station();
    s->_called_by=id;
    s->_conference_id=number;
    s->_in_call=1;
    _db->update_station_parameters(s);
    delete s;
    s = _db->get_station_by_id(id);
    QString caller = s->_callsign;
    delete s;
    voice = "Called by " + caller;
    emit speak(voice);


#ifdef MUMBLE
    _mumble->connectToServer(server->_ip,_settings->_voice_server_port);
    _mumble->joinChannel(number);
#else
    QObject::connect(_client,SIGNAL(callEnded()),this,SLOT(disconnectedFromCall()));
    _client->init();
    //_client->setProperties(server->_username,server->_password,server->_ip);
    _client->setProperties("adrian","supersecret","192.168.1.2");
    _client->makeCall(number.toStdString());
#endif
    _in_conference =1;

    //_conference_stations->append(_current_station);
}

void Controller::disconnectedFromCall()
{
    QString voice= "Conference has ended.";
    emit speak(voice);
#ifndef MUMBLE
    QObject::disconnect(_client,SIGNAL(callEnded()),this,SLOT(disconnectedFromCall()));
#endif
    if(_in_conference==1)
    {
        for(int i =0;i<_conference_stations->size();i++)
        {
            Station *s=_conference_stations->at(i);
            if(s->_called_by==_id)
            {
                //FIXME:
                _telnet->disconnectHost();
                _telnet->connectHost(s->_ip,_settings->_control_port);
                int retries = 0;
                while((_telnet->connectionStatus()!=1) && (retries<3))
                {
                    QCoreApplication::processEvents();
                    retries++;
                }
                if(_telnet->connectionStatus()==1)
                {
                    QRadioLink::LeaveConference msg_leave;
                    msg_leave.set_leave(true);
                    int size = msg_leave.ByteSize();
                    char data[size+2];
                    msg_leave.SerializeToArray(data+2, size);
                    data[0] = LeaveConference;
                    data[1] = size;
                    QByteArray bin_msg(data, size+2);
                    _telnet->sendBin(bin_msg.constData(), bin_msg.size());
                }
            }
            s->_in_call=0;
            s->_called_by=0;
            s->_conference_id=-1;
            _db->update_station_parameters(s);
            delete s;
        }
        _conference_stations->clear();
    }
    _in_conference=0;
    Station *s = _db->get_local_station();
    s->_called_by=0;
    s->_conference_id=-1;
    s->_in_call=0;
    _db->update_station_parameters(s);
    delete s;
}

void Controller::disconnectedLink()
{

}

void Controller::leaveConference(int number, int id, int server_id)
{
    Q_UNUSED(number);
    Q_UNUSED(id);
    Q_UNUSED(server_id);
    disconnectedFromCall();
#ifndef MUMBLE
    _client->disconnectCall();
#endif
    _mumble->disconnectFromServer();
}
