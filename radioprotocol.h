// Written by Adrian Musceac YO8RZZ , started December 2017.
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

#ifndef RADIOPROTOCOL_H
#define RADIOPROTOCOL_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QByteArray>
#include <QDebug>
#include "mumblechannel.h"
#include "station.h"
#include "ext/QRadioLink.pb.h"

typedef QVector<MumbleChannel*> ChannelList;
class RadioProtocol : public QObject
{
    Q_OBJECT
public:
    enum
    {
        MsgTypeChannel,
        MsgTypeUser
    };
    explicit RadioProtocol(QObject *parent = 0);

    QByteArray buildRepeaterInfo();
    void addChannel(MumbleChannel *chan);
    void dataIn(QByteArray data);
    void setStations(QVector<Station*> list);
    void setChannels(ChannelList channels);

signals:

public slots:

private:
    QVector<MumbleChannel*> _voip_channels;
    QVector<Station*> _voip_users;
    QByteArray *_buffer;
    void processPayload(QByteArray data);

};

#endif // RADIOPROTOCOL_H
