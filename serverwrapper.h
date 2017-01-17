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

#ifndef SERVERWRAPPER_H
#define SERVERWRAPPER_H

#include <QObject>
#include <QVector>
#include <QString>
#include <mumbleclient.h>
#include <unistd.h>
#include "telnetserver.h"
#include "databaseapi.h"
#include "speech.h"
#include "mumbleclient.h"
#include "settings.h"
#include "station.h"


typedef QVector<Station> StationList;
class ServerWrapper : public QObject
{
    Q_OBJECT
public:
    explicit ServerWrapper(Settings *settings, DatabaseApi *db, QObject *parent = 0);
    void stop();

signals:
    void finished();
    void joinConference(int number, int id, int server_id);
    void leaveConference(int number, int id, int server_id);
    void pingServer();
public slots:
    void run();
    void addSpeech(QString);
    void connectToConference(int number, int id, int server_id);
    void disconnectFromConference(int number, int id, int server_id);
    void tellOnlineStations();
    void updateOnlineStations(StationList stations);

private:
    bool _stop;

    QVector<QString> *_speech_text;
    DatabaseApi *_db;
    Settings *_settings;
    Speech *_speaker;
};

#endif // SERVERWRAPPER_H
