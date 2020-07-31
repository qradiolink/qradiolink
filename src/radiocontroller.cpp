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


RadioController::RadioController(Settings *settings, Logger *logger,
                                 RadioChannels *radio_channels, QObject *parent) :
    QObject(parent)
{
    /// these pointers are owned by main()
    _settings = settings;
    _logger = logger;
    _radio_channels = radio_channels;

    _modem = new gr_modem(settings, logger);
    _codec = new AudioEncoder(settings);
    _audio_mixer_in = new AudioMixer;
    _layer2 = new Layer2Protocol(logger);
    _relay_controller = new RelayController(logger);
    _video = new VideoEncoder(logger);
    //_camera = new ImageCapture(settings, logger);
    _net_device = new NetDevice(logger, 0, _settings->ip_address);
    _mutex = new QMutex;

    _rand_frame_data = new unsigned char[4000];
    /// one way queue from radio and local voice to Mumble
    _to_voip_buffer = new QVector<short>;
    /// pre-allocated at maximum possible FFT size (make it a constant?)
    _fft_data = new float[1048576];
    _end_rec_sound = nullptr;

    _voice_led_timer = new QTimer(this);
    _voice_led_timer->setSingleShot(true);
    _data_led_timer = new QTimer(this);
    _data_led_timer->setSingleShot(true);
    _voip_tx_timer = new QTimer(this);
    _voip_tx_timer->setSingleShot(true);
    _vox_timer = new QTimer(this);
    _vox_timer->setSingleShot(true);
    _end_tx_timer = new QTimer(this);
    _end_tx_timer->setSingleShot(true);
    _radio_time_out_timer = new QTimer(this);
    _data_read_timer = new QElapsedTimer();
    _data_modem_reset_timer = new QElapsedTimer();
    _data_modem_sleep_timer = new QElapsedTimer();
    _scan_timer = new QElapsedTimer();
    _const_read_timer = new QElapsedTimer();
    _const_read_timer->start();
    _rssi_read_timer = new QElapsedTimer();
    _rssi_read_timer->start();
    _fft_read_timer = new QElapsedTimer();
    _fft_read_timer->start();
    _fft_poll_time = 75;
    _cw_timer = new QElapsedTimer();
    _cw_timer->start();

    _stop_thread =false;

    _scan_stop = false;
    _scan_done = true;
    _memory_scan_done = true;

    _data_modem_sleeping = false;
    _radio_to_voip_on = false;
    _video_on = false;
    _text_transmit_on = false;
    _proto_transmit_on = false;
    _cw_tone = false;
    _transmitting = false;
    _process_text = false;
    _process_data = false;
    _repeat_text = false;

    _rx_volume = 1e-3*exp(((float)_settings->rx_volume/100.0)*6.908);
    _tx_volume = 1e-3*exp(((float)_settings->tx_volume/50.0)*6.908);;
    _tx_frequency = _settings->rx_frequency + _settings->demod_offset;
    _autotune_freq = 0;
    _tune_limit_lower = -500000;
    _tune_limit_upper = 500000;
    _step_hz = 10;

    _rx_mode = gr_modem_types::ModemTypeBPSK2K;
    _tx_mode = gr_modem_types::ModemTypeBPSK2K;
    _rx_radio_type = radio_type::RADIO_TYPE_DIGITAL;
    _tx_radio_type = radio_type::RADIO_TYPE_DIGITAL;

    setCallsign();

    /// timers connections
    QObject::connect(_voice_led_timer, SIGNAL(timeout()), this, SLOT(receiveEnd()));
    QObject::connect(_data_led_timer, SIGNAL(timeout()), this, SLOT(receiveEnd()));
    QObject::connect(_data_led_timer, SIGNAL(timeout()), this, SLOT(receiveEnd()));
    QObject::connect(_voip_tx_timer, SIGNAL(timeout()), this, SLOT(stopVoipTx()));
    QObject::connect(_end_tx_timer, SIGNAL(timeout()), this, SLOT(endTx()));
    QObject::connect(_radio_time_out_timer, SIGNAL(timeout()), this, SLOT(radioTimeout()));

    /// Modem connections
    QObject::connect(_modem,SIGNAL(protoReceived(QByteArray)),this,
                     SLOT(protoReceived(QByteArray)));
    QObject::connect(_modem,SIGNAL(textReceived(QString)),this,SLOT(textReceived(QString)));
    QObject::connect(_modem,SIGNAL(callsignReceived(QString)),this,
                     SLOT(callsignReceived(QString)));
    QObject::connect(_modem,SIGNAL(audioFrameReceived()),this,SLOT(audioFrameReceived()));
    QObject::connect(_modem,SIGNAL(dataFrameReceived()),this,SLOT(dataFrameReceived()));
    QObject::connect(_modem,SIGNAL(receiveEnd()),this,SLOT(receiveEnd()));
    QObject::connect(_modem,SIGNAL(endAudioTransmission()),this,SLOT(endAudioTransmission()));
    QObject::connect(this,SIGNAL(audioData(unsigned char*,int)),_modem,
                     SLOT(transmitDigitalAudio(unsigned char*,int)));
    QObject::connect(this,SIGNAL(pcmData(std::vector<float>*)),_modem,
                     SLOT(transmitPCMAudio(std::vector<float>*)));
    QObject::connect(this,SIGNAL(videoData(unsigned char*,int)),_modem,
                     SLOT(transmitVideoData(unsigned char*,int)));
    QObject::connect(this,SIGNAL(netData(unsigned char*,int)),_modem,
                     SLOT(transmitNetData(unsigned char*,int)));
    QObject::connect(_modem,SIGNAL(digitalAudio(unsigned char*,int)),this,
                     SLOT(receiveDigitalAudio(unsigned char*,int)));
    QObject::connect(_modem,SIGNAL(pcmAudio(std::vector<float>*)),this,
                     SLOT(receivePCMAudio(std::vector<float>*)));
    QObject::connect(_modem,SIGNAL(videoData(unsigned char*,int)),this,
                     SLOT(receiveVideoData(unsigned char*,int)));
    QObject::connect(_modem,SIGNAL(netData(unsigned char*,int)),this,
                     SLOT(receiveNetData(unsigned char*,int)));

    //QObject::connect(_camera,SIGNAL(imageCaptured(unsigned char*,int)),this,
    //                 SLOT(processVideoFrame(unsigned char*,int)));

    /// Garbage to fill unused video and net frames with
    for (int j = 0;j<4000;j++)
        _rand_frame_data[j] = rand() % 256;

    QFile resfile(":/res/data_rec.raw");
    if(resfile.open(QIODevice::ReadOnly))
    {
        _data_rec_sound = new QByteArray(resfile.readAll());
    }

    QFile resfile1(":/res/BeepBeep.raw");
    if(resfile1.open(QIODevice::ReadOnly))
    {
        _timeout_sound = new QByteArray(resfile1.readAll());
    }

    setEndBeep(_settings->end_beep);

    toggleRxMode(_settings->rx_mode);
    toggleTxMode(_settings->tx_mode);
}

RadioController::~RadioController()
{
    if(_settings->rx_inited)
        toggleRX(false);
    if(_settings->tx_inited)
        toggleTX(false);
    delete _codec;
    delete _audio_mixer_in;
    delete _video;
    delete _net_device;
    delete _layer2;
    delete _voice_led_timer;
    delete _data_led_timer;
    delete _voip_tx_timer;
    delete _vox_timer;
    delete _end_tx_timer;
    delete _cw_timer;
    delete _modem;
    delete[] _rand_frame_data;
    _to_voip_buffer->clear();
    delete _to_voip_buffer;
    delete _relay_controller;
    delete _end_rec_sound;
    delete _data_rec_sound;
}

void RadioController::stop()
{
    if(_settings->rx_inited)
        toggleRX(false);
    if(_settings->tx_inited)
        toggleTX(false);
    _stop_thread=true;
}

void RadioController::run()
{
    /// Radioop thread where most things happen
    bool ptt_activated = false;
    bool data_to_process = false;
    bool buffers_filling = false;
    int last_channel_broadcast_time = 0;
    while(!_stop_thread)
    {
        bool transmitting = _transmitting;
        bool voip_forwarding = _settings->voip_forwarding;
        bool data_modem_sleeping = _data_modem_sleeping;

        QCoreApplication::processEvents(); // process signals
        if(_settings->voip_connected)
            QtConcurrent::run(this, &RadioController::flushRadioToVoipBuffer);
        buffers_filling = processMixerQueue();

        int time = QDateTime::currentDateTime().toTime_t();
        if((time - last_channel_broadcast_time) > 30)
        {
            last_channel_broadcast_time = time;
            if(voip_forwarding && !transmitting && !ptt_activated &&
                    (_tx_radio_type == radio_type::RADIO_TYPE_DIGITAL))
            {
                // FIXME: poke repeater to VOIP logic here
                //transmitServerInfoBeacon();
            }
        }

        updateDataModemReset(transmitting, ptt_activated); // for IP modem latency buildup

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

        if(_text_transmit_on)
            _process_text = false;
        if(_proto_transmit_on)
            _process_data = false;

        /// Get all available data from the demodulator
        QtConcurrent::run(this, &RadioController::getFFTData);
        QtConcurrent::run(this, &RadioController::getConstellationData);
        QtConcurrent::run(this, &RadioController::getRSSI);

        if(transmitting)
        {
            if(_tx_mode == gr_modem_types::ModemTypeCW600USB)
                updateCWK();
                //QtConcurrent::run(this, &RadioController::updateCWK);
            if(!_video_on)
                QtConcurrent::run(this, &RadioController::processVideoFrame);
            //triggerImageCapture(); // only partly implemented

            /// if not in the process of resetting the modem read interface
            /// data will accumulate in the interface buffer...
            if(!data_modem_sleeping)
                processInputNetStream();

        }

        data_to_process = getDemodulatorData();

        if(_process_text && !_text_transmit_on && (_tx_radio_type == radio_type::RADIO_TYPE_DIGITAL))
        {
            QtConcurrent::run(this, &RadioController::transmitTextData);
        }
        if(_process_data && !_proto_transmit_on && (_tx_radio_type == radio_type::RADIO_TYPE_DIGITAL))
        {
            QtConcurrent::run(this, &RadioController::transmitBinData);
        }

        /// Needed to keep the thread from using the CPU
        if(!data_to_process && !buffers_filling && !_process_text && !_process_data)
        {
            /// nothing to output from demodulator
            struct timespec time_to_sleep = {0, 20000000L };
            nanosleep(&time_to_sleep, NULL);
        }
        else
        {
            struct timespec time_to_sleep = {0, 1000L };
            nanosleep(&time_to_sleep, NULL);
        }
    }
    emit finished();
}


/// this code runs only in startTx and stopTx
void RadioController::updateInputAudioStream()
{
    /// Cases where not using local audio
    if(_settings->voip_forwarding
            || (_settings->repeater_enabled)
            || (!_transmitting && !_settings->vox_enabled)
            || (_tx_mode == gr_modem_types::ModemTypeQPSK250K)
            || (_tx_mode == gr_modem_types::ModemType4FSK100K)
            || (_tx_mode == gr_modem_types::ModemTypeCW600USB)
            || (_text_transmit_on || _proto_transmit_on))
    {
        emit setAudioReadMode(false, false, AudioProcessor::AUDIO_MODE_ANALOG);
        return;
    }

    /// If we got here we are using local audio input
    int audio_mode;
    if((_tx_mode == gr_modem_types::ModemTypeBPSK2K) ||
            (_tx_mode == gr_modem_types::ModemType2FSK2KFM) ||
            (_tx_mode == gr_modem_types::ModemType2FSK2K) ||
            (_tx_mode == gr_modem_types::ModemType4FSK2K) ||
            (_tx_mode == gr_modem_types::ModemType4FSK2KFM) ||
            (_tx_mode == gr_modem_types::ModemTypeQPSK2K) ||
            (_tx_mode == gr_modem_types::ModemTypeBPSK1K) ||
            (_tx_mode == gr_modem_types::ModemType2FSK1KFM) ||
            (_tx_mode == gr_modem_types::ModemType2FSK1K) ||
            (_tx_mode == gr_modem_types::ModemType4FSK1KFM))
    {
        audio_mode = AudioProcessor::AUDIO_MODE_CODEC2;
        emit setAudioReadMode(true, (bool)_settings->audio_compressor, audio_mode);
    }
    else if((_tx_mode == gr_modem_types::ModemTypeQPSK20K) ||
            (_tx_mode == gr_modem_types::ModemType2FSK20K) ||
            (_tx_mode == gr_modem_types::ModemType4FSK20K) ||
            (_tx_mode == gr_modem_types::ModemType4FSK20KFM))
    {
        audio_mode = AudioProcessor::AUDIO_MODE_OPUS;
        emit setAudioReadMode(true, (bool)_settings->audio_compressor, audio_mode);
    }
    else
    {
        audio_mode = AudioProcessor::AUDIO_MODE_ANALOG;
        emit setAudioReadMode(true, (bool)_settings->audio_compressor, audio_mode);
    }
}

/// Test tone
void RadioController::updateCWK()
{
    if(!_settings->tx_inited || _tx_mode != gr_modem_types::ModemTypeCW600USB)
    {
        return;
    }
    quint64 msec = (quint64)_cw_timer->nsecsElapsed() / 1000000;
    if(msec < 250)
        return;

    if(!_cw_tone)
    {
        _cw_tone = true;
    }
    else
    {
        _cw_tone = false;
    }
    _modem->setK(_cw_tone);
    _cw_timer->restart();
}

void RadioController::flushRadioToVoipBuffer()
{
    if(!_settings->voip_connected || _radio_to_voip_on)
        return;
    _radio_to_voip_on = true;
    /// large size of frames (120 ms) are better for Mumble client compatibility, but...
    int samples_in_buffer = _to_voip_buffer->size();
    if(samples_in_buffer >= 320)
    {

        short *pcm = new short[320];
        for(int i =0; i< 320;i++)
        {
            pcm[i] = _to_voip_buffer->at(i) * _voip_volume;
        }
        int packet_size = 0;
        unsigned char *encoded_audio;
        /// encode the PCM with higher quality and bitrate
        encoded_audio = _codec->encode_opus_voip(pcm, 320*sizeof(short), packet_size);
        emit voipDataOpus(encoded_audio,packet_size);
        delete[] pcm;
        _to_voip_buffer->remove(0,320);
    }
    _radio_to_voip_on = false;
}

bool RadioController::processMixerQueue()
{
    if(_audio_mixer_in->buffers_available())
    {
        short *pcm = _audio_mixer_in->mix_samples(_rx_volume);
        if(pcm == nullptr)
            return false;
        short *local_pcm = new short[320];
        memcpy(local_pcm, pcm, 320*sizeof(short));

        if(_settings->voip_forwarding || _settings->repeater_enabled)
        {
            if(!_voip_tx_timer->isActive())
            {
                _transmitting = true;
            }
            _voip_tx_timer->start(500);
            /// Out to radio and don't loop back to Mumble
            txAudio(pcm, 320*sizeof(short), 1, true);
        }
        else
        {
            /// nothing towards radio or VOIP
            delete[] pcm;
        }
        if((_settings->voip_forwarding || _settings->repeater_enabled) &&
                _settings->mute_forwarded_audio)
        {
            delete[] local_pcm;
        }
        else
        {
            /// Routed to local audio output
            emit writePCM(local_pcm, 320*sizeof(short), (bool)_settings->audio_compressor,
                          AudioProcessor::AUDIO_MODE_OPUS);
            audioFrameReceived();
        }
        return true;
    }
    return false;
}


void RadioController::txAudio(short *audiobuffer, int audiobuffer_size,
                              int vad, bool radio_only)
{
    /// first check the other places we need to send it
    if(_settings->vox_enabled)
    {
        if(vad)
        {
            _vox_timer->start(500);
            if(!_settings->tx_started && !_settings->voip_ptt_enabled)
                _transmitting = true;
        }
        if(!vad && !_vox_timer->isActive())
        {
            /// Vox timer ran out, stopping TX
            if(_settings->tx_started && !_settings->voip_ptt_enabled)
                _transmitting = false;
            /// Audio input stream will be stopped at the next thread iteration
            delete[] audiobuffer;
            return;
        }
    }

    if(_transmitting && _settings->voip_ptt_enabled && !radio_only)
    {

        for(unsigned int i=0;i< (unsigned int)audiobuffer_size/sizeof(short);i++)
        {
            _to_voip_buffer->push_back(audiobuffer[i] * _tx_volume);
        }
    }

    if(!_settings->tx_inited || !_settings->tx_started)
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

    /// Digital voice
    ///
    int packet_size = 0;
    unsigned char *encoded_audio;
    /// digital volume adjust
    for(unsigned int i = 0;i< audiobuffer_size/sizeof(short);i++)
    {
        audiobuffer[i] = audiobuffer[i] * _tx_volume;
    }
    if((_tx_mode == gr_modem_types::ModemTypeBPSK2K) ||
            (_tx_mode == gr_modem_types::ModemType2FSK2KFM) ||
            (_tx_mode == gr_modem_types::ModemType2FSK2K) ||
            (_tx_mode == gr_modem_types::ModemType4FSK2K) ||
            (_tx_mode == gr_modem_types::ModemType4FSK2KFM) ||
            (_tx_mode == gr_modem_types::ModemTypeQPSK2K))
        encoded_audio = _codec->encode_codec2_1400(audiobuffer, audiobuffer_size, packet_size);
    else if((_tx_mode == gr_modem_types::ModemTypeBPSK1K) ||
            (_tx_mode == gr_modem_types::ModemType2FSK1KFM) ||
            (_tx_mode == gr_modem_types::ModemType2FSK1K) ||
            (_tx_mode == gr_modem_types::ModemType4FSK1KFM))
        encoded_audio = _codec->encode_codec2_700(audiobuffer, audiobuffer_size, packet_size);
    else
        encoded_audio = _codec->encode_opus(audiobuffer, audiobuffer_size, packet_size);

    emit audioData(encoded_audio,packet_size);
    delete[] audiobuffer;
}

void RadioController::triggerImageCapture()
{
    // requires gstreamer-plugins-bad, only partly implemented
    //_camera->capture_image();
}

void RadioController::processVideoFrame()
{
    if((_tx_mode != gr_modem_types::ModemTypeQPSKVideo) || _video_on)
        return;
    _video_on = true;
    unsigned int max_video_frame_size = 3122;
    unsigned long encoded_size;

    /// Large alloc
    unsigned char *videobuffer = (unsigned char*)calloc(max_video_frame_size,
                                                        sizeof(unsigned char));
    memset(videobuffer, 0, max_video_frame_size);
    QElapsedTimer timer;
    qint64 microsec;
    timer.start();

    /// This includes V4L2 capture time as well
    _video->encode_jpeg(&(videobuffer[24]), encoded_size, max_video_frame_size - 24);

    if(_settings->voip_ptt_enabled && _transmitting && (encoded_size > 0))
    {
        unsigned char *video_frame = (unsigned char*)calloc(encoded_size,
                                                            sizeof(unsigned char));
        memcpy(video_frame, &(videobuffer[24]), encoded_size);
        emit voipVideoData(video_frame, encoded_size);
        delete[] videobuffer;
        microsec = (quint64)timer.nsecsElapsed();
        if(microsec < 100000000)
        {
            struct timespec time_to_sleep = {0, (100000000 - (long)microsec)};
            nanosleep(&time_to_sleep, NULL);
        }
        _video_on = false;
        return;
    }

    microsec = (quint64)timer.nsecsElapsed();
    if(microsec < 100000000)
    {
        struct timespec time_to_sleep = {0, (100000000 - (long)microsec)};
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
    /// Rest of video frame filled with garbage to keep the same radio frame size
    for(unsigned int k=encoded_size+24,i=0;k<max_video_frame_size;k++,i++)
    {
        videobuffer[k] = _rand_frame_data[i];
    }

    emit videoData(videobuffer,max_video_frame_size);
    _video_on = false;
}

void RadioController::processInputNetStream()
{
    if((_tx_mode != gr_modem_types::ModemTypeQPSK250K)
            && (_tx_mode != gr_modem_types::ModemType4FSK100K))
        return;
    // 48400 microsec per frame, during this time data enters the interface socket buffer
    qint64 time_per_frame = 48400000;
    qint64 microsec, time_left;
    int max_frame_size = 1516;
    int read_size = 1500;

    if(_tx_mode == gr_modem_types::ModemTypeQPSK250K)
    {
        max_frame_size = 1516;
        read_size = 1500;
        time_per_frame = 48400000;
    }
    if(_tx_mode == gr_modem_types::ModemType4FSK100K)
    {
        max_frame_size = 622;
        read_size = 600;
        time_per_frame = 50000000; // ???
    }

    microsec = (quint64)_data_read_timer->nsecsElapsed();
    if((_tx_mode == gr_modem_types::ModemTypeQPSK250K) && (microsec < 40000000))
    {
        return;
    }
    if((_tx_mode == gr_modem_types::ModemType4FSK100K) && (microsec < 20000000))
    {
        return;
    }
    time_left = time_per_frame - microsec;
    struct timespec time_to_sleep = {0, (long)time_left };

    if(time_left > 0)
        nanosleep(&time_to_sleep, NULL);
    _data_read_timer->restart();

    unsigned char *netbuffer = (unsigned char*)calloc(max_frame_size, sizeof(unsigned char));
    int nread;
    unsigned char *buffer = _net_device->read_buffered(nread, read_size);

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
    else if(_settings->burst_ip_modem)
    {
        delete[] buffer;
        delete[] netbuffer;
    }
    else
    {
        /// no data from the net if
        unsigned int fake_nread = 0;
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

void RadioController::transmitTextData()
{
    if(!_settings->tx_inited)
    {
        _process_text = false;
        return;
    }
    if(_text_transmit_on)
        return;

    _text_transmit_on = true;
    _transmitting = true;
    /// callsign frame
    // FIXME: this doesn't seem to work, callsign is actually sent
    // after the first text frame most of the time
    struct timespec time_to_sleep = {0, 39800000L };
    nanosleep(&time_to_sleep, NULL);

    start_text_tx:
    int frame_size;
    if((_tx_mode == gr_modem_types::ModemTypeBPSK2K) ||
            (_tx_mode == gr_modem_types::ModemType2FSK2KFM) ||
            (_tx_mode == gr_modem_types::ModemType2FSK2K) ||
            (_tx_mode == gr_modem_types::ModemType4FSK2K) ||
            (_tx_mode == gr_modem_types::ModemType4FSK2KFM) ||
            (_tx_mode == gr_modem_types::ModemTypeQPSK2K))
        frame_size = 7;
    else if((_tx_mode == gr_modem_types::ModemTypeBPSK1K) ||
            (_tx_mode == gr_modem_types::ModemType2FSK1KFM) ||
            (_tx_mode == gr_modem_types::ModemType2FSK1K) ||
            (_tx_mode == gr_modem_types::ModemType4FSK1KFM))
        frame_size = 4;
    else
        frame_size = 47;
    int no_frames = _text_out.size() / frame_size;
    int leftover = _text_out.size() % frame_size;
    if(leftover > 0)
    {
        no_frames += 1;
    }
    for(int i=0; i < no_frames; i++)
    {
        QString text_frame;
        if((i == (no_frames - 1)) && leftover > 0)
        {
            /// last frame
            text_frame = _text_out.mid(i * frame_size, leftover);
        }
        else
        {
            text_frame = _text_out.mid(i * frame_size, frame_size);
        }
        _modem->transmitTextData(text_frame, FrameTypeText);
        // FIXME: frame should last about 40 msec average, however in practice the clock
        // is not very precise so the last bytes may be truncated because TX is switched off
        struct timespec time_to_sleep = {0, 39800000L };
        nanosleep(&time_to_sleep, NULL);

        /// Stop when PTT is triggered by user
        if(!_transmitting)
        {
            _text_transmit_on = false;
            return;
        }
    }
    if(_repeat_text)
    {
        goto start_text_tx;
    }
    _transmitting = false;
    _text_transmit_on = false;
}

void RadioController::transmitBinData()
{
    if(!_settings->tx_inited)
    {
        _process_data = false;
        return;
    }
    if(_proto_transmit_on)
        return;

    _proto_transmit_on = true;
    _transmitting = true;
    /// callsign frame
    // FIXME: this doesn't seem to work, callsign is actually sent
    // after the first text frame most of the time
    struct timespec time_to_sleep = {0, 39800000L };
    nanosleep(&time_to_sleep, NULL);

    start_data_tx:
    int frame_size;
    if((_tx_mode == gr_modem_types::ModemTypeBPSK2K) ||
            (_tx_mode == gr_modem_types::ModemType2FSK2KFM) ||
            (_tx_mode == gr_modem_types::ModemType2FSK2K) ||
            (_tx_mode == gr_modem_types::ModemType4FSK2K) ||
            (_tx_mode == gr_modem_types::ModemType4FSK2KFM) ||
            (_tx_mode == gr_modem_types::ModemTypeQPSK2K))
        frame_size = 7;
    else if((_tx_mode == gr_modem_types::ModemTypeBPSK1K) ||
            (_tx_mode == gr_modem_types::ModemType2FSK1KFM) ||
            (_tx_mode == gr_modem_types::ModemType2FSK1K) ||
            (_tx_mode == gr_modem_types::ModemType4FSK1KFM))
        frame_size = 4;
    else
        frame_size = 47;

    int no_frames = _proto_out.size() / frame_size;
    int leftover = _proto_out.size() % frame_size;
    if(leftover > 0)
    {
        no_frames += 1;
    }
    for(int i=0; i < no_frames; i++)
    {
        QByteArray data_frame;
        if((i == (no_frames - 1)) && leftover > 0)
        {
            /// last frame
            data_frame = _proto_out.mid(i * frame_size, leftover);
        }
        else
        {
            data_frame = _proto_out.mid(i * frame_size, frame_size);
        }
        _modem->transmitBinData(data_frame, FrameTypeProto);
        // FIXME: frame should last about 40 msec average, however in practice the clock
        // is not very precise so the last bytes may be truncated because TX is switched off
        struct timespec time_to_sleep = {0, 39800000L };
        nanosleep(&time_to_sleep, NULL);

        /// Stop when PTT is triggered by user
        if(!_transmitting)
        {
            _proto_transmit_on = false;
            _proto_out.clear();
            return;
        }
    }
    if(_repeat_text)
    {
        goto start_data_tx;
    }
    _transmitting = false;
    _proto_transmit_on = false;
    _proto_out.clear();
}

void RadioController::sendTxBeep(int sound)
{
    short *samples;
    int size;
    switch(sound)
    {
    case 0:
        size = 8192 * sizeof(short);
        samples = new short[size];
        memset(samples, 0, size);
        break;
    default:
        samples = (short*) _end_rec_sound->data();
        size = _end_rec_sound->size();
        break;
    }

    std::vector<float> *pcm = new std::vector<float>;

    for(unsigned int i=0;i<size/sizeof(short);i++)
    {
        pcm->push_back((float)samples[i] / 32767.0f * 0.4);
    }
    for(unsigned int i=0;i<320*4;i++)
        pcm->push_back(0.0);
    emit pcmData(pcm);
}

void RadioController::transmitServerInfoBeacon()
{
    _proto_out = _layer2->buildRepeaterInfo();
    _process_data = true;
}

void RadioController::setRelays(bool transmitting)
{
    if(!_settings->enable_relays)
        return;
    int res;
    /// The reason for the loop with individual bit and not the direct call with a bitmask
    /// is the possibility of adding delays between each relay switch
    if(transmitting)
    {
        for(int i=0;i<8;i++)
        {
            if(bool((_settings->relay_sequence >> i) & 0x1))
            {
                res = _relay_controller->enableRelay(i);
                if(!res)
                {
                    _logger->log(Logger::LogLevelCritical,
                                 "Relay control failed, stopping to avoid damage");
                    exit(EXIT_FAILURE); // bit drastic, ain't it?
                }
            }
        }
    }
    else
    {
        for(int i=7;i>-1;i--)
        {
            if(bool((_settings->relay_sequence >> i) & 0x1))
            {
                res = _relay_controller->disableRelay(i);
                if(!res)
                {
                    _logger->log(Logger::LogLevelCritical,
                                 "Relay control failed, stopping to avoid damage");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }

}

void RadioController::startTx()
{
    updateInputAudioStream(); // moved here, LimeSDR specific thing (calibration at low power)
    if(_settings->tx_inited)
    {
        if(_end_tx_timer->isActive())
        {
            _end_tx_timer->stop();
        }
        if((_tx_mode == gr_modem_types::ModemTypeQPSK250K)
                || (_tx_mode == gr_modem_types::ModemType4FSK100K))
        {
            _data_modem_reset_timer->start();
            _data_modem_sleep_timer->start();
            _data_read_timer->start();
        }

        if(!_settings->enable_duplex)
        {
            _modem->enableDemod(false);
            _modem->setRxSensitivity(0.01);
        }

        setRelays(true);
        _modem->tuneTx(_tx_frequency + _settings->tx_shift);
        _modem->setTxPower((float)_settings->tx_power/100);

        /** old code
        if(_settings->_rx_inited && !_settings->_repeater_enabled &&
                        (_rx_mode != gr_modem_types::ModemTypeQPSK250000))
            _modem->stopRX();


        if(_settings->_tx_started)
            _modem->stopTX();

        /// LimeSDR calibration procedure happens after every tune request
        struct timespec time_to_sleep = {0, 10000000L };
        nanosleep(&time_to_sleep, NULL);
        _modem->startTX(_settings->block_buffer_size);
        */

        _settings->tx_started = true;
        if((_tx_radio_type == radio_type::RADIO_TYPE_DIGITAL))
        {
            _modem->startTransmission(_callsign);
        }
        if((_tx_radio_type == radio_type::RADIO_TYPE_ANALOG)
                && ((_tx_mode == gr_modem_types::ModemTypeNBFM2500) ||
                    (_tx_mode == gr_modem_types::ModemTypeNBFM5000)))
        {
            // disabled. breaks things
            //sendTxBeep(_settings->end_beep);
        }
        _radio_time_out_timer->start(1000 * _settings->radio_tot);
        emit displayTransmitStatus(true);
    }
}

void RadioController::stopTx()
{
    updateInputAudioStream();
    if(_settings->tx_inited)
    {
        int tx_tail_msec = 100;
        if(_tx_radio_type == radio_type::RADIO_TYPE_DIGITAL)
        {
            _modem->endTransmission(_callsign);
            if((_tx_mode == gr_modem_types::ModemTypeBPSK2K) ||
                    (_tx_mode == gr_modem_types::ModemType2FSK2KFM) ||
                    (_tx_mode == gr_modem_types::ModemType2FSK2K) ||
                    (_tx_mode == gr_modem_types::ModemType4FSK2K) ||
                    (_tx_mode == gr_modem_types::ModemType4FSK2KFM) ||
                    (_tx_mode == gr_modem_types::ModemTypeQPSK2K))
                tx_tail_msec = 800;
            else
                tx_tail_msec = 500;
        }
        if((_tx_radio_type == radio_type::RADIO_TYPE_ANALOG)
                && ((_tx_mode == gr_modem_types::ModemTypeNBFM2500) ||
                    (_tx_mode == gr_modem_types::ModemTypeNBFM5000)))
        {
            sendTxBeep(_settings->end_beep);
            tx_tail_msec = 800;
        }
        // FIXME: end tail length should be calculated exactly
        _end_tx_timer->start(tx_tail_msec);
        _radio_time_out_timer->stop();

        /** old code
        if(_settings->_rx_inited && !_settings->_repeater_enabled &&
                    (_rx_mode != gr_modem_types::ModemTypeQPSK250000))
           _modem->startRX(_settings->block_buffer_size);
        */
    }
}

void RadioController::endTx()
{
    _modem->setTxPower(0.01);
    _modem->flushSources();
    /// On the LimeSDR mini, whenever I call setTxPower I get a brief spike of the LO
    setRelays(false);
    if(!_settings->enable_duplex)
    {
        _modem->enableDemod(true);
        _modem->setRxSensitivity(((double)_settings->rx_sensitivity)/100.0);
    }
    emit displayTransmitStatus(false);
    _settings->tx_started = false;
}

void RadioController::radioTimeout()
{
    QString time= QDateTime::currentDateTime().toString("d/MMM/yyyy hh:mm:ss");
    emit printText("<br/><b>" + time +
                   "</b> <font color=\"#77FF77\">Radio timeout</font><br/>\n",true);
    short *origin = (short*) _timeout_sound->data();
    int size = _timeout_sound->size();
    short *samples = new short[size/sizeof(short)];
    for(unsigned int i=0;i<size/sizeof(short);i++)
    {
        samples[i] = short(origin[i] / 2);
        if(_settings->voip_forwarding)
        {
            /// routed to Mumble
            _to_voip_buffer->push_back(samples[i]);
        }
    }

    //_audio_mixer_in->addSamples(samples, size, -1000);
    emit writePCM(samples, size, false, AudioProcessor::AUDIO_MODE_ANALOG);
    if(_settings->tot_tx_end)
        _transmitting = false;
}

void RadioController::stopVoipTx()
{
    /// Called by voip tx timer
    _transmitting = false;
}

void RadioController::setAudioRecord(bool value)
{
    _settings->recording_audio = value;
    emit recordAudio(value);
}


void RadioController::updateDataModemReset(bool transmitting, bool ptt_activated)
{
    if(((_tx_mode == gr_modem_types::ModemTypeQPSK250K)
        || (_tx_mode == gr_modem_types::ModemType4FSK100K)) && !_data_modem_sleeping
            && transmitting && ptt_activated)
    {
        qint64 sec_modem_running;
        sec_modem_running = (quint64)_data_modem_reset_timer->nsecsElapsed()/1000000000;
        if(sec_modem_running > 300)
        {
            _logger->log(Logger::LogLevelInfo, "Resetting modem");
            _data_modem_sleeping = true;
            _data_modem_sleep_timer->restart();
        }
    }
    if(((_tx_mode == gr_modem_types::ModemTypeQPSK250K)
        || (_tx_mode == gr_modem_types::ModemType4FSK100K)) && _data_modem_sleeping)
    {
        qint64 sec_modem_sleeping;
        sec_modem_sleeping = (quint64)_data_modem_sleep_timer->nsecsElapsed()/1000000000;
        if(sec_modem_sleeping > 2)
        {
            _data_modem_sleep_timer->restart();
            _data_modem_sleeping = false;
            _data_modem_reset_timer->restart();
            _modem->startTransmission(_callsign);
            _logger->log(Logger::LogLevelInfo, "Modem reset complete");
        }
    }
}

bool RadioController::getDemodulatorData()
{
    bool data_to_process = false;
    if(_settings->rx_inited)
    {
        if(_rx_radio_type == radio_type::RADIO_TYPE_DIGITAL)
            data_to_process = _modem->demodulate();
        else if(_rx_radio_type == radio_type::RADIO_TYPE_ANALOG)
        {
            data_to_process = _modem->demodulateAnalog();
        }
        /// scanning
        if(!_scan_done)
            scan(data_to_process);
        if(!_memory_scan_done)
            memoryScan(data_to_process);
    }
    return data_to_process;
}


void RadioController::getRSSI()
{
    qint64 msec = (quint64)_rssi_read_timer->nsecsElapsed() / 1000000;
    if(msec < _fft_poll_time)
    {
        return;
    }
    float rssi = _modem->getRSSI();
    _rssi_read_timer->restart();
    _settings->rssi = rssi;
    if(!_settings->show_controls)
        return;
    if(rssi < 99.0f)
        emit newRSSIValue(rssi);
}

void RadioController::getFFTData()
{
    if(!_settings->show_fft)
    {
        _fft_read_timer->restart();
        return;
    }
    qint64 msec = (quint64)_fft_read_timer->nsecsElapsed() / 1000000;
    if(msec < _fft_poll_time)
    {
        return;
    }

    unsigned int fft_size = 0; // this is a reference
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
    if(!_settings->show_constellation)
    {
        _const_read_timer->restart();
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

/// callback from gr_modem via signal
void RadioController::receiveDigitalAudio(unsigned char *data, int size)
{
    short *audio_out;
    int samples; // reference
    int audio_mode = AudioProcessor::AUDIO_MODE_OPUS;
    if((_rx_mode == gr_modem_types::ModemTypeBPSK2K) ||
            (_rx_mode == gr_modem_types::ModemType2FSK2KFM) ||
            (_rx_mode == gr_modem_types::ModemType2FSK2K) ||
            (_rx_mode == gr_modem_types::ModemType4FSK2K) ||
            (_rx_mode == gr_modem_types::ModemType4FSK2KFM) ||
            (_rx_mode == gr_modem_types::ModemTypeQPSK2K))
    {
        audio_out = _codec->decode_codec2_1400(data, size, samples);
    }
    else if((_rx_mode == gr_modem_types::ModemTypeBPSK1K) ||
            (_rx_mode == gr_modem_types::ModemType2FSK1KFM) ||
            (_rx_mode == gr_modem_types::ModemType2FSK1K) ||
            (_rx_mode == gr_modem_types::ModemType4FSK1KFM))
        audio_out = _codec->decode_codec2_700(data, size, samples);
    else
    {
        audio_out = _codec->decode_opus(data, size, samples);
    }
    delete[] data;
    if(samples > 0)
    {
        if((_rx_mode == gr_modem_types::ModemTypeBPSK2K) ||
                (_rx_mode == gr_modem_types::ModemType2FSK2KFM) ||
                (_rx_mode == gr_modem_types::ModemType2FSK2K) ||
                (_rx_mode == gr_modem_types::ModemType4FSK2K) ||
                (_rx_mode == gr_modem_types::ModemType4FSK2KFM) ||
                (_rx_mode == gr_modem_types::ModemTypeQPSK2K) ||
                (_rx_mode == gr_modem_types::ModemTypeBPSK1K) ||
                (_rx_mode == gr_modem_types::ModemType2FSK1KFM) ||
                (_rx_mode == gr_modem_types::ModemType4FSK1KFM) ||
                (_rx_mode == gr_modem_types::ModemType2FSK1K))
        {
            audio_mode = AudioProcessor::AUDIO_MODE_CODEC2;
        }
        for(int i=0;i<samples;i++)
        {
            audio_out[i] = (short)((float)audio_out[i] * _rx_volume);
            if(_settings->voip_forwarding)
            {
                /// routing to Mumble
                _to_voip_buffer->push_back(audio_out[i]);
            }
        }
        if(_settings->voip_forwarding && _settings->mute_forwarded_audio
                && !_settings->repeater_enabled)
        {
            /// no local audio
            delete[] audio_out;
        }
        else
        {
            if(_settings->voip_connected || _settings->repeater_enabled)
            {
                /// need to mix several audio channels
                _audio_mixer_in->addSamples(audio_out, samples, -9999); // radio id hardcoded
            }
            else
            {
                emit writePCM(audio_out,samples*sizeof(short),
                              (bool)_settings->audio_compressor, audio_mode);
            }
        }
    }
}

/// callback from gr_modem via signal
void RadioController::receivePCMAudio(std::vector<float> *audio_data)
{
    int size = audio_data->size();
    short *pcm = new short[size];
    for(int i=0;i<size;i++)
    {
        pcm[i] = (short)(audio_data->at(i) * _rx_volume * 32767.0f);
        if(_settings->voip_forwarding)
        {
            /// routed to Mumble
            _to_voip_buffer->push_back(pcm[i]);
        }
    }

    if(_settings->voip_forwarding && !_settings->repeater_enabled
            && _settings->mute_forwarded_audio)
    {
        /// No local audio
        delete[] pcm;
    }
    else
    {
        if(_settings->voip_connected || _settings->repeater_enabled)
        {
            /// Need to mix several audio channels
            _audio_mixer_in->addSamples(pcm, size, -9999); // radio id hardcoded
        }
        else
        {
            // FIXME: compressor expects 40 ms frames, and if we give it shorter frames
            // it will introduce gaps in audio
            emit writePCM(pcm, size*sizeof(short), false, AudioProcessor::AUDIO_MODE_ANALOG);
        }
    }

    audio_data->clear();
    delete audio_data;
    audioFrameReceived();
}

/// Used by video and IP modem
unsigned int RadioController::getFrameLength(unsigned char *data)
{
    unsigned int frame_size1;
    unsigned int frame_size2;
    unsigned int frame_size3;
    /// do we really need this redundancy?
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

/// Used by video and IP modem
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

/// callback from gr_modem via signal
void RadioController::receiveVideoData(unsigned char *data, int size)
{
    Q_UNUSED(size);
    unsigned int frame_size = getFrameLength(data);
    unsigned int crc = getFrameCRC32(data);
    if(frame_size == 0)
    {
        _logger->log(Logger::LogLevelWarning, "received wrong video frame size, dropping frame ");
        delete[] data;
        return;
    }
    if(frame_size > 3122 - 24)
    {
        _logger->log(Logger::LogLevelWarning, "video frame size too large, dropping frame ");
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
        _logger->log(Logger::LogLevelWarning, "Video CRC check failed, dropping frame");
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

/// callback from gr_modem via signal
void RadioController::receiveNetData(unsigned char *data, int size)
{
    Q_UNUSED(size); // size comes from frame header
    unsigned int frame_size = getFrameLength(data);

    if(frame_size > 1500) // FIXME: The MTU setting in netdevice
    {
        _logger->log(Logger::LogLevelWarning, "received wrong IP frame size, dropping frame ");
        delete[] data;
        return;
    }
    if(frame_size == 0) // fill-up garbage
    {
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
        _logger->log(Logger::LogLevelWarning, "IP frame CRC check failed, dropping frame ");
        delete[] net_frame;
        return;
    }

    int res = _net_device->write_buffered(net_frame,frame_size);
    Q_UNUSED(res); // FIXME: what if ioctl fails?
}

void RadioController::protoReceived(QByteArray data)
{
    _incoming_proto_buffer.append(data);
}

/// signal from Mumble
void RadioController::processVoipAudioFrame(short *pcm, int samples, quint64 sid)
{
    _audio_mixer_in->addSamples(pcm, samples, sid);
}

void RadioController::processVoipVideoFrame(unsigned char *video_frame, int size, quint64 sid)
{
    Q_UNUSED(sid); // multiple video users not implemented yet
    unsigned char *raw_output = _video->decode_jpeg(video_frame, size);

    delete[] video_frame;
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

void RadioController::startTransmission()
{
    if(_settings->rx_inited && _settings->rx_sample_rate != 1000000 &&
            (_settings->rx_device_args == _settings->tx_device_args))
    {
        /// Trying to transmit and receive at different sample rates
        /// might work if using different devices so just log a warning
        _logger->log(Logger::LogLevelWarning,
            "Trying to transmit and receive at different sample rates "
                     "works only with separate devices");
        return;
    }
    if(_settings->tx_inited || _settings->voip_ptt_enabled)
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
    //_proto_out = _radio_protocol->buildPageMessage(_callsign, text, false, _callsign);
    //_process_data = true;
}

void RadioController::textMumble(QString text, bool channel)
{
    // FIXME: This will loop endlessly if radio is on duplex on the same freq as RX
    if(!channel || !_settings->voip_forwarding)
        return;
    _repeat_text = false;
    _text_out = text;
    _process_text = true;
}

/// callback from gr_modem via signal
void RadioController::textReceived(QString text)
{
    if(_settings->repeater_enabled && _tx_radio_type == radio_type::RADIO_TYPE_DIGITAL)
    {
        _modem->transmitTextData(text);
    }
    /// Disallow too large messages
    /// If end TX is not received, this buffer would fill forever
    if(_settings->voip_forwarding)
        _incoming_text_buffer.append(text);
    if(_incoming_text_buffer.size() >= 1024)
    {
        emit newMumbleMessage(_incoming_text_buffer);
        _incoming_text_buffer = "";
    }
    emit printText(text, false);
}

/// GUI only
void RadioController::callsignReceived(QString callsign)
{
    if(_settings->repeater_enabled && _tx_radio_type == radio_type::RADIO_TYPE_DIGITAL)
    {
        _modem->sendCallsign(callsign);
    }
    QString time= QDateTime::currentDateTime().toString("dd/MMM/yyyy hh:mm:ss");
    QString text = "\n\n<br/><b>" + time + "</b> " + "<font color=\"#FF5555\">"
            + callsign + " </font><br/>\n";
    /** disabled because of unwanted interaction
    short *samples = new short[_data_rec_sound->size()/sizeof(short)];
    short *origin = (short*) _data_rec_sound->data();
    memcpy(samples, origin, _data_rec_sound->size());
    emit writePCM(samples, _data_rec_sound->size(), false, AudioProcessor::AUDIO_MODE_ANALOG);
    */
    emit printText(text,true);
    emit printCallsign(callsign);
}

/// GUI only
void RadioController::audioFrameReceived()
{
    emit displayReceiveStatus(true);
    _voice_led_timer->start(500);
}

/// GUI only
void RadioController::dataFrameReceived()
{
    emit displayDataReceiveStatus(true);
    _data_led_timer->start(500);
    /*
    if((_rx_mode != gr_modem_types::ModemTypeQPSK250000)
            && (_rx_mode != gr_modem_types::ModemTypeQPSKVideo) && !_settings->voip_forwarding)
    {
        short *sound = (short*) _data_rec_sound->data();
        short *samples = new short[_data_rec_sound->size()/sizeof(short)];
        memcpy(samples, sound, _data_rec_sound->size());
        emit writePCM(samples, _data_rec_sound->size(), false, AudioProcessor::AUDIO_MODE_ANALOG);
    }
    */
}

/// GUI leds and Mumble text signal
void RadioController::receiveEnd()
{
    if(_incoming_text_buffer.size() > 0 && _settings->voip_forwarding)
    {
        emit newMumbleMessage(_incoming_text_buffer);
        _incoming_text_buffer = "";
    }
    if(_incoming_proto_buffer.size() > 0)
    {
        _layer2->processRadioMessage(_incoming_proto_buffer);
        _incoming_proto_buffer.clear();
    }
    emit displayReceiveStatus(false);
    emit displayDataReceiveStatus(false);
}

void RadioController::endAudioTransmission()
{
    if(_settings->repeater_enabled && _tx_radio_type == radio_type::RADIO_TYPE_DIGITAL)
    {
        _modem->endTransmission(_callsign);
    }
    QString time= QDateTime::currentDateTime().toString("d/MMM/yyyy hh:mm:ss");
    emit printText("<b>" + time +
                   "</b> <font color=\"#77FF77\">Transmission end</font><br/>\n",true);
    unsigned int size;
    short *origin;
    switch(_settings->end_beep)
    {
    case 0:
        return;
        break;
    default:
        origin = (short*) _end_rec_sound->data();
        size = _end_rec_sound->size();
        break;
    }
    short *samples = new short[size/sizeof(short)];
    for(unsigned int i=0;i<size/sizeof(short);i++)
    {
        samples[i] = short(origin[i] / 2);
    }
    emit writePCM(samples, size, false, AudioProcessor::AUDIO_MODE_ANALOG);
    short *silence = new short[4096/sizeof(short)];
    memset(silence, 0, 4096);
    emit writePCM(silence, 4096, false, AudioProcessor::AUDIO_MODE_ANALOG);
}

/// These two are not used currently
void RadioController::setChannels(ChannelList channels)
{
    _layer2->setChannels(channels);
}

void RadioController::setStations(StationList list)
{
    _layer2->setStations(list);
}

/// Needed to keep the frame size
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


/// Radio control functions start
///
///
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
            _logger->log(Logger::LogLevelFatal,
                         "Could not init RX device, check settings and restart");
            emit initError("Could not init RX device, check settings");
            return;
        }

        _mutex->lock();
        _modem->enableGUIFFT((bool)_settings->show_fft);
        _modem->enableGUIConst((bool)_settings->show_constellation);
        _modem->enableRSSI((bool)_settings->show_controls);
        _modem->setRxSensitivity(((double)_settings->rx_sensitivity)/100.0);
        _modem->setSquelch(_settings->squelch);
        _modem->setGain(_settings->if_gain);
        _modem->setRxCTCSS(_settings->rx_ctcss);
        _modem->setCarrierOffset(_settings->demod_offset);
        _modem->setSampRate(_settings->rx_sample_rate);
        _modem->tune(_settings->rx_frequency);
        _modem->calibrateRSSI(_settings->rssi_calibration_value);
        _modem->startRX(_settings->block_buffer_size);
        _mutex->unlock();
        const QMap<std::string,QVector<int>> rx_gains = _modem->getRxGainNames();
        /// hold a local copy of stage gains
        QMap<std::string, QVector<int>>::const_iterator iter = rx_gains.constBegin();
        while (iter != rx_gains.constEnd())
        {
            _rx_stage_gains[iter.key()] = 0;
            ++iter;
        }
        emit rxGainStages(rx_gains);

        _settings->rx_inited = true;
    }
    else if (_settings->rx_inited)
    {
        _mutex->lock();
        _modem->stopRX();
        _modem->deinitRX(_rx_mode);
        _mutex->unlock();

        _settings->rx_inited = false;
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
            if(_settings->rx_inited)
                _modem->stopRX();
            _modem->initTX(_tx_mode, _settings->tx_device_args.toStdString(),
                           _settings->tx_antenna.toStdString(), _settings->tx_freq_corr);
            _mutex->unlock();
        }
        catch(std::runtime_error &e)
        {
            if(_settings->rx_inited)
                _modem->startRX(_settings->block_buffer_size);
            _modem->deinitTX(_tx_mode);
            _mutex->unlock();
            _logger->log(Logger::LogLevelFatal,
                         "Could not init TX device, check settings and restart");
            emit initError("Could not init TX device, check settings");
            return;
        }
        if(_tx_mode == gr_modem_types::ModemTypeQPSKVideo)
            _video->init(_settings->video_device);
            //_camera->init();
        else
            _video->deinit();

        _mutex->lock();
        _modem->setBbGain(_settings->bb_gain);
        _modem->tuneTx(433000000);
        _modem->setTxCTCSS(_settings->tx_ctcss);
        _modem->startTX(_settings->block_buffer_size);
        if(_settings->rx_inited)
            _modem->startRX(_settings->block_buffer_size);
        _mutex->unlock();

        const QMap<std::string,QVector<int>> tx_gains = _modem->getTxGainNames();
        /// hold a local copy of stage gains
        QMap<std::string, QVector<int>>::const_iterator iter = tx_gains.constBegin();
        while (iter != tx_gains.constEnd())
        {
            _tx_stage_gains[iter.key()] = 0;
            ++iter;
        }
        emit txGainStages(tx_gains);
        _mutex->lock();
        _modem->setTxPower(0.01);
        _mutex->unlock();

        _settings->tx_inited = true;
    }
    else if(_settings->tx_inited)
    {
        _mutex->lock();
        _modem->stopTX();

        _modem->deinitTX(_tx_mode);
        _mutex->unlock();
        _video->deinit();

        _settings->tx_inited = false;
    }
}

void RadioController::toggleRxMode(int value)
{
    if((_settings->rx_mode == value) && _settings->rx_inited)
        return;
    if(_settings->rx_inited)
    {
        _mutex->lock();
        _modem->stopRX();
        _mutex->unlock();
    }

    bool rx_inited_before = _settings->rx_inited;
    if(rx_inited_before)
    {
        _settings->rx_inited = false;
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
        _rx_mode = gr_modem_types::ModemTypeBPSK2K;
        _step_hz = 5;
        _scan_step_hz = 12500;
        break;
    case 13:
        _rx_mode = gr_modem_types::ModemTypeBPSK1K;
        _step_hz = 5;
        _scan_step_hz = 6250;
        break;
    case 14:
        _rx_mode = gr_modem_types::ModemTypeQPSK2K;
        _step_hz = 5;
        _scan_step_hz = 6250;
        break;
    case 15:
        _rx_mode = gr_modem_types::ModemTypeQPSK20K;
        _step_hz = 50;
        _scan_step_hz = 25000;
        break;
    case 16:
        _rx_mode = gr_modem_types::ModemType2FSK2KFM;
        _step_hz = 10;
        _scan_step_hz = 12500;
        break;
    case 17:
        _rx_mode = gr_modem_types::ModemType2FSK1KFM;
        _step_hz = 10;
        _scan_step_hz = 12500;
        break;
    case 18:
        _rx_mode = gr_modem_types::ModemType2FSK2K;
        _step_hz = 10;
        _scan_step_hz = 12500;
        break;
    case 19:
        _rx_mode = gr_modem_types::ModemType2FSK1K;
        _step_hz = 10;
        _scan_step_hz = 12500;
        break;
    case 20:
        _rx_mode = gr_modem_types::ModemType2FSK20K;
        _step_hz = 500;
        _scan_step_hz = 50000;
        break;
    case 21:
        _rx_mode = gr_modem_types::ModemType4FSK2K;
        _step_hz = 5;
        _scan_step_hz = 12500;
        break;
    case 22:
        _rx_mode = gr_modem_types::ModemType4FSK2KFM;
        _step_hz = 5;
        _scan_step_hz = 12500;
        break;
    case 23:
        _rx_mode = gr_modem_types::ModemType4FSK1KFM;
        _step_hz = 5;
        _scan_step_hz = 12500;
        break;
    case 24:
        _rx_mode = gr_modem_types::ModemType4FSK20KFM;
        _step_hz = 500;
        _scan_step_hz = 50000;
        break;
    case 25:
        _rx_mode = gr_modem_types::ModemTypeQPSKVideo;
        _step_hz = 1000;
        break;
    case 26:
        _rx_mode = gr_modem_types::ModemTypeQPSK250K;
        _step_hz = 1000;
        _scan_step_hz = 500000;
        break;
    case 27:
        _rx_mode = gr_modem_types::ModemType4FSK100K;
        _step_hz = 1000;
        _scan_step_hz = 500000;
        break;
    case 28:
        _rx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _rx_mode = gr_modem_types::ModemTypeUSB2500;
        _step_hz = 10;
        _scan_step_hz = 2500;
        break;
    default:
        _rx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _rx_mode = gr_modem_types::ModemTypeUSB2500;
        _step_hz = 10;
        _scan_step_hz = 2500;
        break;
    }

    if((_rx_mode == gr_modem_types::ModemType4FSK100K) ||
        (_rx_mode == gr_modem_types::ModemTypeQPSK250K))
        _net_device->tun_init(_settings->ip_address);
    if((_rx_mode == gr_modem_types::ModemType4FSK100K))
        _net_device->set_mtu(580);
    if((_rx_mode == gr_modem_types::ModemTypeQPSK250K))
        _net_device->set_mtu(1480);
    _modem->toggleRxMode(_rx_mode);
    if(rx_inited_before)
    {
        _settings->rx_inited = true;
    }
    if(_settings->rx_inited)
    {
        _mutex->lock();
        _modem->startRX(_settings->block_buffer_size);
        _mutex->unlock();
    }
    _settings->rx_mode = value;
}

void RadioController::toggleTxMode(int value)
{
    if((_settings->tx_mode == value) && _settings->tx_inited)
        return;
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
        _tx_mode = gr_modem_types::ModemTypeBPSK2K;
        break;
    case 13:
        _tx_mode = gr_modem_types::ModemTypeBPSK1K;
        break;
    case 14:
        _tx_mode = gr_modem_types::ModemTypeQPSK2K;
        break;
    case 15:
        _tx_mode = gr_modem_types::ModemTypeQPSK20K;
        break;
    case 16:
        _tx_mode = gr_modem_types::ModemType2FSK2KFM;
        break;
    case 17:
        _tx_mode = gr_modem_types::ModemType2FSK1KFM;
        break;
    case 18:
        _tx_mode = gr_modem_types::ModemType2FSK2K;
        break;
    case 19:
        _tx_mode = gr_modem_types::ModemType2FSK1K;
        break;
    case 20:
        _tx_mode = gr_modem_types::ModemType2FSK20K;
        break;
    case 21:
        _tx_mode = gr_modem_types::ModemType4FSK2K;
        break;
    case 22:
        _tx_mode = gr_modem_types::ModemType4FSK2KFM;
        break;
    case 23:
        _tx_mode = gr_modem_types::ModemType4FSK1KFM;
        break;
    case 24:
        _tx_mode = gr_modem_types::ModemType4FSK20KFM;
        break;
    case 25:
        _tx_mode = gr_modem_types::ModemTypeQPSKVideo;
        break;
    case 26:
        _tx_mode = gr_modem_types::ModemTypeQPSK250K;
        break;
    case 27:
        _tx_mode = gr_modem_types::ModemType4FSK100K;
        break;
    case 28:
        _tx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _tx_mode = gr_modem_types::ModemTypeCW600USB;
        break;
    default:
        _tx_mode = gr_modem_types::ModemTypeBPSK2K;
        break;
    }
    if(_tx_mode == gr_modem_types::ModemTypeQPSKVideo)
        _video->init(_settings->video_device);
        //_camera->init();
    else
        _video->deinit();
    if((_tx_mode == gr_modem_types::ModemType4FSK100K) ||
        (_tx_mode == gr_modem_types::ModemTypeQPSK250K))
        _net_device->tun_init(_settings->ip_address);
    if((_tx_mode == gr_modem_types::ModemType4FSK100K))
        _net_device->set_mtu(580);
    if((_tx_mode == gr_modem_types::ModemTypeQPSK250K))
        _net_device->set_mtu(1480);
        //_camera->init();

    _mutex->lock();
    if(_settings->tx_inited)
        _modem->stopTX();
    _modem->toggleTxMode(_tx_mode);
    if(_settings->tx_inited)
        _modem->startTX(_settings->block_buffer_size);
    _mutex->unlock();
    _settings->tx_mode = value;
}

void RadioController::usePTTForVOIP(bool value)
{
    _settings->voip_ptt_enabled = value;
}

void RadioController::setVOIPForwarding(bool value)
{
    _settings->voip_forwarding = value;
}

void RadioController::setVox(bool value)
{
    _settings->vox_enabled = value;
    if(!_settings->vox_enabled)
    {
        _transmitting = false;
    }
    if(_settings->vox_enabled)
    {
        _transmitting = true;
    }
}

void RadioController::toggleRepeat(bool value)
{
    if(value && !_settings->repeater_enabled)
    {
        if(!_settings->enable_duplex)
        {
            _logger->log(Logger::LogLevelInfo, "Repeater mode can only function in duplex mode");
            return;
        }
        _settings->repeater_enabled = value;
    }
    else if(!value && _settings->repeater_enabled)
    {
        _settings->repeater_enabled = value;
    }
    /// old code: this enables direct repeat in same mode only (direct loopback)
    /**
     if(_rx_mode == _tx_mode)
        _modem->setRepeater(value);
    */
}

void RadioController::fineTuneFreq(long long center_freq)
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
    /// we don't use carrier_offset for TX, fixed sample rate and carrier offset
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

void RadioController::setRxSensitivity(int value, std::string gain_stage)
{
    if(gain_stage.size() > 0)
    {
        qDebug() << value;
        _rx_stage_gains[gain_stage] = value;
        _modem->setRxSensitivity((float)value, gain_stage);
    }
    else
    {
        _settings->rx_sensitivity = value;
        _modem->setRxSensitivity(((double)_settings->rx_sensitivity)/100.0);
    }
}

void RadioController::setTxPower(int value, std::string gain_stage)
{
    if(gain_stage.size() > 0)
    {
        _tx_stage_gains[gain_stage] = value;
        _modem->setTxPower((float)value/100.0, gain_stage);
    }
    else
    {
        _settings->tx_power = value;
        _modem->setTxPower((float)_settings->tx_power/100.0);
    }
}

void RadioController::setBbGain(int value)
{
    /// Dangerous to use
    _settings->bb_gain = value;
    _modem->setBbGain(_settings->bb_gain);
}

void RadioController::setIfGain(int value)
{
    _settings->if_gain = value;
    _modem->setGain(_settings->if_gain);
}

void RadioController::setAgcAttack(float value)
{
    _modem->setAgcAttack(value);
}

void RadioController::setAgcDecay(float value)
{
    _modem->setAgcDecay(value);
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
    _settings->audio_compressor = (int)value;
}

void RadioController::setVolume(int value)
{
    _rx_volume = 1e-3*exp(((float)value/100.0)*6.908);
}

void RadioController::setTxVolume(int value)
{
    _tx_volume = 1e-3*exp(((float)value/50.0)*6.908);
}

void RadioController::setVoipVolume(int value)
{
    _voip_volume = 1e-3*exp(((float)value/50.0)*6.908);
}

void RadioController::setVoxLevel(int value)
{
    _settings->vox_level = value;
}

void RadioController::setVoipBitrate(int value)
{
    _settings->voip_bitrate = value;
    _logger->log(Logger::LogLevelInfo,QString("Setting VOIP bitrate to %1").arg(value));
    _codec->set_voip_bitrate(value);
}

void RadioController::setEndBeep(int value)
{
    _settings->end_beep = value;
    QString filename = ":/res/end_beep1.raw";
    switch(_settings->end_beep)
    {
    case 0:
        break;
    case 1:
        filename = ":/res/end_beep1.raw";
        break;
    case 2:
        filename = ":/res/end_beep2.raw";
        break;
    case 3:
        filename = ":/res/MDC1200.raw";
        break;
    case 4:
        filename = ":/res/MODAT.raw";
        break;
    case 5:
        filename = ":/res/talk_permit.raw";
        break;
    case 6:
        filename = ":/res/MDC1200_preamble.raw";
        break;
    case 7:
        filename = ":/res/StatusChange.raw";
        break;
    case 8:
        filename = ":/res/BeepBeep.raw";
        break;
    case 9:
        filename = ":/res/Caution.raw";
        break;
    default:
        break;
    }

    QFile resfile(filename);
    if(resfile.open(QIODevice::ReadOnly))
    {
        if(_end_rec_sound != nullptr)
            delete _end_rec_sound;
        _end_rec_sound = new QByteArray(resfile.readAll());
    }
}

void RadioController::setRxCTCSS(float value)
{
    if(std::abs(_settings->rx_ctcss - value) > 0.001f)
    {
        _settings->rx_ctcss = value;
        _modem->setRxCTCSS(value);
    }
}

void RadioController::setTxCTCSS(float value)
{
    if(std::abs(_settings->tx_ctcss - value) > 0.001f)
    {
        _settings->tx_ctcss = value;
        _modem->setTxCTCSS(value);
    }
}

void RadioController::enableGUIConst(bool value)
{
    _settings->show_constellation = (int)value;
    _modem->enableGUIConst(value);
}

void RadioController::enableRSSI(bool value)
{
    _settings->show_controls = (int)value;
    _modem->enableRSSI(value);
}

void RadioController::enableGUIFFT(bool value)
{
    _settings->show_fft = (int)value;
    _modem->enableGUIFFT(value);
}

void RadioController::enableDuplex(bool value)
{
    if(_settings->tx_started)
        return;
    _settings->enable_duplex = (int)value;
}

void RadioController::enableReverseShift(bool value)
{
    Q_UNUSED(value);
    tuneFreq(_settings->rx_frequency + _settings->tx_shift);
    _settings->tx_shift = -_settings->tx_shift;
    tuneTxFreq(_settings->rx_frequency + _settings->demod_offset);
}

void RadioController::enableRelays(bool value)
{
    _settings->enable_relays = (int)value;
    if(_settings->enable_relays)
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

void RadioController::setScanResumeTime(int value)
{
    _settings->scan_resume_time = value;
}

void RadioController::setMuteForwardedAudio(bool value)
{
    _settings->mute_forwarded_audio = (int)value;
}

void RadioController::setBlockBufferSize(int value)
{
    _settings->block_buffer_size = value;
}

void RadioController::setRadioToT(int value)
{
    _settings->radio_tot = value;
}

void RadioController::setTotTxEnd(bool value)
{
    _settings->tot_tx_end = (int)value;
}

void RadioController::setTxLimits(bool value)
{
    _settings->tx_band_limits = (int)value;
}


/// Start of scan functions
///
///
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
        if(receiving)
            _scan_timer->restart();
        qint64 msec = (quint64)_scan_timer->nsecsElapsed() / 1000000;
        if(msec < _settings->scan_resume_time * 1000)
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
        if(msec < 120)
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
        _modem->tune(_settings->rx_frequency);
    }
    if(decrement_main_frequency)
    {
        _settings->rx_frequency = _settings->rx_frequency - _settings->rx_sample_rate;
        _modem->tune(_settings->rx_frequency);
    }
    _modem->setCarrierOffset(_autotune_freq);

    _settings->demod_offset = _autotune_freq;
    emit freqToGUI(_settings->rx_frequency, _settings->demod_offset);
    _scan_timer->restart();
}

void RadioController::startScan(int step, int direction)
{
    if(!_settings->rx_inited || !_scan_done || !_memory_scan_done)
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

void RadioController::startMemoryScan(int direction)
{
    _memory_channels = _radio_channels->getChannels()->toList();
    if(!_settings->rx_inited || !_scan_done || !_memory_scan_done || _memory_channels.size() < 1)
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
        if(receiving)
            _scan_timer->restart();
        qint64 msec = (quint64)_scan_timer->nsecsElapsed() / 1000000;
        if(msec < _settings->scan_resume_time * 1000)
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
        if(msec < 120) // FIXME: hardcoded
        {
            return;
        }
    }
    struct timespec time_to_sleep;
    radiochannel *chan = _memory_channels.at(_memory_scan_index);
    if(chan->skip)
    {
        _memory_scan_index++;
        if(_memory_scan_index >= _memory_channels.size())
            _memory_scan_index = 0;
        return;
    }
    _settings->rx_frequency = chan->rx_frequency - _settings->demod_offset;
    tuneFreq(_settings->rx_frequency);
    time_to_sleep = {0, 1000L }; /// Give PLL time to settle
    nanosleep(&time_to_sleep, NULL);
    toggleRxMode(chan->rx_mode);

    _settings->tx_shift = chan->tx_shift;
    tuneTxFreq(chan->rx_frequency);
    time_to_sleep = {0, 1000L };
    nanosleep(&time_to_sleep, NULL);
    toggleTxMode(chan->tx_mode);

    emit tuneToMemoryChannel(chan);

    _memory_scan_index++;
    if(_memory_scan_index >= _memory_channels.size())
        _memory_scan_index = 0;

    _scan_timer->restart();
}
