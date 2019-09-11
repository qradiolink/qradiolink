// Written by Adrian Musceac YO8RZZ , started October 2013.
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

#include "radioop.h"

RadioOp::RadioOp(Settings *settings, QObject *parent) :
    QObject(parent)
{
    _rx_mode = gr_modem_types::ModemTypeBPSK2000;
    _tx_mode = gr_modem_types::ModemTypeBPSK2000;
    _rx_radio_type = radio_type::RADIO_TYPE_DIGITAL;
    _tx_radio_type = radio_type::RADIO_TYPE_DIGITAL;
    _codec = new AudioEncoder;
    _audio = new AudioInterface;
    _radio_protocol = new RadioProtocol;
    _mutex = new QMutex;
    _m_queue = new std::vector<short>;
    _last_session_id = 0;
    _net_device = 0;
    _video = 0;
    _stop =false;
    _tx_inited = false;
    _rx_inited = false;
    _tx_started = false;
    _voip_enabled = false;
    _vox_enabled = false;
    _voip_forwarding = false;
    _last_voiced_frame_timer.start();
    _voip_tx_timer = new QTimer(this);
    _voip_tx_timer->setSingleShot(true);
    _vox_timer = new QTimer(this);
    _vox_timer->setSingleShot(true);
    _data_read_timer = new QElapsedTimer();
    _data_modem_reset_timer = new QElapsedTimer();
    _data_modem_sleep_timer = new QElapsedTimer();
    _scan_timer = new QElapsedTimer();
    _scan_stop = false;
    _fft_read_timer = new QElapsedTimer();
    _fft_read_timer->start();
    _fft_poll_time = 75;
    _fft_enabled = true;
    _const_read_timer = new QElapsedTimer();
    _const_read_timer->start();
    _constellation_enabled = false;
    _data_modem_sleeping = false;
    _settings = settings;
    _transmitting_audio = false;
    _process_text = false;
    _repeat_text = false;
    _repeat = false;
    _settings = settings;
    _tx_power = 0;
    _bb_gain = 0;
    _rx_sensitivity = 0;
    _rx_volume = 0.0;
    _squelch = 0;
    _rx_ctcss = 0.0;
    _tx_ctcss = 0.0;
    _rx_frequency = 0;
    _tx_frequency = 0;
    _autotune_freq = 0;
    _tune_shift_freq = 0;
    _tune_limit_lower = -500000;
    _tune_limit_upper = 500000;
    _step_hz = 12500;
    _carrier_offset = 0;
    _rx_sample_rate = 0;
    _tuning_done = true;
    _tune_counter = 0;
    _freq_gui_counter = 0;
    _tx_modem_started = false;
    _voice_led_timer = new QTimer(this);
    _voice_led_timer->setSingleShot(true);
    _data_led_timer = new QTimer(this);
    _data_led_timer->setSingleShot(true);
    _rand_frame_data = new unsigned char[5000];
    _voip_encode_buffer = new QVector<short>;
    _fft_data = new std::complex<float>[1024*1024];


    QObject::connect(_voice_led_timer, SIGNAL(timeout()), this, SLOT(receiveEnd()));
    QObject::connect(_data_led_timer, SIGNAL(timeout()), this, SLOT(receiveEnd()));
    QObject::connect(_data_led_timer, SIGNAL(timeout()), this, SLOT(receiveEnd()));
    QObject::connect(_voip_tx_timer, SIGNAL(timeout()), this, SLOT(stopTx()));
    _modem = new gr_modem(_settings);

    QObject::connect(_modem,SIGNAL(textReceived(QString)),this,SLOT(textReceived(QString)));
    QObject::connect(_modem,SIGNAL(repeaterInfoReceived(QByteArray)),this,SLOT(repeaterInfoReceived(QByteArray)));
    QObject::connect(_modem,SIGNAL(callsignReceived(QString)),this,SLOT(callsignReceived(QString)));
    QObject::connect(_modem,SIGNAL(audioFrameReceived()),this,SLOT(audioFrameReceived()));
    QObject::connect(_modem,SIGNAL(dataFrameReceived()),this,SLOT(dataFrameReceived()));
    QObject::connect(_modem,SIGNAL(receiveEnd()),this,SLOT(receiveEnd()));
    QObject::connect(_modem,SIGNAL(endAudioTransmission()),this,SLOT(endAudioTransmission()));
    QObject::connect(this,SIGNAL(audioData(unsigned char*,int)),_modem,SLOT(processAudioData(unsigned char*,int)));
    QObject::connect(this,SIGNAL(pcmData(std::vector<float>*)),_modem,SLOT(processPCMAudio(std::vector<float>*)));
    QObject::connect(this,SIGNAL(videoData(unsigned char*,int)),_modem,SLOT(processVideoData(unsigned char*,int)));
    QObject::connect(this,SIGNAL(netData(unsigned char*,int)),_modem,SLOT(processNetData(unsigned char*,int)));
    QObject::connect(_modem,SIGNAL(digitalAudio(unsigned char*,int)),this,SLOT(receiveAudioData(unsigned char*,int)));
    QObject::connect(_modem,SIGNAL(pcmAudio(std::vector<float>*)),this,SLOT(receivePCMAudio(std::vector<float>*)));
    QObject::connect(_modem,SIGNAL(videoData(unsigned char*,int)),this,SLOT(receiveVideoData(unsigned char*,int)));
    QObject::connect(_modem,SIGNAL(netData(unsigned char*,int)),this,SLOT(receiveNetData(unsigned char*,int)));
    for (int j = 0;j<5000;j++)
        _rand_frame_data[j] = rand() % 256;

    QFile resfile(":/res/data_rec.raw");
    if(resfile.open(QIODevice::ReadOnly))
    {
        _data_rec_sound = new QByteArray(resfile.readAll());
    }
    _settings->readConfig();
    toggleRxMode(_settings->rx_mode);
    toggleTxMode(_settings->tx_mode);

}

RadioOp::~RadioOp()
{
    if(_rx_inited)
        toggleRX(false);
    if(_tx_inited)
        toggleTX(false);
    delete _codec;
    if(_video != 0)
        delete _video;
    if(_net_device != 0)
        delete _net_device;
    delete _audio;
    delete _radio_protocol;
    delete _voice_led_timer;
    delete _data_led_timer;
    delete _voip_tx_timer;
    delete _modem;
    delete[] _rand_frame_data;
    _voip_encode_buffer->clear();
    delete _voip_encode_buffer;
    _m_queue->clear();
    delete _m_queue;
}

void RadioOp::stop()
{
    if(_rx_inited)
        toggleRX(false);
    if(_tx_inited)
        toggleTX(false);
    _stop=true;
}

void RadioOp::readConfig(std::string &rx_device_args, std::string &tx_device_args,
                         std::string &rx_antenna, std::string &tx_antenna, int &rx_freq_corr,
                         int &tx_freq_corr, std::string &callsign, std::string &video_device)
{
    _settings->readConfig();
    int tx_power, rx_sensitivity, squelch, rx_volume, bb_gain;
    long long rx_frequency, tx_shift, demod_offset, rx_sample_rate;
    rx_device_args = _settings->rx_device_args.toStdString();
    tx_device_args = _settings->tx_device_args.toStdString();
    rx_antenna = _settings->rx_antenna.toStdString();
    tx_antenna = _settings->tx_antenna.toStdString();
    rx_freq_corr = _settings->rx_freq_corr;
    tx_freq_corr = _settings->tx_freq_corr;
    video_device = _settings->video_device.toStdString();
    tx_power = _settings->tx_power;
    bb_gain = _settings->bb_gain;
    rx_sensitivity = _settings->rx_sensitivity;
    squelch = _settings->squelch;
    rx_volume = _settings->rx_volume;
    rx_frequency = _settings->rx_frequency;
    demod_offset = _settings->demod_offset;
    tx_shift = _settings->tx_shift;
    rx_sample_rate = _settings->rx_sample_rate;

    _callsign = _settings->callsign;
    if(_callsign.size() < 7)
    {
        QString pad = QString(" ").repeated(7 - _callsign.size());
        _callsign = _callsign + pad;
    }
    if(_callsign.size() > 7)
    {
        _callsign = _callsign.mid(0,7);
    }
    if(_tx_power == 0)
    {
        _tx_power = (float)tx_power/100;
    }
    _bb_gain = bb_gain;

    if(_rx_sensitivity == 0)
    {
        _rx_sensitivity = (float)rx_sensitivity/100.0;
    }
    if(_rx_frequency == 0)
    {
        _rx_frequency = rx_frequency;
    }
    if(_tx_frequency == 0)
    {
        _tx_frequency = rx_frequency + demod_offset;
    }
    if(_carrier_offset == 0)
    {
        _carrier_offset = demod_offset;
    }
    if(_rx_sample_rate == 0)
    {
        _rx_sample_rate = rx_sample_rate;
    }
    if(_tune_shift_freq == 0)
    {
        _tune_shift_freq = tx_shift;
    }
    if(_squelch == 0)
    {
        _squelch = squelch;
    }
    if(_rx_volume == 0)
    {
        _rx_volume = (float)rx_volume/50.0;
    }

}

void RadioOp::processAudioStream()
{
    if(!_transmitting_audio && !_vox_enabled)
        return;
    if(_tx_mode == gr_modem_types::ModemTypeQPSK250000)
        return;
    int audio_mode;
    if((_tx_mode == gr_modem_types::ModemTypeBPSK2000) ||
            (_tx_mode == gr_modem_types::ModemType2FSK2000) ||
            (_tx_mode == gr_modem_types::ModemType4FSK2000) ||
            (_tx_mode == gr_modem_types::ModemTypeQPSK2000))
        audio_mode = AudioInterface::AUDIO_MODE_CODEC2;
    else
        audio_mode = AudioInterface::AUDIO_MODE_OPUS;
    int audiobuffer_size = 640; //40 ms @ 8k
    short *audiobuffer = new short[audiobuffer_size/sizeof(short)];
    int vad = _audio->read_short(audiobuffer,audiobuffer_size, true, audio_mode);
    if(_vox_enabled)
    {
        if(vad)
        {
            _vox_timer->start(100);
            if(!_tx_started && !_voip_enabled)
                _transmitting_audio = true;
        }
        if(!vad && !_vox_timer->isActive())
        {
            if(_tx_started && !_voip_enabled)
                _transmitting_audio = false;
            delete[] audiobuffer;
            return;
        }
    }

    if(_voip_enabled)
    {
        emit voipData(audiobuffer,audiobuffer_size);
        return;
    }
    if(_tx_inited)
        txAudio(audiobuffer, audiobuffer_size);
    else
        delete[] audiobuffer;
}

void RadioOp::txAudio(short *audiobuffer, int audiobuffer_size)
{
    if(_tx_radio_type == radio_type::RADIO_TYPE_ANALOG)
    {
        std::vector<float> *pcm = new std::vector<float>;

        for(unsigned int i=0;i<audiobuffer_size/sizeof(short);i++)
        {
            pcm->push_back((float)audiobuffer[i] / 32767.0f);
        }

        emit pcmData(pcm);
        delete[] audiobuffer;
        return;
    }

    int packet_size = 0;
    unsigned char *encoded_audio;
    if((_tx_mode == gr_modem_types::ModemTypeBPSK2000) ||
            (_tx_mode == gr_modem_types::ModemType2FSK2000) ||
            (_tx_mode == gr_modem_types::ModemType4FSK2000) ||
            (_tx_mode == gr_modem_types::ModemTypeQPSK2000))
        encoded_audio = _codec->encode_codec2_1400(audiobuffer, audiobuffer_size, packet_size);
    else if(_tx_mode == gr_modem_types::ModemTypeBPSK1000)
        encoded_audio = _codec->encode_codec2_700(audiobuffer, audiobuffer_size, packet_size);
    else
        encoded_audio = _codec->encode_opus(audiobuffer, audiobuffer_size, packet_size);

    emit audioData(encoded_audio,packet_size);
    delete[] audiobuffer;
}

int RadioOp::processVideoStream(bool &frame_flag)
{
    unsigned int max_video_frame_size = 3122;
    unsigned long encoded_size;

    unsigned char *videobuffer = (unsigned char*)calloc(max_video_frame_size, sizeof(unsigned char));

    QElapsedTimer timer;
    qint64 microsec;
    timer.start();

    _video->encode_jpeg(&(videobuffer[24]), encoded_size, max_video_frame_size - 24);

    microsec = (quint64)timer.nsecsElapsed();
    if(microsec < 100000000)
    {
        struct timespec time_to_sleep = {0, (100000000 - microsec) - 2000000 };
        nanosleep(&time_to_sleep, NULL);
    }

    //qDebug() << "video out " << microsec << " / " << encoded_size;

    if(encoded_size > max_video_frame_size - 24)
    {
        encoded_size = max_video_frame_size - 24;
    }
    unsigned int crc = gr::digital::crc32(&(videobuffer[24]), encoded_size);
    memcpy(&(videobuffer[0]), &encoded_size, 4);
    memcpy(&(videobuffer[4]), &encoded_size, 4);
    memcpy(&(videobuffer[8]), &encoded_size, 4);
    memcpy(&(videobuffer[12]), &crc, 4);
    memcpy(&(videobuffer[16]), &crc, 4);
    memcpy(&(videobuffer[20]), &crc, 4);
    for(unsigned int k=encoded_size+24,i=0;k<max_video_frame_size;k++,i++)
    {

        videobuffer[k] = _rand_frame_data[i];

    }

    emit videoData(videobuffer,max_video_frame_size);
    return 1;
}

void RadioOp::processNetStream()
{
    // 48400
    qint64 time_per_frame = 48400000;
    qint64 microsec, time_left;
    microsec = (quint64)_data_read_timer->nsecsElapsed();
    if(microsec < 46400000)
    {
        return;
    }
    time_left = time_per_frame - microsec;
    struct timespec time_to_sleep = {0, time_left };

    if(time_left > 0)
        nanosleep(&time_to_sleep, NULL);
    _data_read_timer->restart();
    int max_frame_size = 1516;
    unsigned char *netbuffer = (unsigned char*)calloc(max_frame_size, sizeof(unsigned char));
    int nread;
    unsigned char *buffer = _net_device->read_buffered(nread);

    if(nread > 0)
    {
        unsigned int crc = gr::digital::crc32(buffer, nread);
        memcpy(&(netbuffer[0]), &nread, 4);
        memcpy(&(netbuffer[4]), &nread, 4);
        memcpy(&(netbuffer[8]), &nread, 4);
        memcpy(&(netbuffer[12]), &crc, 4);
        memcpy(&(netbuffer[16]), buffer, nread);
        for(int k=nread+16,i=0;k<max_frame_size;k++,i++)
        {
            netbuffer[k] = _rand_frame_data[i];
        }

        emit netData(netbuffer,max_frame_size);
        delete[] buffer;
    }
    else
    {
        int fake_nread = -1;
        memcpy(&(netbuffer[0]), &fake_nread, 4);
        memcpy(&(netbuffer[4]), &fake_nread, 4);
        memcpy(&(netbuffer[8]), &fake_nread, 4);
        for(int k=12,i=0;k<max_frame_size;k++,i++)
        {
            netbuffer[k] = _rand_frame_data[i];
        }

        emit netData(netbuffer,max_frame_size);
        delete[] buffer;
    }
}

void RadioOp::sendEndBeep()
{
    QFile resfile(":/res/end_beep.raw");
    if(resfile.open(QIODevice::ReadOnly))
    {
        QByteArray *data = new QByteArray(resfile.readAll());
        short *samples = (short*) data->data();
        std::vector<float> *pcm = new std::vector<float>;

        for(unsigned int i=0;i<data->size()/sizeof(short);i++)
        {
            pcm->push_back((float)samples[i] / 32767.0f);
        }

        emit pcmData(pcm);
        delete data;
    }
}

void RadioOp::sendChannels()
{
    QByteArray data = _radio_protocol->buildRepeaterInfo();
    sendBinData(data,gr_modem::FrameTypeRepeaterInfo);
}

void RadioOp::startTx()
{
    if(_tx_inited)
    {
        if(_tx_mode == gr_modem_types::ModemTypeQPSK250000)
        {
            _data_modem_reset_timer->start();
            _data_modem_sleep_timer->start();
        }
        if(_rx_inited && !_repeat && (_rx_mode != gr_modem_types::ModemTypeQPSK250000))
            _modem->stopRX();
        if(_tx_modem_started)
            _modem->stopTX();
        _modem->startTX();
        struct timespec time_to_sleep = {0, 10000000L };
        nanosleep(&time_to_sleep, NULL);
        _modem->tuneTx(_tx_frequency + _tune_shift_freq);
        _tx_modem_started = false;
        _tx_started = true;
        if((_tx_radio_type == radio_type::RADIO_TYPE_DIGITAL))
            _modem->startTransmission(_callsign);
    }
}

void RadioOp::stopTx()
{
    if(_tx_inited)
    {
        if(_tx_radio_type == radio_type::RADIO_TYPE_DIGITAL)
            _modem->endTransmission(_callsign);
        if((_tx_radio_type == radio_type::RADIO_TYPE_ANALOG)
                && ((_tx_mode == gr_modem_types::ModemTypeNBFM2500) || (_tx_mode == gr_modem_types::ModemTypeNBFM5000)))
        {
            sendEndBeep();
        }
        struct timespec time_to_sleep = {1, 0L };
        nanosleep(&time_to_sleep, NULL);
        _modem->stopTX();
        _modem->tuneTx(430000000);
        _tx_modem_started = false;
        _tx_started = false;
        if(_rx_inited && !_repeat && (_rx_mode != gr_modem_types::ModemTypeQPSK250000))
            _modem->startRX();
    }
}

void RadioOp::updateFrequency()
{

}

void RadioOp::sendTextData(QString text, int frame_type)
{
    if(_tx_inited)
    {
        if(!_tx_modem_started)
        {
            stopTx();
            startTx();
        }
        _tx_modem_started = true;
        _modem->startTransmission(_callsign);
        _modem->textData(text, frame_type);
        _modem->endTransmission(_callsign);
    }
    if(!_repeat_text)
    {
        _mutex->lock();
        _process_text = false;
        _mutex->unlock();
    }
}

void RadioOp::sendBinData(QByteArray data, int frame_type)
{
    if(_tx_inited)
    {
        if(!_tx_modem_started)
        {
            stopTx();
            startTx();
        }
        _tx_modem_started = true;
        _modem->binData(data, frame_type);
        _modem->endTransmission(_callsign);
    }
    if(!_repeat_text)
    {
        _mutex->lock();
        _process_text = false;
        _mutex->unlock();
    }
}

void RadioOp::flushVoipBuffer()
{
    if(_voip_encode_buffer->size() > 320)
    {
        int frames = floor(_voip_encode_buffer->size() / 320);
        for(int j = 0;j<frames;j++)
        {
            short *pcm = new short[320];
            for(int i =0; i< 320;i++)
            {
                pcm[i] = _voip_encode_buffer->at(i);
            }
            _voip_encode_buffer->remove(0,320);
            emit voipData(pcm,320*sizeof(short));
        }
    }
}

void RadioOp::updateDataModemReset(bool transmitting, bool ptt_activated)
{
    if((_tx_mode == gr_modem_types::ModemTypeQPSK250000) && !_data_modem_sleeping
            && transmitting && ptt_activated)
    {
        qint64 sec_modem_running;
        sec_modem_running = (quint64)_data_modem_reset_timer->nsecsElapsed()/1000000000;
        if(sec_modem_running > 300)
        {
            std::cout << "resetting modem" << std::endl;
            _data_modem_sleeping = true;
            _data_modem_sleep_timer->restart();
        }
    }
    if((_tx_mode == gr_modem_types::ModemTypeQPSK250000) && _data_modem_sleeping)
    {
        qint64 sec_modem_sleeping;
        sec_modem_sleeping = (quint64)_data_modem_sleep_timer->nsecsElapsed()/1000000000;
        if(sec_modem_sleeping > 2)
        {
            _data_modem_sleep_timer->restart();
            _data_modem_sleeping = false;
            _data_modem_reset_timer->restart();
            _modem->startTransmission(_callsign);
            std::cout << "modem reset complete" << std::endl;
        }
    }
}

void RadioOp::run()
{

    bool ptt_activated = false;
    bool data_to_process = false;
    bool frame_flag = true;
    int last_ping_time = 0;
    int last_channel_broadcast_time = 0;
    while(true)
    {
        bool transmitting = _transmitting_audio;
        QCoreApplication::processEvents();
        flushVoipBuffer();

        int time = QDateTime::currentDateTime().toTime_t();
        if((time - last_ping_time) > 10)
        {
            emit pingServer();
            last_ping_time = time;
        }

        if((time - last_channel_broadcast_time) > 10)
        {
            last_channel_broadcast_time = time;
            if(_voip_forwarding && !transmitting && !ptt_activated)
            {
                sendChannels();
            }
        }

        updateDataModemReset(transmitting, ptt_activated);

        if(transmitting && !ptt_activated)
        {
            ptt_activated = true;
            startTx();
        }
        if(!transmitting && ptt_activated && !_data_modem_sleeping)
        {
            ptt_activated = false;
            stopTx();
        }
        processAudioStream();
        if(transmitting)
        {
            if(_tx_mode == gr_modem_types::ModemTypeQPSKVideo)
                processVideoStream(frame_flag);
            else if((_tx_mode == gr_modem_types::ModemTypeQPSK250000) && (_net_device != 0))
            {
                if(_rx_inited)
                {
                    _modem->demodulate();
                }
                if(!_data_modem_sleeping)
                    processNetStream();
            }
        }
        else
        {
            bool rx_inited = _rx_inited;
            if(rx_inited)
            {
                if(_rx_radio_type == radio_type::RADIO_TYPE_DIGITAL)
                    data_to_process = _modem->demodulate();
                else if(_rx_radio_type == radio_type::RADIO_TYPE_ANALOG)
                {
                    data_to_process = _modem->demodulateAnalog();
                }
                getFFTData();
                getConstellationData();
                if(!_tuning_done)
                    scan(data_to_process);
            }
        }
        if(_process_text && (_tx_radio_type == radio_type::RADIO_TYPE_DIGITAL))
        {
            sendTextData(_text_out, gr_modem::FrameTypeText);
            emit displayTransmitStatus(false);
        }
        if(!transmitting && !_vox_enabled && !_process_text)
        {
            if(data_to_process)
            {
                struct timespec time_to_sleep = {0, 10000000L };
                nanosleep(&time_to_sleep, NULL);
            }
            else
            {
                struct timespec time_to_sleep = {0, 30000000L };
                nanosleep(&time_to_sleep, NULL);
            }
        }
        else
        {
            struct timespec time_to_sleep = {0, 10000L };
            nanosleep(&time_to_sleep, NULL);
        }
        if(_stop)
            break;
    }

    emit finished();
}

void RadioOp::getFFTData()
{
    if(!_fft_enabled)
    {
        _fft_read_timer->restart();
        return;
    }
    qint64 msec = (quint64)_fft_read_timer->nsecsElapsed() / 1000000;
    if(msec < _fft_poll_time)
    {
        return;
    }

    unsigned int fft_size = 0;
    _mutex->lock();
    float rssi = _modem->getRSSI();
    if(rssi != 0)
        emit newRSSIValue(rssi);
    _modem->get_fft_data(_fft_data, fft_size);
    _mutex->unlock();
    if(fft_size > 0)
    {
        emit newFFTData(_fft_data, (int)fft_size);
        _fft_read_timer->restart();
    }
}

void RadioOp::setFFTPollTime(int fps)
{
    _fft_poll_time = (int)(1000 / fps);
}

void RadioOp::getConstellationData()
{
    if(!_constellation_enabled)
    {
        return;
    }
    qint64 msec = (quint64)_const_read_timer->nsecsElapsed() / 1000000;
    if(msec < _fft_poll_time)
    {
        return;
    }
    _mutex->lock();
    std::vector<std::complex<float>> *const_data = _modem->getConstellation();
    _mutex->unlock();
    if(const_data->size() > 1)
    {
        emit newConstellationData(const_data);
        _const_read_timer->restart();
    }

}


void RadioOp::receiveAudioData(unsigned char *data, int size)
{
    short *audio_out;
    int samples;
    int audio_mode = AudioInterface::AUDIO_MODE_OPUS;
    if((_rx_mode == gr_modem_types::ModemTypeBPSK2000) ||
            (_rx_mode == gr_modem_types::ModemType2FSK2000) ||
            (_rx_mode == gr_modem_types::ModemType4FSK2000) ||
            (_rx_mode == gr_modem_types::ModemTypeQPSK2000))
    {
        audio_out = _codec->decode_codec2_1400(data, size, samples);
    }
    else if((_rx_mode == gr_modem_types::ModemTypeBPSK1000))
        audio_out = _codec->decode_codec2_700(data, size, samples);
    else
        audio_out = _codec->decode_opus(data, size, samples);
    delete[] data;
    if(samples > 0)
    {
        float amplif = 1.0;
        if((_rx_mode == gr_modem_types::ModemTypeBPSK2000) ||
                (_rx_mode == gr_modem_types::ModemType2FSK2000) ||
                (_rx_mode == gr_modem_types::ModemType4FSK2000) ||
                (_rx_mode == gr_modem_types::ModemTypeQPSK2000) ||
                (_rx_mode == gr_modem_types::ModemTypeBPSK1000))
        {
            amplif = 1.0;
            audio_mode = AudioInterface::AUDIO_MODE_CODEC2;
        }
        for(int i=0;i<samples;i++)
        {
            audio_out[i] = (short)((float)audio_out[i] * amplif * 1e-1*exp(_rx_volume*log(10)));
        }
        if(_voip_forwarding)
        {
            emit voipData(audio_out,samples*sizeof(short));
        }
        else
        {
            _audio->write_short(audio_out,samples*sizeof(short),true, audio_mode);
        }
    }
}

void RadioOp::receivePCMAudio(std::vector<float> *audio_data)
{
    int size = audio_data->size();

    short *pcm = new short[size];
    for(int i=0;i<size;i++)
    {
        pcm[i] = (short)(audio_data->at(i) * 1e-1*exp(_rx_volume*log(10)) * 32767.0f);
    }
    if(_voip_forwarding)
    {
        for(int i=0;i<size;i++)
        {
            _voip_encode_buffer->push_back(pcm[i]);
        }
        delete[] pcm;
    }
    else
    {
        _audio->write_short(pcm, size*sizeof(short),false, AudioInterface::AUDIO_MODE_ANALOG);
    }
    audio_data->clear();
    delete audio_data;
    audioFrameReceived();
}

int RadioOp::getFrameLength(unsigned char *data)
{
    int frame_size1;
    int frame_size2;
    int frame_size3;

    memcpy(&frame_size1, &data[0], 4);
    memcpy(&frame_size2, &data[4], 4);
    memcpy(&frame_size3, &data[8], 4);
    if(frame_size1 == frame_size2)
        return (int)frame_size1;
    else if(frame_size1 == frame_size3)
        return (int)frame_size1;
    else if(frame_size2 == frame_size3)
        return (int)frame_size2;
    else
        return 0;
}

unsigned int RadioOp::getFrameCRC32(unsigned char *data)
{
    unsigned int crc1;
    unsigned int crc2;
    unsigned int crc3;

    memcpy(&crc1, &data[12], 4);
    memcpy(&crc2, &data[16], 4);
    memcpy(&crc3, &data[20], 4);
    if(crc1 == crc2)
        return (unsigned int)crc1;
    else if(crc1 == crc3)
        return (unsigned int)crc1;
    else if(crc2 == crc3)
        return (unsigned int)crc2;
    else
        return 0;
}

void RadioOp::receiveVideoData(unsigned char *data, int size)
{
    int frame_size = getFrameLength(data);
    unsigned int crc = getFrameCRC32(data);
    if(frame_size == 0)
    {
        std::cerr << "received wrong frame size, dropping frame " << std::endl;
        delete[] data;
        return;
    }
    if(frame_size > 3122 - 24)
    {
        std::cerr << "frame size too large, dropping frame " << std::endl;
        delete[] data;
        return;
    }
    unsigned char *jpeg_frame = new unsigned char[frame_size];
    memcpy(jpeg_frame, &data[24], frame_size);
    delete[] data;
    unsigned int crc_check = gr::digital::crc32(jpeg_frame, frame_size);
    if(crc != crc_check)
    {
        // JPEG decoder has this nasty habit of segfaulting on image errors
        std::cerr << "CRC check failed, dropping frame to avoid JPEG segfault " << std::endl;
        delete[] jpeg_frame;
        return;
    }

    unsigned char *raw_output = _video->decode_jpeg(jpeg_frame,frame_size);

    delete[] jpeg_frame;
    if(!raw_output)
    {

        return;
    }
    QImage img (raw_output, 320,240, QImage::Format_RGB888);
    img.convertToFormat(QImage::Format_RGB32);
    QImage out_img(img.scaled(160, 120));

    emit videoImage(out_img);
    delete[] raw_output;

}

void RadioOp::receiveNetData(unsigned char *data, int size)
{
    int frame_size = getFrameLength(data);
    if(frame_size < 0)
    {
        dataFrameReceived();
        delete[] data;
        return;
    }
    if((frame_size == 0) || (frame_size > 1500))
    {
        std::cerr << "received wrong frame size, dropping frame " << std::endl;
        delete[] data;
        return;
    }
    dataFrameReceived();
    unsigned char *net_frame = new unsigned char[frame_size];
    memcpy(net_frame, &data[16], frame_size);
    unsigned int crc;
    memcpy(&crc, &data[12], 4);
    delete[] data;
    unsigned int crc_check = gr::digital::crc32(net_frame, frame_size);

    if(crc != crc_check)
    {
        std::cerr << "CRC check failed, dropping frame " << std::endl;
        delete[] net_frame;
        return;
    }

    int res = _net_device->write_buffered(net_frame,frame_size);
}

void RadioOp::processVoipAudioFrame(short *pcm, int samples, quint64 sid)
{

    if(_m_queue->empty())
    {
        for(int i=0;i<samples;i++)
        {
            _m_queue->push_back(pcm[i]);
        }
    }
    else
    {
        unsigned int size = (_m_queue->size() > samples) ? samples : _m_queue->size();
        for(unsigned int i=0;i<size;i++)
        {
            _m_queue->at(i) = _m_queue->at(i)/2-1 + pcm[i]/2-1;
        }
        if(_m_queue->size() < samples)
        {
            for(int i=_m_queue->size();i<samples;i++)
            {
                _m_queue->push_back(pcm[i]);
            }
        }
    }
    delete[] pcm;
    quint64 milisec = (quint64)_last_voiced_frame_timer.nsecsElapsed()/1000000;
    if((milisec >= 20) || (sid == _last_session_id))
    {

        short *pcm = new short[_m_queue->size()];
        for(unsigned int i = 0;i<_m_queue->size();i++)
        {
            pcm[i] = (short)((float)_m_queue->at(i) * 1e-1*exp(_rx_volume*log(10)));
        }
        if(_voip_forwarding && _tx_inited)
        {
            if(!_voip_tx_timer->isActive())
            {
                startTx();
            }
            _voip_tx_timer->start(200);
            txAudio(pcm, samples*sizeof(short));
        }
        else
        {
            _audio->write_short(pcm, samples*sizeof(short));
            audioFrameReceived();
        }
        _last_voiced_frame_timer.restart();
        _m_queue->clear();
        _last_session_id = sid;
    }
}

void RadioOp::startTransmission()
{
    if(_tx_inited || _voip_enabled)
        _transmitting_audio = true;
}

void RadioOp::endTransmission()
{
    _transmitting_audio = false;
}

void RadioOp::textData(QString text, bool repeat)
{
    _repeat_text = repeat;
    _mutex->lock();
    _text_out = text;
    _process_text = true;
    _mutex->unlock();

}

void RadioOp::textReceived(QString text)
{
    emit printText(text, false);
}

void RadioOp::repeaterInfoReceived(QByteArray data)
{
    _radio_protocol->dataIn(data);
}

void RadioOp::callsignReceived(QString callsign)
{
    QString time= QDateTime::currentDateTime().toString("dd/MMM/yyyy hh:mm:ss");
    QString text = "<b>" + time + "</b> " + "<font color=\"#FF5555\">" + callsign + " </font><br/>\n";
    emit printText(text,true);
    emit printCallsign(callsign);
}

void RadioOp::audioFrameReceived()
{
    emit displayReceiveStatus(true);
    _voice_led_timer->start(500);
}

void RadioOp::dataFrameReceived()
{
    emit displayDataReceiveStatus(true);
    _data_led_timer->start(500);
    if((_rx_mode != gr_modem_types::ModemTypeQPSK250000)
            && (_rx_mode != gr_modem_types::ModemTypeQPSKVideo))
    {
        //short *sound = (short*) _data_rec_sound->data();
        //short *samples = new short[_data_rec_sound->size()];
        //memcpy(samples, sound, _data_rec_sound->size());
        //_audio->write_short(samples,_data_rec_sound->size());
    }

}

void RadioOp::receiveEnd()
{
    emit displayReceiveStatus(false);
    emit displayDataReceiveStatus(false);
}

void RadioOp::endAudioTransmission()
{
    QString time= QDateTime::currentDateTime().toString("d/MMM/yyyy hh:mm:ss");
    emit printText("<b>" + time + "</b> <font color=\"#77FF77\">Transmission end</font><br/>\n",true);
    QFile resfile(":/res/end_beep.raw");
    if(resfile.open(QIODevice::ReadOnly))
    {
        QByteArray *data = new QByteArray(resfile.readAll());
        short *samples = (short*) data->data();
        _audio->write_short(samples,data->size());

    }
}

void RadioOp::addChannel(Channel *chan)
{
    _radio_protocol->addChannel(chan);
}

void RadioOp::setStations(StationList list)
{
    _radio_protocol->setStations(list);
}

void RadioOp::toggleRX(bool value)
{

    if(value)
    {
        std::string rx_device_args;
        std::string tx_device_args;
        std::string rx_antenna;
        std::string tx_antenna;
        int rx_freq_corr;
        int tx_freq_corr;
        std::string callsign;
        std::string video_device;
        readConfig(rx_device_args, tx_device_args,
                                 rx_antenna, tx_antenna, rx_freq_corr,
                                 tx_freq_corr, callsign, video_device);
        try
        {
            _mutex->lock();
            _modem->initRX(_rx_mode, rx_device_args, rx_antenna, rx_freq_corr);
            _mutex->unlock();
        }
        catch(std::runtime_error e)
        {
            _mutex->unlock();
            emit initError("Could not init RX device, check settings");
            return;
        }
        _mutex->lock();
        _modem->setRxSensitivity(_rx_sensitivity);
        _modem->setSquelch(_squelch);
        _modem->setRxCTCSS(_rx_ctcss);
        _modem->set_carrier_offset(_carrier_offset);
        _modem->set_samp_rate(_rx_sample_rate);
        _modem->tune(_rx_frequency);
        _modem->startRX();
        _modem->enableGUIConst(true);

        _mutex->unlock();
        if(_rx_mode == gr_modem_types::ModemTypeQPSK250000 && _net_device == 0)
        {
            _net_device = new NetDevice(0, _settings->ip_address);
        }
        _rx_inited = true;
    }
    else if (_rx_inited)
    {
        if(_rx_mode == gr_modem_types::ModemTypeQPSK250000 && _net_device != 0 && !_tx_inited)
        {
            delete _net_device;
            _net_device = 0;
        }
        _mutex->lock();
        _modem->stopRX();
        _modem->deinitRX(_rx_mode);
        _mutex->unlock();
        _rx_inited = false;
    }
}

void RadioOp::toggleTX(bool value)
{
    if(value)
    {
        std::string rx_device_args;
        std::string tx_device_args;
        std::string rx_antenna;
        std::string tx_antenna;
        int rx_freq_corr;
        int tx_freq_corr;
        std::string callsign;
        std::string video_device;
        readConfig(rx_device_args, tx_device_args,
                                 rx_antenna, tx_antenna, rx_freq_corr,
                                 tx_freq_corr, callsign, video_device);
        try
        {
            _mutex->lock();
            _modem->initTX(_tx_mode, tx_device_args, tx_antenna, tx_freq_corr);
            _mutex->unlock();
        }
        catch(std::runtime_error e)
        {
            _mutex->unlock();
            emit initError("Could not init TX device, check settings");
            return;
        }
        _mutex->lock();

        _modem->setTxPower(_tx_power);
        _modem->setBbGain(_bb_gain);
        _modem->tuneTx(430000000);
        _modem->setTxCTCSS(_tx_ctcss);

        _mutex->unlock();
        if(_tx_mode == gr_modem_types::ModemTypeQPSKVideo)
            _video = new VideoEncoder(QString::fromStdString(video_device));
        if(_tx_mode == gr_modem_types::ModemTypeQPSK250000 && _net_device == 0)
        {
            _net_device = new NetDevice(0, _settings->ip_address);
        }
        _tx_inited = true;
    }
    else if(_tx_inited)
    {
        _mutex->lock();
        _modem->deinitTX(_tx_mode);
        _mutex->unlock();
        if(_tx_mode == gr_modem_types::ModemTypeQPSKVideo)
        {
            delete _video;
            _video = 0;
        }
        if(_tx_mode == gr_modem_types::ModemTypeQPSK250000 && _net_device != 0 && !_rx_inited)
        {
            delete _net_device;
            _net_device = 0;
        }
        _tx_inited = false;
    }
}

void RadioOp::toggleRxMode(int value)
{

    bool rx_inited_before = _rx_inited;
    if(rx_inited_before)
    {
        _rx_inited = false;
    }
    _rx_radio_type = radio_type::RADIO_TYPE_DIGITAL;
    switch(value)
    {
    case 0:
        _rx_mode = gr_modem_types::ModemTypeBPSK2000;
        _step_hz = 1000;
        break;
    case 1:
        _rx_mode = gr_modem_types::ModemTypeBPSK1000;
        _step_hz = 1000;
        break;
    case 2:
        _rx_mode = gr_modem_types::ModemTypeQPSK2000;
        _step_hz = 1000;
        break;
    case 3:
        _rx_mode = gr_modem_types::ModemTypeQPSK20000;
        _step_hz = 1000;
        break;
    case 4:
        _rx_mode = gr_modem_types::ModemType4FSK2000;
        _step_hz = 1000;
        break;
    case 5:
        _rx_mode = gr_modem_types::ModemType4FSK20000;
        _step_hz = 5000;
        break;
    case 6:
        _rx_mode = gr_modem_types::ModemType2FSK2000;
        _step_hz = 1000;
        break;
    case 7:
        _rx_mode = gr_modem_types::ModemType2FSK20000;
        _step_hz = 5000;
        break;
    case 8:
        _rx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _rx_mode = gr_modem_types::ModemTypeNBFM2500;
        _step_hz = 6250;
        break;
    case 9:
        _rx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _rx_mode = gr_modem_types::ModemTypeNBFM5000;
        _step_hz = 12500;
        break;
    case 10:
        _rx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _rx_mode = gr_modem_types::ModemTypeWBFM;
        _step_hz = 10000;
        break;
    case 11:
        _rx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _rx_mode = gr_modem_types::ModemTypeUSB2500;
        _step_hz = 100;
        break;
    case 12:
        _rx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _rx_mode = gr_modem_types::ModemTypeLSB2500;
        _step_hz = 100;
        break;
    case 13:
        _rx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _rx_mode = gr_modem_types::ModemTypeAM5000;
        _step_hz = 1000;
        break;
    case 14:
        _rx_mode = gr_modem_types::ModemTypeQPSKVideo;
        _step_hz = 1000;
        break;
    case 15:
        _rx_mode = gr_modem_types::ModemTypeQPSK250000;
        _step_hz = 1000;
        break;
    default:
        _rx_mode = gr_modem_types::ModemTypeBPSK2000;
        _step_hz = 1000;
        break;
    }

    _mutex->lock();
    _modem->toggleRxMode(_rx_mode);
    if(rx_inited_before)
    {
        _rx_inited = true;
    }
    _mutex->unlock();
}

void RadioOp::toggleTxMode(int value)
{

    _tx_radio_type = radio_type::RADIO_TYPE_DIGITAL;
    switch(value)
    {
    case 0:
        _tx_mode = gr_modem_types::ModemTypeBPSK2000;
        break;
    case 1:
        _tx_mode = gr_modem_types::ModemTypeBPSK1000;
        break;
    case 2:
        _tx_mode = gr_modem_types::ModemTypeQPSK2000;
        break;
    case 3:
        _tx_mode = gr_modem_types::ModemTypeQPSK20000;
        break;
    case 4:
        _tx_mode = gr_modem_types::ModemType4FSK2000;
        break;
    case 5:
        _tx_mode = gr_modem_types::ModemType4FSK20000;
        break;
    case 6:
        _tx_mode = gr_modem_types::ModemType2FSK2000;
        break;
    case 7:
        _tx_mode = gr_modem_types::ModemType2FSK20000;
        break;
    case 8:
        _tx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _tx_mode = gr_modem_types::ModemTypeNBFM2500;
        break;
    case 9:
        _tx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _tx_mode = gr_modem_types::ModemTypeNBFM5000;
        break;
    case 10:
        _tx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _tx_mode = gr_modem_types::ModemTypeWBFM;
        break;
    case 11:
        _tx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _tx_mode = gr_modem_types::ModemTypeUSB2500;
        break;
    case 12:
        _tx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _tx_mode = gr_modem_types::ModemTypeLSB2500;
        break;
    case 13:
        _tx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _tx_mode = gr_modem_types::ModemTypeAM5000;
        break;
    case 14:
        _tx_mode = gr_modem_types::ModemTypeQPSKVideo;
        break;
    case 15:
        _tx_mode = gr_modem_types::ModemTypeQPSK250000;
        break;
    default:
        _tx_mode = gr_modem_types::ModemTypeBPSK2000;
        break;
    }

    _mutex->lock();
    _modem->toggleTxMode(_tx_mode);
    _mutex->unlock();
}

void RadioOp::usePTTForVOIP(bool value)
{
    _voip_enabled = value;
}

void RadioOp::setVOIPForwarding(bool value)
{
    _voip_forwarding = value;
}

void RadioOp::setVox(bool value)
{
    _vox_enabled = value;
    if(_transmitting_audio && !_vox_enabled)
        _transmitting_audio = false;
}

void RadioOp::toggleRepeat(bool value)
{
    if((_rx_mode != _tx_mode) && value) // no mixed mode repeat
        return;
    if(value && !_repeat)
    {
        _repeat = value;
        startTx();
    }
    else if(!value && _repeat)
    {
        stopTx();
        _repeat = value;
    }
    _mutex->lock();
    _modem->setRepeater(value);
    _mutex->unlock();

}

void RadioOp::fineTuneFreq(long center_freq)
{
    _mutex->lock();
    _modem->set_carrier_offset(_carrier_offset + center_freq*_step_hz);
    _mutex->unlock();
}

void RadioOp::tuneFreq(qint64 center_freq)
{
    _rx_frequency = center_freq;
    _tx_frequency = center_freq + _carrier_offset;
    _mutex->lock();
    _modem->tune(_rx_frequency);
    _modem->tuneTx(_tx_frequency + _tune_shift_freq);
    _mutex->unlock();
}

void RadioOp::setCarrierOffset(qint64 offset)
{
    _carrier_offset = offset;
    _mutex->lock();
    _modem->set_carrier_offset(offset);
    _mutex->unlock();
}

void RadioOp::setRxSampleRate(int samp_rate)
{
    _rx_sample_rate = samp_rate;
    _mutex->lock();
    _modem->set_samp_rate(samp_rate);
    _mutex->unlock();
}

void RadioOp::setFFTSize(int size)
{
    _mutex->lock();
    _modem->setFFTSize(size);
    _mutex->unlock();
}

void RadioOp::tuneTxFreq(qint64 center_freq)
{
    _tune_shift_freq = center_freq;
    _mutex->lock();
    _modem->tuneTx(_tx_frequency + _tune_shift_freq);
    _mutex->unlock();
}

void RadioOp::setTxPower(int dbm)
{
    _tx_power = (float)dbm/100.0;
    _mutex->lock();
    _modem->setTxPower(_tx_power);
    _mutex->unlock();
}

void RadioOp::setBbGain(int value)
{
    _bb_gain = value;
    _mutex->lock();
    _modem->setBbGain(_bb_gain);
    _mutex->unlock();
}

void RadioOp::setRxSensitivity(int value)
{
    _rx_sensitivity = (float)value/100.0;
    _mutex->lock();
    _modem->setRxSensitivity(_rx_sensitivity);
    _mutex->unlock();
}

void RadioOp::setSquelch(int value)
{
    _squelch = value;
    _mutex->lock();
    _modem->setSquelch(value);
    _mutex->unlock();
}

void RadioOp::setVolume(int value)
{
    _rx_volume = (float)value/50.0;
}

void RadioOp::setRxCTCSS(float value)
{
    _rx_ctcss = value;
    _mutex->lock();
    _modem->setRxCTCSS(value);
    _mutex->unlock();
}

void RadioOp::setTxCTCSS(float value)
{
    _tx_ctcss = value;
    _mutex->lock();
    _modem->setTxCTCSS(value);
    _mutex->unlock();
}

void RadioOp::enableGUIConst(bool value)
{
    _constellation_enabled = value;
}

void RadioOp::enableGUIFFT(bool value)
{
    _fft_enabled = value;
    _mutex->lock();
    _modem->enableGUIFFT(value);
    _mutex->unlock();
}

void RadioOp::scan(bool receiving, bool wait_for_timer)
{
    if(receiving && !_scan_stop)
    {
        _scan_stop = true;
    }
    if(_scan_stop)
    {
        qint64 msec = (quint64)_scan_timer->nsecsElapsed() / 1000000;
        if(msec < 5000)
        {
            return;
        }
        else
        {
            _scan_stop = false;
        }
    }
    if(wait_for_timer)
    {
        qint64 msec = (quint64)_scan_timer->nsecsElapsed() / 1000000;
        if(msec < _fft_poll_time)
        {
            return;
        }
    }
    _autotune_freq = _autotune_freq + _step_hz;
    if(_autotune_freq >= _tune_limit_upper)
        _autotune_freq = _tune_limit_lower;
    _mutex->lock();
    _modem->set_carrier_offset(_autotune_freq);
    _mutex->unlock();
    emit freqToGUI(_autotune_freq);
    _scan_timer->restart();
}

void RadioOp::startAutoTune(int step)
{
    if(!_rx_inited)
        return;
    if(step != 0)
        _step_hz = step;
    _tune_limit_lower = -_rx_sample_rate / 2;
    _tune_limit_upper = _rx_sample_rate / 2;
    _autotune_freq = _carrier_offset;
    _scan_timer->start();
    _tuning_done = false;
    scan(false, false);
}

void RadioOp::stopAutoTune()
{
    _tuning_done = true;
    _scan_stop = false;
    _carrier_offset = _autotune_freq;
    emit freqToGUI(_autotune_freq);
}
