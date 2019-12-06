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

#include "station.h"

Station::Station()
{
    id = 0;
    callsign="";
    radio_id = "";
    channel_id=-1;
    repeater=0;
    local=0;
    active=1;
    is_user = false;

}

Station::~Station()
{

}

void Station::initWidget()
{
    _tree_item->setText(0,callsign);
    _tree_item->setText(3,QString::number(id));
    _tree_item->setIcon(0,QIcon(":/res/im-user.png"));
    _tree_item->setBackgroundColor(0,QColor("#ffffff"));
    _tree_item->setBackgroundColor(1,QColor("#ffffff"));
    _tree_item->setBackgroundColor(2,QColor("#ffffff"));
    _tree_item->setBackgroundColor(3,QColor("#ffffff"));
    if(is_user)
        _tree_item->setTextColor(0,QColor("#cc0000"));
}

QTreeWidgetItem* Station::getWidget() const
{
    return _tree_item;
}
