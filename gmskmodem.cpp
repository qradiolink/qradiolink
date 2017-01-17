// Written by Adrian Musceac YO8RZZ at gmail dot com, started October 2013.
// Parts of the code are based on previous work by Kristoff ON1ARF
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

#include "gmskmodem.h"

GMSKModem::GMSKModem(Settings *settings, AlsaAudio *audio, AudioInterface *audio2, QObject *parent) :
    QObject(parent)
{
    _audio = audio;
    _audio2 = audio2;
    _codec = new AudioEncoder;
    _settings = settings;
    int retval;
    _transmitting = false;
    _pchain=&_chain;
    retval=c2gmsk_param_init(&_parameters);

    _parameters.expected_apiversion=0;
    // modulation parameters
    _parameters.m_version=0; // version 0 for 4800 bps = versioncode 0x27F301
    _parameters.m_bitrate=C2GMSK_MODEMBITRATE_4800;
    _parameters.d_disableaudiolevelcheck=1;
    if (retval != C2GMSK_RET_OK)
    {
        qDebug() << "Error: Could not initialise parameter structures!";
    }

    _gmsk_session=c2gmsk_sess_new(&_parameters,&retval, _pchain);
    c2gmsk_demod_init(_gmsk_session, &_parameters);
    c2gmsk_mod_init(_gmsk_session, &_parameters);
    if (retval != C2GMSK_RET_OK)
    {
        qDebug() << "Error: could not create C2GMSK session: " << retval;
    }
    // c2gmsk_sess_new returns list of supported versions/modes
    _chain=*_pchain;
    _frame_counter = 0;
    _last_frame_type = FrameTypeNone;

}

GMSKModem::~GMSKModem()
{
    c2gmsk_sess_destroy (_gmsk_session);
    delete _codec;
}

void GMSKModem::startTransmission()
{

    int ret=c2gmsk_mod_start(_gmsk_session,_pchain);

    if (ret != C2GMSK_RET_OK)
    {
        qDebug() << "c2gmsk mod start returned an error: " << ret;
    }
    _transmitting = true;
    int tod=C2GMSK_MSG_PCM48K;
    int numsamples;
    c2gmsk_msg *message;
    while ((message = reinterpret_cast<c2gmsk_msg*>(c2gmsk_msgchain_search_tod(C2GMSK_SEARCH_POSCURRENT, _chain, tod, &numsamples, NULL))))
    {

        short *pcmbuffer;
        int nsamples=c2gmsk_msgdecode_pcm48k_p(message,&pcmbuffer);
        if (nsamples != 1920)
        {
               fprintf(stderr,"Internal error in bufferfill_audio_pcm48msg: msgdecode should return 1920 samples, got %d \n",nsamples);
        }
        else
        {
            _audio->write_short(pcmbuffer,nsamples);
        }
    }

}

void GMSKModem::endTransmission()
{
    _frame_counter = 0;
    _transmitting = false;
    int ret=c2gmsk_mod_voice1400_end(_gmsk_session,_pchain);

    if (ret != C2GMSK_RET_OK)
    {
        fprintf(stderr,"c2gmsk mod voice1400/end returned an error: %d \n",ret);
    }
    int tod=C2GMSK_MSG_PCM48K;
    int numsamples;
    c2gmsk_msg *message;
    while ((message = reinterpret_cast<c2gmsk_msg*>(c2gmsk_msgchain_search_tod(C2GMSK_SEARCH_POSCURRENT, _chain, tod, &numsamples, NULL))))
    {
        short *pcmbuffer;
        int nsamples=c2gmsk_msgdecode_pcm48k_p(message,&pcmbuffer);
        if (nsamples != 1920)
        {
               fprintf(stderr,"Internal error in bufferfill_audio_pcm48msg: msgdecode should return 1920 samples, got %d \n",nsamples);
        }
        else
        {
            _audio->write_short(pcmbuffer,nsamples);
        }
    }

    // flush remaining audio in audiobuffer
    ret=c2gmsk_mod_audioflush(_gmsk_session,_pchain);

    if (ret != C2GMSK_RET_OK)
    {
        fprintf(stderr,"Error: c2gmsk_mod_audioflush failed: %d \n",ret);
    }

    while ((message = reinterpret_cast<c2gmsk_msg*>(c2gmsk_msgchain_search_tod(C2GMSK_SEARCH_POSCURRENT, _chain, tod, &numsamples, NULL))))
    {

        short *pcmbuffer;
        int nsamples=c2gmsk_msgdecode_pcm48k_p(message,&pcmbuffer);
        if (nsamples != 1920)
        {
               fprintf(stderr,"Internal error in bufferfill_audio_pcm48msg: msgdecode should return 1920 samples, got %d \n",nsamples);
        }
        else
        {
            _audio->write_short(pcmbuffer,nsamples);
        }
    }

}

void GMSKModem::interleaveHeader()
{
    if(!_transmitting)
        return;
    if(_frame_counter > 24)
    {
        _frame_counter = 0;
        startTransmission();
    }
    else
        _frame_counter++;
}

void GMSKModem::processAudio(short *audiobuffer, short audiobuffersize)
{
    if(!_transmitting)
        return;

    int packet_size = 0;
    unsigned char *encoded_audio;
    encoded_audio = _codec->encode_codec2(audiobuffer, audiobuffersize, packet_size);

    unsigned char *data = new unsigned char[packet_size+1];
    data[0] = 3 << 6;
    memcpy(data+1,encoded_audio,packet_size);
    modulate(data, packet_size+1);
    delete[] encoded_audio;
    delete[] data;
}

void GMSKModem::modulate(unsigned char *encoded_audio, int data_size)
{
    Q_UNUSED(data_size)

    int numsamples;

    // check all messages, we are only interested in PCM48K messages
    int tod=C2GMSK_MSG_PCM48K;
    c2gmsk_msg *message;
    /* //unused
    while (message = reinterpret_cast<c2gmsk_msg*>(c2gmsk_msgchain_search_tod(C2GMSK_SEARCH_POSCURRENT, _chain, tod, &numsamples, NULL)))
    {
        short *pcmbuffer = new short[1920];
        int nsamples=c2gmsk_msgdecode_pcm48k_p(message,&pcmbuffer);
        _audio->write_short(pcmbuffer,nsamples);
        delete[] pcmbuffer;
    }
    */
    int ret=c2gmsk_mod_voice1400(_gmsk_session,(unsigned char *)encoded_audio,_pchain);

    if (ret != C2GMSK_RET_OK)
    {
        fprintf(stderr,"c2gmsk mod voice1400 returned an error: %d \n",ret);
    }

    while ((message = reinterpret_cast<c2gmsk_msg*>(c2gmsk_msgchain_search_tod(C2GMSK_SEARCH_POSCURRENT, _chain, tod, &numsamples, NULL))))
    {
        short *pcmbuffer = new short[1920];

        int nsamples=c2gmsk_msgdecode_pcm48k_p(message,&pcmbuffer);

        if (nsamples != 1920)
        {
               fprintf(stderr,"Internal error in bufferfill_audio_pcm48msg: msgdecode should return 1920 samples, got %d \n",nsamples);
        }
        else
        {

            _audio->write_short(pcmbuffer,nsamples);
        }

    }
}

void GMSKModem::textData(QString text)
{
    QStringList list;

    for( int k=0;k<text.length();k+=6)
    {
        list.append(text.mid(k,6));
    }


    startTransmission();
    for(int o = 0;o < list.length();o++)
    {
        QString chunk=list.at(o);
        unsigned char *data = new unsigned char[7];
        memset(data, 0, 7);
        data[0] = 0;
        memcpy(data+1,chunk.toStdString().c_str(),chunk.length());
        modulate(data,7);
        delete[] data;
    }
    endTransmission();
}

void GMSKModem::demodulate(short *pcm, short size)
{
    Q_UNUSED(size)
    c2gmsk_msg *message;
    int ret2=c2gmsk_demod(_gmsk_session,pcm,_pchain);
    _chain=*_pchain;
    int tod;
    int datasize;
    int data[4];
    unsigned char *all_data = new unsigned char[7];

    if (ret2 != C2GMSK_RET_OK)
    {
        fprintf(stderr,"Error: c2gmsk_dmod failed: %d (%s) \n",ret2,c2gmsk_printstr_ret(ret2));
    }
    while ((message = reinterpret_cast<c2gmsk_msg*>(c2gmsk_msgchain_search(C2GMSK_SEARCH_POSCURRENT, _chain, &tod, &datasize, NULL))))
    {
        memset(all_data, 0 ,7);
        if (tod == C2GMSK_PRINTBIT_MOD)
        {
            //printf("PRINTBIT DEMODULATED DATA:\n%s \n",c2gmsk_msgdecode_printbit (message,txtline,1));
            continue;
        }

        if (tod == C2GMSK_PRINTBIT_ALL)
        {
            //printf("PRINTBIT ALL:\n%s \n",c2gmsk_msgdecode_printbit (message,txtline,1));
            continue;
        }
        ret2=c2gmsk_msgdecode_numeric (message, data);
        if (ret2 > 0)
        {
            if (ret2 > 1)
            {
                //printf("type-of-data %d (%s): %d data fields:\n",tod, c2gmsk_printstr_msg(tod),ret2);
            } else
            {
                //printf("type-of-data %d (%s): 1 data field:\n", tod, c2gmsk_printstr_msg(tod));
            }


            if (ret2 <= 4)
            {
                /*
                // print all data
                int l;

                if (tod == C2GMSK_MSG_VERSIONID)
                {

                    printf("%08X",data[0]);

                    for (l=2; l<=ret2; l++)
                    {
                        printf(", %08X",data[l-1]);
                    }
                    printf("\n");
                }
                else
                {
                    printf("%d",data[0]);
                    for (l=2; l<=ret2; l++)
                    {
                        printf(", %d",data[l-1]);
                    }
                    printf("\n");
                }
                */
            }
            else
            {
                fprintf(stderr,"Error: number of vars returned by msgdecode - numeric not supported!\n");
            }

            continue;
        }
        else if (ret2 < 0)
        {
            fprintf(stderr,"Error: message-decode numeric fails on sanity check!\n");
            continue;
        }; // end else - if


        // codec2?
        ret2=c2gmsk_msgdecode_c2(message, all_data);

        if (ret2 > 0)
        {
            if (ret2 != C2GMSK_CODEC2_1400)
            {
                fprintf(stderr,"Error: unsupported codec2 bitrate %d\n",ret2);
                continue;
            }

            processReceivedData(all_data);

            continue;
        }
        else if (ret2 < 0)
        {
            fprintf(stderr,"Error: message-decode codec2 fails on sanity check!\n");
            continue;
        }


        // end-of-stream notifications
        if ((tod >= 0x50) && (tod <= 0x5f))
        {
            printf("End of stream notification: %s \n",c2gmsk_printstr_msg(tod));
            handleStreamEnd(all_data);
            continue;
        }


        // other things, not processed
        printf("********* Type-of-data: %02X (%s) not processed!!!!  \n",tod,c2gmsk_printstr_msg(tod));

        continue;
    }
    delete[] all_data;
}

void GMSKModem::processReceivedData(unsigned char *all_data)
{
    unsigned char buf = all_data[0];
    buf = buf >> 6;

    if (buf == 0)
    {
        //qDebug("text packet received");
        emit dataFrameReceived();
        _last_frame_type = FrameTypeText;
        char *text_data = new char[6];
        memset(text_data, 0, 6);
        memcpy(text_data, all_data+1, 6);
        quint8 string_length = 6;

        for(int ii=5;ii>=0;ii--)
        {
            QChar x(text_data[ii]);
            if(x.unicode()==0)
            {
                string_length--;
            }
            else
            {
                break;
            }
        }

        emit textReceived( QString::fromLocal8Bit(text_data,string_length));
        delete[] text_data;
    }
    else if (buf == 3)
    {
        //qDebug("audio packet received");
        emit audioFrameReceived();
        _last_frame_type = FrameTypeVoice;
        unsigned char *codec2_data = new unsigned char[6];
        memcpy(codec2_data, all_data+1, 6);
        short *audio_out;
        int samples;
        audio_out = _codec->decode_codec2(codec2_data, 0, samples);
        delete[] codec2_data;
        _audio2->write_short(audio_out,samples*sizeof(short));
    }
}

void GMSKModem::handleStreamEnd(unsigned char *all_data)
{
    Q_UNUSED(all_data)
    if(_last_frame_type == FrameTypeText)
    {
        emit textReceived( QString("\n"));
    }
    else if(_last_frame_type == FrameTypeVoice)
    {
        // do stuff
    }
    emit receiveEnd();
}
