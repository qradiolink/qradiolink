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

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QObject>
#include <QString>
#ifndef MUMBLE
//#include "iaxclient.h"
#endif
#include "speech.h"
#include "telnetclient.h"
#include "databaseapi.h"
#include "station.h"
#include "config_defines.h"
#include "mumbleclient.h"
#include "ext/dec.h"
#include "ext/QRadioLink.pb.h"
#include "settings.h"

class Controller : public QObject
{
    Q_OBJECT

public:
    Controller(Settings *settings, DatabaseApi *db, MumbleClient *mumble, QObject *parent = 0);
    ~Controller();
    int getChannel();

public slots:
    void haveCall(QVector<char> *dtmf);
    void haveCommand(QVector<char> *dtmf);
    void readyConnect();
    void noConnection();
    void setStationParameters(QByteArray data);
    void joinConference(int number, int id, int server_id);
    void leaveConference(int number, int id, int server_id);
    void disconnectedFromCall();
    void disconnectedLink();
signals:
    void readyInput();
    void speak(QString);
private:
#ifndef MUMBLE
    IaxClient *_client;
#endif
    DatabaseApi *_db;
    int _in_conference;
    int _id;
    int _conference_id;
    QVector<Station*> *_conference_stations;
    std::string _dialing_number;
    bool _connectable;
    bool testConnection(QString host);
    void getStationParameters(Station *s);

    Station *_current_station;
    TelnetClient *_telnet;
    MumbleClient *_mumble;
    Settings *_settings;
};

#endif // CONTROLLER_H
