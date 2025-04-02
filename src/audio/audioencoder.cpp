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
#include <dlfcn.h>


AudioEncoder::AudioEncoder(const Settings *settings)
{
    _settings = settings;
    _processor = new AudioProcessor(settings);
    _vocoder_loaded = false;
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
    _codec2_700 = codec2_create(CODEC2_MODE_700C);
    _codec2_2400 = codec2_create(CODEC2_MODE_2400);
    _codec2_3200 = codec2_create(CODEC2_MODE_3200);



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
    opus_encoder_ctl(_enc, OPUS_SET_EXPERT_FRAME_DURATION(OPUS_FRAMESIZE_ARG));
    opus_decoder_ctl(_dec, OPUS_SET_GAIN(0));

    // VOIP
    int opus_bandwidth_voip;
    opus_encoder_ctl(_enc_voip, OPUS_SET_VBR(0));
    opus_encoder_ctl(_enc_voip, OPUS_SET_BITRATE(_settings->voip_bitrate));
    opus_encoder_ctl(_enc_voip, OPUS_SET_COMPLEXITY(10));
    //opus_encoder_ctl(_enc, OPUS_SET_DTX(0));
    opus_encoder_ctl(_enc_voip, OPUS_SET_LSB_DEPTH(16));
    opus_encoder_ctl(_enc_voip, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
    opus_encoder_ctl(_enc_voip, OPUS_SET_APPLICATION(OPUS_APPLICATION_AUDIO));
    opus_encoder_ctl(_enc_voip, OPUS_SET_MAX_BANDWIDTH(OPUS_BANDWIDTH_WIDEBAND));
    opus_encoder_ctl(_enc_voip, OPUS_SET_PACKET_LOSS_PERC(0));
    opus_encoder_ctl(_enc_voip, OPUS_SET_PREDICTION_DISABLED(0));
    opus_encoder_ctl(_enc_voip, OPUS_GET_BANDWIDTH(&opus_bandwidth_voip));
    opus_encoder_ctl(_enc_voip, OPUS_SET_EXPERT_FRAME_DURATION(OPUS_FRAMESIZE_ARG));
    //opus_encoder_ctl(_enc_voip, OPUS_SET_INBAND_FEC(0));
    opus_decoder_ctl(_dec_voip, OPUS_SET_GAIN(2048));
    _vocoder_plugin = nullptr;
    _vocoder_loaded = loadVocoderPlugin();
}

bool AudioEncoder::loadVocoderPlugin()
{
    QString path = _settings->vocoder_plugin_path + QString("/vocoder_plugin");
    QDir root_dir = QDir::rootPath();
    QFileInfo vocoder_file = root_dir.filePath(path);
    if(!vocoder_file.exists())
    {
        std::cout << "No vocoder plugin found at: " << vocoder_file.absoluteFilePath().toStdString() << std::endl;
        return false;
    }
    void* a = dlopen(path.toLocal8Bit(), RTLD_LAZY);
    if (!a)
    {
        std::cerr << "Unable to load vocoder plugin: " << QString::fromLocal8Bit(dlerror()).toStdString() << std::endl;
        return false;
    }
    create_t* create_a = (create_t*) dlsym(a, "create");
    const char* dlsym_error = dlerror();
    if (dlsym_error)
    {
        std::cerr << "Unable to find vocoder symbol: " << QString::fromLocal8Bit(dlsym_error).toStdString() << std::endl;
        return false;
    }
    _vocoder_plugin = create_a();
    return true;
}

AudioEncoder::~AudioEncoder()
{
    opus_encoder_destroy(_enc);
    opus_decoder_destroy(_dec);
    opus_encoder_destroy(_enc_voip);
    opus_decoder_destroy(_dec_voip);
    codec2_destroy(_codec2_1400);
    codec2_destroy(_codec2_700);
    codec2_destroy(_codec2_2400);
    codec2_destroy(_codec2_3200);
    delete _processor;
    if(_vocoder_loaded)
        delete _vocoder_plugin;
}

void AudioEncoder::set_voip_bitrate(int bitrate)
{
    opus_encoder_ctl(_enc_voip, OPUS_SET_BITRATE(bitrate));
}

unsigned char* AudioEncoder::encode_opus(short *audiobuffer, int audiobuffersize, int &encoded_size)
{
    unsigned char *encoded_audio = new unsigned char[256];
    encoded_size = opus_encode(_enc, audiobuffer, audiobuffersize/sizeof(short), encoded_audio, 256);
    return encoded_audio;
}

unsigned char* AudioEncoder::encode_opus_voip(short *audiobuffer, int audiobuffersize, int &encoded_size)
{
    unsigned char *encoded_audio = new unsigned char[4096];
    encoded_size = opus_encode(_enc_voip, audiobuffer, audiobuffersize/sizeof(short), encoded_audio, 4096);
    return encoded_audio;
}

short* AudioEncoder::decode_opus(unsigned char *audiobuffer, int audiobuffersize, int &samples, int fs)
{
    if(audiobuffersize > 256)
    {
        samples = 0;
        delete[] audiobuffer;
        return nullptr;
    }
    short *pcm = new short[fs];
    //memset(pcm,0,(fs)*sizeof(short));
    samples = opus_decode(_dec,audiobuffer,audiobuffersize, pcm, fs, 0);
    if(samples <= 0)
    {
        delete[] pcm;
        return nullptr;
    }
    return pcm;
}

short* AudioEncoder::decode_opus_voip(unsigned char *audiobuffer, int audiobuffersize, int &samples)
{
    if(audiobuffersize > 512)
    {
        samples = 0;
        delete[] audiobuffer;
        return nullptr;
    }
    int fs = 4096;
    short *pcm = new short[fs];
    //memset(pcm,0,(fs)*sizeof(short));
    samples = opus_decode(_dec_voip,audiobuffer,audiobuffersize, pcm, fs, 0);
    if(samples <= 0)
    {
        delete[] pcm;
        return nullptr;
    }
    return pcm;
}



unsigned char* AudioEncoder::encode_codec2_1400(short *audiobuffer, int audiobuffersize, int &length)
{
    _processor->filter_audio(audiobuffer, audiobuffersize,true,false);
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
    _processor->filter_audio(audiobuffer, audiobuffersize, true, false);
    int bits = codec2_bits_per_frame(_codec2_700);
    int bytes = (bits + 4) / 8;
    unsigned char *encoded = new unsigned char[bytes];
    codec2_encode(_codec2_700, encoded, audiobuffer);
    length = bytes;
    return encoded;
}

unsigned char* AudioEncoder::encode_codec2_2400(short *audiobuffer, int audiobuffersize, int &length)
{
    _processor->filter_audio(audiobuffer, audiobuffersize);
    int bits = codec2_bits_per_frame(_codec2_2400);
    int bytes = bits / 8;
    unsigned char *encoded = new unsigned char[bytes];
    codec2_encode(_codec2_2400, encoded, audiobuffer);
    length = bytes;
    return encoded;
}

unsigned char* AudioEncoder::encode_codec2_3200(short *audiobuffer, int audiobuffersize, int &length)
{
    // 64 bits from 120 16 bit samples
    _processor->filter_audio(audiobuffer, audiobuffersize, true, false);
    int bits = codec2_bits_per_frame(_codec2_3200);
    int bytes = bits / 8;
    unsigned char *encoded = new unsigned char[bytes];
    codec2_encode(_codec2_3200, encoded, audiobuffer);
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
    _processor->filter_audio(decoded, samples*sizeof(short),false,true);
    return decoded;
}

short* AudioEncoder::decode_codec2_700(unsigned char *audiobuffer, int audiobuffersize, int &samples)
{
    Q_UNUSED(audiobuffersize);
    samples = codec2_samples_per_frame(_codec2_700);
    short* decoded = new short[samples];
    memset(decoded,0,(samples)*sizeof(short));
    codec2_decode(_codec2_700, decoded, audiobuffer);
    _processor->filter_audio(decoded, samples*sizeof(short), false, true);
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

short* AudioEncoder::decode_codec2_3200(unsigned char *audiobuffer, int audiobuffersize, int &samples)
{
    Q_UNUSED(audiobuffersize);
    samples = codec2_samples_per_frame(_codec2_3200);
    short* decoded = new short[samples];
    memset(decoded,0,(samples)*sizeof(short));
    codec2_decode(_codec2_3200, decoded, audiobuffer);
    _processor->filter_audio(decoded, samples*sizeof(short),false,true);
    return decoded;
}


short* AudioEncoder::decode_vocoder(unsigned char *audiobuffer, int audiobuffersize, int &samples)
{
    Q_UNUSED(audiobuffersize);
    samples = 160;
    short* decoded;
    if((!_vocoder_loaded) || (!_settings->dmr_vocoder))
    {
        if(_settings->dmr_codec2_bitrate == DMR_CODEC2_BITRATE::CODEC2_2400)
        {
            unsigned char *codec2_frame = new unsigned char[6U];
            memcpy(codec2_frame, audiobuffer, 6U);
            decoded = decode_codec2_2400(codec2_frame, 6U, samples);
            delete[] codec2_frame;
        }
        else
        {
            unsigned char *codec2_frame = new unsigned char[8U];
            memcpy(codec2_frame, audiobuffer, 8U);
            decoded = decode_codec2_3200(codec2_frame, 8U, samples);
            delete[] codec2_frame;
        }
        return decoded;
    }
    else
    {
        decoded = new short[samples];
        memset(decoded,0,(samples)*sizeof(short));
        _vocoder_plugin->decode_2450x1150(decoded, audiobuffer);
        return decoded;
    }
}

unsigned char* AudioEncoder::encode_vocoder(short *audiobuffer, int audiobuffersize, int &length)
{
    length = 9;
    unsigned char* encoded = new unsigned char[length];
    memset(encoded, 0, length);
    if((!_vocoder_loaded) || (!_settings->dmr_vocoder))
    {
        unsigned char *codec2_frame;
        if(_settings->dmr_codec2_bitrate == DMR_CODEC2_BITRATE::CODEC2_2400)
        {
            codec2_frame = encode_codec2_2400(audiobuffer, audiobuffersize, length);
            memcpy(encoded, codec2_frame, 6U);
        }
        else
        {
            codec2_frame = encode_codec2_3200(audiobuffer, audiobuffersize, length);
            memcpy(encoded, codec2_frame, 8U);
        }
        delete[] codec2_frame;
        return encoded;
    }
    else
    {
        _vocoder_plugin->encode_2450x1150(audiobuffer, encoded);
        return encoded;
    }
}




