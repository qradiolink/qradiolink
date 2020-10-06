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

#include "layer2.h"

Layer2Protocol::Layer2Protocol(Logger *logger, QObject *parent) :
    QObject(parent)
{
    _logger = logger;
    _buffer = new QByteArray;
}

void Layer2Protocol::processRadioMessage(QByteArray data)
{
    int msg_type;
    unsigned int data_len;
    unsigned int crc;
    QDataStream stream(&data, QIODevice::ReadOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream >> msg_type;
    stream >> data_len;
    if(data_len > 1024*1024)
        return;
    char *tmp = new char[data_len];
    stream >> crc;
    stream.readBytes(tmp, data_len);
    QByteArray message(tmp, data_len);
    delete[] tmp;

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
    case MsgTypeRepeaterInfo:
        processRepeaterInfo(message);
        break;
    default:
        _logger->log(Logger::LogLevelDebug,
                     QString("Radio message type %1 not implemented").arg(msg_type));
        break;
    }

}

QByteArray Layer2Protocol::buildRadioMessage(QByteArray data, int msg_type)
{
    unsigned int crc = gr::digital::crc32(data.toStdString());
    QByteArray message;
    QDataStream stream(&message, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << msg_type;
    stream << data.length();
    stream << crc;
    stream << data;
    return message;
}

QByteArray Layer2Protocol::buildPageMessage(QString calling_callsign, QString called_callsign, QString message,
                                           bool retransmit, QString via_node)
{

    QRadioLink::PageMessage *p = new QRadioLink::PageMessage;
    p->set_calling_user(calling_callsign.toStdString());
    p->set_called_user(called_callsign.toStdString());
    p->set_msg(message.toStdString());
    p->set_retransmit(retransmit);
    p->set_via_node(via_node.toStdString());
    int size = p->ByteSizeLong();
    unsigned char data[size];
    p->SerializeToArray(data,size);
    QByteArray msg(reinterpret_cast<const char*>(data));
    delete p;
    return buildRadioMessage(msg, MsgTypePageMessage);
}

QByteArray Layer2Protocol::buildRepeaterInfo()
{
    QByteArray data;
    QRadioLink::RepeaterInfo repeater_info;
    for(int i=0;i <_voip_channels.size();i++)
    {
        QRadioLink::RepeaterInfo::Channel *ch = repeater_info.add_channels();
        ch->set_channel_id(_voip_channels.at(i)->id);
        ch->set_parent_id(_voip_channels.at(i)->parent_id);
        ch->set_name(_voip_channels.at(i)->name.toStdString().c_str());
        ch->set_description(_voip_channels.at(i)->description.toStdString().c_str());
    }
    for(int i=0;i <_voip_users.size();i++)
    {
        QRadioLink::RepeaterInfo::User *u = repeater_info.add_users();;
        u->set_user_id(_voip_users.at(i)->id);
        u->set_channel_id(_voip_users.at(i)->channel_id);
        u->set_name(_voip_users.at(i)->callsign.toStdString().c_str());
    }
    char bin[repeater_info.ByteSizeLong()];
    repeater_info.SerializeToArray(bin,repeater_info.ByteSizeLong());
    data.append(bin, repeater_info.ByteSizeLong());
    return buildRadioMessage(data, MsgTypeRepeaterInfo);
}


void Layer2Protocol::setStations(QVector<Station *> list)
{

    _voip_users = list;
}

void Layer2Protocol::setChannels(QVector<MumbleChannel *> list)
{
    _voip_channels = list;
}

void Layer2Protocol::processRepeaterInfo(QByteArray message)
{
    QRadioLink::RepeaterInfo info;
    info.ParseFromArray(message.data(), message.size());
    for(int i=0; i<info.channels_size();i++)
    {
        qDebug() << QString::fromStdString(info.channels(i).name());
    }
    for(int i=0; i<info.users_size();i++)
    {
        qDebug() << QString::fromStdString(info.users(i).name());
    }
}

void Layer2Protocol::processPageMessage(QByteArray message)
{
    QRadioLink::PageMessage page;
    page.ParseFromArray(message.data(), message.size());
    emit havePageMessage(QString::fromStdString(page.calling_user()),
                        QString::fromStdString(page.called_user()),
                        QString::fromStdString(page.msg()));
}


