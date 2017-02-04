// Written by Adrian Musceac YO8RZZ at gmail dot com, started August 2014.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
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
    _codec2 = codec2_create(CODEC2_MODE_1300);

    _gsm = gsm_create();

    int opus_bandwidth;
    opus_encoder_ctl(_enc, OPUS_SET_VBR(0));
    opus_encoder_ctl(_enc, OPUS_SET_BITRATE(19400));
    opus_encoder_ctl(_enc, OPUS_SET_COMPLEXITY(5));
    //opus_encoder_ctl(_enc, OPUS_SET_DTX(1));
    opus_encoder_ctl(_enc, OPUS_GET_BANDWIDTH(&opus_bandwidth));
    opus_encoder_ctl(_enc, OPUS_SET_INBAND_FEC(0));
}

AudioEncoder::~AudioEncoder()
{
    opus_encoder_destroy(_enc);
    opus_decoder_destroy(_dec);
    codec2_destroy(_codec2);
    gsm_destroy(_gsm);
}

unsigned char* AudioEncoder::encode_opus(short *audiobuffer, int audiobuffersize, int &encoded_size)
{


    unsigned char *encoded_audio = new unsigned char[97];
    encoded_size = opus_encode(_enc, audiobuffer, audiobuffersize/sizeof(short), encoded_audio, 97);
    return encoded_audio;

}

short* AudioEncoder::decode_opus(unsigned char *audiobuffer, int audiobuffersize, int &samples)
{
    int fs = 320;
    short *pcm = new short[fs];

    samples = opus_decode(_dec,audiobuffer,audiobuffersize, pcm, fs, 0);
    if(samples <= 0)
    {
        delete[] pcm;
        return NULL;
    }
    return pcm;
}

unsigned char* AudioEncoder::encode_codec2(short *audiobuffer, int audiobuffersize, int &length)
{
    Q_UNUSED(audiobuffersize);
    int bits = codec2_bits_per_frame(_codec2);
    int bytes = (bits + 7) / 8;
    unsigned char *encoded = new unsigned char[bytes];
    codec2_encode(_codec2, encoded, audiobuffer);
    length = bytes;
    return encoded;
}

short* AudioEncoder::decode_codec2(unsigned char *audiobuffer, int audiobuffersize, int &samples)
{
    Q_UNUSED(audiobuffersize);
    samples = codec2_samples_per_frame(_codec2);
    short* decoded = new short[samples];
    codec2_decode(_codec2, decoded, audiobuffer);
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
