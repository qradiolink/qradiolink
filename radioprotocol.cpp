#include "radioprotocol.h"

RadioProtocol::RadioProtocol(QObject *parent) :
    QObject(parent)
{
    _voip_channels = new QVector<Channel*>;
}


QByteArray RadioProtocol::buildChannelList()
{
    QString repeater_info = "";
    QXmlStreamWriter stream(&repeater_info);
    stream.setAutoFormatting(true);
    stream.writeStartElement("i");
    stream.writeStartElement("channels");
    for(int i=0;i <_voip_channels->size();i++)
    {
        stream.writeStartElement("channel");
        stream.writeAttribute("id",QString::number(_voip_channels->at(i)->id));
        stream.writeAttribute("parent_id",QString::number(_voip_channels->at(i)->parent_id));
        stream.writeAttribute("name",_voip_channels->at(i)->name);
        stream.writeAttribute("description",_voip_channels->at(i)->description);
        stream.writeEndElement();
    }
    stream.writeEndElement();
    stream.writeEndElement();
    return repeater_info.toLocal8Bit();
}

void RadioProtocol::addChannel(Channel *chan)
{
    if(!chan->name.isEmpty())
        _voip_channels->push_back(chan);
}


