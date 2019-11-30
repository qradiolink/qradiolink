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

#include "radioprotocol.h"

RadioProtocol::RadioProtocol(Logger *logger, QObject *parent) :
    QObject(parent)
{
    _logger = logger;
    _buffer = new QByteArray;
}

void RadioProtocol::processRadioMessage(QByteArray data)
{
    QByteArray message;
    int msg_type;
    unsigned int data_len;
    unsigned int crc;
    QDataStream stream(&data, QIODevice::ReadOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream >> msg_type;
    stream >> data_len;
    stream >> message;
    stream >> crc;
    if(crc != gr::digital::crc32(message.toStdString()))
    {
        _logger->log(Logger::LogLevelCritical, "Radio packet CRC32 failed, dropping packet");
        return;
    }

    switch(msg_type)
    {
    case MsgTypePageMessage:
        processPageMessage(message);
        break;
    default:
        _logger->log(Logger::LogLevelDebug, QString("Radio message type %1 not implemented").arg(msg_type));
        break;
    }
}

QByteArray RadioProtocol::buildRadioMessage(QByteArray data, int msg_type)
{
    QByteArray message;
    QDataStream stream(&message, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << msg_type;
    stream << data.length();
    stream << data;
    unsigned int crc = gr::digital::crc32(data.toStdString());
    stream << crc;
    return message;
}

QByteArray RadioProtocol::buildPageMessage(QString calling_callsign, QString called_callsign,
                                           bool retransmit, QString via_node)
{

    QRadioLink::PageMessage p;
    p.set_calling_user(calling_callsign.toStdString());
    p.set_called_user(called_callsign.toStdString());
    p.set_retransmit(retransmit);
    p.set_via_node(via_node.toStdString());
    int size = p.ByteSize();
    unsigned char data[size];
    p.SerializeToArray(data,size);
    QByteArray msg(reinterpret_cast<const char*>(data));
    return buildRadioMessage(msg, MsgTypePageMessage);
}

QByteArray RadioProtocol::buildRepeaterInfo()
{
    QByteArray data;

    for(int i=0;i <_voip_channels.size();i++)
    {
        data.append(0xCF);
        data.append(0x77);
        data.append(0x07);
        data.append(0xAB);
        data.append(0x1); // TODO: msg type channel
        QRadioLink::Channel ch;
        ch.set_channel_id(_voip_channels.at(i)->id);
        ch.set_parent_id(_voip_channels.at(i)->parent_id);
        ch.set_name(_voip_channels.at(i)->name.toStdString().c_str());
        ch.set_description(_voip_channels.at(i)->description.toStdString().c_str());
        char bin[ch.ByteSize()];
        ch.SerializeToArray(bin,ch.ByteSize());
        data.append(bin, ch.ByteSize());
        data.append(0xAB);
        data.append(0x07);
        data.append(0x77);
        data.append(0xCF);
    }
    for(int i=0;i <_voip_users.size();i++)
    {
        data.append(0xCF);
        data.append(0x77);
        data.append(0x07);
        data.append(0xAB);
        data.append(0x2); // TODO: msg type user
        QRadioLink::User u;
        u.set_user_id(_voip_users.at(i)->id);
        u.set_channel_id(_voip_users.at(i)->channel_id);
        u.set_name(_voip_users.at(i)->callsign.toStdString().c_str());
        char bin[u.ByteSize()];
        u.SerializeToArray(bin,u.ByteSize());
        data.append(bin, u.ByteSize());
        data.append(0xAB);
        data.append(0x07);
        data.append(0x77);
        data.append(0xCF);
    }
    return data;
}


void RadioProtocol::setStations(QVector<Station *> list)
{

    _voip_users = list;
}

void RadioProtocol::setChannels(QVector<MumbleChannel *> list)
{
    _voip_channels = list;
}

void RadioProtocol::dataIn(QByteArray data)
{
    _buffer->append(data);
    QByteArray tail;
    tail.append(0xAB);
    tail.append(0x07);
    tail.append(0x77);
    tail.append(0xCF);
    QByteArray head;
    head.append(0xCF);
    head.append(0x77);
    head.append(0x07);
    head.append(0xAB);
    int t = _buffer->indexOf(tail);
    int h = _buffer->indexOf(head);
    if((t != -1) && (h != -1) && (t < h))
    {
        QByteArray payload = _buffer->left(t);
        QByteArray rest = _buffer->mid(t+4);
        processPayload(payload);
        _buffer->clear();
        dataIn(rest);
    }
    else if((t != -1) && (h == -1))
    {
        QByteArray payload = _buffer->left(t);
        processPayload(payload);
        QByteArray rest = _buffer->mid(t+4);
        _buffer->clear();
        _buffer->append(rest);
    }
    else if((t != -1) && (h != -1) && (t > h))
    {
        QByteArray rest = _buffer->mid(h+4);
        _buffer->clear();
        dataIn(rest);
    }
    else if((t == -1) && (h != -1))
    {
        QByteArray rest = _buffer->mid(h+4);
        _buffer->clear();
        _buffer->append(rest);
    }

}

void RadioProtocol::processPayload(QByteArray data)
{

    int msg_type = data.at(0);
    data = data.mid(1);
    switch(msg_type)
    {
    case 1:
    {
        QRadioLink::Channel ch;
        ch.ParseFromArray(data,data.size());
        /*
        MumbleChannel *chan = new MumbleChannel(
                    ch.channel_id(),ch.parent_id(),QString::fromStdString(ch.name()),
                    QString::fromStdString(ch.description()));
        emit newChannel(chan);
        */
        break;
    }
    case 2:
    {
        QRadioLink::User u;
        u.ParseFromArray(data,data.size());
        qDebug() << QString::fromStdString(u.name());
        break;
    }
    }
}

void RadioProtocol::processPageMessage(QByteArray message)
{
    QRadioLink::PageMessage page;
    page.ParseFromArray(message.data(), message.size());
    _logger->log(Logger::LogLevelDebug, QString("Paging message from %1 to %2 via %3").arg(
                     QString::fromStdString(page.calling_user())).arg(
                     QString::fromStdString(page.called_user())).arg(
                     QString::fromStdString(page.via_node())));
}


