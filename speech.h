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

#ifndef SPEECH_H
#define SPEECH_H

#include <QObject>
#if 0
#include <festival/festival.h>
#endif

class Speech : public QObject
{
    Q_OBJECT
public:
    explicit Speech(QObject *parent = 0);
    void fspeak(char* text);

    
signals:
    
public slots:

private:

    
};


#endif // SPEECH_H
