// Written by Adrian Musceac YO8RZZ at gmail dot com, started October 2013.
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

#include "radioop.h"

RadioOp::RadioOp(Settings *settings, QObject *parent) :
    QObject(parent)
{
    _codec = new AudioEncoder;
    _audio = new AudioInterface;
    _stop =false;
    _tx_inited = false;
    _rx_inited = false;
    _settings = settings;
    _transmitting = false;
    _process_text = false;
    _repeat_text = false;
    _settings = settings;
    _led_timer = new QTimer(this);
    QObject::connect(_led_timer, SIGNAL(timeout()), this, SLOT(syncIssue()));
    _modem = new gr_modem(_settings);
    QObject::connect(_modem,SIGNAL(textReceived(QString)),this,SLOT(textReceived(QString)));
    QObject::connect(_modem,SIGNAL(audioFrameReceived()),this,SLOT(audioFrameReceived()));
    QObject::connect(_modem,SIGNAL(dataFrameReceived()),this,SLOT(dataFrameReceived()));
    QObject::connect(_modem,SIGNAL(receiveEnd()),this,SLOT(receiveEnd()));
    QObject::connect(this,SIGNAL(audioData(unsigned char*,short)),_modem,SLOT(processC2Data(unsigned char*,short)));
    QObject::connect(_modem,SIGNAL(codec2Audio(unsigned char*,short)),this,SLOT(receiveC2Data(unsigned char*,short)));
}

RadioOp::~RadioOp()
{
    delete _codec;
    delete _audio;
    delete _led_timer;
    delete _modem;
}

void RadioOp::stop()
{
    _stop=true;
}

void RadioOp::run()
{
#if 0
    int audiobuffer_size = 1920; //40 ms @ 48k
    AlsaAudio *audio = new AlsaAudio("pulseusb");
    GMSKModem *modem = new GMSKModem(_settings, audio, audio2);
#endif
    int audiobuffer_size2 = 640; //40 ms @ 8k

    bool transmit_activated = false;
    while(true)
    {
        _mutex.lock();
        bool transmitting = _transmitting;
        _mutex.unlock();
        QCoreApplication::processEvents();
        short *audiobuffer2 = new short[audiobuffer_size2/2];
#if 0
        short *audiobuffer = new short[audiobuffer_size];
        memset(audiobuffer,0,audiobuffer_size2*sizeof(short));
        audio->read_short(audiobuffer,audiobuffer_size);
        modem->demodulate(audiobuffer, audiobuffer_size);
#endif
        if(_rx_inited)
            _modem->demodulate();

        if(transmitting && !transmit_activated)
        {
            transmit_activated = true;
            if(_tx_inited)
                _modem->startTransmission();
        }
        if(!transmitting && transmit_activated)
        {
            transmit_activated = false;
            if(_tx_inited)
                _modem->endTransmission();
        }
        if(transmitting)
        {
            _audio->read_short(audiobuffer2,audiobuffer_size2);
            int packet_size = 0;
            unsigned char *encoded_audio;
            encoded_audio = _codec->encode_codec2(audiobuffer2, audiobuffer_size2, packet_size);
            unsigned char *data = new unsigned char[packet_size];
            memcpy(data,encoded_audio,packet_size);
            if(_tx_inited)
                emit audioData(data,packet_size);
            delete[] encoded_audio;
        }
        else
        {
            usleep(40000);
        }
        if(_process_text)
        {
            if(_tx_inited)
                _modem->textData(_text_out);
            if(!_repeat_text)
            {
                _mutex.lock();
                _process_text = false;
                _mutex.unlock();
            }
            emit displayTransmitStatus(false);
        }

#if 0
        delete[] audiobuffer;
#endif
        delete[] audiobuffer2;
        if(_stop)
            break;
    }
#if 0
    delete audio;
#endif
    //delete modem;
    emit finished();
}

void RadioOp::receiveC2Data(unsigned char *data, short size)
{
    short *audio_out;
    int samples;
    audio_out = _codec->decode_codec2(data, size, samples);
    delete[] data;
    _audio->write_short(audio_out,samples*sizeof(short));
}

void RadioOp::startTransmission()
{
    _mutex.lock();
    _transmitting = true;
    _mutex.unlock();
}

void RadioOp::endTransmission()
{
    _transmitting = false;
}

void RadioOp::textData(QString text, bool repeat)
{
    _repeat_text = repeat;
    _mutex.lock();
    _text_out = text;
    _process_text = true;
    _mutex.unlock();

}
void RadioOp::textReceived(QString text)
{
    emit printText(text);
}

void RadioOp::audioFrameReceived()
{
    emit displayReceiveStatus(true);
    _led_timer->start(1000);
}

void RadioOp::dataFrameReceived()
{
    emit displayDataReceiveStatus(true);
    _led_timer->start(1000);
}

void RadioOp::receiveEnd()
{
    emit displayReceiveStatus(false);
    emit displayDataReceiveStatus(false);
#if 0
    QSoundEffect *end_beep = new QSoundEffect;
    end_beep->setSource(QUrl("qrc:res/end_beep.wav"));
    end_beep->play();
    delete end_beep;
#endif
}

void RadioOp::syncIssue()
{
    emit displaySyncIssue(true);
}

void RadioOp::toggleRX(bool value)
{
    if(value)
    {
        _rx_inited = true;
        _modem->initRX(gr_modem_types::ModemTypeBPSK2000);
    }
    else
    {
        _rx_inited = false;
        _modem->deinitRX(gr_modem_types::ModemTypeBPSK2000);
    }
}

void RadioOp::toggleTX(bool value)
{
    if(value)
    {
        _tx_inited = true;
        _modem->initTX(gr_modem_types::ModemTypeBPSK2000);
    }
    else
    {
        _tx_inited = false;
        _modem->deinitTX(gr_modem_types::ModemTypeBPSK2000);
    }
}

void RadioOp::tuneFreq(long center_freq)
{
    _modem->tune(434025000 + center_freq*100);
}
