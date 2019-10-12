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

#include "radiocontroller.h"


RadioController::RadioController(Settings *settings, QObject *parent) :
    QObject(parent)
{
    _settings = settings;
    _codec = new AudioEncoder;
    _radio_protocol = new RadioProtocol;
    _relay_controller = new RelayController;
    _video = new VideoEncoder;
    _net_device = new NetDevice(0, _settings->ip_address);
    _mutex = new QMutex;
    _voip_to_radio_queue = new QVector<short>;

    _voip_tx_timer = new QTimer(this);
    _voip_tx_timer->setSingleShot(true);
    _vox_timer = new QTimer(this);
    _vox_timer->setSingleShot(true);
    _end_tx_timer = new QTimer(this);
    _end_tx_timer->setSingleShot(true);
    _data_read_timer = new QElapsedTimer();
    _data_modem_reset_timer = new QElapsedTimer();
    _data_modem_sleep_timer = new QElapsedTimer();
    _scan_timer = new QElapsedTimer();
    _fft_read_timer = new QElapsedTimer();
    _fft_read_timer->start();
    _fft_poll_time = 75;
    _fft_enabled = true;
    _const_read_timer = new QElapsedTimer();
    _const_read_timer->start();
    _rssi_read_timer = new QElapsedTimer();
    _rssi_read_timer->start();

    _last_session_id = 0;

    _stop =false;
    _tx_inited = false;
    _rx_inited = false;
    _tx_started = false;
    _voip_enabled = false;
    _vox_enabled = false;
    _audio_compressor_enabled = false;
    _voip_forwarding = false;
    _last_voiced_frame_timer.start();

    _scan_stop = false;
    _scan_done = true;
    _memory_scan_done = true;

    _constellation_enabled = false;
    _data_modem_sleeping = false;
    _transmitting = false;
    _process_text = false;
    _repeat_text = false;
    _repeat = false;
    _rx_volume = 1e-3*exp(((float)_settings->rx_volume/100.0)*6.908);
    _tx_volume = 1e-3*exp(((float)_settings->tx_volume/50.0)*6.908);;
    _tx_frequency = _settings->rx_frequency + _settings->demod_offset;
    _autotune_freq = 0;
    _tune_limit_lower = -500000;
    _tune_limit_upper = 500000;
    _step_hz = 10;

    _rx_mode = gr_modem_types::ModemTypeBPSK2000;
    _tx_mode = gr_modem_types::ModemTypeBPSK2000;
    _rx_radio_type = radio_type::RADIO_TYPE_DIGITAL;
    _tx_radio_type = radio_type::RADIO_TYPE_DIGITAL;

    _tune_counter = 0;
    _freq_gui_counter = 0;
    _tx_modem_started = false;
    _voice_led_timer = new QTimer(this);
    _voice_led_timer->setSingleShot(true);
    _data_led_timer = new QTimer(this);
    _data_led_timer->setSingleShot(true);
    _rand_frame_data = new unsigned char[5000];
    _voip_encode_buffer = new QVector<short>;
    _fft_data = new float[1024*1024];

    setCallsign();

    QObject::connect(_voice_led_timer, SIGNAL(timeout()), this, SLOT(receiveEnd()));
    QObject::connect(_data_led_timer, SIGNAL(timeout()), this, SLOT(receiveEnd()));
    QObject::connect(_data_led_timer, SIGNAL(timeout()), this, SLOT(receiveEnd()));
    QObject::connect(_voip_tx_timer, SIGNAL(timeout()), this, SLOT(stopTx()));
    QObject::connect(_end_tx_timer, SIGNAL(timeout()), this, SLOT(endTx()));

    // FIXME: there is no reason for the modem to use the settings
    // All control happens in the radioop or main thread
    _modem = new gr_modem(_settings);

    QObject::connect(_modem,SIGNAL(textReceived(QString)),this,SLOT(textReceived(QString)));
    QObject::connect(_modem,SIGNAL(repeaterInfoReceived(QByteArray)),this,SLOT(repeaterInfoReceived(QByteArray)));
    QObject::connect(_modem,SIGNAL(callsignReceived(QString)),this,SLOT(callsignReceived(QString)));
    QObject::connect(_modem,SIGNAL(audioFrameReceived()),this,SLOT(audioFrameReceived()));
    QObject::connect(_modem,SIGNAL(dataFrameReceived()),this,SLOT(dataFrameReceived()));
    QObject::connect(_modem,SIGNAL(receiveEnd()),this,SLOT(receiveEnd()));
    QObject::connect(_modem,SIGNAL(endAudioTransmission()),this,SLOT(endAudioTransmission()));
    QObject::connect(this,SIGNAL(audioData(unsigned char*,int)),_modem,SLOT(transmitDigitalAudio(unsigned char*,int)));
    QObject::connect(this,SIGNAL(pcmData(std::vector<float>*)),_modem,SLOT(transmitPCMAudio(std::vector<float>*)));
    QObject::connect(this,SIGNAL(videoData(unsigned char*,int)),_modem,SLOT(transmitVideoData(unsigned char*,int)));
    QObject::connect(this,SIGNAL(netData(unsigned char*,int)),_modem,SLOT(transmitNetData(unsigned char*,int)));
    QObject::connect(_modem,SIGNAL(digitalAudio(unsigned char*,int)),this,SLOT(receiveDigitalAudio(unsigned char*,int)));
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
    QFile resfile_end_tx(":/res/end_beep1.raw");
    if(resfile_end_tx.open(QIODevice::ReadOnly))
    {
        _end_rec_sound = new QByteArray(resfile_end_tx.readAll());
    }

    toggleRxMode(_settings->rx_mode);
    toggleTxMode(_settings->tx_mode);

}

RadioController::~RadioController()
{
    if(_rx_inited)
        toggleRX(false);
    if(_tx_inited)
        toggleTX(false);
    delete _codec;
    delete _video;
    delete _net_device;
    delete _radio_protocol;
    delete _voice_led_timer;
    delete _data_led_timer;
    delete _voip_tx_timer;
    delete _vox_timer;
    delete _end_tx_timer;
    delete _modem;
    delete[] _rand_frame_data;
    _voip_encode_buffer->clear();
    delete _voip_encode_buffer;
    _voip_to_radio_queue->clear();
    delete _voip_to_radio_queue;
    delete _relay_controller;
    delete _end_rec_sound;
    delete _data_rec_sound;
}

void RadioController::stop()
{
    if(_rx_inited)
        toggleRX(false);
    if(_tx_inited)
        toggleTX(false);
    _stop=true;
}

void RadioController::run()
{

    bool ptt_activated = false;
    bool data_to_process = false;
    bool frame_flag = true;
    int last_ping_time = 0;
    int last_channel_broadcast_time = 0;
    while(!_stop)
    {
        _mutex->lock();
        bool transmitting = _transmitting;
        bool rx_inited = _rx_inited;
        bool process_text = _process_text;
        bool vox_enabled = _vox_enabled;
        bool voip_forwarding = _voip_forwarding;
        QString text_out = _text_out;
        int tx_mode = _tx_mode;
        bool data_modem_sleeping = _data_modem_sleeping;
        _mutex->unlock();

        QCoreApplication::processEvents();
        flushRadioToVoipBuffer();
        processVoipToRadioQueue();

        // FIXME: this is the wrong place to control the Mumble client
        int time = QDateTime::currentDateTime().toTime_t();
        if((time - last_ping_time) > 10)
        {
            emit pingServer();
            last_ping_time = time;
        }

        if((time - last_channel_broadcast_time) > 10)
        {
            last_channel_broadcast_time = time;
            if(voip_forwarding && !transmitting && !ptt_activated)
            {
                // FIXME: poke repeater to VOIP logic here
                //sendChannels();
            }
        }

        updateDataModemReset(transmitting, ptt_activated);

        if(transmitting && !ptt_activated)
        {
            ptt_activated = true;
            startTx();
        }
        if(!transmitting && ptt_activated && !data_modem_sleeping)
        {
            ptt_activated = false;
            stopTx();
        }

        // FIXME: large data transfer blocking demod
        getFFTData();
        getConstellationData();
        getRSSI();

        if(transmitting)
        {
            if(tx_mode == gr_modem_types::ModemTypeQPSKVideo)
                processInputVideoStream(frame_flag);
            else if(tx_mode == gr_modem_types::ModemTypeQPSK250000)
            {
                if(rx_inited)
                {
                    _mutex->lock();
                    _modem->demodulate();
                    _mutex->unlock();
                }
                if(!data_modem_sleeping)
                    processInputNetStream();
            }
        }

        data_to_process = getDemodulatorData();

        if(process_text && (_tx_radio_type == radio_type::RADIO_TYPE_DIGITAL))
        {
            sendTextData(text_out, gr_modem::FrameTypeText);
            // FIXME: how do I know when transmit is done?
            emit displayTransmitStatus(false);
        }

        // FIXME: remove these hardcoded sleeps
        if(!transmitting && !vox_enabled && !process_text)
        {
            if(!data_to_process)
            {
                struct timespec time_to_sleep = {0, 5000000L };
                nanosleep(&time_to_sleep, NULL);
            }
            else
            {
                struct timespec time_to_sleep = {0, 1000L };
                nanosleep(&time_to_sleep, NULL);
            }
        }
        else
        {
            struct timespec time_to_sleep = {0, 2000000L };
            nanosleep(&time_to_sleep, NULL);
        }
    }

    emit finished();
}


/// this code runs only in startTx and stopTx and setVox
void RadioController::updateInputAudioStream()
{
    if(!_transmitting && !_vox_enabled)
    {
        emit setAudioReadMode(false, false, AudioProcessor::AUDIO_MODE_ANALOG);
        return;
    }
    if(_tx_mode == gr_modem_types::ModemTypeQPSK250000)
    {
        emit setAudioReadMode(false, false, AudioProcessor::AUDIO_MODE_ANALOG);
        return;
    }
    if(_repeat)
    {
        emit setAudioReadMode(false, false, AudioProcessor::AUDIO_MODE_ANALOG);
        return;
    }
    int audio_mode;
    if((_tx_mode == gr_modem_types::ModemTypeBPSK2000) ||
            (_tx_mode == gr_modem_types::ModemType2FSK2000) ||
            (_tx_mode == gr_modem_types::ModemType4FSK2000) ||
            (_tx_mode == gr_modem_types::ModemTypeQPSK2000) ||
            (_tx_mode == gr_modem_types::ModemTypeBPSK1000))
    {
        audio_mode = AudioProcessor::AUDIO_MODE_CODEC2;
        emit setAudioReadMode(true, _audio_compressor_enabled, audio_mode);
    }
    else if((_tx_mode == gr_modem_types::ModemTypeQPSK20000) ||
            (_tx_mode == gr_modem_types::ModemType2FSK20000) ||
            (_tx_mode == gr_modem_types::ModemType4FSK20000))
    {
        audio_mode = AudioProcessor::AUDIO_MODE_OPUS;
        emit setAudioReadMode(true, _audio_compressor_enabled, audio_mode);
    }
    else
    {
        audio_mode = AudioProcessor::AUDIO_MODE_ANALOG;
        emit setAudioReadMode(true, _audio_compressor_enabled, audio_mode);
    }
}

void RadioController::flushRadioToVoipBuffer()
{

    /// Large size of frames (120 ms) helps Mumble client
    if(_voip_encode_buffer->size() >= 960)
    {

        short *pcm = new short[960];
        for(int i =0; i< 960;i++)
        {
            pcm[i] = _voip_encode_buffer->at(i);
        }

        emit voipDataPCM(pcm,960*sizeof(short));
        _voip_encode_buffer->remove(0,960);
    }
}

void RadioController::processVoipToRadioQueue()
{
    if(_voip_to_radio_queue->size() >= 320)
    {
        short *pcm = new short[320];
        for(unsigned int i = 0;i<320;i++)
        {
            pcm[i] = (short)((float)_voip_to_radio_queue->at(i) * _rx_volume);
        }
        if(_voip_forwarding)
        {
            if(!_voip_tx_timer->isActive())
            {
                // FIXME: these should be called from the thread loop only
                startTx();
            }
            _voip_tx_timer->start(500);
            txAudio(pcm, 320*sizeof(short), 1, true); // don't loop back to Mumble
        }
        else
        {
            emit writePCM(pcm, 320*sizeof(short), _audio_compressor_enabled,
                          AudioProcessor::AUDIO_MODE_OPUS);
            audioFrameReceived();
        }
        _last_voiced_frame_timer.restart();
        _voip_to_radio_queue->remove(0,320);
    }
}


void RadioController::txAudio(short *audiobuffer, int audiobuffer_size,
                              int vad, bool radio_only)
{
    /// first check the other places we need to send it
    if(_vox_enabled)
    {
        if(vad)
        {
            _vox_timer->start(500);
            if(!_tx_started && !_voip_enabled)
                _transmitting = true;
        }
        if(!vad && !_vox_timer->isActive())
        {
            if(_tx_started && !_voip_enabled)
                _transmitting = false;
            delete[] audiobuffer;
            return;
        }
    }

    if(_transmitting && _voip_enabled && !radio_only)
    {

        for(unsigned int i=0;i< (unsigned int)audiobuffer_size/sizeof(short);i++)
        {
            _voip_encode_buffer->push_back(audiobuffer[i] * _tx_volume);
        }
    }

    if(!_tx_inited || !_tx_started)
    {
        delete[] audiobuffer; // safety
        return;
    }


    /// Now it goes to the radio
    ///
    if(_tx_radio_type == radio_type::RADIO_TYPE_ANALOG)
    {
        std::vector<float> *pcm = new std::vector<float>;

        for(unsigned int i=0;i<audiobuffer_size/sizeof(short);i++)
        {
            pcm->push_back((float)audiobuffer[i] / 32767.0f * _tx_volume);
        }

        emit pcmData(pcm);
        delete[] audiobuffer;
        return;
    }

    int packet_size = 0;
    unsigned char *encoded_audio;
    /// digital volume adjust
    for(unsigned int i = 0;i< audiobuffer_size/sizeof(short);i++)
    {
        audiobuffer[i] = audiobuffer[i] * _tx_volume;
    }
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

int RadioController::processInputVideoStream(bool &frame_flag)
{
    Q_UNUSED(frame_flag);
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
        struct timespec time_to_sleep = {0, (100000000 - (long)microsec) - 5000000};
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

void RadioController::processInputNetStream()
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
    struct timespec time_to_sleep = {0, (long)time_left };

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

void RadioController::sendTextData(QString text, int frame_type)
{
    if(_tx_inited)
    {
        if(!_tx_modem_started)
        {
            // FIXME: these should be called from the thread loop only
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

void RadioController::sendBinData(QByteArray data, int frame_type)
{
    if(_tx_inited)
    {
        if(!_tx_modem_started)
        {
            // FIXME: these should be called from the thread loop only
            stopTx();
            startTx();
        }
        _tx_modem_started = true;
        _modem->binData(data, frame_type);
        _modem->endTransmission(_callsign);
    }
    if(!_repeat_text)
    {

        _process_text = false;

    }
}

void RadioController::sendEndBeep()
{
    short *samples = (short*) _end_rec_sound->data();
    std::vector<float> *pcm = new std::vector<float>;

    for(unsigned int i=0;i<_end_rec_sound->size()/sizeof(short);i++)
    {
        pcm->push_back((float)samples[i] / 32767.0f * 0.5);
    }

    emit pcmData(pcm);
}

void RadioController::sendChannels()
{
    QByteArray data = _radio_protocol->buildRepeaterInfo();
    sendBinData(data,gr_modem::FrameTypeRepeaterInfo);
}

void RadioController::setRelays(bool transmitting)
{
    if(!_relays_enabled)
        return;
    int res;
    struct timespec time_to_sleep;
    if(transmitting)
    {
        res = _relay_controller->enableRelay(0);
        if(!res)
        {
            std::cerr << "Relay control failed, stopping to avoid damage" << std::endl;
            exit(EXIT_FAILURE);
        }
        time_to_sleep = {0, 10000L };
        nanosleep(&time_to_sleep, NULL);
        res = _relay_controller->enableRelay(1);
        if(!res)
        {
            std::cerr << "Relay control failed, stopping to avoid damage" << std::endl;
            exit(EXIT_FAILURE);
        }
        time_to_sleep = {0, 10000L };
        nanosleep(&time_to_sleep, NULL);
    }
    else
    {
        res = _relay_controller->disableRelay(1);
        if(!res)
        {
            std::cerr << "Relay control failed, stopping to avoid damage" << std::endl;
            exit(EXIT_FAILURE);
        }
        time_to_sleep = {0, 10000L };
        nanosleep(&time_to_sleep, NULL);
        res = _relay_controller->disableRelay(0);
        if(!res)
        {
            std::cerr << "Relay control failed, stopping to avoid damage" << std::endl;
            exit(EXIT_FAILURE);
        }
        time_to_sleep = {0, 10000L };
        nanosleep(&time_to_sleep, NULL);
    }

}

void RadioController::startTx()
{
    updateInputAudioStream(); // moved here, LimeSDR specific thing (calibration at low power)
    if(_tx_inited)
    {
        if(_end_tx_timer->isActive())
        {
            _end_tx_timer->stop();
        }
        if(_tx_mode == gr_modem_types::ModemTypeQPSK250000)
        {
            _data_modem_reset_timer->start();
            _data_modem_sleep_timer->start();
        }

        if(!_duplex_enabled)
        {
            _modem->enableDemod(false);
            _modem->setRxSensitivity(0.01);
        }

        setRelays(true);
        _modem->tuneTx(_tx_frequency + _settings->tx_shift);
        _modem->setTxPower((float)_settings->tx_power/100);

        /**
        if(_rx_inited && !_repeat && (_rx_mode != gr_modem_types::ModemTypeQPSK250000))
            _modem->stopRX();


        if(_tx_modem_started)
            _modem->stopTX();
        */

        /// LimeSDR calibration procedure happens after every tune request
        /**
        struct timespec time_to_sleep = {0, 10000000L };
        nanosleep(&time_to_sleep, NULL);
        _modem->startTX();
        */

        _settings->_in_transmission = true;
        _tx_modem_started = false;
        _tx_started = true;
        if((_tx_radio_type == radio_type::RADIO_TYPE_DIGITAL))
        {
            _modem->startTransmission(_callsign);
        }
        if((_tx_radio_type == radio_type::RADIO_TYPE_ANALOG)
                && ((_tx_mode == gr_modem_types::ModemTypeNBFM2500) || (_tx_mode == gr_modem_types::ModemTypeNBFM5000)))
        {
            sendEndBeep();
        }
        emit displayTransmitStatus(true);
    }
}

void RadioController::stopTx()
{
    updateInputAudioStream();
    if(_tx_inited)
    {
        _tx_started = false;
        int tx_tail_msec = 100;
        if(_tx_radio_type == radio_type::RADIO_TYPE_DIGITAL)
        {
            _modem->endTransmission(_callsign);
            tx_tail_msec = 1500;
        }
        if((_tx_radio_type == radio_type::RADIO_TYPE_ANALOG)
                && ((_tx_mode == gr_modem_types::ModemTypeNBFM2500) || (_tx_mode == gr_modem_types::ModemTypeNBFM5000)))
        {
            sendEndBeep();
            tx_tail_msec = 1000;
        }
        // FIXME: end tail length should be calculated exactly
        _end_tx_timer->start(tx_tail_msec);
        _tx_modem_started = false;

        /**
        if(_rx_inited && !_repeat && (_rx_mode != gr_modem_types::ModemTypeQPSK250000))
           _modem->startRX();
        */
    }
}

void RadioController::endTx()
{

    _modem->setTxPower(0.01);
    _modem->flushSources();
    /// On the LimeSDR mini, whenever I call setTxPower I get a brief spike of the LO
    setRelays(false);

    if(!_duplex_enabled)
    {
        _modem->enableDemod(true);
        _modem->setRxSensitivity(((double)_settings->rx_sensitivity)/100.0);
    }
    emit displayTransmitStatus(false);
    _settings->_in_transmission = false;
}


void RadioController::updateDataModemReset(bool transmitting, bool ptt_activated)
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
            _mutex->lock();
            _modem->startTransmission(_callsign);
            _mutex->unlock();
            std::cout << "modem reset complete" << std::endl;
        }
    }
}

bool RadioController::getDemodulatorData()
{
    bool data_to_process = false;
    if(_rx_inited)
    {
        if(_rx_radio_type == radio_type::RADIO_TYPE_DIGITAL)
            data_to_process = _modem->demodulate();
        else if(_rx_radio_type == radio_type::RADIO_TYPE_ANALOG)
        {
            data_to_process = _modem->demodulateAnalog();
        }

        if(!_scan_done)
            scan(data_to_process);
        if(!_memory_scan_done)
            memoryScan(data_to_process);
    }
    return data_to_process;
}


void RadioController::getRSSI()
{
    if(!_rssi_enabled)
        return;

    qint64 msec = (quint64)_rssi_read_timer->nsecsElapsed() / 1000000;
    if(msec < _fft_poll_time)
    {
        return;
    }
    float rssi = _modem->getRSSI();
    if(rssi < 99.0f)
        emit newRSSIValue(rssi);
    _rssi_read_timer->restart();
}

void RadioController::getFFTData()
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
    _modem->getFFTData(_fft_data, fft_size);
    if(fft_size > 0)
    {
        emit newFFTData(_fft_data, (int)fft_size);
        _fft_read_timer->restart();
    }
}

void RadioController::setFFTPollTime(int fps)
{
    _fft_poll_time = (int)(1000 / fps);
}

void RadioController::getConstellationData()
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
    std::vector<std::complex<float>> *const_data = _modem->getConstellation();
    if(const_data->size() > 1)
    {
        emit newConstellationData(const_data);
        _const_read_timer->restart();
    }
}


void RadioController::receiveDigitalAudio(unsigned char *data, int size)
{
    short *audio_out;
    int samples;
    int audio_mode = AudioProcessor::AUDIO_MODE_OPUS;
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
    {
        audio_out = _codec->decode_opus(data, size, samples);
    }
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
            audio_mode = AudioProcessor::AUDIO_MODE_CODEC2;
        }
        for(int i=0;i<samples;i++)
        {
            audio_out[i] = (short)((float)audio_out[i] * amplif * _rx_volume);
        }
        if(_voip_forwarding)
        {
            for(int i=0;i< samples;i++)
            {
                _voip_encode_buffer->push_back(audio_out[i]);
            }
        }
        else if(!_voip_forwarding)
        {
            emit writePCM(audio_out,samples*sizeof(short), _audio_compressor_enabled, audio_mode);
        }
    }
}

void RadioController::receivePCMAudio(std::vector<float> *audio_data)
{
    int size = audio_data->size();
    short *pcm = new short[size];
    for(int i=0;i<size;i++)
    {
        pcm[i] = (short)(audio_data->at(i) * _rx_volume * 32767.0f);
        if(_voip_forwarding)
        {
            _voip_encode_buffer->push_back(pcm[i]);
        }
    }
    if(_voip_forwarding)
    {
        delete[] pcm;
    }
    else
    {
        /// Noise kills the compressor, so disabled
        emit writePCM(pcm, size*sizeof(short), false, AudioProcessor::AUDIO_MODE_ANALOG);
    }
    audio_data->clear();
    delete audio_data;
    audioFrameReceived();
}

unsigned int RadioController::getFrameLength(unsigned char *data)
{
    unsigned int frame_size1;
    unsigned int frame_size2;
    unsigned int frame_size3;

    memcpy(&frame_size1, &data[0], 4);
    memcpy(&frame_size2, &data[4], 4);
    memcpy(&frame_size3, &data[8], 4);
    if(frame_size1 == frame_size2)
        return frame_size1;
    else if(frame_size1 == frame_size3)
        return frame_size1;
    else if(frame_size2 == frame_size3)
        return frame_size2;
    else
        return 0;
}

unsigned int RadioController::getFrameCRC32(unsigned char *data)
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

void RadioController::receiveVideoData(unsigned char *data, int size)
{
    Q_UNUSED(size);
    unsigned int frame_size = getFrameLength(data);
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
        /// JPEG decoder has this nasty habit of segfaulting on image errors
        std::cerr << "CRC check failed, dropping frame" << std::endl;
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
    if(img.isNull())
    {
        delete[] raw_output;
        return;
    }
    QImage image = img.convertToFormat(QImage::Format_RGB32);

    emit videoImage(image);
    delete[] raw_output;

}

void RadioController::receiveNetData(unsigned char *data, int size)
{
    Q_UNUSED(size);
    unsigned int frame_size = getFrameLength(data);

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
    Q_UNUSED(res);
}


void RadioController::processVoipAudioFrame(short *pcm, int samples, quint64 sid)
{

    for(int i=0;i<samples;i++)
    {
        _voip_to_radio_queue->push_back(pcm[i]);
    }
    delete[] pcm;
    /*
    quint64 milisec = (quint64)_last_voiced_frame_timer.nsecsElapsed()/1000000;
    if((milisec >= 120) || (sid == _last_session_id))
    {
    }
    */
    _last_session_id = sid;
}

void RadioController::startTransmission()
{
    if(_rx_inited && _settings->rx_sample_rate != 1000000)
        return;
    if(_tx_inited || _voip_enabled)
        _transmitting = true;
}

void RadioController::endTransmission()
{
    _transmitting = false;
}

void RadioController::textData(QString text, bool repeat)
{
    _repeat_text = repeat;

    _text_out = text;
    _process_text = true;


}

void RadioController::textReceived(QString text)
{
    emit printText(text, false);
}

void RadioController::repeaterInfoReceived(QByteArray data)
{
    _radio_protocol->dataIn(data);
}

void RadioController::callsignReceived(QString callsign)
{
    QString time= QDateTime::currentDateTime().toString("dd/MMM/yyyy hh:mm:ss");
    QString text = "\n\n<b>" + time + "</b> " + "<font color=\"#FF5555\">" + callsign + " </font><br/>\n";
    /*
    short *samples = new short[_data_rec_sound->size()/sizeof(short)];
    short *origin = (short*) _data_rec_sound->data();
    memcpy(samples, origin, _data_rec_sound->size());
    emit writePCM(samples, _data_rec_sound->size(), false, AudioProcessor::AUDIO_MODE_ANALOG);
    */
    emit printText(text,true);
    emit printCallsign(callsign);
}

void RadioController::audioFrameReceived()
{
    emit displayReceiveStatus(true);
    _voice_led_timer->start(500);
}

void RadioController::dataFrameReceived()
{
    emit displayDataReceiveStatus(true);
    _data_led_timer->start(500);
    if((_rx_mode != gr_modem_types::ModemTypeQPSK250000)
            && (_rx_mode != gr_modem_types::ModemTypeQPSKVideo))
    {
        short *sound = (short*) _data_rec_sound->data();
        short *samples = new short[_data_rec_sound->size()/sizeof(short)];
        memcpy(samples, sound, _data_rec_sound->size());
        emit writePCM(samples, _data_rec_sound->size(), false, AudioProcessor::AUDIO_MODE_ANALOG);
    }

}

void RadioController::receiveEnd()
{
    emit displayReceiveStatus(false);
    emit displayDataReceiveStatus(false);
}

void RadioController::endAudioTransmission()
{
    QString time= QDateTime::currentDateTime().toString("d/MMM/yyyy hh:mm:ss");
    emit printText("<b>" + time + "</b> <font color=\"#77FF77\">Transmission end</font><br/>\n",true);
    unsigned int size = _end_rec_sound->size();
    short *samples = new short[size/sizeof(short)];
    short *origin = reinterpret_cast<short*>(_end_rec_sound->data());
    memcpy(samples, origin, size);
    emit writePCM(samples, size, false, AudioProcessor::AUDIO_MODE_ANALOG);
}

void RadioController::setChannels(ChannelList channels)
{
    _radio_protocol->setChannels(channels);
}

void RadioController::setStations(StationList list)
{
    _radio_protocol->setStations(list);
}

void RadioController::setCallsign()
{
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
}

void RadioController::toggleRX(bool value)
{

    if(value)
    {
        try
        {
            _mutex->lock();
            _modem->initRX(_rx_mode, _settings->rx_device_args.toStdString(),
                           _settings->rx_antenna.toStdString(), _settings->rx_freq_corr);
            _mutex->unlock();
        }
        catch(std::runtime_error &e)
        {
            _modem->deinitRX(_rx_mode);
            _mutex->unlock();

            emit initError("Could not init RX device, check settings");
            return;
        }
        // FIXME:
        struct timespec time_to_sleep = {1, 30000000L };
        nanosleep(&time_to_sleep, NULL);
        _mutex->lock();
        _modem->enableGUIFFT(_fft_enabled);
        _modem->enableGUIConst(_constellation_enabled);
        _modem->enableRSSI(_rssi_enabled);
        _modem->setRxSensitivity(((double)_settings->rx_sensitivity)/100.0);
        _modem->setSquelch(_settings->squelch);
        _modem->setRxCTCSS(_settings->rx_ctcss);
        _modem->setCarrierOffset(_settings->demod_offset);
        _modem->setSampRate(_settings->rx_sample_rate);
        _modem->tune(_settings->rx_frequency);
        _modem->calibrateRSSI(_settings->rssi_calibration_value);
        _modem->startRX();
        _mutex->unlock();

        _rx_inited = true;
    }
    else if (_rx_inited)
    {
        _mutex->lock();
        _modem->stopRX();
        _modem->deinitRX(_rx_mode);
        _mutex->unlock();

        _rx_inited = false;
    }
}

void RadioController::toggleTX(bool value)
{
    if(value)
    {
        setCallsign();
        try
        {
            _mutex->lock();
            if(_rx_inited)
                _modem->stopRX();
            _modem->initTX(_tx_mode, _settings->tx_device_args.toStdString(),
                           _settings->tx_antenna.toStdString(), _settings->tx_freq_corr);
            _mutex->unlock();
        }
        catch(std::runtime_error &e)
        {
            if(_rx_inited)
                _modem->startRX();
            _modem->deinitTX(_tx_mode);
            _mutex->unlock();
            emit initError("Could not init TX device, check settings");
            return;
        }
        if(_tx_mode == gr_modem_types::ModemTypeQPSKVideo)
            _video->init(_settings->video_device);
        else
            _video->deinit();

        _mutex->lock();
        _modem->setTxPower(0.01);
        _modem->setBbGain(_settings->bb_gain);
        _modem->tuneTx(430000000);
        _modem->setTxCTCSS(_settings->tx_ctcss);
        _modem->startTX();
        if(_rx_inited)
            _modem->startRX();
        _mutex->unlock();

        _tx_inited = true;
    }
    else if(_tx_inited)
    {
        _mutex->lock();
        _modem->stopTX();

        _modem->deinitTX(_tx_mode);
        _mutex->unlock();
        _video->deinit();

        _tx_inited = false;
    }
}

void RadioController::toggleRxMode(int value)
{
    if(_rx_inited)
    {
        _mutex->lock();
        _modem->stopRX();
        _mutex->unlock();
    }

    bool rx_inited_before = _rx_inited;
    if(rx_inited_before)
    {
        _rx_inited = false;
    }
    _rx_radio_type = radio_type::RADIO_TYPE_DIGITAL;
    switch(value)
    {
    case 0:
        _rx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _rx_mode = gr_modem_types::ModemTypeNBFM5000;
        _step_hz = 10;
        _scan_step_hz = 6250;
        break;
    case 1:
        _rx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _rx_mode = gr_modem_types::ModemTypeNBFM2500;
        _step_hz = 10;
        _scan_step_hz = 12500;
        break;
    case 2:
        _rx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _rx_mode = gr_modem_types::ModemTypeWBFM;
        _step_hz = 1000;
        _scan_step_hz = 200000;
        break;
    case 3:
        _rx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _rx_mode = gr_modem_types::ModemTypeUSB2500;
        _step_hz = 10;
        _scan_step_hz = 2500;
        break;
    case 4:
        _rx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _rx_mode = gr_modem_types::ModemTypeLSB2500;
        _step_hz = 10;
        _scan_step_hz = 2500;
        break;
    case 5:
        _rx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _rx_mode = gr_modem_types::ModemTypeFREEDV1600USB;
        _step_hz = 5;
        _scan_step_hz = 2500;
        break;
    case 6:
        _rx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _rx_mode = gr_modem_types::ModemTypeFREEDV700DUSB;
        _step_hz = 5;
        _scan_step_hz = 2500;
        break;
    case 7:
        _rx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _rx_mode = gr_modem_types::ModemTypeFREEDV800XAUSB;
        _step_hz = 5;
        _scan_step_hz = 2500;
        break;
    case 8:
        _rx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _rx_mode = gr_modem_types::ModemTypeFREEDV1600LSB;
        _step_hz = 5;
        _scan_step_hz = 2500;
        break;
    case 9:
        _rx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _rx_mode = gr_modem_types::ModemTypeFREEDV700DLSB;
        _step_hz = 5;
        _scan_step_hz = 2500;
        break;
    case 10:
        _rx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _rx_mode = gr_modem_types::ModemTypeFREEDV800XALSB;
        _step_hz = 5;
        _scan_step_hz = 2500;
        break;
    case 11:
        _rx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _rx_mode = gr_modem_types::ModemTypeAM5000;
        _step_hz = 10;
        _scan_step_hz = 10000;
        break;
    case 12:
        _rx_mode = gr_modem_types::ModemTypeBPSK2000;
        _step_hz = 5;
        _scan_step_hz = 12500;
        break;
    case 13:
        _rx_mode = gr_modem_types::ModemTypeBPSK1000;
        _step_hz = 5;
        _scan_step_hz = 6250;
        break;
    case 14:
        _rx_mode = gr_modem_types::ModemTypeQPSK2000;
        _step_hz = 5;
        _scan_step_hz = 6250;
        break;
    case 15:
        _rx_mode = gr_modem_types::ModemTypeQPSK20000;
        _step_hz = 50;
        _scan_step_hz = 25000;
        break;
    case 16:
        _rx_mode = gr_modem_types::ModemType2FSK2000;
        _step_hz = 10;
        _scan_step_hz = 12500;
        break;
    case 17:
        _rx_mode = gr_modem_types::ModemType2FSK20000;
        _step_hz = 500;
        _scan_step_hz = 50000;
        break;
    case 18:
        _rx_mode = gr_modem_types::ModemType4FSK2000;
        _step_hz = 5;
        _scan_step_hz = 12500;
        break;
    case 19:
        _rx_mode = gr_modem_types::ModemType4FSK20000;
        _step_hz = 500;
        _scan_step_hz = 50000;
        break; 
    case 20:
        _rx_mode = gr_modem_types::ModemTypeQPSKVideo;
        _step_hz = 1000;
        break;
    case 21:
        _rx_mode = gr_modem_types::ModemTypeQPSK250000;
        _step_hz = 1000;
        _scan_step_hz = 500000;
        break;
    default:
        _rx_mode = gr_modem_types::ModemTypeBPSK2000;
        _step_hz = 5;
        _scan_step_hz = 12500;
        break;
    }

    _mutex->lock();
    _modem->toggleRxMode(_rx_mode);
    _mutex->unlock();
    if(rx_inited_before)
    {
        _rx_inited = true;
    }
    if(_rx_inited)
    {
        _mutex->lock();
        _modem->startRX();
        _mutex->unlock();
    }
}

void RadioController::toggleTxMode(int value)
{
    _tx_radio_type = radio_type::RADIO_TYPE_DIGITAL;
    switch(value)
    {
    case 0:
        _tx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _tx_mode = gr_modem_types::ModemTypeNBFM5000;
        break;
    case 1:
        _tx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _tx_mode = gr_modem_types::ModemTypeNBFM2500;
        break;
    case 2:
        _tx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _tx_mode = gr_modem_types::ModemTypeWBFM;
        break;
    case 3:
        _tx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _tx_mode = gr_modem_types::ModemTypeUSB2500;
        break;
    case 4:
        _tx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _tx_mode = gr_modem_types::ModemTypeLSB2500;
        break;
    case 5:
        _tx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _tx_mode = gr_modem_types::ModemTypeFREEDV1600USB;
        break;
    case 6:
        _tx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _tx_mode = gr_modem_types::ModemTypeFREEDV700DUSB;
        break;
    case 7:
        _tx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _tx_mode = gr_modem_types::ModemTypeFREEDV800XAUSB;
        break;
    case 8:
        _tx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _tx_mode = gr_modem_types::ModemTypeFREEDV1600LSB;
        break;
    case 9:
        _tx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _tx_mode = gr_modem_types::ModemTypeFREEDV700DLSB;
        break;
    case 10:
        _tx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _tx_mode = gr_modem_types::ModemTypeFREEDV800XALSB;
        break;
    case 11:
        _tx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _tx_mode = gr_modem_types::ModemTypeAM5000;
        break;
    case 12:
        _tx_mode = gr_modem_types::ModemTypeBPSK2000;
        break;
    case 13:
        _tx_mode = gr_modem_types::ModemTypeBPSK1000;
        break;
    case 14:
        _tx_mode = gr_modem_types::ModemTypeQPSK2000;
        break;
    case 15:
        _tx_mode = gr_modem_types::ModemTypeQPSK20000;
        break;
    case 16:
        _tx_mode = gr_modem_types::ModemType2FSK2000;
        break;
    case 17:
        _tx_mode = gr_modem_types::ModemType2FSK20000;
        break;
    case 18:
        _tx_mode = gr_modem_types::ModemType4FSK2000;
        break;
    case 19:
        _tx_mode = gr_modem_types::ModemType4FSK20000;
        break;
    case 20:
        _tx_mode = gr_modem_types::ModemTypeQPSKVideo;
        break;
    case 21:
        _tx_mode = gr_modem_types::ModemTypeQPSK250000;
        break;
    default:
        _tx_mode = gr_modem_types::ModemTypeBPSK2000;
        break;
    }
    if(_tx_mode == gr_modem_types::ModemTypeQPSKVideo)
        _video->init(_settings->video_device);
    else
        _video->deinit();
    _mutex->lock();
    if(_tx_inited)
        _modem->stopTX();
    _modem->toggleTxMode(_tx_mode);
    if(_tx_inited)
        _modem->startTX();
    _mutex->unlock();
}

void RadioController::usePTTForVOIP(bool value)
{
    _voip_enabled = value;
}

void RadioController::setVOIPForwarding(bool value)
{
    _voip_forwarding = value;
}

void RadioController::setVox(bool value)
{
    _vox_enabled = value;
    if(!_vox_enabled)
    {
        _transmitting = false;
        updateInputAudioStream();
    }
    if(_vox_enabled)
    {
        _transmitting = true;
        updateInputAudioStream();
    }
}

void RadioController::toggleRepeat(bool value)
{
    if((_rx_mode != _tx_mode) && value) // no mixed mode repeat
        return;
    if(value && !_repeat)
    {
        if(!_duplex_enabled)
        {
            std::cerr << "Repeater mode can only function in duplex mode";
            return;
        }
        _repeat = value;
        _transmitting = true;
    }
    else if(!value && _repeat)
    {
        _transmitting = false;
        _repeat = value;
    }
    _modem->setRepeater(value); // ?mutex?
}

void RadioController::fineTuneFreq(long center_freq)
{
    _modem->setCarrierOffset(_settings->demod_offset + center_freq*_step_hz);
}

void RadioController::tuneFreq(qint64 center_freq)
{
    /// rx_frequency is the source center frequency
    _settings->rx_frequency = center_freq;
    _modem->tune(_settings->rx_frequency);
}

void RadioController::tuneTxFreq(qint64 actual_freq)
{
    _tx_frequency = actual_freq;
    /// LimeSDR mini tune requests are blocking
    _modem->tuneTx(_tx_frequency + _settings->tx_shift);

}

void RadioController::setCarrierOffset(qint64 offset)
{
    _settings->demod_offset = offset;
    /// we don't use carrier_offset for TX, fixed sample rate
    _modem->setCarrierOffset(offset);
}


void RadioController::changeTxShift(qint64 shift_freq)
{
    _settings->tx_shift = shift_freq;
    _modem->tuneTx(_tx_frequency + _settings->tx_shift);
}

void RadioController::setSquelch(int value)
{
    _settings->squelch = value;
    _modem->setSquelch(value);
}

void RadioController::setFilterWidth(int width)
{
    _modem->setFilterWidth(width);
}

void RadioController::setRxSensitivity(int value)
{
    _settings->rx_sensitivity = value;
    _modem->setRxSensitivity(((double)_settings->rx_sensitivity)/100.0);
}

void RadioController::setTxPower(int dbm)
{
    _settings->tx_power = dbm;
    _modem->setTxPower((float)dbm/100.0);
}

void RadioController::setBbGain(int value)
{
    _settings->bb_gain = value;
    _modem->setBbGain(_settings->bb_gain);
}

void RadioController::setRxSampleRate(int samp_rate)
{
    _settings->rx_sample_rate = samp_rate;
    _mutex->lock();
    _modem->setSampRate(samp_rate);
    _mutex->unlock();
}

void RadioController::setFFTSize(int size)
{
    _modem->setFFTSize(size);
}

void RadioController::enableAudioCompressor(bool value)
{
    _audio_compressor_enabled = value;
}

void RadioController::setVolume(int value)
{
    _rx_volume = 1e-3*exp(((float)value/100.0)*6.908);
}

void RadioController::setTxVolume(int value)
{
    _tx_volume = 1e-3*exp(((float)value/50.0)*6.908);
}

void RadioController::setRxCTCSS(float value)
{
    _settings->rx_ctcss = value;
    _modem->setRxCTCSS(value);
}

void RadioController::setTxCTCSS(float value)
{
    _settings->tx_ctcss = value;
    _modem->setTxCTCSS(value);
}

void RadioController::enableGUIConst(bool value)
{
    _constellation_enabled = value;
    _modem->enableGUIConst(value);
}

void RadioController::enableRSSI(bool value)
{
    _rssi_enabled = value;
    _modem->enableRSSI(value);
}

void RadioController::enableGUIFFT(bool value)
{
    _fft_enabled = value;
    _modem->enableGUIFFT(value);
}

void RadioController::enableDuplex(bool value)
{
    _duplex_enabled = value;
}

void RadioController::enableRelays(bool value)
{
    _relays_enabled = value;
    if(_relays_enabled)
    {
        _relay_controller->init();
    }
    else
    {
        _relay_controller->deinit();
    }
}

void RadioController::calibrateRSSI(float value)
{
    _modem->calibrateRSSI(value);
}

void RadioController::scan(bool receiving, bool wait_for_timer)
{
    bool increment_main_frequency = false;
    bool decrement_main_frequency = false;
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
        // Buffers are at least 40 msec, so we need at least twice as much time
        if(msec < 100)
        {
            return;
        }
    }
    _autotune_freq = _autotune_freq + _scan_step_hz;
    if(_autotune_freq >= _tune_limit_upper)
    {
        _autotune_freq = _tune_limit_lower + (_autotune_freq - _tune_limit_upper);
        increment_main_frequency = true;
    }
    if(_autotune_freq <= _tune_limit_lower)
    {
        _autotune_freq = _tune_limit_upper - (_tune_limit_lower - _autotune_freq);
        decrement_main_frequency = true;

    }


    if(increment_main_frequency)
    {
        _settings->rx_frequency = _settings->rx_frequency + _settings->rx_sample_rate;
        _mutex->lock();
        _modem->tune(_settings->rx_frequency);
        _mutex->unlock();
    }
    if(decrement_main_frequency)
    {
        _settings->rx_frequency = _settings->rx_frequency - _settings->rx_sample_rate;
        _mutex->lock();
        _modem->tune(_settings->rx_frequency);
        _mutex->unlock();
    }
    _modem->setCarrierOffset(_autotune_freq);

    _settings->demod_offset = _autotune_freq;
    emit freqToGUI(_settings->rx_frequency, _settings->demod_offset);
    _scan_timer->restart();
}

void RadioController::startScan(int step, int direction)
{
    if(!_rx_inited || !_scan_done || !_memory_scan_done)
        return;
    if(step != 0)
        _scan_step_hz = step;
    if(direction == 0)
        _scan_step_hz = -_scan_step_hz;
    _tune_limit_lower = -_settings->rx_sample_rate / 2;
    _tune_limit_upper = _settings->rx_sample_rate / 2;
    _autotune_freq = _settings->demod_offset;
    _scan_timer->start();
    _scan_done = false;
    scan(false, false);
}

void RadioController::stopScan()
{
    _scan_done = true;
    _scan_stop = false;
    _settings->demod_offset = _autotune_freq;
    emit freqToGUI(_settings->rx_frequency, _autotune_freq);
}

void RadioController::startMemoryScan(RadioChannels *channels, int direction)
{
    _memory_channels = channels->getChannels()->toList();
    if(!_rx_inited || !_scan_done || !_memory_scan_done || _memory_channels.size() < 1)
        return;

    if(direction == 0)
    {
        std::reverse(_memory_channels.begin(), _memory_channels.end());
    }
    _scan_timer->start();
    _memory_scan_done = false;
    _memory_scan_index = 0;
    memoryScan(false, false);
}

void RadioController::stopMemoryScan()
{
    _memory_scan_done = true;
    _scan_stop = false;
    emit freqToGUI(_settings->rx_frequency, _settings->demod_offset);
}

void RadioController::memoryScan(bool receiving, bool wait_for_timer)
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
        /// Buffers are at least 40 msec, so we need at least twice as much time
        if(msec < 100) // FIXME: hardcoded
        {
            return;
        }
    }
    struct timespec time_to_sleep;
    radiochannel *chan = _memory_channels.at(_memory_scan_index);
    _settings->rx_frequency = chan->rx_frequency - _settings->demod_offset;

    tuneFreq(_settings->rx_frequency);
    time_to_sleep = {0, 1000L }; /// Give PLL time to settle
    nanosleep(&time_to_sleep, NULL);

    emit freqToGUI(_settings->rx_frequency, _settings->demod_offset);
    toggleRxMode(chan->rx_mode);

    _settings->tx_shift = chan->tx_shift;
    tuneTxFreq(chan->rx_frequency);
    time_to_sleep = {0, 1000L };
    nanosleep(&time_to_sleep, NULL);

    toggleTxMode(chan->tx_mode);
    _memory_scan_index++;
    if(_memory_scan_index >= _memory_channels.size())
        _memory_scan_index = 0;

    _scan_timer->restart();
}
