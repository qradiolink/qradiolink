// Written by Adrian Musceac YO8RZZ , started October 2019.
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

#ifndef RELAYCONTROLLER_H
#define RELAYCONTROLLER_H

#include <QObject>
#include <ftdi.h>
#include <iostream>

class RelayController : public QObject
{
    Q_OBJECT
public:
    explicit RelayController(QObject *parent = nullptr);
    ~RelayController();

signals:

public slots:
    int enableRelay(int relay_number);
    int disableRelay(int relay_number);

private:
    struct ftdi_context *_ftdi_relay;
    bool _ftdi_relay_enabled;
    unsigned char *_relay_mask;
};

#endif // RELAYCONTROLLER_H
