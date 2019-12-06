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

#include "aprspixmapitem.h"


AprsPixmapItem::AprsPixmapItem(QPixmap &pixmap) :
    QGraphicsPixmapItem(pixmap)
{
    this->setAcceptHoverEvents(true);
    _callsign = "";
    _via = "";
    _pos.setX(0);
    _pos.setY(0);
    _item_text = NULL;

}

void AprsPixmapItem::hoverEnterEvent(QGraphicsSceneHoverEvent * event)
{
    event->setAccepted(true);
    QGraphicsScene *scene = this->scene();
    QGraphicsTextItem * message = new QGraphicsTextItem;
    message->setPos(_pos - QPoint(0,16));
    message->setHtml("<div style=\"background:white;color:blue;font-height:15px;\"><b>"+
                      _callsign+"</b></div><div style=\"background:white;color:black;\"><br/>"+_message+"<br/><b>Via:</b>"+_via+"</span>");
    scene->addItem(message);
    _item_text = message;

}

void AprsPixmapItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    event->setAccepted(true);
    this->scene()->removeItem(_item_text);
}

void AprsPixmapItem::setMessage(QString &callsign, QString &via, QString &message)
{
    _callsign = callsign;
    _via = via;
    _message = message;

}
