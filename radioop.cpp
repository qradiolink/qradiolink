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

RadioOp::RadioOp(Settings *settings, gr::qtgui::sink_c::sptr fft_gui, gr::qtgui::const_sink_c::sptr const_gui,
                 gr::qtgui::number_sink::sptr rssi_gui, QObject *parent) :
    QObject(parent)
{
    _rx_mode = gr_modem_types::ModemTypeBPSK2000;
    _tx_mode = gr_modem_types::ModemTypeBPSK2000;
    _rx_radio_type = radio_type::RADIO_TYPE_DIGITAL;
    _tx_radio_type = radio_type::RADIO_TYPE_DIGITAL;
    _codec = new AudioEncoder;
    _audio = new AudioInterface;
    _mutex = new QMutex;
    _m_queue = new std::vector<short>;
    _last_session_id = 0;
    _net_device = 0;
    _video = 0;
    _stop =false;
    _tx_inited = false;
    _rx_inited = false;
    _voip_enabled = false;
    _voip_forwarding = false;
    _last_voiced_frame_timer.start();
    _voip_tx_timer = new QTimer(this);
    _voip_tx_timer->setSingleShot(true);
    _settings = settings;
    _transmitting = false;
    _process_text = false;
    _repeat_text = false;
    _settings = settings;
    _tx_power = 0;
    _rx_sensitivity = 0;
    _rx_volume = 1.0;
    _squelch = 0;
    _rx_ctcss = 0.0;
    _tx_ctcss = 0.0;
    _tune_center_freq = 0;
    _tune_shift_freq = 0;
    _tune_limit_lower = -5000;
    _tune_limit_upper = 5000;
    _step_hz = 1;
    _tuning_done = true;
    _tune_counter = 0;
    _tx_modem_started = false;
    _voice_led_timer = new QTimer(this);
    _data_led_timer = new QTimer(this);
    _rand_frame_data = new unsigned char[5000];
    _fft_gui = fft_gui;
    QObject::connect(_voice_led_timer, SIGNAL(timeout()), this, SLOT(receiveEnd()));
    QObject::connect(_data_led_timer, SIGNAL(timeout()), this, SLOT(receiveEnd()));
    QObject::connect(_voip_tx_timer, SIGNAL(timeout()), this, SLOT(stopTx()));
    _modem = new gr_modem(_settings, fft_gui,const_gui, rssi_gui);

    QObject::connect(_modem,SIGNAL(textReceived(QString)),this,SLOT(textReceived(QString)));
    QObject::connect(_modem,SIGNAL(callsignReceived(QString)),this,SLOT(callsignReceived(QString)));
    QObject::connect(_modem,SIGNAL(audioFrameReceived()),this,SLOT(audioFrameReceived()));
    QObject::connect(_modem,SIGNAL(dataFrameReceived()),this,SLOT(dataFrameReceived()));
    QObject::connect(_modem,SIGNAL(receiveEnd()),this,SLOT(receiveEnd()));
    QObject::connect(_modem,SIGNAL(endAudioTransmission()),this,SLOT(endAudioTransmission()));
    QObject::connect(this,SIGNAL(audioData(unsigned char*,int)),_modem,SLOT(processAudioData(unsigned char*,int)));
    QObject::connect(this,SIGNAL(pcmData(float*,int)),_modem,SLOT(processPCMAudio(float*,int)));
    QObject::connect(this,SIGNAL(videoData(unsigned char*,int)),_modem,SLOT(processVideoData(unsigned char*,int)));
    QObject::connect(this,SIGNAL(netData(unsigned char*,int)),_modem,SLOT(processNetData(unsigned char*,int)));
    QObject::connect(_modem,SIGNAL(digitalAudio(unsigned char*,int)),this,SLOT(receiveAudioData(unsigned char*,int)));
    QObject::connect(_modem,SIGNAL(pcmAudio(std::vector<float>*)),this,SLOT(receivePCMAudio(std::vector<float>*)));
    QObject::connect(_modem,SIGNAL(videoData(unsigned char*,int)),this,SLOT(receiveVideoData(unsigned char*,int)));
    QObject::connect(_modem,SIGNAL(netData(unsigned char*,int)),this,SLOT(receiveNetData(unsigned char*,int)));
    for (int j = 0;j<5000;j++)
        _rand_frame_data[j] = rand() % 256;

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
    delete _voice_led_timer;
    delete _data_led_timer;
    delete _modem;
    delete[] _rand_frame_data;
}

void RadioOp::stop()
{
    _stop=true;
}

void RadioOp::readConfig(std::string &rx_device_args, std::string &tx_device_args,
                         std::string &rx_antenna, std::string &tx_antenna, int &rx_freq_corr,
                         int &tx_freq_corr, std::string &callsign, std::string &video_device)
{
    int tx_power, rx_sensitivity, squelch, rx_volume;
    long long rx_frequency, tx_shift;
    QDir files = QDir::homePath();

    QFileInfo config_file = files.filePath(".config/qradiolink.cfg");
    libconfig::Config cfg;
    try
    {
        cfg.readFile(config_file.absoluteFilePath().toStdString().c_str());
    }
    catch(const libconfig::FileIOException &fioex)
    {
        std::cerr << "I/O error while reading configuration file." << std::endl;
        exit(EXIT_FAILURE);
    }
    catch(const libconfig::ParseException &pex)
    {
        std::cerr << "Configuration parse error at " << pex.getFile() << ":" << pex.getLine()
                  << " - " << pex.getError() << std::endl;
        exit(EXIT_FAILURE);
    }
    try
    {
        const libconfig::Setting& root = cfg.getRoot();
        root.lookupValue("rx_device_args", rx_device_args);
        root.lookupValue("tx_device_args", tx_device_args);
        root.lookupValue("rx_antenna", rx_antenna);
        root.lookupValue("tx_antenna", tx_antenna);
        root.lookupValue("rx_freq_corr", rx_freq_corr);
        root.lookupValue("tx_freq_corr", tx_freq_corr);
        root.lookupValue("callsign", callsign);
        root.lookupValue("video_device", video_device);
        root.lookupValue("tx_power", tx_power);
        root.lookupValue("rx_sensitivity", rx_sensitivity);
        root.lookupValue("squelch", squelch);
        root.lookupValue("rx_volume", rx_volume);
        root.lookupValue("rx_frequency", rx_frequency);
        root.lookupValue("tx_shift", tx_shift);
        _callsign = QString::fromStdString(callsign);
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
        if(_rx_sensitivity == 0)
        {
            _rx_sensitivity = (float)rx_sensitivity/100.0;
        }
        if(_tune_center_freq == 0)
        {
            _tune_center_freq = rx_frequency;
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
            _rx_volume = (float)rx_volume / 10.0;
        }
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        std::cerr << "Settings not found in configuration file." << std::endl;
        exit(EXIT_FAILURE);
    }
    catch(const libconfig::SettingTypeException &tex)
    {
        std::cerr << "Configuration error." << std::endl;
        exit(EXIT_FAILURE);
    }
}

void RadioOp::vox(short *audiobuffer, int audiobuffer_size)
{
    double treshhold = -136;
    double hyst = 0.5;
    bool treshhold_set = false;
    bool hyst_active = false;
    int hyst_counter = 0;
    float sum = 0;
    short max = 0;

    for(int j=0;j< audiobuffer_size;j++)
    {
        sum += ((audiobuffer[j]/32768.0f)*(audiobuffer[j]/32768.0f));
        max = (max > abs(audiobuffer[j])) ? max : abs(audiobuffer[j]);

    }

    float rms = sqrt(sum/audiobuffer_size);
    double power = 20*log10(rms/32768.0f);

    qDebug() << power;
    if((power < treshhold+hyst))
    {
        delete[] audiobuffer;
        return;
    }
}

void RadioOp::processAudioStream()
{
    int audiobuffer_size = 640; //40 ms @ 8k
    short *audiobuffer = new short[audiobuffer_size/sizeof(short)];
    _audio->read_short(audiobuffer,audiobuffer_size);
    if(_voip_enabled)
    {
        emit voipData(audiobuffer,audiobuffer_size);
        return;
    }
    txAudio(audiobuffer, audiobuffer_size);
}

void RadioOp::txAudio(short *audiobuffer, int audiobuffer_size)
{
    if(_tx_radio_type == radio_type::RADIO_TYPE_ANALOG)
    {
        float *pcm = new float[audiobuffer_size/sizeof(short)];

        for(int i=0;i<audiobuffer_size/sizeof(short);i++)
        {
            pcm[i] = (float)audiobuffer[i] / 32767.0f;
        }

        emit pcmData(pcm, audiobuffer_size/sizeof(short));
        delete[] audiobuffer;
        return;
    }

    int packet_size = 0;
    unsigned char *encoded_audio;
    if((_tx_mode == gr_modem_types::ModemTypeBPSK2000) ||
            (_tx_mode == gr_modem_types::ModemType2FSK2000) ||
            (_tx_mode == gr_modem_types::ModemType4FSK2000) ||
            (_tx_mode == gr_modem_types::ModemTypeQPSK2000))
        encoded_audio = _codec->encode_codec2(audiobuffer, audiobuffer_size, packet_size);
    else if(_tx_mode == gr_modem_types::ModemTypeBPSK1000)
        encoded_audio = _codec->encode_codec2_700(audiobuffer, audiobuffer_size, packet_size);
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
        usleep((100000 - microsec) - 2000);
    }

    //qDebug() << "video out " << microsec << " / " << encoded_size;

    if(encoded_size > max_video_frame_size)
    {
        encoded_size = max_video_frame_size;
    }
    memcpy(&(videobuffer[0]), &encoded_size, 4);
    memcpy(&(videobuffer[4]), &encoded_size, 4);
    memcpy(&(videobuffer[8]), &encoded_size, 4);
    for(int k=encoded_size+12,i=0;k<max_video_frame_size;k++,i++)
    {

        videobuffer[k] = _rand_frame_data[i];

    }

    emit videoData(videobuffer,max_video_frame_size);
    return 1;
}

void RadioOp::processNetStream()
{
    int max_frame_size = 1512;
    unsigned char *netbuffer = (unsigned char*)calloc(max_frame_size, sizeof(unsigned char));
    int nread;
    unsigned char *buffer = _net_device->read_buffered(nread);

    if(nread > 0)
    {
        memcpy(&(netbuffer[0]), &nread, 4);
        memcpy(&(netbuffer[4]), &nread, 4);
        memcpy(&(netbuffer[8]), &nread, 4);
        memcpy(&(netbuffer[12]), buffer, nread);
        for(int k=nread+12,i=0;k<max_frame_size;k++,i++)
        {
            netbuffer[k] = _rand_frame_data[i];
        }

        emit netData(netbuffer,max_frame_size);
        delete[] buffer;
    }
    else
    {
        delete[] netbuffer;
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
        float *pcm = new float[data->size()/sizeof(short)];

        for(int i=0;i<data->size()/sizeof(short);i++)
        {
            pcm[i] = (float)samples[i] / 32767.0f;
        }

        emit pcmData(pcm, data->size()/sizeof(short));
        delete data;
    }
}

void RadioOp::startTx()
{
    if(_tx_inited)
    {
        if(_rx_inited && (_tx_mode != gr_modem_types::ModemTypeQPSK250000))
            _modem->stopRX();
        if(_tx_modem_started)
            _modem->stopTX();
        _modem->startTX();
        usleep(100000);
        _modem->tuneTx(_tune_center_freq + _tune_shift_freq);
        _tx_modem_started = false;
        if(_tx_radio_type == radio_type::RADIO_TYPE_DIGITAL)
            _modem->startTransmission(_callsign,_callsign.size());
    }
}

void RadioOp::stopTx()
{
    if(_tx_inited)
    {
        if(_tx_radio_type == radio_type::RADIO_TYPE_DIGITAL)
            _modem->endTransmission(_callsign, _callsign.size());
        if((_tx_radio_type == radio_type::RADIO_TYPE_ANALOG)
                && ((_tx_mode == gr_modem_types::ModemTypeNBFM2500) || (_tx_mode == gr_modem_types::ModemTypeNBFM5000)))
        {
            sendEndBeep();
        }
        usleep(1000000);
        _modem->stopTX();
        _modem->tuneTx(50000000);
        _tx_modem_started = false;
        if(_rx_inited)
            _modem->startRX();
    }
}

void RadioOp::updateFrequency()
{
    long long freq = (long long)_modem->getFreqGUI();
    if(freq != 0 && freq != _tune_center_freq)
    {
        _tune_center_freq = freq;
        _modem->tune(_tune_center_freq);
        _modem->tuneTx(_tune_center_freq + _tune_shift_freq);
        emit freqFromGUI(_tune_center_freq);
    }
}

void RadioOp::run()
{

    bool ptt_activated = false;
    bool frame_flag = true;
    int last_ping_time = 0;
    while(true)
    {
        bool transmitting = _transmitting;
        QCoreApplication::processEvents();
        int time = QDateTime::currentDateTime().toTime_t();
        if((time - last_ping_time) > 20)
        {
            emit pingServer();
            last_ping_time = time;
        }

        updateFrequency();
        if(transmitting && !ptt_activated)
        {
            ptt_activated = true;
            startTx();
        }
        if(!transmitting && ptt_activated)
        {
            ptt_activated = false;
            stopTx();
        }
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
                processNetStream();
            }
            else
            {
                processAudioStream();
            }
        }
        else
        {
            if(_rx_inited)
            {
                if(!_tuning_done)
                    autoTune();
                if(_rx_radio_type == radio_type::RADIO_TYPE_DIGITAL)
                    _modem->demodulate();
                else if(_rx_radio_type == radio_type::RADIO_TYPE_ANALOG)
                {
                    _modem->demodulateAnalog();
                }

            }
        }
        if(_process_text && (_tx_radio_type == radio_type::RADIO_TYPE_DIGITAL))
        {
            if(_tx_inited) {
                if(!_tx_modem_started)
                {
                    _modem->stopTX();
                    _modem->startTX();
                }
                _tx_modem_started = true;
                _modem->startTransmission(_callsign,_callsign.size());
                _modem->textData(_text_out);
            }
            if(!_repeat_text)
            {
                _mutex->lock();
                _process_text = false;
                _mutex->unlock();
            }
            emit displayTransmitStatus(false);
        }

        usleep(2000);
        if(_stop)
            break;
    }

    emit finished();
}

void RadioOp::receiveAudioData(unsigned char *data, int size)
{
    short *audio_out;
    int samples;
    if((_rx_mode == gr_modem_types::ModemTypeBPSK2000) ||
            (_rx_mode == gr_modem_types::ModemType2FSK2000) ||
            (_rx_mode == gr_modem_types::ModemType4FSK2000) ||
            (_rx_mode == gr_modem_types::ModemTypeQPSK2000))
    {
        audio_out = _codec->decode_codec2(data, size, samples);
    }
    else if((_rx_mode == gr_modem_types::ModemTypeBPSK1000))
        audio_out = _codec->decode_codec2_700(data, size, samples);
    else
        audio_out = _codec->decode_opus(data, size, samples);
    delete[] data;
    if(samples > 0)
    {
        for(int i=0;i<samples;i++)
        {
            audio_out[i] = (short)((float)audio_out[i] * _rx_volume);
        }
        if(_voip_forwarding)
        {
            emit voipData(audio_out,samples*sizeof(short));
        }
        else
        {
            _audio->write_short(audio_out,samples*sizeof(short));
        }
    }
}

void RadioOp::receivePCMAudio(std::vector<float> *audio_data)
{
    if(audio_data->size() > 4096)
    {
        delete audio_data;
        return;
    }
    short *pcm = new short[audio_data->size()];
    for(int i=0;i<audio_data->size();i++)
    {
        pcm[i] = (short)(audio_data->at(i) *_rx_volume * 32767.0f);
    }
    if(_voip_forwarding)
    {
        emit voipData(pcm,audio_data->size()*sizeof(short));
    }
    else
    {
        _audio->write_short(pcm, audio_data->size()*sizeof(short));
    }
    audio_data->clear();
    delete audio_data;
    audioFrameReceived();
}

int RadioOp::getFrameLength(unsigned char *data)
{
    unsigned long frame_size1;
    unsigned long frame_size2;
    unsigned long frame_size3;

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

void RadioOp::receiveVideoData(unsigned char *data, int size)
{
    int frame_size = getFrameLength(data);
    if(frame_size == 0)
    {
        qDebug() << "received corrupted frame size, dropping frame ";
        delete[] data;
        return;
    }
    unsigned char *jpeg_frame = new unsigned char[frame_size];
    memcpy(jpeg_frame, &data[12], frame_size);
    delete[] data;
    unsigned char *raw_output = _video->decode_jpeg(jpeg_frame,frame_size);
    delete[] jpeg_frame;
    if(!raw_output)
    {

        return;
    }
    QImage img (320,240, QImage::Format_RGB888);
    img.loadFromData(raw_output, 230400, 0);
    img.convertToFormat(QImage::Format_RGB32);
    QImage out_img(img.scaled(480, 360));

    emit videoImage(out_img);
    delete[] raw_output;

}

void RadioOp::receiveNetData(unsigned char *data, int size)
{
    int frame_size = getFrameLength(data);
    if(frame_size == 0)
    {
        qDebug() << "received corrupted frame size, dropping frame ";
        delete[] data;
        return;
    }
    unsigned char *net_frame = new unsigned char[frame_size];
    memcpy(net_frame, &data[12], frame_size);
    delete[] data;
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
        int size = (_m_queue->size() > samples) ? samples : _m_queue->size();
        for(int i=0;i<size;i++)
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
        for(int i = 0;i<_m_queue->size();i++)
        {
            pcm[i] = (short)((float)_m_queue->at(i) * _rx_volume);
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
        }
        _last_voiced_frame_timer.restart();
        _m_queue->clear();
        _last_session_id = sid;
    }
}

void RadioOp::startTransmission()
{
    if(_tx_inited || _voip_enabled)
        _transmitting = true;
}

void RadioOp::endTransmission()
{
    _transmitting = false;
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
    emit printText(text);
}

void RadioOp::callsignReceived(QString callsign)
{
    QString time= QDateTime::currentDateTime().toString("d/MMM/yyyy hh:mm:ss");
    QString text = "\n" + time +" >>>> " + callsign + " >>>>\n";
    emit printText(text);
    emit printCallsign(callsign);
}

void RadioOp::audioFrameReceived()
{
    emit displayReceiveStatus(true);
    _voice_led_timer->start(100);
}

void RadioOp::dataFrameReceived()
{
    emit displayDataReceiveStatus(true);
    _data_led_timer->start(100);
}

void RadioOp::receiveEnd()
{
    emit displayReceiveStatus(false);
    emit displayDataReceiveStatus(false);
}

void RadioOp::endAudioTransmission()
{
    QString time= QDateTime::currentDateTime().toString("d/MMM/yyyy hh:mm:ss");
    emit printText(time + " <<<< end transmission <<<<\n");
    QFile resfile(":/res/end_beep.raw");
    if(resfile.open(QIODevice::ReadOnly))
    {
        QByteArray *data = new QByteArray(resfile.readAll());
        short *samples = (short*) data->data();
        _audio->write_short(samples,data->size());

    }
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
        _modem->initRX(_rx_mode, rx_device_args, rx_antenna, rx_freq_corr);
        _fft_gui->set_frequency_range(_tune_center_freq, 1000000);
        _modem->setRxSensitivity(_rx_sensitivity);
        _modem->setSquelch(_squelch);
        _modem->startRX();
        _modem->setRxCTCSS(_rx_ctcss);
        _modem->tune(_tune_center_freq);
        if(_rx_mode == gr_modem_types::ModemTypeQPSK250000 && _net_device == 0)
        {
            _net_device = new NetDevice;
        }
        _rx_inited = true;
    }
    else
    {

        _modem->stopRX();
        _modem->deinitRX(_rx_mode);
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

        _modem->initTX(_tx_mode, tx_device_args, tx_antenna, tx_freq_corr);
        _modem->setTxPower(_tx_power);
        _modem->tuneTx(50000000);
        _modem->setTxCTCSS(_tx_ctcss);
        if(_tx_mode == gr_modem_types::ModemTypeQPSKVideo)
            _video = new VideoEncoder(QString::fromStdString(video_device));
        if(_tx_mode == gr_modem_types::ModemTypeQPSK250000 && _net_device == 0)
        {
            _net_device = new NetDevice;
        }
        _tx_inited = true;
    }
    else
    {
        _modem->deinitTX(_tx_mode);
        if(_tx_mode == gr_modem_types::ModemTypeQPSKVideo)
        {
            delete _video;
            _video = 0;
        }
        _tx_inited = false;
    }
}

void RadioOp::toggleRxMode(int value)
{

    _rx_radio_type = radio_type::RADIO_TYPE_DIGITAL;
    switch(value)
    {
    case 0:
        _rx_mode = gr_modem_types::ModemTypeBPSK2000;
        _tune_limit_lower = -5000;
        _tune_limit_upper = 5000;
        _step_hz = 10;
        break;
    case 1:
        _rx_mode = gr_modem_types::ModemTypeBPSK1000;
        _tune_limit_lower = -5000;
        _tune_limit_upper = 5000;
        _step_hz = 10;
        break;
    case 2:
        _rx_mode = gr_modem_types::ModemTypeQPSK2000;
        _tune_limit_lower = -5000;
        _tune_limit_upper = 5000;
        _step_hz = 10;
        break;
    case 3:
        _rx_mode = gr_modem_types::ModemTypeQPSK20000;
        _tune_limit_lower = -5000;
        _tune_limit_upper = 5000;
        _step_hz = 20;
        break;
    case 4:
        _rx_mode = gr_modem_types::ModemType4FSK2000;
        _tune_limit_lower = -5000;
        _tune_limit_upper = 5000;
        _step_hz = 10;
        break;
    case 5:
        _rx_mode = gr_modem_types::ModemType4FSK20000;
        _tune_limit_lower = -5000;
        _tune_limit_upper = 5000;
        _step_hz = 20;
        break;
    case 6:
        _rx_mode = gr_modem_types::ModemType2FSK2000;
        _tune_limit_lower = -5000;
        _tune_limit_upper = 5000;
        _step_hz = 10;
        break;
    case 7:
        _rx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _rx_mode = gr_modem_types::ModemTypeNBFM2500;
        _tune_limit_lower = -5000;
        _tune_limit_upper = 5000;
        _step_hz = 10;
        break;
    case 8:
        _rx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _rx_mode = gr_modem_types::ModemTypeNBFM5000;
        _tune_limit_lower = -5000;
        _tune_limit_upper = 5000;
        _step_hz = 10;
        break;
    case 9:
        _rx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _rx_mode = gr_modem_types::ModemTypeWBFM;
        _tune_limit_lower = -5000;
        _tune_limit_upper = 5000;
        _step_hz = 10;
        break;
    case 10:
        _rx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _rx_mode = gr_modem_types::ModemTypeSSB2500;
        _tune_limit_lower = -2500;
        _tune_limit_upper = 2500;
        _step_hz = 1;
        break;
    case 11:
        _rx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _rx_mode = gr_modem_types::ModemTypeAM5000;
        _tune_limit_lower = -2500;
        _tune_limit_upper = 2500;
        _step_hz = 1;
        break;
    case 12:
        _rx_mode = gr_modem_types::ModemTypeQPSKVideo;
        _tune_limit_lower = -15000;
        _tune_limit_upper = 15000;
        _step_hz = 100;
        break;
    case 13:
        _rx_mode = gr_modem_types::ModemTypeQPSK250000;
        _tune_limit_lower = -5000;
        _tune_limit_upper = 5000;
        _step_hz = 100;
        break;
    default:
        _rx_mode = gr_modem_types::ModemTypeBPSK2000;
        _tune_limit_lower = -2000;
        _tune_limit_upper = 2000;
        _step_hz = 10;
        break;
    }

    _modem->toggleRxMode(_rx_mode);
}

void RadioOp::toggleTxMode(int value)
{

    _tx_radio_type = radio_type::RADIO_TYPE_DIGITAL;
    switch(value)
    {
    case 0:
        _tx_mode = gr_modem_types::ModemTypeBPSK2000;
        _tune_limit_lower = -5000;
        _tune_limit_upper = 5000;
        _step_hz = 10;
        break;
    case 1:
        _tx_mode = gr_modem_types::ModemTypeBPSK1000;
        _tune_limit_lower = -5000;
        _tune_limit_upper = 5000;
        _step_hz = 10;
        break;
    case 2:
        _tx_mode = gr_modem_types::ModemTypeQPSK2000;
        _tune_limit_lower = -5000;
        _tune_limit_upper = 5000;
        _step_hz = 10;
        break;
    case 3:
        _tx_mode = gr_modem_types::ModemTypeQPSK20000;
        _tune_limit_lower = -5000;
        _tune_limit_upper = 5000;
        _step_hz = 20;
        break;
    case 4:
        _tx_mode = gr_modem_types::ModemType4FSK2000;
        _tune_limit_lower = -5000;
        _tune_limit_upper = 5000;
        _step_hz = 10;
        break;
    case 5:
        _tx_mode = gr_modem_types::ModemType4FSK20000;
        _tune_limit_lower = -5000;
        _tune_limit_upper = 5000;
        _step_hz = 20;
        break;
    case 6:
        _tx_mode = gr_modem_types::ModemType2FSK2000;
        _tune_limit_lower = -5000;
        _tune_limit_upper = 5000;
        _step_hz = 10;
        break;
    case 7:
        _tx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _tx_mode = gr_modem_types::ModemTypeNBFM2500;
        _tune_limit_lower = -5000;
        _tune_limit_upper = 5000;
        _step_hz = 10;
        break;
    case 8:
        _tx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _tx_mode = gr_modem_types::ModemTypeNBFM5000;
        _tune_limit_lower = -5000;
        _tune_limit_upper = 5000;
        _step_hz = 10;
        break;
    case 9:
        _tx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _tx_mode = gr_modem_types::ModemTypeWBFM;
        _tune_limit_lower = -5000;
        _tune_limit_upper = 5000;
        _step_hz = 10;
        break;
    case 10:
        _tx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _tx_mode = gr_modem_types::ModemTypeSSB2500;
        _tune_limit_lower = -2500;
        _tune_limit_upper = 2500;
        _step_hz = 1;
        break;
    case 11:
        _tx_radio_type = radio_type::RADIO_TYPE_ANALOG;
        _tx_mode = gr_modem_types::ModemTypeAM5000;
        _tune_limit_lower = -2500;
        _tune_limit_upper = 2500;
        _step_hz = 1;
        break;
    case 12:
        _tx_mode = gr_modem_types::ModemTypeQPSKVideo;
        _tune_limit_lower = -15000;
        _tune_limit_upper = 15000;
        _step_hz = 100;
        break;
    case 13:
        _tx_mode = gr_modem_types::ModemTypeQPSK250000;
        _tune_limit_lower = -5000;
        _tune_limit_upper = 5000;
        _step_hz = 100;
        break;
    default:
        _tx_mode = gr_modem_types::ModemTypeBPSK2000;
        _tune_limit_lower = -2000;
        _tune_limit_upper = 2000;
        _step_hz = 10;
        break;
    }

    _modem->toggleTxMode(_tx_mode);
}

void RadioOp::usePTTForVOIP(bool value)
{
    _voip_enabled = value;
}

void RadioOp::setVOIPForwarding(bool value)
{
    _voip_forwarding = value;
}

void RadioOp::fineTuneFreq(long center_freq)
{
    _mutex->lock();
    _modem->tune(_tune_center_freq + center_freq*_step_hz);
    _modem->tuneTx(_tune_center_freq + _tune_shift_freq + center_freq*_step_hz);
    _mutex->unlock();
}

void RadioOp::tuneFreq(qint64 center_freq)
{
    _mutex->lock();
    _tune_center_freq = center_freq;
    _modem->tune(_tune_center_freq);
    //_modem->tuneTx(_tune_center_freq + _tune_shift_freq);
    _mutex->unlock();
    _fft_gui->set_frequency_range(_tune_center_freq, 1000000);
}

void RadioOp::tuneTxFreq(qint64 center_freq)
{
    _mutex->lock();
    _tune_shift_freq = center_freq;
    _modem->tuneTx(_tune_center_freq + _tune_shift_freq);
    _mutex->unlock();
}

void RadioOp::setTxPower(int dbm)
{
    _tx_power = (float)dbm/100.0;
    _modem->setTxPower(_tx_power);
}

void RadioOp::setRxSensitivity(int value)
{
    _rx_sensitivity = (float)value/100.0;
    _modem->setRxSensitivity(_rx_sensitivity);
}

void RadioOp::setSquelch(int value)
{
    _squelch = value;
    _modem->setSquelch(value);
}

void RadioOp::setVolume(int value)
{
    _rx_volume = (float)value/10.0;
}

void RadioOp::setRxCTCSS(float value)
{
    _rx_ctcss = value;
    _modem->setRxCTCSS(value);
}

void RadioOp::setTxCTCSS(float value)
{
    _tx_ctcss = value;
    _modem->setTxCTCSS(value);
}

void RadioOp::enableGUIConst(bool value)
{
    _modem->enableGUIConst(value);
}

void RadioOp::enableGUIFFT(bool value)
{
    _modem->enableGUIFFT(value);
}

void RadioOp::syncFrequency()
{
    if(_modem->_frequency_found >= 254)
    {
        _tune_counter = 0;
        _modem->_frequency_found = 10;
    }
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
    if((_rx_mode == gr_modem_types::ModemTypeBPSK2000)
            || (_rx_mode == gr_modem_types::ModemTypeBPSK1000))
        usleep(500);
    else if ((_rx_mode == gr_modem_types::ModemType4FSK2000) ||
             (_rx_mode == gr_modem_types::ModemType2FSK2000) ||
             (_rx_mode == gr_modem_types::ModemTypeQPSK2000))
        usleep(2000);
    else
        usleep(10);
    _tune_center_freq = _tune_center_freq + _step_hz;
    _modem->tune(_tune_center_freq);
    _modem->tuneTx(_tune_center_freq + _tune_shift_freq);
    if(_tune_center_freq >= (_tune_center_freq + _tune_limit_upper))
        _tune_center_freq = _tune_center_freq + _tune_limit_lower;
}

void RadioOp::startAutoTune()
{
    _tuning_done = false;
}

void RadioOp::stopAutoTune()
{
    _tuning_done = true;
}
