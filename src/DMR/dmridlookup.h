// Written by Adrian Musceac YO8RZZ , started October 2023.
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

#ifndef DMRIDLOOKUP_H
#define DMRIDLOOKUP_H

#include <QObject>
#include <QByteArray>
#include <QMap>
#include "src/settings.h"
#include "src/logger.h"

class DMRIdLookup : public QObject
{
    Q_OBJECT
public:
    explicit DMRIdLookup(Settings *settings, Logger *logger, QObject *parent = nullptr);
    ~DMRIdLookup();
    QString lookup(unsigned int id);
    QString getCallsign(unsigned int id);

signals:

private:
    Settings *_settings;
    Logger *_logger;
    QMap<unsigned int, QString> *_ids;

};

#endif // DMRIDLOOKUP_H
