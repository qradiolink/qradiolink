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

#ifndef DATABASEAPI_H
#define DATABASEAPI_H

#include <QtSql>
#include <QVector>
#include <QString>
#include <math.h>
#include "station.h"
#include "server.h"
#include "settings.h"

class DatabaseApi
{
public:
    DatabaseApi();
    ~DatabaseApi();
    Station* get_station_by_radio_id(QString radio_id);
    Station* get_station_by_id(int id);
    Station* get_local_station();
    void update_station_parameters(Station *s);
    QVector<Server*> get_servers(int active = 1);
    Server* get_server_by_id(int id);
    Settings* get_settings();
    void clear_stations();
    void insert_station(Station s);
    QVector<Station> get_stations(int active=1);

private:
    QSqlDatabase _db;
};

#endif // DATABASEAPI_H
