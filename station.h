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

#ifndef STATION_H
#define STATION_H

#include <QString>

class Station
{
public:
    Station();
    int _id;
    QString _callsign;
    QString _radio_id;
    QString _ip;
    QString _hostname;
    int _in_call;
    int _conference_id;
    int _called_by;     // calling station id
    int _call_time;     // time of call
    int _repeater;
    int _local;
    int _active;
    int _waiting;
    bool _mute;
    bool _deaf;
};

#endif // STATION_H
