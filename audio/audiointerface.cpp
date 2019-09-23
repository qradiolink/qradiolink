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

#include "audiointerface.h"

AudioInterface::AudioInterface(QObject *parent, unsigned sample_rate, unsigned channels, int normal) :
    QObject(parent)
{
    _s_rec = NULL;
    _s_play = NULL;
    _s_short_play = NULL;
    _s_short_rec = NULL;
    _error=0;
    _speex_preprocess = speex_preprocess_state_init(320, 8000);

    int i;
    float f;
    i = 1;
    speex_preprocess_ctl(_speex_preprocess, SPEEX_PREPROCESS_SET_DENOISE, &i);
    i = -45;
    speex_preprocess_ctl(_speex_preprocess, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &i);
    //i = 0;
    //speex_preprocess_ctl(_speex_preprocess, SPEEX_PREPROCESS_SET_AGC, &i);
    i = 1;
    speex_preprocess_ctl(_speex_preprocess, SPEEX_PREPROCESS_SET_VAD, &i);
    //i = 80;
    //speex_preprocess_ctl(_speex_preprocess, SPEEX_PREPROCESS_SET_PROB_START, &f);
    //i = 60;
    //speex_preprocess_ctl(_speex_preprocess, SPEEX_PREPROCESS_SET_PROB_CONTINUE, &f);
    //i = 0.8;
    //speex_preprocess_ctl(_speex_preprocess, SPEEX_PREPROCESS_SET_AGC_LEVEL, &i);
    i=1;
    speex_preprocess_ctl(_speex_preprocess, SPEEX_PREPROCESS_SET_DEREVERB, &i);
    f=.5;
    speex_preprocess_ctl(_speex_preprocess, SPEEX_PREPROCESS_SET_DEREVERB_DECAY, &f);
    f=.5;
    speex_preprocess_ctl(_speex_preprocess, SPEEX_PREPROCESS_SET_DEREVERB_LEVEL, &f);

    // need to make these configurable
    sf_simplecomp(&_cm_state_read_codec2,
                  8000, // audio rate
                  0,   // audio boost
                  -35,  // kick in (dB)
                  40,   // knee
                  30,   // inverse scale
                  0.001f,   // attack
                  0.15f    // release
                  );
    sf_simplecomp(&_cm_state_read,
                  8000, // audio rate
                  0,   // audio boost
                  -35,  // kick in (dB)
                  40,   // knee
                  30,   // inverse scale
                  0.009f,   // attack
                  0.125f    // release
                  );
    sf_simplecomp(&_cm_state_write,
                  8000, // audio rate
                  0,   // audio boost
                  -35,  // kick in (dB)
                  40,   // knee
                  40,   // inverse scale
                  0.001f,   // attack
                  0.125f    // release
                  );
    sf_simplecomp(&_cm_state_write_codec2,
                  8000, // audio rate
                  3,   // audio boost
                  -30,  // kick in (dB)
                  20,   // knee
                  20,   // inverse scale
                  0.001f,   // attack
                  0.125f    // release
                  );
    int rand_len = 4;
    char rand[5];
    genRandomStr(rand,rand_len);
    //static QString instance_name = QString::fromLocal8Bit(rand);

    QString dtmf_rec = "qradiolink_dtmf";
    QString q_play = "qradiolink";
    QString audio_rec = "qradiolink_audio";


    pa_buffer_attr attr;
    attr.fragsize = 2048;
    attr.maxlength = 4096;
    attr.minreq = -1;
    attr.prebuf = -1;
    attr.tlength = 4096;

    if(!normal)
    {
        pa_sample_spec ss;
        ss.format = PA_SAMPLE_FLOAT32LE;
        ss.rate = sample_rate;
        ss.channels = channels;


        if (!(_s_rec = pa_simple_new(NULL, dtmf_rec.toStdString().c_str(), PA_STREAM_RECORD, NULL, "record", &ss, NULL, &attr, &_error)))
        {
            fprintf(stderr, __FILE__": pa_simple_new() failed:\n");
        }
        if (!(_s_play = pa_simple_new(NULL, q_play.toStdString().c_str(), PA_STREAM_PLAYBACK, NULL, "play", &ss, NULL, &attr, &_error)))
        {
            fprintf(stderr, __FILE__": pa_simple_new() failed:\n");
        }
    }
    else
    {
        pa_sample_spec ss_short;
        ss_short.format = PA_SAMPLE_S16LE;
        ss_short.rate = sample_rate;
        ss_short.channels = channels;


        if (!(_s_short_play = pa_simple_new(NULL, q_play.toStdString().c_str(), PA_STREAM_PLAYBACK, NULL, "play", &ss_short, NULL, &attr, &_error)))
        {
            fprintf(stderr, __FILE__": pa_simple_new() failed:\n");
        }
        if (!(_s_short_rec = pa_simple_new(NULL, audio_rec.toStdString().c_str(), PA_STREAM_RECORD, NULL, "record", &ss_short, NULL, &attr, &_error)))
        {
            fprintf(stderr, __FILE__": pa_simple_new() failed:\n");
        }
    }
}

AudioInterface::~AudioInterface()
{
    speex_preprocess_state_destroy(_speex_preprocess);
    if (_s_rec)
      pa_simple_free(_s_rec);
    if (_s_play)
      pa_simple_free(_s_play);
    if (_s_short_play)
      pa_simple_free(_s_short_play);
    if (_s_short_rec)
      pa_simple_free(_s_short_rec);
}



int AudioInterface::read(float *buf, short bufsize)
{
    if(!_s_rec)
        return 1;
    if (pa_simple_read(_s_rec, buf, bufsize, &_error) < 0)
    {
        fprintf(stderr, __FILE__": pa_simple_read() failed:\n");
        return 1;
    }
    return 0;
}

int AudioInterface::write(float *buf, short bufsize)
{
    if(!_s_play)
        return 1;
    if(pa_simple_write(_s_play, buf, bufsize, &_error) < 0)
    {
        fprintf(stderr, __FILE__": pa_simple_write() failed:\n");
        return 1;
    }
    return 0;
}

int AudioInterface::write_short(short *buf, short bufsize, bool preprocess, int audio_mode)
{
    if(preprocess)
    {
        int i = 3;
        //speex_preprocess_ctl(_speex_preprocess, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &i);
        //speex_preprocess_run(_speex_preprocess, buf);
        compress_audio(buf, bufsize, 1, audio_mode);
    }

    if(!_s_short_play)
        return 1;
    if(pa_simple_write(_s_short_play, buf, bufsize, &_error) < 0)
    {
        fprintf(stderr, __FILE__": pa_simple_write() failed:\n");
        return 1;
    }
    return 0;
}

int AudioInterface::read_short(short *buf, short bufsize, bool preprocess, int audio_mode)
{
    if(!_s_short_rec)
        return 1;
    if(pa_simple_read(_s_short_rec, buf, bufsize, &_error) < 0)
    {
        fprintf(stderr, __FILE__": pa_simple_read() failed:\n");
        return 1;
    }
    int vad = 1;
    if(preprocess)
    {
        //vad = speex_preprocess_run(_speex_preprocess, buf);
    }

    compress_audio(buf, bufsize, 0, audio_mode);
    float power = calc_audio_power(buf, bufsize/sizeof(short));
    return (power > 2.0);
}

float AudioInterface::calc_audio_power(short *buf, short samples)
{
    float power = 0.0;
    for (int i = 0; i < samples; i++)
    {
        float a = abs(((float)buf[i]) / 32768.0f);
        power += a * a;
    }
    return 32768.0f * power / ((float) samples);
}

void AudioInterface::compress_audio(short *buf, short bufsize, int direction, int audio_mode)
{
    sf_snd output_snd = sf_snd_new(bufsize/sizeof(short), 8000, true);
    sf_snd input_snd = sf_snd_new(bufsize/sizeof(short), 8000, true);
    for(unsigned int i=0;i<bufsize/sizeof(short);i++)
    {
        input_snd->samples[i].L = (float)buf[i] / 32767.0f;
    }
    if(direction == 0)
    {
        switch(audio_mode)
        {
        case AUDIO_MODE_ANALOG:
        case AUDIO_MODE_OPUS:
            sf_compressor_process(&_cm_state_read, bufsize/sizeof(short), input_snd->samples, output_snd->samples);
            break;
        case AUDIO_MODE_CODEC2:
            sf_compressor_process(&_cm_state_read_codec2, bufsize/sizeof(short), input_snd->samples, output_snd->samples);
            break;
        }
    }
    if(direction == 1)
    {
        switch(audio_mode)
        {
        case AUDIO_MODE_ANALOG:
        case AUDIO_MODE_OPUS:
            sf_compressor_process(&_cm_state_write, bufsize/sizeof(short), input_snd->samples, output_snd->samples);
            break;
        case AUDIO_MODE_CODEC2:
            sf_compressor_process(&_cm_state_write_codec2, bufsize/sizeof(short), input_snd->samples, output_snd->samples);
            break;
        }
    }
    for(unsigned int i=0;i<bufsize/sizeof(short);i++)
    {
        buf[i] = (short)(output_snd->samples[i].L * 32767.0f);
    }
    sf_snd_free(input_snd);
    sf_snd_free(output_snd);
}
