// Written by Adrian Musceac YO8RZZ at gmail dot com, started August 2013.
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

#ifndef APRSSTATION_H
#define APRSSTATION_H

#include <QString>

/**
 * @brief Object holding the properties of an APRS station
 */
class AprsStation
{
public:
    AprsStation();
    QString callsign;
    QString adressee;
    QString via;
    QString symbol;
    QString payload;
    QString message;
    double latitude;
    double longitude;
    uint time_seen;
    QString getImage();
};

#endif // APRSSTATION_H
