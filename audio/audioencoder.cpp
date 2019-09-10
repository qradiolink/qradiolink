// Written by Adrian Musceac YO8RZZ , started August 2014.
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

#include "audioencoder.h"


AudioEncoder::AudioEncoder()
{
    int error, error1;
    _enc = opus_encoder_create(8000,1,OPUS_APPLICATION_VOIP,&error);
    if(error != OPUS_OK)
    {
        qDebug() << "audio encoder creation failed";
    }
    _dec = opus_decoder_create(8000,1,&error1);
    if(error1 != OPUS_OK)
    {
        qDebug() << "audio decoder creation failed";
    }
    _codec2_1400 = codec2_create(CODEC2_MODE_1400);
    _codec2_700 = codec2_create(CODEC2_MODE_700B);
    _codec2_2400 = codec2_create(CODEC2_MODE_2400);

    _gsm = gsm_create();
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
    _audio_filter_700 = new Filter(BPF,32,8,0.2,2.4); // 16,8,0.12,3.8
    if( _audio_filter_700->get_error_flag() != 0 )
    {
        qDebug() << "audio filter creation failed";
    }
    _audio_filter2_700 = new Filter(BPF,32,8,0.2,2.4);
    if( _audio_filter2_700->get_error_flag() != 0 )
    {
        qDebug() << "audio filter creation failed";
    }
    _emph_last_input = 0.0;

    int opus_bandwidth;
    opus_encoder_ctl(_enc, OPUS_SET_VBR(0));
    opus_encoder_ctl(_enc, OPUS_SET_BITRATE(9400));
    opus_encoder_ctl(_enc, OPUS_SET_COMPLEXITY(8));
    //opus_encoder_ctl(_enc, OPUS_SET_DTX(0));
    opus_encoder_ctl(_enc, OPUS_SET_LSB_DEPTH(16));
    opus_encoder_ctl(_enc, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
    opus_encoder_ctl(_enc, OPUS_SET_APPLICATION(OPUS_APPLICATION_VOIP));
    opus_encoder_ctl(_enc, OPUS_SET_MAX_BANDWIDTH(OPUS_BANDWIDTH_WIDEBAND));
    opus_encoder_ctl(_enc, OPUS_SET_PACKET_LOSS_PERC(0));
    //opus_encoder_ctl(_enc, OPUS_SET_PREDICTION_DISABLED(0));
    opus_encoder_ctl(_enc, OPUS_GET_BANDWIDTH(&opus_bandwidth));
    opus_encoder_ctl(_enc, OPUS_SET_INBAND_FEC(0));
    opus_decoder_ctl(_dec, OPUS_SET_GAIN(0));

}

AudioEncoder::~AudioEncoder()
{
    opus_encoder_destroy(_enc);
    opus_decoder_destroy(_dec);
    codec2_destroy(_codec2_1400);
    codec2_destroy(_codec2_700);
    gsm_destroy(_gsm);
    delete _audio_filter_1400;
    delete _audio_filter2_1400;
}

unsigned char* AudioEncoder::encode_opus(short *audiobuffer, int audiobuffersize, int &encoded_size)
{
    unsigned char *encoded_audio = new unsigned char[47];
    encoded_size = opus_encode(_enc, audiobuffer, audiobuffersize/sizeof(short), encoded_audio, 47);
    return encoded_audio;
}

short* AudioEncoder::decode_opus(unsigned char *audiobuffer, int audiobuffersize, int &samples)
{
    int fs = 320;
    short *pcm = new short[fs];
    //memset(pcm,0,(fs)*sizeof(short));
    samples = opus_decode(_dec,audiobuffer,audiobuffersize, pcm, fs, 0);
    if(samples <= 0)
    {
        delete[] pcm;
        return NULL;
    }
    return pcm;
}



unsigned char* AudioEncoder::encode_codec2_1400(short *audiobuffer, int audiobuffersize, int &length)
{
    filter_audio(audiobuffer, audiobuffersize,true,false);
    int bits = codec2_bits_per_frame(_codec2_1400);
    //int bytes = (bits + 7) / 8;
    int bytes = bits / 8;
    unsigned char *encoded = new unsigned char[bytes];
    codec2_encode(_codec2_1400, encoded, audiobuffer);
    length = bytes;
    return encoded;
}

unsigned char* AudioEncoder::encode_codec2_700(short *audiobuffer, int audiobuffersize, int &length)
{
    filter_audio(audiobuffer, audiobuffersize, true, false);
    int bits = codec2_bits_per_frame(_codec2_700);
    int bytes = (bits + 4) / 8;
    unsigned char *encoded = new unsigned char[bytes];
    codec2_encode(_codec2_700, encoded, audiobuffer);
    length = bytes;
    return encoded;
}

unsigned char* AudioEncoder::encode_codec2_2400(short *audiobuffer, int audiobuffersize, int &length)
{
    filter_audio(audiobuffer, audiobuffersize);
    int bits = codec2_bits_per_frame(_codec2_2400);
    int bytes = (bits + 4) / 8;
    unsigned char *encoded = new unsigned char[bytes];
    codec2_encode(_codec2_2400, encoded, audiobuffer);
    length = bytes;
    return encoded;
}

short* AudioEncoder::decode_codec2_1400(unsigned char *audiobuffer, int audiobuffersize, int &samples)
{
    Q_UNUSED(audiobuffersize);
    samples = codec2_samples_per_frame(_codec2_1400);
    short* decoded = new short[samples];
    memset(decoded,0,(samples)*sizeof(short));
    codec2_decode(_codec2_1400, decoded, audiobuffer);
    filter_audio(decoded, samples*sizeof(short),false,true);
    return decoded;
}

short* AudioEncoder::decode_codec2_700(unsigned char *audiobuffer, int audiobuffersize, int &samples)
{
    Q_UNUSED(audiobuffersize);
    samples = codec2_samples_per_frame(_codec2_700);
    short* decoded = new short[samples];
    memset(decoded,0,(samples)*sizeof(short));
    codec2_decode(_codec2_700, decoded, audiobuffer);
    filter_audio(decoded, samples*sizeof(short), false, true);
    return decoded;
}

short* AudioEncoder::decode_codec2_2400(unsigned char *audiobuffer, int audiobuffersize, int &samples)
{
    Q_UNUSED(audiobuffersize);
    samples = codec2_samples_per_frame(_codec2_2400);
    short* decoded = new short[samples];
    memset(decoded,0,(samples)*sizeof(short));
    codec2_decode(_codec2_2400, decoded, audiobuffer);
    return decoded;
}

unsigned char* AudioEncoder::encode_gsm(short *audiobuffer, int audiobuffersize, int &length)
{
    length = sizeof(gsm_frame);
    unsigned char *encoded = new unsigned char[length];
    gsm_encode(_gsm,audiobuffer,encoded);
    return encoded;
}

short* AudioEncoder::decode_gsm(unsigned char *audiobuffer, int data_length, int &samples)
{
    samples = 160;
    short* decoded = new short[160];
    gsm_decode(_gsm,audiobuffer,decoded);
    return decoded;
}
// FIXME: enum for mode
void AudioEncoder::filter_audio(short *audiobuffer, int audiobuffersize, bool pre_emphasis, bool de_emphasis, int mode)
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
                output = _audio_filter2_1400->do_sample(sample) + 0.35 * _emph_last_input + 0.01 * (rand() % 1000); // 0.9
                _emph_last_input = output;
                audiobuffer[i] = (short) (output * 0.8);
            }
            else
            {
                output = _audio_filter2_700->do_sample(sample) + 0.9375f * _emph_last_input; + 0.1 * (rand() % 1000); // 0.9
                _emph_last_input = output;
                audiobuffer[i] = (short) (output * 1.5);
            }
        }
        if(pre_emphasis)
        {
            double output;
            // FIXME:
            if(mode == 0)
            {
                output = _audio_filter_1400->do_sample(sample) - 0.35 * _emph_last_input + 0.1 * (rand() % 1000); // 0.9
                _emph_last_input = output;
                audiobuffer[i] = (short) (output * 0.3);
            }
            else
            {
                output = _audio_filter_700->do_sample(sample) - 0.9375f * _emph_last_input + 0.015 * (rand() % 1000); // 0.9
                _emph_last_input = output;
                audiobuffer[i] = (short) (output * 0.9); // I'm still getting clipping and don't know where
            }
        }
    }
}


