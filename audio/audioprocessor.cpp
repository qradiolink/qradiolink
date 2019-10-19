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

#include "audioprocessor.h"

AudioProcessor::AudioProcessor(QObject *parent) : QObject(parent)
{
    _error=0;
    /*
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
    */

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
                  20,   // knee
                  20,   // inverse scale
                  0.009f,   // attack
                  0.125f    // release
                  );
    sf_simplecomp(&_cm_state_write,
                  8000, // audio rate
                  0,   // audio boost
                  -35,  // kick in (dB)
                  20,   // knee
                  20,   // inverse scale
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

    _audio_filter_1400 = new Filter(BPF,256,8,0.2,3.8); // 16,8,0.12,3.8
    if( _audio_filter_1400->get_error_flag() != 0 )
    {
        qDebug() << "audio filter creation failed";
    }
    _audio_filter2_1400 = new Filter(BPF,256,8,0.2,3.8);
    if( _audio_filter2_1400->get_error_flag() != 0 )
    {
        qDebug() << "audio filter creation failed";
    }
    _audio_filter_700 = new Filter(BPF,256,8,0.2,3.0); // 16,8,0.12,3.8
    if( _audio_filter_700->get_error_flag() != 0 )
    {
        qDebug() << "audio filter creation failed";
    }
    _audio_filter2_700 = new Filter(BPF,256,8,0.2,3.0);
    if( _audio_filter2_700->get_error_flag() != 0 )
    {
        qDebug() << "audio filter creation failed";
    }
    _emph_last_input = 0.0;
}

AudioProcessor::~AudioProcessor()
{
    //speex_preprocess_state_destroy(_speex_preprocess);
    delete _audio_filter_1400;
    delete _audio_filter2_1400;
    delete _audio_filter_700;
    delete _audio_filter2_700;
}

void AudioProcessor::write_preprocess(short *buf, int bufsize, bool preprocess, int audio_mode)
{
    if(preprocess)
    {
        //int i = 3;
        //speex_preprocess_ctl(_speex_preprocess, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &i);
        //speex_preprocess_run(_speex_preprocess, buf);
        compress_audio(buf, bufsize, 1, audio_mode);
    }
}

int AudioProcessor::read_preprocess(short *buf, int bufsize, bool preprocess, int audio_mode)
{
    //int vad;
    if(preprocess)
    {
        compress_audio(buf, bufsize, 0, audio_mode);
        //vad = speex_preprocess_run(_speex_preprocess, buf);
    }

    float power = calc_audio_power(buf, bufsize/sizeof(short));
    return (power > 2.0);
}

float AudioProcessor::calc_audio_power(short *buf, short samples)
{
    float power = 0.0;
    for (int i = 0; i < samples; i++)
    {
        float a = abs(((float)buf[i]) / 32768.0f);
        power += a * a;
    }
    return 32768.0f * power / ((float) samples);
}

void AudioProcessor::compress_audio(short *buf, short bufsize, int direction, int audio_mode)
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

// FIXME: enum for mode
void AudioProcessor::filter_audio(short *audiobuffer, int audiobuffersize, bool pre_emphasis, bool de_emphasis, int mode)
{

    for(unsigned int i = 0;i<audiobuffersize/sizeof(short);i++)
    {
        double sample = (double) audiobuffer[i];
        if(!pre_emphasis && !de_emphasis)
        {
            // FIXME:
            if(mode == 0)
            {
                audiobuffer[i] = (short) _audio_filter_1400->do_sample(sample);
            }
            else
            {
                audiobuffer[i] = (short) _audio_filter_700->do_sample(sample);
            }
        }
        if(de_emphasis)
        {
            double output;
            // FIXME:
            if(mode == 0)
            {
                output = _audio_filter2_1400->do_sample(sample) + 0.1 * (rand() % 1000);// + 0.6375f * _emph_last_input ; // 0.9
                _emph_last_input = output;
                audiobuffer[i] = (short) (output);
            }
            else
            {
                output = _audio_filter2_700->do_sample(sample) + 0.1 * (rand() % 1000);// + 0.9375f * _emph_last_input;  // 0.9
                _emph_last_input = output;
                audiobuffer[i] = (short) (output);
            }
        }
        if(pre_emphasis)
        {
            double output;
            // FIXME:
            if(mode == 0)
            {
                output = _audio_filter_1400->do_sample(sample);// - 0.9375f * _emph_last_input;
                _emph_last_input = output;
                audiobuffer[i] = (short) (output);
            }
            else
            {
                output = _audio_filter_700->do_sample(sample); // 0.9
                _emph_last_input = output;
                audiobuffer[i] = (short) (output); // I'm still getting clipping and don't know where
            }
        }
    }
}

