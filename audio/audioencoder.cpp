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
    _enc_voip = opus_encoder_create(8000,1,OPUS_APPLICATION_VOIP,&error);
    if(error != OPUS_OK)
    {
        qDebug() << "audio encoder creation failed";
    }
    _dec_voip = opus_decoder_create(8000,1,&error1);
    if(error1 != OPUS_OK)
    {
        qDebug() << "audio decoder creation failed";
    }
    _codec2_1400 = codec2_create(CODEC2_MODE_1400);
    _codec2_700 = codec2_create(CODEC2_MODE_700B);
    _codec2_2400 = codec2_create(CODEC2_MODE_2400);

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

    // VOIP
    int opus_bandwidth_voip;
    opus_encoder_ctl(_enc_voip, OPUS_SET_VBR(0));
    opus_encoder_ctl(_enc_voip, OPUS_SET_BITRATE(20000));
    opus_encoder_ctl(_enc_voip, OPUS_SET_COMPLEXITY(5));
    //opus_encoder_ctl(_enc, OPUS_SET_DTX(0));
    opus_encoder_ctl(_enc_voip, OPUS_SET_LSB_DEPTH(16));
    opus_encoder_ctl(_enc_voip, OPUS_SET_SIGNAL(OPUS_SIGNAL_MUSIC));
    opus_encoder_ctl(_enc_voip, OPUS_SET_APPLICATION(OPUS_APPLICATION_AUDIO));
    opus_encoder_ctl(_enc_voip, OPUS_SET_MAX_BANDWIDTH(OPUS_BANDWIDTH_FULLBAND));
    opus_encoder_ctl(_enc_voip, OPUS_SET_PACKET_LOSS_PERC(0));
    //opus_encoder_ctl(_enc, OPUS_SET_PREDICTION_DISABLED(0));
    opus_encoder_ctl(_enc_voip, OPUS_GET_BANDWIDTH(&opus_bandwidth_voip));
    opus_encoder_ctl(_enc_voip, OPUS_SET_EXPERT_FRAME_DURATION(OPUS_FRAMESIZE_40_MS));
    //opus_encoder_ctl(_enc_voip, OPUS_SET_INBAND_FEC(0));
    opus_decoder_ctl(_dec_voip, OPUS_SET_GAIN(2048));

}

AudioEncoder::~AudioEncoder()
{
    opus_encoder_destroy(_enc);
    opus_decoder_destroy(_dec);
    opus_encoder_destroy(_enc_voip);
    opus_decoder_destroy(_dec_voip);
    codec2_destroy(_codec2_1400);
    codec2_destroy(_codec2_700);
    delete _audio_filter_1400;
    delete _audio_filter2_1400;
}

unsigned char* AudioEncoder::encode_opus(short *audiobuffer, int audiobuffersize, int &encoded_size)
{
    unsigned char *encoded_audio = new unsigned char[47];
    encoded_size = opus_encode(_enc, audiobuffer, audiobuffersize/sizeof(short), encoded_audio, 47);
    return encoded_audio;
}

unsigned char* AudioEncoder::encode_opus_voip(short *audiobuffer, int audiobuffersize, int &encoded_size)
{
    unsigned char *encoded_audio = new unsigned char[120];
    encoded_size = opus_encode(_enc_voip, audiobuffer, audiobuffersize/sizeof(short), encoded_audio, 120);
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

short* AudioEncoder::decode_opus_voip(unsigned char *audiobuffer, int audiobuffersize, int &samples)
{
    int fs = 320;
    short *pcm = new short[fs];
    //memset(pcm,0,(fs)*sizeof(short));
    samples = opus_decode(_dec_voip,audiobuffer,audiobuffersize, pcm, fs, 0);
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
                output = _audio_filter2_1400->do_sample(sample) + 0.1 * (rand() % 1000);// + 0.6375f * _emph_last_input ; // 0.9
                _emph_last_input = output;
                audiobuffer[i] = (short) (output * 0.9);
            }
            else
            {
                output = _audio_filter2_700->do_sample(sample) + 0.1 * (rand() % 1000); //+ 0.9375f * _emph_last_input;  // 0.9
                _emph_last_input = output;
                audiobuffer[i] = (short) (output * 0.9);
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
                audiobuffer[i] = (short) (output * 0.9);
            }
            else
            {
                output = _audio_filter_700->do_sample(sample); // 0.9
                _emph_last_input = output;
                audiobuffer[i] = (short) (output * 0.9); // I'm still getting clipping and don't know where
            }
        }
    }
}


