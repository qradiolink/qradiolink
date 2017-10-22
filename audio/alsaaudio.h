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

#ifndef ALSAAUDIO_H
#define ALSAAUDIO_H

#include <alsa/asoundlib.h>
#include <QObject>
#include <QString>
#include <stdio.h>
#include "ext/utils.h"

class AlsaAudio
{
public:
    AlsaAudio(QString device, int sample_rate=48000);
    ~AlsaAudio();
    int read(float *buf, short bufsize);
    int write(float *buf, short bufsize);
    int write_short(short *buf, short bufsize);
    int read_short(short *buf, short bufsize);
    int initDevices(QString device);
    int configureDevices(int sample_rate);

private:
    snd_pcm_t *_pcm_rec;
    snd_pcm_t *_pcm_play;
};

#endif // ALSAAUDIO_H
