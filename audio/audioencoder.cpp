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
    _codec2 = codec2_create(CODEC2_MODE_1400);
    _codec2_700 = codec2_create(CODEC2_MODE_700B);

    _gsm = gsm_create();
    _agc = hvdi::initAGC(0.5);

    int opus_bandwidth;
    opus_encoder_ctl(_enc, OPUS_SET_VBR(0));
    opus_encoder_ctl(_enc, OPUS_SET_BITRATE(9400));
    opus_encoder_ctl(_enc, OPUS_SET_COMPLEXITY(10));
    //opus_encoder_ctl(_enc, OPUS_SET_DTX(1));
    opus_encoder_ctl(_enc, OPUS_SET_LSB_DEPTH(16));
    opus_encoder_ctl(_enc, OPUS_SET_MAX_BANDWIDTH(OPUS_BANDWIDTH_FULLBAND));
    opus_encoder_ctl(_enc, OPUS_SET_PACKET_LOSS_PERC(100));
    opus_encoder_ctl(_enc, OPUS_SET_PREDICTION_DISABLED(0));
    opus_encoder_ctl(_enc, OPUS_GET_BANDWIDTH(&opus_bandwidth));
    opus_encoder_ctl(_enc, OPUS_SET_INBAND_FEC(0));
    opus_decoder_ctl(_dec, OPUS_SET_GAIN(-3));
}

AudioEncoder::~AudioEncoder()
{
    opus_encoder_destroy(_enc);
    opus_decoder_destroy(_dec);
    codec2_destroy(_codec2);
    codec2_destroy(_codec2_700);
    gsm_destroy(_gsm);
}

unsigned char* AudioEncoder::encode_opus(short *audiobuffer, int audiobuffersize, int &encoded_size)
{
    unsigned char *encoded_audio = new unsigned char[47];
    memset(encoded_audio,0,47);
    encoded_size = opus_encode(_enc, audiobuffer, audiobuffersize/sizeof(short), encoded_audio, 47);
    return encoded_audio;

}

short* AudioEncoder::decode_opus(unsigned char *audiobuffer, int audiobuffersize, unsigned int &samples)
{
    int fs = 320;
    short *pcm = new short[fs];
    memset(pcm,0,(fs)*sizeof(short));
    samples = opus_decode(_dec,audiobuffer,audiobuffersize, pcm, fs, 0);
    if(samples <= 0)
    {
        delete[] pcm;
        return NULL;
    }
    hvdi::AGC(_agc,pcm,samples);
    return pcm;
}

unsigned char* AudioEncoder::encode_codec2(short *audiobuffer, int audiobuffersize, int &length)
{
    Q_UNUSED(audiobuffersize);
    int bits = codec2_bits_per_frame(_codec2);
    //int bytes = (bits + 7) / 8;
    int bytes = bits / 8;
    unsigned char *encoded = new unsigned char[bytes];
    codec2_encode(_codec2, encoded, audiobuffer);
    length = bytes;
    return encoded;
}

short* AudioEncoder::decode_codec2(unsigned char *audiobuffer, int audiobuffersize, unsigned int &samples)
{
    Q_UNUSED(audiobuffersize);
    samples = codec2_samples_per_frame(_codec2);
    short* decoded = new short[samples];
    memset(decoded,0,(samples)*sizeof(short));
    codec2_decode(_codec2, decoded, audiobuffer);
    hvdi::AGC(_agc,decoded,samples);
    return decoded;
}

unsigned char* AudioEncoder::encode_codec2_700(short *audiobuffer, int audiobuffersize, int &length)
{
    Q_UNUSED(audiobuffersize);
    int bits = codec2_bits_per_frame(_codec2_700);
    int bytes = (bits + 4) / 8;
    unsigned char *encoded = new unsigned char[bytes];
    codec2_encode(_codec2_700, encoded, audiobuffer);
    length = bytes;
    return encoded;
}

short* AudioEncoder::decode_codec2_700(unsigned char *audiobuffer, int audiobuffersize, unsigned int &samples)
{
    Q_UNUSED(audiobuffersize);
    samples = codec2_samples_per_frame(_codec2_700);
    short* decoded = new short[samples];
    memset(decoded,0,(samples)*sizeof(short));
    codec2_decode(_codec2_700, decoded, audiobuffer);
    hvdi::AGC(_agc,decoded,samples);
    return decoded;
}

unsigned char* AudioEncoder::encode_gsm(short *audiobuffer, int audiobuffersize, int &length)
{
    length = sizeof(gsm_frame);
    unsigned char *encoded = new unsigned char[length];
    gsm_encode(_gsm,audiobuffer,encoded);
    return encoded;
}

short* AudioEncoder::decode_gsm(unsigned char *audiobuffer, int data_length, unsigned int &samples)
{
    samples = 160;
    short* decoded = new short[160];
    gsm_decode(_gsm,audiobuffer,decoded);
    return decoded;
}


