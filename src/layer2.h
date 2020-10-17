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

#ifndef LAYER2_H
#define LAYER2_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QByteArray>
#include <QDataStream>
#include <QDebug>
#include <gnuradio/digital/crc32.h>
#include "mumblechannel.h"
#include "station.h"
#include "logger.h"
#include "ext/QRadioLink.pb.h"

typedef QVector<MumbleChannel*> ChannelList;
class Layer2Protocol : public QObject
{
    Q_OBJECT
public:
    enum
    {
        MsgTypeChannel,
        MsgTypeUser,
        MsgTypePageMessage,
        MsgTypeRepeaterInfo,
    };
    explicit Layer2Protocol(Logger *logger, QObject *parent = 0);
    ~Layer2Protocol();

    QByteArray buildRepeaterInfo();
    void addChannel(MumbleChannel *chan);
    void setStations(QVector<Station*> list);
    void setChannels(ChannelList channels);
    QByteArray buildRadioMessage(QByteArray data, int msg_type);
    QByteArray buildPageMessage(QString calling_callsign, QString called_callsign, QString message,
                                bool retransmit=false, QString via_node="");

signals:
    void havePageMessage(QString calling_user, QString called_user, QString page_message);

public slots:
    void processRadioMessage(QByteArray data);

private:
    Logger *_logger;
    QVector<MumbleChannel*> _voip_channels;
    QVector<Station*> _voip_users;
    QByteArray *_buffer;
    void processPageMessage(QByteArray message);
    void processRepeaterInfo(QByteArray message);

};

#endif // LAYER2_H
