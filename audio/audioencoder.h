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

#ifndef AUDIOENCODER_H
#define AUDIOENCODER_H

#include <QDebug>
#include <opus/opus.h>
#include <codec2/codec2.h>
#include "audio/audioprocessor.h"



class AudioEncoder
{
public:
    AudioEncoder();
    ~AudioEncoder();
    unsigned char *encode_opus(short *audiobuffer, int audiobuffersize, int &encoded_size);
    short *decode_opus(unsigned char *audiobuffer, int data_length, int &samples);
    unsigned char *encode_opus_voip(short *audiobuffer, int audiobuffersize, int &encoded_size);
    short *decode_opus_voip(unsigned char *audiobuffer, int data_length, int &samples);
    unsigned char* encode_codec2_1400(short *audiobuffer, int audiobuffersize, int &length);
    unsigned char* encode_codec2_700(short *audiobuffer, int audiobuffersize, int &length);
    unsigned char* encode_codec2_2400(short *audiobuffer, int audiobuffersize, int &length);
    short* decode_codec2_1400(unsigned char *audiobuffer, int audiobuffersize, int &samples);
    short* decode_codec2_700(unsigned char *audiobuffer, int audiobuffersize, int &samples);
    short* decode_codec2_2400(unsigned char *audiobuffer, int audiobuffersize, int &samples);


private:
    OpusEncoder *_enc;
    OpusDecoder *_dec;
    OpusEncoder *_enc_voip;
    OpusDecoder *_dec_voip;
    struct CODEC2 *_codec2_1400;
    struct CODEC2 *_codec2_700;
    struct CODEC2 *_codec2_2400;
    AudioProcessor *_processor;

};

#endif // AUDIOENCODER_H
