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

#ifndef AUDIOPROCESSOR_H
#define AUDIOPROCESSOR_H

#include <QObject>
#include <QString>
#include <QDebug>
#include <stdio.h>
#include "ext/utils.h"
#include "ext/filt.h"
#include <math.h>
extern "C"
{
#include "ext/compressor.h"
#include "ext/snd.h"
}
#include <speex/speex_preprocess.h>
#include "unistd.h"

class AudioProcessor : public QObject
{
    Q_OBJECT
public:
    explicit AudioProcessor(QObject *parent = nullptr);
    ~AudioProcessor();
    enum
    {
        AUDIO_MODE_CODEC2,
        AUDIO_MODE_ANALOG,
        AUDIO_MODE_OPUS
    };
    void compress_audio(short *buf, short bufsize, int direction, int audio_mode);
    void write_preprocess(short *buf, int bufsize, bool preprocess, int audio_mode);
    int read_preprocess(short *buf, int bufsize, bool preprocess, int audio_mode);

    void filter_audio(short *audiobuffer, int audiobuffersize,
                      bool pre_emphasis=false, bool de_emphasis=false, int mode=0);

signals:

public slots:

private:
    SpeexPreprocessState *_speex_preprocess;
    sf_compressor_state_st _cm_state_read;
    sf_compressor_state_st _cm_state_read_codec2;
    sf_compressor_state_st _cm_state_write;
    sf_compressor_state_st _cm_state_write_codec2;
    Filter *_audio_filter_1400;
    Filter *_audio_filter2_1400;
    Filter *_audio_filter_700;
    Filter *_audio_filter2_700;
    double _emph_last_input;
    int _error;
    float calc_audio_power(short *buf, short samples);

};

#endif // AUDIOPROCESSOR_H
