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

    /// Local to RadioController
    _dmr_control = new DMRControl(settings, logger);
    _modem = new gr_modem(settings, logger, _dmr_control);
    _codec = new AudioEncoder(settings);
    _audio_mixer_in = new AudioMixer;
    _layer2 = new Layer2Protocol(logger);
    _relay_controller = new RelayController(logger);
    _lime_rfe_controller = new LimeRFEController(_settings, _logger);
    _video = new VideoEncoder(logger);
    _net_device = new NetDevice(logger, 0, _settings->ip_address);
    _mutex = new QMutex;

    _rand_frame_data = new unsigned char[4000];
    /// one way queue from radio and local voice to Mumble
    _to_voip_buffer = new QVector<short>;
    /// pre-allocated at maximum possible FFT size (make it a constant?)
    _fft_data = new float[1048576];
    _sample_data = new float[1048576];
    _end_rec_sound = nullptr;

    _voice_led_timer = new QTimer(this);
    _voice_led_timer->setSingleShot(true);
    _data_led_timer = new QTimer(this);
    _data_led_timer->setSingleShot(true);
    _voip_tx_timer = new QTimer(this);
    _voip_tx_timer->setSingleShot(true);
    _rx_timer = new QTimer(this);
    _rx_timer->setSingleShot(true);
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
    _text_transmit_on = false;
    _proto_transmit_on = false;
    _cw_tone = false;
    _enable_rssi = true;
    _transmitting = false;
    _receiving = false;
    _process_text = false;
    _process_data = false;
    _repeat_text = false;

    _rx_volume = 1e-3*exp(((float)_settings->rx_volume/100.0)*6.908);
    _tx_volume = 1e-3*exp(((float)_settings->tx_volume/50.0)*6.908);
    _voip_volume = 1e-3*exp(((float)_settings->voip_volume/50.0)*6.908);
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
    QObject::connect(this, SIGNAL(startReceiveTimer(int)), _rx_timer, SLOT(start(int)));
    QObject::connect(_rx_timer, SIGNAL(timeout()), this, SLOT(callbackStopReceive()));
    QObject::connect(_end_tx_timer, SIGNAL(timeout()), this, SLOT(endTx()));
    QObject::connect(_radio_time_out_timer, SIGNAL(timeout()), this, SLOT(radioTimeout()));

    /// Modem connections
    QObject::connect(_modem,SIGNAL(protoReceived(QByteArray)),this,
                     SLOT(protoReceived(QByteArray)));
    QObject::connect(_modem,SIGNAL(textReceived(QString, bool)),this,SLOT(textReceived(QString, bool)));
    QObject::connect(_modem,SIGNAL(callsignReceived(QString)),this,
                     SLOT(callsignReceived(QString)));
    QObject::connect(_modem,SIGNAL(m17FrameInfoReceived(QString, QString, uint16_t)),this,
                     SLOT(m17FrameInfoReceived(QString, QString, uint16_t)));
    QObject::connect(_modem,SIGNAL(audioFrameReceived()),this,SLOT(audioFrameReceived()));
    QObject::connect(_modem,SIGNAL(dataFrameReceived()),this,SLOT(dataFrameReceived()));
    QObject::connect(_modem,SIGNAL(receiveEnd()),this,SLOT(receiveEnd()));
    QObject::connect(_modem,SIGNAL(endAudioTransmission()),this,SLOT(endAudioTransmission()));
    QObject::connect(_modem,SIGNAL(endBeep()),this,SLOT(endBeep()));
    QObject::connect(this,SIGNAL(audioData(unsigned char*,int)),_modem,
                     SLOT(transmitDigitalAudio(unsigned char*,int)));
    QObject::connect(this,SIGNAL(m17AudioData(unsigned char*,int)),_modem,
                     SLOT(transmitM17Audio(unsigned char*,int)));
    QObject::connect(this,SIGNAL(dmrAudioData(unsigned char*,int)),_modem,
                     SLOT(transmitDMR(unsigned char*,int)));
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

    QObject::connect(_layer2,SIGNAL(havePageMessage(QString,QString,QString)),this,
                     SLOT(receivedPageMessage(QString,QString,QString)));

    QObject::connect(_dmr_control,SIGNAL(digitalAudio(unsigned char*,int)),this,
                     SLOT(receiveDigitalAudio(unsigned char*,int)));
    QObject::connect(_dmr_control,SIGNAL(talkerAlias(QString, bool)),this,
                     SLOT(textReceived(QString, bool)));
    QObject::connect(_dmr_control,SIGNAL(gpsInfo(QString, bool)),this,
                     SLOT(textReceived(QString, bool)));
    QObject::connect(_dmr_control,SIGNAL(headerReceived(QString, bool)),this,
                     SLOT(textReceived(QString, bool)));
    QObject::connect(_dmr_control,SIGNAL(terminatorReceived(QString, bool)),this,
                     SLOT(textReceived(QString, bool)));
    QObject::connect(_dmr_control,SIGNAL(endBeep()),this,
                     SLOT(endBeep()));


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
    delete _rx_timer;
    delete _vox_timer;
    delete _end_tx_timer;
    delete _cw_timer;
    delete _modem;
    delete _dmr_control;
    delete _mutex;
    delete[] _rand_frame_data;
    delete[] _fft_data;
    delete[] _sample_data;
    _to_voip_buffer->clear();
    delete _to_voip_buffer;
    delete _relay_controller;
    delete _lime_rfe_controller;
    delete _end_rec_sound;
    delete _data_rec_sound;
    delete _timeout_sound;
    delete _data_read_timer;
    delete _data_modem_reset_timer;
    delete _data_modem_sleep_timer;
    delete _scan_timer;
    delete _const_read_timer;
    delete _rssi_read_timer;
    delete _fft_read_timer;
}

void RadioController::stop()
{
    if(_settings->rx_inited)
        toggleRX(false);
    if(_settings->tx_inited)
        toggleTX(false);
    _logger->log(Logger::LogLevelInfo, QString("Stopping radio controller thread"));
    emit terminateConnections();
    struct timespec time_to_sleep = {0, 200000000L };
    nanosleep(&time_to_sleep, NULL);
    _stop_thread=true;
}

void RadioController::run()
{
    /// Radioop thread where most things happen
    bool ptt_activated = false;
    bool data_to_process = false;
    bool buffers_filling = false;
    bool receiving_on = false;
    int last_channel_broadcast_time = 0;
    while(!_stop_thread)
    {
        _mutex->lock();
        bool transmitting = _transmitting;
        bool receiving = _receiving;
        bool voip_forwarding = _settings->voip_forwarding;
        bool data_modem_sleeping = _data_modem_sleeping;
        _mutex->unlock();

        QCoreApplication::processEvents(); // process signals
        if(_settings->voip_connected || _settings->udp_enabled)
            QtConcurrent::run(this, &RadioController::flushRadioToVoipBuffer);

        if(voip_forwarding && !transmitting && !ptt_activated &&
                (_tx_radio_type == radio_type::RADIO_TYPE_DIGITAL))
        {
            int time = QDateTime::currentDateTime().toTime_t();
            if((time - last_channel_broadcast_time) > 30)
            {
                last_channel_broadcast_time = time;

                    // FIXME: poke repeater to VOIP logic here
                    //transmitServerInfoBeacon();
            }
        }

        updateDataModemReset(transmitting, ptt_activated); // for IP modem latency buildup

        if(receiving && !receiving_on)
        {
            receiving_on = true;
            QtConcurrent::run(this, &RadioController::callbackOnReceive);
        }
        if(!receiving && receiving_on)
        {
            receiving_on = false;
        }

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

        _mutex->lock();
        if(_text_transmit_on)
            _process_text = false;
        if(_proto_transmit_on)
            _process_data = false;
        _mutex->unlock();

        /// Get all available data from the demodulator
        if((_rx_mode != gr_modem_types::ModemTypeMMDVM) && (_rx_mode != gr_modem_types::ModemTypeMMDVMmulti)
                && !_settings->headless_mode)
        {
            QtConcurrent::run(this, &RadioController::getFFTData);
            QtConcurrent::run(this, &RadioController::getSampleData);
            QtConcurrent::run(this, &RadioController::getConstellationData);
            QtConcurrent::run(this, &RadioController::getRSSI);

        }
        if(transmitting)
        {
            if(_tx_mode == gr_modem_types::ModemTypeCW600USB)
                updateCWK();
                //QtConcurrent::run(this, &RadioController::updateCWK);

            /// if not in the process of resetting the modem read interface
            /// data will accumulate in the interface buffer...
            if(!data_modem_sleeping)
                processInputNetStream();

        }

        if((_rx_mode != gr_modem_types::ModemTypeMMDVM) && (_rx_mode != gr_modem_types::ModemTypeMMDVMmulti))
        {
            data_to_process = getDemodulatorData();
            if(data_to_process)
            {
                _receiving = true;
                _rx_timer->start(200);
            }
        }

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
            || (_settings->udp_enabled)
            || (_settings->repeater_enabled)
            || (!_transmitting && !_settings->vox_enabled)
            || (_tx_mode == gr_modem_types::ModemTypeQPSK250K)
            || (_tx_mode == gr_modem_types::ModemTypeBPSK8)
            || (_tx_mode == gr_modem_types::ModemType4FSK100K)
            || (_tx_mode == gr_modem_types::ModemTypeCW600USB)
            || (_text_transmit_on || _proto_transmit_on))
    {
        emit setAudioReadMode(false, false, AudioProcessor::AUDIO_MODE_ANALOG, 640);
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
            (_tx_mode == gr_modem_types::ModemTypeGMSK1K) ||
            (_tx_mode == gr_modem_types::ModemTypeGMSK2K) ||
            (_tx_mode == gr_modem_types::ModemType4FSK1KFM) ||
            (_tx_mode == gr_modem_types::ModemTypeDMR))
    {
        audio_mode = AudioProcessor::AUDIO_MODE_CODEC2;
        emit setAudioReadMode(true, (bool)_settings->audio_compressor, audio_mode, 640);
    }
    else if((_tx_mode == gr_modem_types::ModemTypeM17))
    {
        audio_mode = AudioProcessor::AUDIO_MODE_OPUS;
        emit setAudioReadMode(true, (bool)_settings->audio_compressor, audio_mode, 640);
    }
    else if((_tx_mode == gr_modem_types::ModemTypeQPSK20K) ||
            (_tx_mode == gr_modem_types::ModemType2FSK10KFM) ||
            (_tx_mode == gr_modem_types::ModemTypeGMSK10K) ||
            (_tx_mode == gr_modem_types::ModemType4FSK10KFM))
    {
        audio_mode = AudioProcessor::AUDIO_MODE_OPUS;
        emit setAudioReadMode(true, (bool)_settings->audio_compressor, audio_mode, 640);
    }
    else if(_tx_mode == gr_modem_types::ModemTypeQPSKVideo)
    {
        audio_mode = AudioProcessor::AUDIO_MODE_OPUS;
        emit setAudioReadMode(true, (bool)_settings->audio_compressor, audio_mode, 1600);
    }
    else
    {
        audio_mode = AudioProcessor::AUDIO_MODE_ANALOG;
        emit setAudioReadMode(true, (bool)_settings->audio_compressor, audio_mode, 640);
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
    if((!_settings->voip_connected && !_settings->udp_enabled) || _radio_to_voip_on)
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
        if(_settings->voip_connected)
        {
            int packet_size = 0;
            unsigned char *encoded_audio;
            /// encode the PCM with higher quality and bitrate
            encoded_audio = _codec->encode_opus_voip(pcm, 320*sizeof(short), packet_size);
            emit voipDataOpus(encoded_audio,packet_size);
        }
        if(_settings->udp_enabled)
        {
            emit udpAudioSamples(pcm, 320);
        }
        else
        {
            delete[] pcm;
        }
        _to_voip_buffer->remove(0,320);
    }
    _radio_to_voip_on = false;
}

bool RadioController::processMixerQueue()
{
    int maximum_frame_size = _settings->udp_enabled ? 1600 : 960;
    if(_audio_mixer_in->buffers_available(maximum_frame_size))
    {
        int tx_timer_value = (_settings->voip_forwarding ? 200 : (_settings->udp_enabled ? 300 : 250));
        if(_settings->voip_forwarding || _settings->repeater_enabled || _settings->udp_enabled)
        {
            if(!_voip_tx_timer->isActive())
            {
                _mutex->lock();
                _transmitting = true;
                _mutex->unlock();
                _voip_tx_timer->start(tx_timer_value);
                return true;
            }
        }
        short *pcm = _audio_mixer_in->mix_samples(_tx_volume, maximum_frame_size);
        if(pcm == nullptr)
            return false;
        short *local_pcm = new short[320];
        memcpy(local_pcm, pcm, 320*sizeof(short));

        if(_settings->voip_forwarding || _settings->repeater_enabled || _settings->udp_enabled)
        {
            _voip_tx_timer->start(tx_timer_value);
            /// Out to radio and don't loop back to Mumble
            txAudio(pcm, 320*sizeof(short), 1, true);
        }
        else
        {
            /// nothing towards radio or VOIP
            delete[] pcm;
        }
        if((_settings->voip_forwarding || _settings->repeater_enabled || _settings->udp_enabled) &&
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
            {
                _mutex->lock();
                _transmitting = true;
                _mutex->unlock();
            }
        }
        if(!vad && !_vox_timer->isActive())
        {
            /// Vox timer ran out, stopping TX
            if(_settings->tx_started && !_settings->voip_ptt_enabled)
            {
                _mutex->lock();
                _transmitting = false;
                _mutex->unlock();
            }
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
            (_tx_mode == gr_modem_types::ModemTypeGMSK2K) ||
            (_tx_mode == gr_modem_types::ModemType4FSK2K) ||
            (_tx_mode == gr_modem_types::ModemType4FSK2KFM) ||
            (_tx_mode == gr_modem_types::ModemTypeQPSK2K))
    {
        encoded_audio = _codec->encode_codec2_1400(audiobuffer, audiobuffer_size, packet_size);
    }
    else if((_tx_mode == gr_modem_types::ModemTypeBPSK1K) ||
            (_tx_mode == gr_modem_types::ModemType2FSK1KFM) ||
            (_tx_mode == gr_modem_types::ModemType2FSK1K) ||
            (_tx_mode == gr_modem_types::ModemTypeGMSK1K) ||
            (_tx_mode == gr_modem_types::ModemType4FSK1KFM))
    {
        encoded_audio = _codec->encode_codec2_700(audiobuffer, audiobuffer_size, packet_size);
    }
    else if((_tx_mode == gr_modem_types::ModemTypeM17))
    {
        short frame1[160];
        short frame2[160];
        memcpy(frame1, audiobuffer, audiobuffer_size/2);
        memcpy(frame2, audiobuffer + 160, audiobuffer_size/2);
        unsigned char *encoded_audio_frame1;
        unsigned char *encoded_audio_frame2;

        encoded_audio_frame1 = _codec->encode_codec2_3200(frame1, audiobuffer_size/2, packet_size);
        encoded_audio_frame2 = _codec->encode_codec2_3200(frame2, audiobuffer_size/2, packet_size);
        encoded_audio = new unsigned char[packet_size * 2];
        memcpy(encoded_audio, encoded_audio_frame1, packet_size);
        memcpy(encoded_audio + packet_size, encoded_audio_frame2, packet_size);
        delete[] encoded_audio_frame1;
        delete[] encoded_audio_frame2;
        emit m17AudioData(encoded_audio, packet_size * 2);
    }
    else if((_tx_mode == gr_modem_types::ModemTypeDMR))
    {
        short frame1[160];
        short frame2[160];
        memcpy(frame1, audiobuffer, audiobuffer_size/2);
        memcpy(frame2, audiobuffer + 160, audiobuffer_size/2);
        unsigned char *encoded_audio_frame1;
        unsigned char *encoded_audio_frame2;

        encoded_audio_frame1 = _codec->encode_dmr(frame1, audiobuffer_size/2, packet_size);
        encoded_audio_frame2 = _codec->encode_dmr(frame2, audiobuffer_size/2, packet_size);
        emit dmrAudioData(encoded_audio_frame1, packet_size);
        emit dmrAudioData(encoded_audio_frame2, packet_size);
    }
    else
    {
        encoded_audio = _codec->encode_opus(audiobuffer, audiobuffer_size, packet_size);
    }

    delete[] audiobuffer;

    if(_tx_mode == gr_modem_types::ModemTypeQPSKVideo)
    {
        processVideoFrame(encoded_audio, packet_size);
    }
    else if((_tx_mode != gr_modem_types::ModemTypeM17)
            && (_tx_mode != gr_modem_types::ModemTypeDMR))
    {
        emit audioData(encoded_audio,packet_size);
    }

}


void RadioController::processVideoFrame(unsigned char *audio_buffer, int audio_size)
{
    if((_tx_mode != gr_modem_types::ModemTypeQPSKVideo) || (!_settings->tx_started))
    {
        delete[] audio_buffer;
        return;
    }

    unsigned int max_video_frame_size = 3122;
    unsigned long encoded_size;
    //audio_size = 118; // Audio frames are 100 msec Opus encoded

    /// Large alloc
    unsigned char *videobuffer = new unsigned char[230400];
    memset(videobuffer, 0, 230400*sizeof(unsigned char));

    memcpy(&(videobuffer[24]), audio_buffer, audio_size*sizeof(unsigned char));
    delete[] audio_buffer;


    /// This includes V4L2 capture time as well
    _video->encode_jpeg(&(videobuffer[24+audio_size]), encoded_size, max_video_frame_size - 24 - audio_size);
    if(encoded_size < 1)
    {
        delete[] videobuffer;
        return;
    }
    if(encoded_size > max_video_frame_size - 24 - audio_size)
    {
        encoded_size = max_video_frame_size - 24 - audio_size;
    }
    u_int32_t real_size = (u_int32_t) encoded_size;
    if(_settings->voip_ptt_enabled && _transmitting && (real_size > 0))
    {
        unsigned char *video_frame = new unsigned char[real_size];
        memcpy(video_frame, &(videobuffer[24+audio_size]), real_size*sizeof(unsigned char));
        emit voipVideoData(video_frame, real_size);
        delete[] videobuffer;
        return;
    }

    /// Out to radio
    ///
    u_int32_t crc = (u_int32_t)crc32(&(videobuffer[24+audio_size]), (size_t)real_size);

    memcpy(&(videobuffer[0]), &real_size, 4*sizeof(unsigned char));
    memcpy(&(videobuffer[4]), &real_size, 4*sizeof(unsigned char));
    memcpy(&(videobuffer[8]), &real_size, 4*sizeof(unsigned char));
    memcpy(&(videobuffer[12]), &crc, 4*sizeof(unsigned char));
    memcpy(&(videobuffer[16]), &crc, 4*sizeof(unsigned char));
    memcpy(&(videobuffer[20]), &crc, 4*sizeof(unsigned char));
    /// Rest of video frame filled with garbage to keep the same radio frame size
    for(unsigned int k=real_size+24+audio_size,i=0;k<max_video_frame_size;k++,i++)
    {
        videobuffer[k] = _rand_frame_data[i];
    }

    emit videoData(videobuffer,max_video_frame_size);
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
        time_per_frame = 48000000;
    }
    if(_tx_mode == gr_modem_types::ModemType4FSK100K)
    {
        max_frame_size = 622;
        read_size = 606;
        time_per_frame = 50000000; // ???
    }

    microsec = (quint64)_data_read_timer->nsecsElapsed();
    if(microsec < 40000000)
    {
        return;
    }

    time_left = time_per_frame - microsec;
    struct timespec time_to_sleep = {0, (long)time_left };

    if(time_left > 0)
        nanosleep(&time_to_sleep, NULL);
    _data_read_timer->restart();

    unsigned char *netbuffer = new unsigned char[max_frame_size];
    int nread;
    unsigned char *buffer = _net_device->read_buffered(nread, read_size);

    if(nread > 0)
    {
        u_int32_t crc = (u_int32_t)crc32(buffer, nread);
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
        _mutex->lock();
        _process_text = false;
        _mutex->unlock();
        return;
    }
    if(_text_transmit_on)
        return;

    _mutex->lock();
    _text_transmit_on = true;
    _transmitting = true;
    _mutex->unlock();
    /// callsign frame
    // FIXME: this doesn't seem to work, callsign is actually sent
    // after the first text frame most of the time
    struct timespec time_to_sleep = {0, 39800000L };
    nanosleep(&time_to_sleep, NULL);

    start_text_tx:
    int frame_size;
    if((_tx_mode == gr_modem_types::ModemTypeBPSK2K) ||
            (_tx_mode == gr_modem_types::ModemTypeBPSK8) ||
            (_tx_mode == gr_modem_types::ModemType2FSK2KFM) ||
            (_tx_mode == gr_modem_types::ModemType2FSK2K) ||
            (_tx_mode == gr_modem_types::ModemTypeGMSK2K) ||
            (_tx_mode == gr_modem_types::ModemType4FSK2K) ||
            (_tx_mode == gr_modem_types::ModemType4FSK2KFM) ||
            (_tx_mode == gr_modem_types::ModemTypeQPSK2K))
        frame_size = 7;
    else if((_tx_mode == gr_modem_types::ModemTypeBPSK1K) ||
            (_tx_mode == gr_modem_types::ModemType2FSK1KFM) ||
            (_tx_mode == gr_modem_types::ModemType2FSK1K) ||
            (_tx_mode == gr_modem_types::ModemTypeGMSK1K) ||
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
    _mutex->lock();
    _transmitting = false;
    _text_transmit_on = false;
    _mutex->unlock();
}

void RadioController::transmitBinData()
{
    if(!_settings->tx_inited)
    {
        _mutex->lock();
        _process_data = false;
        _mutex->unlock();
        return;
    }
    if(_proto_transmit_on)
        return;

    _mutex->lock();
    _proto_transmit_on = true;
    _transmitting = true;
    _mutex->unlock();
    /// callsign frame
    // FIXME: this doesn't seem to work, callsign is actually sent
    // after the first text frame most of the time
    struct timespec time_to_sleep = {0, 39800000L };
    nanosleep(&time_to_sleep, NULL);

    start_data_tx:
    int frame_size;
    if((_tx_mode == gr_modem_types::ModemTypeBPSK2K) ||
            (_tx_mode == gr_modem_types::ModemTypeBPSK8) ||
            (_tx_mode == gr_modem_types::ModemType2FSK2KFM) ||
            (_tx_mode == gr_modem_types::ModemType2FSK2K) ||
            (_tx_mode == gr_modem_types::ModemTypeGMSK2K) ||
            (_tx_mode == gr_modem_types::ModemType4FSK2K) ||
            (_tx_mode == gr_modem_types::ModemType4FSK2KFM) ||
            (_tx_mode == gr_modem_types::ModemTypeQPSK2K))
        frame_size = 7;
    else if((_tx_mode == gr_modem_types::ModemTypeBPSK1K) ||
            (_tx_mode == gr_modem_types::ModemType2FSK1KFM) ||
            (_tx_mode == gr_modem_types::ModemType2FSK1K) ||
            (_tx_mode == gr_modem_types::ModemTypeGMSK1K) ||
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
    _mutex->lock();
    _transmitting = false;
    _proto_transmit_on = false;
    _mutex->unlock();
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
                    exit(EXIT_FAILURE);
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
        if(_settings->enable_lime_rfe)
        {
            _lime_rfe_controller->setTXBand(_tx_frequency + _settings->tx_shift);
            _lime_rfe_controller->setTransmit(true);
        }
        _modem->tuneTx(_tx_frequency + _settings->tx_shift);
        _modem->setTxPower((float)_settings->tx_power/100);

        _settings->tx_started = true;
        if(_tx_radio_type == radio_type::RADIO_TYPE_DIGITAL)
        {
            _modem->startTransmission(_callsign);
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
            if(_tx_mode == gr_modem_types::ModemTypeDMR)
            {
                for(int i=0;i<3*6;i++)
                {
                    // FIXME: generate real silence frames
                    unsigned char *silence_frame = new unsigned char[9];
                    memset(silence_frame, 0, 9U);
                    emit dmrAudioData(silence_frame, 9);
                }
                tx_tail_msec = 900;
            }
             else if((_tx_mode == gr_modem_types::ModemTypeBPSK2K) ||
                    (_tx_mode == gr_modem_types::ModemTypeBPSK8) ||
                    (_tx_mode == gr_modem_types::ModemType2FSK2KFM) ||
                    (_tx_mode == gr_modem_types::ModemType2FSK2K) ||
                    (_tx_mode == gr_modem_types::ModemTypeGMSK2K) ||
                    (_tx_mode == gr_modem_types::ModemType4FSK2K) ||
                    (_tx_mode == gr_modem_types::ModemType4FSK2KFM) ||
                    (_tx_mode == gr_modem_types::ModemTypeQPSK2K) ||
                    (_tx_mode == gr_modem_types::ModemTypeM17))
                tx_tail_msec = 800;
            else
                tx_tail_msec = 500;
        }
        if((_tx_radio_type == radio_type::RADIO_TYPE_ANALOG)
                && ((_tx_mode == gr_modem_types::ModemTypeNBFM2500) ||
                    (_tx_mode == gr_modem_types::ModemTypeNBFM5000)))
        {
            if(_settings->end_beep > 0)
            {
                sendTxBeep(_settings->end_beep);
                tx_tail_msec = 800;
            }
            else
            {
                tx_tail_msec = 200;
            }
        }
        // FIXME: end tail length should be calculated exactly
        _end_tx_timer->start(tx_tail_msec);
        _radio_time_out_timer->stop();
    }
}

void RadioController::endTx()
{
    _modem->setTxPower(0.01);
    _modem->flushSources();
    /// On the LimeSDR mini, whenever I call setTxPower I get a brief spike of the LO
    setRelays(false);
    if(_settings->enable_lime_rfe)
    {
        _lime_rfe_controller->setTransmit(false);
    }
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
    if(_tx_mode == gr_modem_types::ModemTypeMMDVM || _tx_mode == gr_modem_types::ModemTypeMMDVMmulti)
        return;
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
    if(!_settings->voip_forwarding && !_settings->udp_enabled)
    {
        emit writePCM(samples, size, false, AudioProcessor::AUDIO_MODE_ANALOG);
    }
    if(_settings->tot_tx_end)
    {
        _mutex->lock();
        _transmitting = false;
        _mutex->unlock();
    }
}

void RadioController::stopVoipTx()
{
    /// Called by voip tx timer
    _mutex->lock();
    _transmitting = false;
    _mutex->unlock();
}

void RadioController::callbackStopReceive()
{
    _receiving = false;
    if(_settings->udp_enabled && !_settings->sql_pty_path.isEmpty()) /// used only for SVXlink
    {
        int fd = open(_settings->sql_pty_path.toStdString().c_str(), O_RDWR | O_NONBLOCK | O_SYNC);
        if(fd < 0)
        {
            _logger->log(Logger::LogLevelWarning, QString("Could not find SVXlink squelch PTY file descriptor: %1").arg(_settings->sql_pty_path));
            return;
        }
        write(fd, "Z", 1);
        close(fd);
    }
}

void RadioController::callbackOnReceive()
{
    if(_settings->udp_enabled && !_settings->sql_pty_path.isEmpty())
    {
        int fd = open(_settings->sql_pty_path.toStdString().c_str(), O_RDWR | O_NONBLOCK | O_SYNC);
        if(fd < 0)
        {
            _logger->log(Logger::LogLevelWarning, QString("Could not find SVXlink squelch PTY file descriptor: %1").arg(_settings->sql_pty_path));
            return;
        }
        write(fd, "O", 1);
        close(fd);
    }
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
        {
            data_to_process = _modem->demodulate();
        }
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
    if((msec < _fft_poll_time) || !_enable_rssi)
    {
        return;
    }
    float rssi = _modem->getRSSI();
    _rssi_read_timer->restart();
    _settings->rssi = rssi;

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

void RadioController::setSampleWindow(uint size)
{
    _modem->setSampleWindow(size);
}

void RadioController::getSampleData()
{
    if(!_settings->show_time_domain)
    {
        return;
    }
    unsigned int size = 0;
    _modem->getSampleData(_sample_data, size);
    if(size > 0)
    {
        emit newSampleData(_sample_data, (int)size);
    }
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
    if(const_data == nullptr)
        return;
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
            (_rx_mode == gr_modem_types::ModemTypeGMSK2K) ||
            (_rx_mode == gr_modem_types::ModemType4FSK2K) ||
            (_rx_mode == gr_modem_types::ModemType4FSK2KFM) ||
            (_rx_mode == gr_modem_types::ModemTypeQPSK2K))
    {
        audio_out = _codec->decode_codec2_1400(data, size, samples);
    }
    else if((_rx_mode == gr_modem_types::ModemTypeBPSK1K) ||
            (_rx_mode == gr_modem_types::ModemType2FSK1KFM) ||
            (_rx_mode == gr_modem_types::ModemType2FSK1K) ||
            (_rx_mode == gr_modem_types::ModemTypeGMSK1K) ||
            (_rx_mode == gr_modem_types::ModemType4FSK1KFM))
    {
        audio_out = _codec->decode_codec2_700(data, size, samples);
    }
    else if((_rx_mode == gr_modem_types::ModemTypeDMR))
    {
        unsigned char frame1[size/3];
        unsigned char frame2[size/3];
        unsigned char frame3[size/3];
        memcpy(frame1, data, size/3);
        memcpy(frame2, data + size/3, size/3);
        memcpy(frame3, data + 2*size/3, size/3);
        short *audio_frame1 = _codec->decode_dmr(frame1, size/3, samples);
        short *audio_frame2 = _codec->decode_dmr(frame2, size/3, samples);
        short *audio_frame3 = _codec->decode_dmr(frame3, size/3, samples);
        audio_out = new short[samples * 3];
        memcpy(audio_out, audio_frame1, samples * sizeof(short));
        memcpy(audio_out + samples, audio_frame2, samples * sizeof(short));
        memcpy(audio_out + 2 * samples, audio_frame3, samples * sizeof(short));
        delete[] audio_frame1;
        delete[] audio_frame2;
        delete[] audio_frame3;
        samples = samples * 3;
    }
    else if((_rx_mode == gr_modem_types::ModemTypeM17))
    {
        unsigned char frame1[size/2];
        unsigned char frame2[size/2];
        memcpy(frame1, data, size/2);
        memcpy(frame2, data + size/2, size/2);
        short *audio_frame1 = _codec->decode_codec2_3200(frame1, size/2, samples);
        short *audio_frame2 = _codec->decode_codec2_3200(frame2, size/2, samples);
        audio_out = new short[samples * 2];
        memcpy(audio_out, audio_frame1, samples * sizeof(short));
        memcpy(audio_out + samples, audio_frame2, samples * sizeof(short));
        delete[] audio_frame1;
        delete[] audio_frame2;
        samples = samples * 2;
    }
    else if(_rx_mode == gr_modem_types::ModemTypeQPSKVideo)
    {
        audio_out = _codec->decode_opus(data, size, samples, 800);
    }
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
                (_rx_mode == gr_modem_types::ModemTypeGMSK2K) ||
                (_rx_mode == gr_modem_types::ModemTypeGMSK1K) ||
                (_rx_mode == gr_modem_types::ModemType4FSK2K) ||
                (_rx_mode == gr_modem_types::ModemType4FSK2KFM) ||
                (_rx_mode == gr_modem_types::ModemTypeQPSK2K) ||
                (_rx_mode == gr_modem_types::ModemTypeBPSK1K) ||
                (_rx_mode == gr_modem_types::ModemType2FSK1KFM) ||
                (_rx_mode == gr_modem_types::ModemType4FSK1KFM) ||
                (_rx_mode == gr_modem_types::ModemType2FSK1K) ||
                (_rx_mode == gr_modem_types::ModemTypeDMR))
        {
            audio_mode = AudioProcessor::AUDIO_MODE_CODEC2;
        }
        float scaling_factor = 1.0f;
        if(_rx_mode == gr_modem_types::ModemTypeDMR)
        {
            scaling_factor = 1e-3*exp(1.2f*6.908);
        }
        for(int i=0;i<samples;i++)
        {
            audio_out[i] = (short)((float)audio_out[i] * _rx_volume * scaling_factor);
            if(_settings->voip_forwarding || _settings->udp_enabled)
            {
                /// routing to Mumble or UDP connection
                _to_voip_buffer->push_back(audio_out[i]);
            }
        }
        if(_settings->voip_forwarding && _settings->mute_forwarded_audio
                && !_settings->repeater_enabled)
        {
            /// no local audio
            delete[] audio_out;
        }
        else if(_settings->udp_enabled && _settings->mute_forwarded_audio
                && !_settings->repeater_enabled)
        {
            /// no local audio
            delete[] audio_out;
        }
        else
        {
            if(_settings->voip_connected || _settings->repeater_enabled || _settings->udp_enabled)
            {
                /// need to mix several audio channels
                _audio_mixer_in->addSamples(audio_out, samples, 9900); // radio id hardcoded
            }
            else
            {
                emit writePCM(audio_out,samples*sizeof(short),
                              (bool)_settings->audio_compressor, audio_mode);
            }
        }
        dataFrameReceived();
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
        if(_settings->voip_forwarding || _settings->udp_enabled)
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
    else if(_settings->udp_enabled && !_settings->repeater_enabled
            && _settings->mute_forwarded_audio)
    {
        /// No local audio
        delete[] pcm;
    }
    else
    {
        if(_settings->voip_connected || _settings->repeater_enabled || _settings->udp_enabled)
        {
            /// Need to mix several audio channels
            _audio_mixer_in->addSamples(pcm, size, 9900); // radio id hardcoded
            processMixerQueue();
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
u_int32_t RadioController::getFrameLength(unsigned char *data)
{
    u_int32_t frame_size1 = 0;
    u_int32_t frame_size2 = 0;
    u_int32_t frame_size3 = 0;
    /// do we really need this redundancy?
    memcpy(&frame_size1, &data[0], 4*sizeof(unsigned char));
    memcpy(&frame_size2, &data[4], 4*sizeof(unsigned char));
    memcpy(&frame_size3, &data[8], 4*sizeof(unsigned char));
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
u_int32_t RadioController::getFrameCRC32(unsigned char *data)
{
    u_int32_t crc1 = 0;
    u_int32_t crc2 = 0;
    u_int32_t crc3 = 0;

    memcpy(&crc1, &data[12], 4*sizeof(unsigned char));
    memcpy(&crc2, &data[16], 4*sizeof(unsigned char));
    memcpy(&crc3, &data[20], 4*sizeof(unsigned char));
    if(crc1 == crc2)
        return crc1;
    else if(crc1 == crc3)
        return crc1;
    else if(crc2 == crc3)
        return crc2;
    else
        return 0;
}

/// callback from gr_modem via signal
void RadioController::receiveVideoData(unsigned char *data, int size)
{
    Q_UNUSED(size);
    u_int32_t frame_size = getFrameLength(data);
    unsigned int audio_size = 118;

    if(frame_size <= 0)
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
    unsigned char *audio_frame = new unsigned char[audio_size];
    memcpy(audio_frame, &(data[24]), audio_size*sizeof(unsigned char));
    receiveDigitalAudio(audio_frame, audio_size);
    memcpy(jpeg_frame, &(data[24+audio_size]), frame_size*sizeof(unsigned char));
    u_int32_t crc = getFrameCRC32(data);
    delete[] data;
    u_int32_t crc_check = (u_int32_t) crc32(jpeg_frame, (size_t)frame_size);
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
    u_int32_t frame_size = getFrameLength(data);

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
    memcpy(net_frame, &(data[16]), frame_size);
    u_int32_t crc;
    memcpy(&crc, &data[12], 4);
    delete[] data;
    u_int32_t crc_check = (u_int32_t)crc32(net_frame, (size_t)frame_size);

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
    dataFrameReceived();
}

/// signal from Mumble or UDP client
void RadioController::processVoipAudioFrame(short *pcm, int samples, quint64 sid)
{
    _audio_mixer_in->addSamples(pcm, samples, sid);
    processMixerQueue();
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
    if(_settings->rx_inited && _settings->rx_sample_rate >= 2000000 &&
            (_settings->rx_device_args == _settings->tx_device_args))
    {
        _logger->log(Logger::LogLevelWarning,
            "Transmitting at a sample rate different than 1 Msps. "
            "This feature is still experimental and will use more CPU.");
    }

    if(_settings->tx_inited || _settings->voip_ptt_enabled)
    {
        _mutex->lock();
        _transmitting = true;
        _mutex->unlock();
    }
}

void RadioController::endTransmission()
{
    _mutex->lock();
    _transmitting = false;
    _mutex->unlock();
}

void RadioController::textData(QString text, bool repeat)
{
    _repeat_text = repeat;
    _text_out = text;
    _process_text = true;
}

void RadioController::pageUser(QString user, QString message)
{
    _proto_out = _layer2->buildPageMessage(_callsign, user, message, false, _callsign);
    _process_data = true;
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
void RadioController::textReceived(QString text, bool html)
{
    if(_settings->repeater_enabled && _tx_radio_type == radio_type::RADIO_TYPE_DIGITAL
            && _rx_mode != gr_modem_types::ModemTypeDMR)
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
    emit printText(text, html);
}

void RadioController::receivedPageMessage(QString calling_user,
                                          QString called_user, QString page_message)
{
    _logger->log(Logger::LogLevelDebug, QString("Paging message from %1 to %2, text: %3").arg(
                     calling_user).arg(called_user).arg(page_message));

    if(QString::compare(called_user, _callsign.trimmed(), Qt::CaseInsensitive)  == 0)
    {
        emit newPageMessage(calling_user, page_message);
        QString filename = ":/res/BeepBeep.raw";
        QFile resfile(filename);
        resfile.open(QIODevice::ReadOnly);
        QByteArray *sound = new QByteArray(resfile.readAll());

        short* origin = (short*) sound->data();
        int size = sound->size();

        short *samples = new short[size/sizeof(short)];
        for(unsigned int i=0;i<size/sizeof(short);i++)
        {
            samples[i] = short(origin[i] / 2);
        }
        emit writePCM(samples, size, false, AudioProcessor::AUDIO_MODE_ANALOG);
        short *silence = new short[4096/sizeof(short)];
        memset(silence, 0, 4096);
        emit writePCM(silence, 4096, false, AudioProcessor::AUDIO_MODE_ANALOG);
        delete sound;
    }
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
    emit printText(text,true);
    emit printCallsign(callsign);
}

void RadioController::m17FrameInfoReceived(QString src, QString dest, uint16_t CAN)
{
    QString time= QDateTime::currentDateTime().toString("dd/MMM/yyyy hh:mm:ss");
    QString text = "\n\n<br/><b>" + time + "</b>  <font color=\"#00d000\">CAN </font><font color=\"#FFFFFF\"><b>"
             + QString::number(CAN) + "</b></font> "
             + "<font color=\"#FF5555\">" + src + "</font> > "
             + "<font color=\"#55AAFF\">" + dest + " </font><br/>\n";
    emit printText(text,true);
    emit printCallsign(src);
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
    _data_led_timer->start(100);
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
    endBeep();
}

void RadioController::endBeep()
{
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
                           _settings->rx_antenna.toStdString(), _settings->rx_freq_corr,
                           _settings->mmdvm_channels, _settings->mmdvm_channel_separation);
            _mutex->unlock();
        }
        catch(std::runtime_error &e)
        {
            _modem->deinitRX(_rx_mode);
            _mutex->unlock();
            _logger->log(Logger::LogLevelFatal, e.what());
            _logger->log(Logger::LogLevelFatal,
                         "Could not init RX device, check settings and restart");
            emit initError("Could not init RX device, check settings", 0);
            return;
        }

        _mutex->lock();
        _modem->enableGUIFFT((bool)_settings->show_fft);
        _modem->enableGUIConst((bool)_settings->show_constellation);
        _modem->enableRSSI(true);
        _modem->setRxSensitivity(((double)_settings->rx_sensitivity)/100.0);
        _modem->setSquelch(_settings->squelch);
        _modem->setGain(_settings->if_gain);
        _modem->setRxCTCSS(_settings->rx_ctcss);
        _modem->setAgcAttack(_settings->agc_attack);
        _modem->setAgcDecay(_settings->agc_decay);

        if((_rx_mode == gr_modem_types::ModemTypeMMDVM) || (_rx_mode == gr_modem_types::ModemTypeMMDVMmulti))
        {
            _modem->setSampRate(MMDVM_SAMPLE_RATE);
        }
        else
        {
            _modem->setSampRate(_settings->rx_sample_rate);
        }
        _modem->setCarrierOffset(_settings->demod_offset);
        _modem->tune(_settings->rx_frequency);
        _modem->calibrateRSSI(_settings->rssi_calibration_value);
        _modem->enableTimeDomain((bool)_settings->show_time_domain);
        _modem->setTimeDomainSampleRate(_settings->time_domain_sample_rate);
        _modem->setSampleWindow(_settings->time_domain_sample_speed);
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
            _modem->initTX(_tx_mode, _tx_frequency + _settings->tx_shift,
                           _settings->tx_device_args.toStdString(),
                           _settings->tx_antenna.toStdString(), _settings->tx_freq_corr, _settings->tx_power,
                           _settings->mmdvm_channels, _settings->mmdvm_channel_separation);
            _mutex->unlock();
        }
        catch(std::runtime_error &e)
        {
            _modem->deinitTX(_tx_mode);
            if(_settings->rx_inited)
                _modem->startRX(_settings->block_buffer_size);
            _mutex->unlock();
            _logger->log(Logger::LogLevelFatal, e.what());
            _logger->log(Logger::LogLevelFatal,
                         "Could not init TX device, check settings and restart");
            emit initError("Could not init TX device, check settings", 1);
            return;
        }

        _mutex->lock();
        _modem->setBbGain(_settings->bb_gain);
        if((_tx_mode == gr_modem_types::ModemTypeMMDVM) || (_tx_mode == gr_modem_types::ModemTypeMMDVMmulti))
        {
            _modem->setSampRate(MMDVM_SAMPLE_RATE);
        }
        else
        {
            _modem->setSampRate(_settings->rx_sample_rate);
        }
        _modem->setTxCarrierOffset(_settings->tx_carrier_offset);
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
        if((_tx_mode != gr_modem_types::ModemTypeMMDVM) && (_tx_mode != gr_modem_types::ModemTypeMMDVMmulti))
        {
            _mutex->lock();
            _modem->setTxPower(0.01);
            _mutex->unlock();
        }

        _settings->tx_inited = true;
    }
    else if(_settings->tx_inited)
    {
        _mutex->lock();
        _modem->stopTX();

        _modem->deinitTX(_tx_mode);
        _mutex->unlock();

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
    case OperationMode::ModeFM:
        _rx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _rx_mode = gr_modem_types::ModemTypeNBFM5000;
        _step_hz = 10;
        _scan_step_hz = 12500;
        break;
    case OperationMode::ModeNBFM:
        _rx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _rx_mode = gr_modem_types::ModemTypeNBFM2500;
        _step_hz = 10;
        _scan_step_hz = 6250;
        break;
    case OperationMode::ModeWBFM:
        _rx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _rx_mode = gr_modem_types::ModemTypeWBFM;
        _step_hz = 1000;
        _scan_step_hz = 200000;
        break;
    case OperationMode::ModeUSB:
        _rx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _rx_mode = gr_modem_types::ModemTypeUSB2500;
        _step_hz = 10;
        _scan_step_hz = 2500;
        break;
    case OperationMode::ModeLSB:
        _rx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _rx_mode = gr_modem_types::ModemTypeLSB2500;
        _step_hz = 10;
        _scan_step_hz = 2500;
        break;
    case OperationMode::ModeFreeDV1600USB:
        _rx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _rx_mode = gr_modem_types::ModemTypeFREEDV1600USB;
        _step_hz = 5;
        _scan_step_hz = 2500;
        break;
    case OperationMode::ModeFreeDV700CUSB:
        _rx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _rx_mode = gr_modem_types::ModemTypeFREEDV700CUSB;
        _step_hz = 5;
        _scan_step_hz = 2500;
        break;
    case OperationMode::ModeFreeDV700DUSB:
        _rx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _rx_mode = gr_modem_types::ModemTypeFREEDV700DUSB;
        _step_hz = 5;
        _scan_step_hz = 2500;
        break;
    case OperationMode::ModeFreeDV800XAUSB:
        _rx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _rx_mode = gr_modem_types::ModemTypeFREEDV800XAUSB;
        _step_hz = 5;
        _scan_step_hz = 2500;
        break;
    case OperationMode::ModeFreeDV1600LSB:
        _rx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _rx_mode = gr_modem_types::ModemTypeFREEDV1600LSB;
        _step_hz = 5;
        _scan_step_hz = 2500;
        break;
    case OperationMode::ModeFreeDV700CLSB:
        _rx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _rx_mode = gr_modem_types::ModemTypeFREEDV700CLSB;
        _step_hz = 5;
        _scan_step_hz = 2500;
        break;
    case OperationMode::ModeFreeDV700DLSB:
        _rx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _rx_mode = gr_modem_types::ModemTypeFREEDV700DLSB;
        _step_hz = 5;
        _scan_step_hz = 2500;
        break;
    case OperationMode::ModeFreeDV800XALSB:
        _rx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _rx_mode = gr_modem_types::ModemTypeFREEDV800XALSB;
        _step_hz = 5;
        _scan_step_hz = 2500;
        break;
    case OperationMode::ModeAM:
        _rx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _rx_mode = gr_modem_types::ModemTypeAM5000;
        _step_hz = 10;
        _scan_step_hz = 10000;
        break;
    case OperationMode::ModeBPSK2K:
        _rx_mode = gr_modem_types::ModemTypeBPSK2K;
        _step_hz = 5;
        _scan_step_hz = 12500;
        break;
    case OperationMode::ModeBPSK1K:
        _rx_mode = gr_modem_types::ModemTypeBPSK1K;
        _step_hz = 5;
        _scan_step_hz = 6250;
        break;
    case OperationMode::ModeQPSK2K:
        _rx_mode = gr_modem_types::ModemTypeQPSK2K;
        _step_hz = 5;
        _scan_step_hz = 6250;
        break;
    case OperationMode::ModeQPSK10K:
        _rx_mode = gr_modem_types::ModemTypeQPSK20K;
        _step_hz = 10;
        _scan_step_hz = 25000;
        break;
    case OperationMode::Mode2FSK2KFM:
        _rx_mode = gr_modem_types::ModemType2FSK2KFM;
        _step_hz = 10;
        _scan_step_hz = 12500;
        break;
    case OperationMode::Mode2FSK1KFM:
        _rx_mode = gr_modem_types::ModemType2FSK1KFM;
        _step_hz = 10;
        _scan_step_hz = 12500;
        break;
    case OperationMode::Mode2FSK2K:
        _rx_mode = gr_modem_types::ModemType2FSK2K;
        _step_hz = 10;
        _scan_step_hz = 12500;
        break;
    case OperationMode::Mode2FSK1K:
        _rx_mode = gr_modem_types::ModemType2FSK1K;
        _step_hz = 10;
        _scan_step_hz = 12500;
        break;
    case OperationMode::Mode2FSK10KFM:
        _rx_mode = gr_modem_types::ModemType2FSK10KFM;
        _step_hz = 10;
        _scan_step_hz = 50000;
        break;
    case OperationMode::ModeGMSK2K:
        _rx_mode = gr_modem_types::ModemTypeGMSK2K;
        _step_hz = 10;
        _scan_step_hz = 12500;
        break;
    case OperationMode::ModeGMSK1K:
        _rx_mode = gr_modem_types::ModemTypeGMSK1K;
        _step_hz = 10;
        _scan_step_hz = 12500;
        break;
    case OperationMode::ModeGMSK10K:
        _rx_mode = gr_modem_types::ModemTypeGMSK10K;
        _step_hz = 10;
        _scan_step_hz = 50000;
        break;
    case OperationMode::Mode4FSK2K:
        _rx_mode = gr_modem_types::ModemType4FSK2K;
        _step_hz = 5;
        _scan_step_hz = 12500;
        break;
    case OperationMode::Mode4FSK2KFM:
        _rx_mode = gr_modem_types::ModemType4FSK2KFM;
        _step_hz = 5;
        _scan_step_hz = 12500;
        break;
    case OperationMode::Mode4FSK1KFM:
        _rx_mode = gr_modem_types::ModemType4FSK1KFM;
        _step_hz = 5;
        _scan_step_hz = 12500;
        break;
    case OperationMode::Mode4FSK10KFM:
        _rx_mode = gr_modem_types::ModemType4FSK10KFM;
        _step_hz = 10;
        _scan_step_hz = 50000;
        break;
    case OperationMode::ModeQPSKVideo:
        _rx_mode = gr_modem_types::ModemTypeQPSKVideo;
        _step_hz = 500;
        break;
    case OperationMode::ModeQPSK250K:
        _rx_mode = gr_modem_types::ModemTypeQPSK250K;
        _step_hz = 500;
        _scan_step_hz = 500000;
        break;
    case OperationMode::Mode4FSK100K:
        _rx_mode = gr_modem_types::ModemType4FSK100K;
        _step_hz = 200;
        _scan_step_hz = 500000;
        break;
    case OperationMode::ModeMMDVM:
        _rx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _rx_mode = gr_modem_types::ModemTypeMMDVM;
        _step_hz = 10;
        _scan_step_hz = 12500;
        break;
    case OperationMode::ModeMMDVMMulti:
        _rx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _rx_mode = gr_modem_types::ModemTypeMMDVMmulti;
        _step_hz = 10;
        _scan_step_hz = 12500;
        break;
    case OperationMode::ModeM17:
        _rx_radio_type = radio_type::RADIO_TYPE_DIGITAL;
        _rx_mode = gr_modem_types::ModemTypeM17;
        _step_hz = 10;
        _scan_step_hz = 12500;
        break;
    case OperationMode::ModeDMR:
        _rx_radio_type = radio_type::RADIO_TYPE_DIGITAL;
        _rx_mode = gr_modem_types::ModemTypeDMR;
        _step_hz = 10;
        _scan_step_hz = 12500;
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
    if(_rx_mode == gr_modem_types::ModemType4FSK100K)
        _net_device->set_mtu(580);
    if(_rx_mode == gr_modem_types::ModemTypeQPSK250K)
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
    if(_settings->tx_inited && _transmitting)
    {
        _logger->log(Logger::LogLevelWarning, "Cannot change mode while transmitting");
        return;
    }

    _tx_radio_type = radio_type::RADIO_TYPE_DIGITAL;
    switch(value)
    {
    case OperationMode::ModeFM:
        _tx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _tx_mode = gr_modem_types::ModemTypeNBFM5000;
        break;
    case OperationMode::ModeNBFM:
        _tx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _tx_mode = gr_modem_types::ModemTypeNBFM2500;
        break;
    case OperationMode::ModeWBFM:
        _tx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _tx_mode = gr_modem_types::ModemTypeNBFM5000; // safeguard for WBFM
        break;
    case OperationMode::ModeUSB:
        _tx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _tx_mode = gr_modem_types::ModemTypeUSB2500;
        break;
    case OperationMode::ModeLSB:
        _tx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _tx_mode = gr_modem_types::ModemTypeLSB2500;
        break;
    case OperationMode::ModeFreeDV1600USB:
        _tx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _tx_mode = gr_modem_types::ModemTypeFREEDV1600USB;
        break;
    case OperationMode::ModeFreeDV700CUSB:
        _tx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _tx_mode = gr_modem_types::ModemTypeFREEDV700CUSB;
        break;
    case OperationMode::ModeFreeDV700DUSB:
        _tx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _tx_mode = gr_modem_types::ModemTypeFREEDV700DUSB;
        break;
    case OperationMode::ModeFreeDV800XAUSB:
        _tx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _tx_mode = gr_modem_types::ModemTypeFREEDV800XAUSB;
        break;
    case OperationMode::ModeFreeDV1600LSB:
        _tx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _tx_mode = gr_modem_types::ModemTypeFREEDV1600LSB;
        break;
    case OperationMode::ModeFreeDV700CLSB:
        _tx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _tx_mode = gr_modem_types::ModemTypeFREEDV700CLSB;
        break;
    case OperationMode::ModeFreeDV700DLSB:
        _tx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _tx_mode = gr_modem_types::ModemTypeFREEDV700DLSB;
        break;
    case OperationMode::ModeFreeDV800XALSB:
        _tx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _tx_mode = gr_modem_types::ModemTypeFREEDV800XALSB;
        break;
    case OperationMode::ModeAM:
        _tx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _tx_mode = gr_modem_types::ModemTypeAM5000;
        break;
    case OperationMode::ModeBPSK2K:
        _tx_mode = gr_modem_types::ModemTypeBPSK2K;
        break;
    case OperationMode::ModeBPSK1K:
        _tx_mode = gr_modem_types::ModemTypeBPSK1K;
        break;
    case OperationMode::ModeQPSK2K:
        _tx_mode = gr_modem_types::ModemTypeQPSK2K;
        break;
    case OperationMode::ModeQPSK10K:
        _tx_mode = gr_modem_types::ModemTypeQPSK20K;
        break;
    case OperationMode::Mode2FSK2KFM:
        _tx_mode = gr_modem_types::ModemType2FSK2KFM;
        break;
    case OperationMode::Mode2FSK1KFM:
        _tx_mode = gr_modem_types::ModemType2FSK1KFM;
        break;
    case OperationMode::Mode2FSK2K:
        _tx_mode = gr_modem_types::ModemType2FSK2K;
        break;
    case OperationMode::Mode2FSK1K:
        _tx_mode = gr_modem_types::ModemType2FSK1K;
        break;
    case OperationMode::Mode2FSK10KFM:
        _tx_mode = gr_modem_types::ModemType2FSK10KFM;
        break;
    case OperationMode::ModeGMSK2K:
        _tx_mode = gr_modem_types::ModemTypeGMSK2K;
        break;
    case OperationMode::ModeGMSK1K:
        _tx_mode = gr_modem_types::ModemTypeGMSK1K;
        break;
    case OperationMode::ModeGMSK10K:
        _tx_mode = gr_modem_types::ModemTypeGMSK10K;
        break;
    case OperationMode::Mode4FSK2K:
        _tx_mode = gr_modem_types::ModemType4FSK2K;
        break;
    case OperationMode::Mode4FSK2KFM:
        _tx_mode = gr_modem_types::ModemType4FSK2KFM;
        break;
    case OperationMode::Mode4FSK1KFM:
        _tx_mode = gr_modem_types::ModemType4FSK1KFM;
        break;
    case OperationMode::Mode4FSK10KFM:
        _tx_mode = gr_modem_types::ModemType4FSK10KFM;
        break;
    case OperationMode::ModeQPSKVideo:
        _tx_mode = gr_modem_types::ModemTypeQPSKVideo;
        break;
    case OperationMode::ModeQPSK250K:
        _tx_mode = gr_modem_types::ModemTypeQPSK250K;
        break;
    case OperationMode::Mode4FSK100K:
        _tx_mode = gr_modem_types::ModemType4FSK100K;
        break;
    case OperationMode::ModeMMDVM:
        _tx_mode = gr_modem_types::ModemTypeMMDVM;
        break;
    case OperationMode::ModeMMDVMMulti:
        _tx_mode = gr_modem_types::ModemTypeMMDVMmulti;
        break;
    case OperationMode::ModeM17:
        _tx_mode = gr_modem_types::ModemTypeM17;
        break;
    case OperationMode::ModeDMR:
        _tx_mode = gr_modem_types::ModemTypeDMR;
        break;
    default:
        _tx_mode = gr_modem_types::ModemTypeBPSK2K;
        break;
    }
    if(_tx_mode == gr_modem_types::ModemTypeQPSKVideo)
        _video->init(_settings->video_device);
    else
        _video->deinit();
    if((_tx_mode == gr_modem_types::ModemType4FSK100K) ||
        (_tx_mode == gr_modem_types::ModemTypeQPSK250K))
        _net_device->tun_init(_settings->ip_address);
    if(_tx_mode == gr_modem_types::ModemType4FSK100K)
        _net_device->set_mtu(580);
    if(_tx_mode == gr_modem_types::ModemTypeQPSK250K)
        _net_device->set_mtu(1480);

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

void RadioController::setUDPAudio(bool value)
{
    _settings->udp_enabled = value;
    emit enableUDPAudio(value);
}

void RadioController::setVox(bool value)
{
    _settings->vox_enabled = value;
    _mutex->lock();
    if(!_settings->vox_enabled)
    {
        _transmitting = false;
    }
    if(_settings->vox_enabled)
    {
        _transmitting = true;
    }
    _mutex->unlock();
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
}

void RadioController::fineTuneFreq(qint64 center_freq)
{
    int64_t clarifier_offset = center_freq*_step_hz;
    _modem->setCarrierOffset(_settings->demod_offset + center_freq*_step_hz);
    emit clarifierFreqToGUI((int)clarifier_offset);
}

void RadioController::tuneFreq(qint64 center_freq)
{
    /// rx_frequency is the source center frequency
    _settings->rx_frequency = center_freq;
    if(_settings->enable_lime_rfe)
        _lime_rfe_controller->setRXBand(_settings->rx_frequency + _settings->demod_offset);
    _modem->tune(_settings->rx_frequency);
}

void RadioController::tuneTxFreq(qint64 actual_freq)
{
    _tx_frequency = actual_freq;
    /// LimeSDR mini tune requests are blocking
    if(_settings->enable_lime_rfe)
        _lime_rfe_controller->setTXBand(_tx_frequency + _settings->tx_shift);
    _modem->tuneTx(_tx_frequency + _settings->tx_shift);

}

void RadioController::setCarrierOffset(qint64 offset)
{
    _settings->demod_offset = offset;
    _modem->setCarrierOffset(offset);
    if(_settings->enable_lime_rfe)
        _lime_rfe_controller->setRXBand(_settings->rx_frequency + _settings->demod_offset);
}

void RadioController::setTxCarrierOffset(qint64 offset)
{
    _settings->tx_carrier_offset = offset;
    /// we don't use carrier_offset for normal TX operation, fixed sample rate and carrier offset
    _modem->setTxCarrierOffset(offset);
}

void RadioController::resetTxCarrierOffset()
{
    _settings->tx_carrier_offset = _modem->resetTxCarrierOffset();
}

void RadioController::changeTxShift(qint64 shift_freq)
{
    _settings->tx_shift = shift_freq;
    if(_settings->enable_lime_rfe)
        _lime_rfe_controller->setTXBand(_tx_frequency + _settings->tx_shift);
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
        _modem->setTxPower((float)value, gain_stage);
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

void RadioController::setLimeRFEAttenuation(int value)
{
    _settings->lime_rfe_attenuation = value;
    _lime_rfe_controller->setAttenuator(value);
}

void RadioController::setLimeRFENotch(bool value)
{
    _settings->lime_rfe_notch = (int)value;
    _lime_rfe_controller->setNotchFilter(value);
}

void RadioController::setIfGain(int value)
{
    _settings->if_gain = value;
    _modem->setGain(_settings->if_gain);
}

void RadioController::setAgcAttack(int value)
{
    _settings->agc_attack = value;
    _modem->setAgcAttack(value);
}

void RadioController::setAgcDecay(int value)
{
    _settings->agc_decay = value;
    _modem->setAgcDecay(value);
}

void RadioController::setRxSampleRate(int samp_rate)
{
    _settings->rx_sample_rate = samp_rate;
    _mutex->lock();
    _modem->setSampRate(samp_rate);
    _mutex->unlock();
}

void RadioController::setTimeDomainSampleRate(int samp_rate)
{
    _settings->time_domain_sample_rate = samp_rate;
    _mutex->lock();
    _modem->setTimeDomainSampleRate(samp_rate);
    _mutex->unlock();
}

void RadioController::setTimeDomainFilterWidth(int filter_width)
{
    _settings->time_domain_filter_width = filter_width;
    _mutex->lock();
    _modem->setTimeDomainFilterWidth((double)filter_width);
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
    _settings->rx_volume = value;
    _rx_volume = 1e-3*exp(((float)value/100.0)*6.908);
}

void RadioController::setTxVolume(int value)
{
    _settings->tx_volume = value;
    _tx_volume = 1e-3*exp(((float)value/50.0)*6.908);
}

void RadioController::setVoipVolume(int value)
{
    _settings->voip_volume = value;
    _voip_volume = 1e-3*exp(((float)_settings->voip_volume/50.0)*6.908);
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

void RadioController::setUDPAudioSampleRate(int value)
{
    _settings->udp_audio_sample_rate = value;
    _logger->log(Logger::LogLevelInfo,QString("Setting UDP audio sample rate to %1").arg(value));
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
    _modem->enableRSSI(value);
    _enable_rssi = value;
}

void RadioController::enableGUIFFT(bool value)
{
    _settings->show_fft = (int)value;
    _modem->enableGUIFFT(value);
}

void RadioController::enableTimeDomain(bool value)
{
    _settings->show_time_domain = (int)value;
    _modem->enableTimeDomain(value);
}

void RadioController::enableDuplex(bool value)
{
    if(_settings->tx_started)
        return;
    _settings->enable_duplex = (int)value;
    if(_settings->enable_lime_rfe)
        _lime_rfe_controller->setDuplex(value);
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

void RadioController::enableLimeRFE(bool value)
{
    _settings->enable_lime_rfe = (int)value;
    if(_settings->enable_lime_rfe)
    {
        _lime_rfe_controller->init();
        _lime_rfe_controller->setRXBand(_settings->rx_frequency + _settings->demod_offset);
        _lime_rfe_controller->setTXBand(_tx_frequency + _settings->tx_shift);
        _lime_rfe_controller->setDuplex((bool)_settings->enable_duplex);
    }
    else
    {
        _lime_rfe_controller->deinit();
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
        _scan_timer->restart();
    }
    if(_scan_stop)
    {

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
        _scan_timer->restart();
    }
    if(_scan_stop)
    {
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

    radiochannel *chan = _memory_channels.at(_memory_scan_index);
    if(chan->skip)
    {
        _memory_scan_index++;
        if(_memory_scan_index >= _memory_channels.size())
            _memory_scan_index = 0;
        return;
    }
    tuneMemoryChannel(chan);

    _memory_scan_index++;
    if(_memory_scan_index >= _memory_channels.size())
        _memory_scan_index = 0;

    _scan_timer->restart();
}

void RadioController::tuneMemoryChannel(radiochannel *chan)
{
    if(_settings->tx_inited && _transmitting)
    {
        _logger->log(Logger::LogLevelWarning, "Cannot change memory channel while transmitting");
        return;
    }
    _settings->rx_frequency = chan->rx_frequency - _settings->demod_offset;
    tuneFreq(_settings->rx_frequency);
    struct timespec time_to_sleep;
    time_to_sleep = {0, 1000L }; /// Give PLL time to settle
    nanosleep(&time_to_sleep, NULL);
    toggleRxMode(chan->rx_mode);

    _settings->tx_shift = chan->tx_shift;
    tuneTxFreq(chan->rx_frequency);
    time_to_sleep = {0, 1000L };
    nanosleep(&time_to_sleep, NULL);
    toggleTxMode(chan->tx_mode);

    setSquelch(chan->squelch);
    setVolume(chan->rx_volume);
    setTxPower(chan->tx_power);
    setRxSensitivity(chan->rx_sensitivity);
    setRxCTCSS(chan->rx_ctcss);
    setTxCTCSS(chan->tx_ctcss);

    emit tuneToMemoryChannel(chan);
}
