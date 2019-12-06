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

#ifndef AUDIOINTERFACE_H
#define AUDIOINTERFACE_H

#include <QObject>
#include <QString>
#include <QDebug>
#include <stdio.h>
#include "ext/utils.h"
#include "audio/audioprocessor.h"
#include <math.h>
extern "C"
{
#include "ext/compressor.h"
#include "ext/snd.h"
}
#include <pulse/simple.h>
#include <pulse/error.h>
#include <speex/speex_preprocess.h>
#include "unistd.h"

class AudioInterface : public QObject
{
    Q_OBJECT
public:
    enum
    {
        AUDIO_MODE_CODEC2,
        AUDIO_MODE_ANALOG,
        AUDIO_MODE_OPUS
    };
    explicit AudioInterface(QObject *parent = 0, unsigned sample_rate = 8000, unsigned channels = 1, int normal = 1);
    ~AudioInterface();
    int read(float *buf, short bufsize);
    int write(float *buf, short bufsize);
    int write_short(short *buf, short bufsize, bool preprocess=false, int audio_mode=AUDIO_MODE_ANALOG);
    int read_short(short *buf, short bufsize, bool preprocess=false, int audio_mode=AUDIO_MODE_ANALOG);
    void compress_audio(short *buf, short bufsize, int direction, int audio_mode);
signals:
    
public slots:

private:
    pa_simple *_s_rec;
    pa_simple *_s_play;
    pa_simple *_s_short_play;
    pa_simple *_s_short_rec;
    AudioProcessor *_processor;
    int _error;
    
};

#endif // AUDIOINTERFACE_H
