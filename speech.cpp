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

#include "speech.h"

Speech::Speech(QObject *parent) :
    QObject(parent)
{
    //_audio = new AudioInterface;
    int heap_size = 210000;
    int load_init_files = 1;
#if 0
    festival_initialize(load_init_files,heap_size);
#endif
}

void Speech::fspeak(char* text)
{
#if 0
    festival_say_text(text);
#endif
    /// If we wait for the spooler to complete speech, we risk
    /// to return too late for other events in the queue
    /// so let's make this asynchronous
    //festival_wait_for_spooler();
}


