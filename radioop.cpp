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

RadioOp::RadioOp(Settings *settings, gr::qtgui::const_sink_c::sptr const_gui,
                 gr::qtgui::number_sink::sptr rssi_gui, QObject *parent) :
    QObject(parent)
{
    _mode = gr_modem_types::ModemTypeBPSK2000;
    _radio_type = radio_type::RADIO_TYPE_DIGITAL;
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
    _tune_center_freq = 0;
    _tune_limit_lower = -5000;
    _tune_limit_upper = 5000;
    _step_hz = 1;
    _tuning_done = true;
    _tune_counter = 0;
    _tx_modem_started = false;
    _led_timer = new QTimer(this);
    _rand_frame_data = new unsigned char[5000];
    QObject::connect(_led_timer, SIGNAL(timeout()), this, SLOT(syncIssue()));
    _modem = new gr_modem(_settings, const_gui, rssi_gui);
    QObject::connect(_modem,SIGNAL(textReceived(QString)),this,SLOT(textReceived(QString)));
    QObject::connect(_modem,SIGNAL(audioFrameReceived()),this,SLOT(audioFrameReceived()));
    QObject::connect(_modem,SIGNAL(dataFrameReceived()),this,SLOT(dataFrameReceived()));
    QObject::connect(_modem,SIGNAL(receiveEnd()),this,SLOT(receiveEnd()));
    QObject::connect(this,SIGNAL(audioData(unsigned char*,int)),_modem,SLOT(processC2Data(unsigned char*,int)));
    QObject::connect(this,SIGNAL(videoData(unsigned char*,int)),_modem,SLOT(processVideoData(unsigned char*,int)));
    QObject::connect(_modem,SIGNAL(codec2Audio(unsigned char*,int)),this,SLOT(receiveC2Data(unsigned char*,int)));
    QObject::connect(_modem,SIGNAL(videoData(unsigned char*,int)),this,SLOT(receiveVideoData(unsigned char*,int)));
    for (int j = 0;j<5000;j++)
        _rand_frame_data[j] = rand() % 256;

}

RadioOp::~RadioOp()
{
    delete _codec;
    delete _video;
    delete _audio;
    delete _led_timer;
    delete _modem;
}

void RadioOp::stop()
{
    _stop=true;
}

void RadioOp::processAudioStream()
{
    int audiobuffer_size = 640; //40 ms @ 8k
    short *audiobuffer = new short[audiobuffer_size/2];
    _audio->read_short(audiobuffer,audiobuffer_size);
    int packet_size = 0;
    unsigned char *encoded_audio;
    if((_mode == gr_modem_types::ModemTypeBPSK2000) ||
            (_mode == gr_modem_types::ModemType4FSK2000) ||
            (_mode == gr_modem_types::ModemTypeQPSK2000))
        encoded_audio = _codec->encode_codec2(audiobuffer, audiobuffer_size, packet_size);
    else
        encoded_audio = _codec->encode_opus(audiobuffer, audiobuffer_size, packet_size);

    unsigned char *data = new unsigned char[packet_size];
    memcpy(data,encoded_audio,packet_size);
    emit audioData(data,packet_size);
    delete[] encoded_audio;
    delete[] audiobuffer;
}

int RadioOp::processVideoStream(bool &frame_flag)
{
    int max_video_frame_size = 3122;
    unsigned long encoded_size;

    unsigned char *videobuffer = (unsigned char*)calloc(max_video_frame_size, sizeof(unsigned char));

    QElapsedTimer timer;
    qint64 microsec;
    timer.start();

    _video->encode_jpeg(&(videobuffer[12]), encoded_size, max_video_frame_size);

    microsec = (quint64)timer.nsecsElapsed()/1000;
    if(microsec < 100000)
    {
        usleep((100000 - microsec) - 10000);
    }

    //qDebug() << "video out " << microsec << " / " << encoded_size;

    if(encoded_size > max_video_frame_size)
    {
        qDebug() << "Too large frame size (dropped): " << encoded_size;
        delete[] videobuffer;
        return -EINVAL;
    }
    memcpy(&(videobuffer[0]), &encoded_size, 4);
    memcpy(&(videobuffer[4]), &encoded_size, 4);
    memcpy(&(videobuffer[8]), &encoded_size, 4);
    for(int k=encoded_size,i=0;k<max_video_frame_size;k++,i++)
    {

        videobuffer[k] = _rand_frame_data[i];

    }

    emit videoData(videobuffer,max_video_frame_size);
    return 1;
}

void RadioOp::run()
{


    bool transmit_activated = false;
    bool frame_flag = true;
    while(true)
    {
        bool transmitting = _transmitting;
        QCoreApplication::processEvents();

        if(transmitting && !transmit_activated)
        {
            transmit_activated = true;
            if(_tx_inited)
            {
                if(_rx_inited)
                    _modem->stopRX();
                if(_tx_modem_started)
                    _modem->stopTX();
                _modem->startTX();
                _tx_modem_started = false;
                if(_radio_type == radio_type::RADIO_TYPE_DIGITAL)
                    _modem->startTransmission();
            }
        }
        if(!transmitting && transmit_activated)
        {
            transmit_activated = false;
            if(_tx_inited)
            {
                if(_radio_type == radio_type::RADIO_TYPE_DIGITAL)
                    _modem->endTransmission();
                _modem->stopTX();
                _tx_modem_started = false;
            }
            usleep(40000);
            if(_rx_inited)
                _modem->startRX();
        }
        if(transmitting && (_radio_type == radio_type::RADIO_TYPE_DIGITAL))
        {
            if(_mode == gr_modem_types::ModemTypeQPSKVideo)
                processVideoStream(frame_flag);

            else
            {
                processAudioStream();
            }
        }
        else
        {
            if(_rx_inited)
            {
                if(_modem->_frequency_found == 0)
                {
                    emit displayReceiveStatus(false);
                    emit displayDataReceiveStatus(false);
                }
                if(!_tuning_done)
                    autoTune();
                if(_radio_type == radio_type::RADIO_TYPE_DIGITAL)
                    _modem->demodulate();
            }
        }
        if(_process_text && (_radio_type == radio_type::RADIO_TYPE_DIGITAL))
        {
            if(_tx_inited) {
                if(!_tx_modem_started)
                {
                    _modem->stopTX();
                    _modem->startTX();
                }
                _tx_modem_started = true;
                _modem->textData(_text_out);
            }
            if(!_repeat_text)
            {
                _mutex.lock();
                _process_text = false;
                _mutex.unlock();
            }
            emit displayTransmitStatus(false);
        }

        usleep(5000);
        if(_stop)
            break;
    }

    emit finished();
}

void RadioOp::receiveC2Data(unsigned char *data, int size)
{
    short *audio_out;
    int samples;
    if((_mode == gr_modem_types::ModemTypeBPSK2000) ||
            (_mode == gr_modem_types::ModemType4FSK2000) ||
            (_mode == gr_modem_types::ModemTypeQPSK2000))
    {
        audio_out = _codec->decode_codec2(data, size, samples);
    }
    else
        audio_out = _codec->decode_opus(data, size, samples);
    delete[] data;
    if(samples > 0)
        _audio->write_short(audio_out,samples*sizeof(short));
}

void RadioOp::receiveVideoData(unsigned char *data, int size)
{
    unsigned long frame_size1;
    unsigned long frame_size2;
    unsigned long frame_size3;
    int frame_size;

    memcpy(&frame_size1, &data[0], 4);
    memcpy(&frame_size2, &data[4], 4);
    memcpy(&frame_size3, &data[8], 4);
    if(frame_size1 == frame_size2)
        frame_size = (int)frame_size1;
    else if(frame_size1 == frame_size3)
        frame_size = (int)frame_size1;
    else if(frame_size2 == frame_size3)
        frame_size = (int)frame_size2;
    else
    {
        qDebug() << "received corrupted frame size, dropping frame ";
        delete[] data;
        return;
    }
    unsigned char *jpeg_frame = new unsigned char[frame_size];
    memcpy(jpeg_frame, &data[12], frame_size);
    delete[] data;
    unsigned char *raw_output = _video->decode_jpeg(jpeg_frame,frame_size);

    if(!raw_output)
    {
        delete[] jpeg_frame;
        return;
    }
    QImage img (320,240,
                             QImage::Format_RGB888);
    img.loadFromData(raw_output, 230400, 0);
    img.convertToFormat(QImage::Format_RGB32);
    QImage out_img(img.scaled(640, 480));

    emit videoImage(out_img);
    delete[] raw_output;

}

void RadioOp::startTransmission()
{
    if(_tx_inited)
        _transmitting = true;
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
        _modem->initRX(_mode);
        _modem->startRX();
        _tune_center_freq = _modem->_requested_frequency_hz;
    }
    else
    {
        _rx_inited = false;
        _modem->stopRX();
        _modem->deinitRX(_mode);
    }
}

void RadioOp::toggleTX(bool value)
{
    if(value)
    {
        _tx_inited = true;
        _modem->initTX(_mode);
        if(_mode == gr_modem_types::ModemTypeQPSKVideo)
            _video = new VideoEncoder;
    }
    else
    {
        _tx_inited = false;
        _modem->deinitTX(_mode);
        if(_mode == gr_modem_types::ModemTypeQPSKVideo)
            delete _video;
    }
}

void RadioOp::toggleMode(int value)
{
    bool rx_inited_before = _rx_inited;
    bool tx_inited_before = _tx_inited;
    if(_rx_inited)
    {
        toggleRX(false);
    }
    if(_tx_inited)
    {
        toggleTX(false);
    }
    _radio_type = radio_type::RADIO_TYPE_DIGITAL;
    switch(value)
    {
    case 0:
        _mode = gr_modem_types::ModemTypeBPSK2000;
        _tune_limit_lower = -2000;
        _tune_limit_upper = 2000;
        _step_hz = 1;
        break;
    case 1:
        _mode = gr_modem_types::ModemTypeQPSK20000;
        _tune_limit_lower = -5000;
        _tune_limit_upper = 5000;
        _step_hz = 10;
        break;
    case 2:
        _mode = gr_modem_types::ModemType4FSK20000;
        _tune_limit_lower = -5000;
        _tune_limit_upper = 5000;
        _step_hz = 10;
        break;
    case 3:
        _mode = gr_modem_types::ModemType4FSK2000;
        _tune_limit_lower = -2000;
        _tune_limit_upper = 2000;
        _step_hz = 1;
        break;
    case 4:
        _mode = gr_modem_types::ModemTypeQPSK2000;
        _tune_limit_lower = -2000;
        _tune_limit_upper = 2000;
        _step_hz = 1;
        break;
    case 5:
        _radio_type = radio_type::RADIO_TYPE_ANALOG;
        _mode = gr_modem_types::ModemTypeNBFM2500;
        _tune_limit_lower = -5000;
        _tune_limit_upper = 5000;
        _step_hz = 5;
        break;
    case 6:
        _radio_type = radio_type::RADIO_TYPE_ANALOG;
        _mode = gr_modem_types::ModemTypeSSB2500;
        _tune_limit_lower = -1500;
        _tune_limit_upper = 1500;
        _step_hz = 1;
        break;
    case 7:
        _mode = gr_modem_types::ModemTypeQPSKVideo;
        _tune_limit_lower = -15000;
        _tune_limit_upper = 15000;
        _step_hz = 100;
        break;
    default:
        _mode = gr_modem_types::ModemTypeBPSK2000;
        _tune_limit_lower = -2000;
        _tune_limit_upper = 2000;
        _step_hz = 1;
        break;
    }
    if(rx_inited_before)
        toggleRX(true);

    if(tx_inited_before)
        toggleTX(true);
}

void RadioOp::fineTuneFreq(long center_freq)
{
    _modem->tune(434025000 + center_freq*10, false);
}

void RadioOp::tuneFreq(long center_freq)
{
    _modem->tune(center_freq*1000, false);
}

void RadioOp::setTxPower(int dbm)
{
    _modem->setTxPower(dbm);
}

void RadioOp::syncFrequency()
{
    if(_modem->_frequency_found >= 254)
    {
        _tune_counter = 0;
        _modem->_frequency_found = 10;
    }
    qDebug() << "modem counter: " << _modem->_frequency_found << " op counter: " << _tune_counter;
    if((_modem->_frequency_found > 20) && (_modem->_frequency_found >= _tune_counter))
    {
        _tune_counter = _modem->_frequency_found;
        return;
    }
    _tune_counter = _modem->_frequency_found;
    autoTune();

}

void RadioOp::autoTune()
{
    if(_mode == gr_modem_types::ModemTypeBPSK2000 )
        usleep(500);
    else if ((_mode == gr_modem_types::ModemType4FSK2000) ||
             (_mode == gr_modem_types::ModemTypeQPSK2000))
        usleep(2000);
    else
        usleep(10);
    _tune_center_freq = _tune_center_freq + _step_hz;
    _modem->tune(_tune_center_freq, true);
    if(_tune_center_freq >= (_modem->_requested_frequency_hz + _tune_limit_upper))
        _tune_center_freq = _modem->_requested_frequency_hz + _tune_limit_lower;
}

void RadioOp::startAutoTune()
{
    _tuning_done = false;
}

void RadioOp::stopAutoTune()
{
    _tuning_done = true;
}
