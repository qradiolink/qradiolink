// Written by Adrian Musceac YO8RZZ , started March 2016.
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

#include "gr_modem.h"

gr_modem::gr_modem(const Settings *settings, Logger *logger, QObject *parent) :
    QObject(parent),
    decoder(),
    m17Tx()

{
    _settings = settings;
    _logger = logger;
    _limits = new Limits;
    _bit_buf_len = 8 *8;
    _bit_buf = new unsigned char[_bit_buf_len];
    _burst_timer = new BurstTimer();
    _modem_type_rx = gr_modem_types::ModemTypeBPSK2K;
    _modem_type_tx = gr_modem_types::ModemTypeBPSK2K;
    _rx_frame_length = 7;
    _tx_frame_length = 7;
    _bit_buf_index = 0;
    _sync_found = false;
    _frame_counter = 0;
    _shift_reg = 0;
    _last_frame_type = FrameTypeNone;
    _current_frame_type = FrameTypeNone;
    _gr_mod_base = 0;
    _gr_demod_base = 0;
    _modem_sync = 0;
    _m17_decoder_locked = false;
}

gr_modem::~gr_modem()
{
    if(_gr_demod_base)
        deinitRX(_modem_type_rx);
    if(_gr_mod_base)
        deinitTX(_modem_type_tx);
    delete[] _bit_buf;
    delete _limits;
    delete _burst_timer;
}

void gr_modem::initTX(int modem_type, std::string device_args, std::string device_antenna,
                      int freq_corr, int mmdvm_channels, int mmdvm_channel_separation)
{
    _modem_type_tx = modem_type;
    _gr_mod_base = new gr_mod_base(_burst_timer,
                0, 433500000, 1.0, device_args, device_antenna, freq_corr, mmdvm_channels, mmdvm_channel_separation);
    toggleTxMode(modem_type);

}

void gr_modem::initRX(int modem_type, std::string device_args, std::string device_antenna, int freq_corr, int mmdvm_channels, int mmdvm_channel_separation)
{
    _modem_type_rx = modem_type;
    _gr_demod_base = new gr_demod_base(_burst_timer,
                0, 433500000, 0.9, device_args, device_antenna, freq_corr, mmdvm_channels, mmdvm_channel_separation);
    toggleRxMode(modem_type);

}

void gr_modem::deinitTX(int modem_type)
{
    if(_gr_mod_base)
    {
        _modem_type_tx = modem_type;

        _gr_mod_base->stop();
        delete _gr_mod_base;
        _gr_mod_base = 0;
    }
}

void gr_modem::deinitRX(int modem_type)
{
    if(_gr_demod_base)
    {
        _modem_type_rx = modem_type;
        _gr_demod_base->stop();
        delete _gr_demod_base;
        _gr_demod_base = 0;
    }
}


void gr_modem::toggleTxMode(int modem_type)
{
    _modem_type_tx = modem_type;
    if(_gr_mod_base)
    {
        _gr_mod_base->set_mode(modem_type);
        if(modem_type == gr_modem_types::ModemTypeBPSK2K)
        {
            _tx_frame_length = 7;
        }
        else if(modem_type == gr_modem_types::ModemTypeBPSK1K)
        {
            _tx_frame_length = 4;
        }
        else if(modem_type == gr_modem_types::ModemTypeBPSK8)
        {
            _tx_frame_length = 7;
        }
        else if(modem_type == gr_modem_types::ModemType2FSK1KFM)
        {
            _tx_frame_length = 4;
        }
        else if(modem_type == gr_modem_types::ModemType2FSK1K)
        {
            _tx_frame_length = 4;
        }
        else if(modem_type == gr_modem_types::ModemTypeQPSK20K)
        {
            _tx_frame_length = 47;
        }
        else if(modem_type == gr_modem_types::ModemTypeQPSK2K)
        {
            _tx_frame_length = 7;
        }
        else if(modem_type == gr_modem_types::ModemType4FSK10KFM)
        {
            _tx_frame_length = 47;
        }
        else if(modem_type == gr_modem_types::ModemType2FSK10KFM)
        {
            _tx_frame_length = 47;
        }
        else if(modem_type == gr_modem_types::ModemType4FSK2K)
        {
            _tx_frame_length = 7;
        }
        else if(modem_type == gr_modem_types::ModemType4FSK2KFM)
        {
            _tx_frame_length = 7;
        }
        else if(modem_type == gr_modem_types::ModemType4FSK1KFM)
        {
            _tx_frame_length = 4;
        }
        else if(modem_type == gr_modem_types::ModemTypeQPSKVideo)
        {
            _tx_frame_length = 3122;
        }
        else if(modem_type == gr_modem_types::ModemType2FSK2KFM)
        {
            _tx_frame_length = 7;
        }
        else if(modem_type == gr_modem_types::ModemType2FSK2K)
        {
            _tx_frame_length = 7;
        }
        else if(modem_type == gr_modem_types::ModemTypeQPSK250K)
        {
            _tx_frame_length = 1516;
        }
        else if(modem_type == gr_modem_types::ModemType4FSK100K)
        {
            _tx_frame_length = 622;
        }
        else if(modem_type == gr_modem_types::ModemTypeGMSK1K)
        {
            _tx_frame_length = 4;
        }
        else if(modem_type == gr_modem_types::ModemTypeGMSK2K)
        {
            _tx_frame_length = 7;
        }
        else if(modem_type == gr_modem_types::ModemTypeGMSK10K)
        {
            _tx_frame_length = 47;
        }
        else if(modem_type == gr_modem_types::ModemTypeM17)
        {
            _tx_frame_length = 16;
        }
    }

}

void gr_modem::toggleRxMode(int modem_type)
{
    _modem_type_rx = modem_type;
    if(_gr_demod_base)
    {
        _gr_demod_base->set_mode(modem_type);
        if(modem_type == gr_modem_types::ModemTypeBPSK2K)
        {
            _bit_buf_len = 8 *8;
            _rx_frame_length = 7;
        }
        else if(modem_type == gr_modem_types::ModemTypeBPSK1K)
        {
            _bit_buf_len = 4 *8;
            _rx_frame_length = 4;
        }
        if(modem_type == gr_modem_types::ModemTypeBPSK8)
        {
            _bit_buf_len = 8 *8;
            _rx_frame_length = 7;
        }
        else if(modem_type == gr_modem_types::ModemType2FSK1KFM)
        {
            _bit_buf_len = 4 *8;
            _rx_frame_length = 4;
        }
        else if(modem_type == gr_modem_types::ModemType2FSK1K)
        {
            _bit_buf_len = 4 *8;
            _rx_frame_length = 4;
        }
        else if (modem_type == gr_modem_types::ModemTypeQPSK20K)
        {
            _bit_buf_len = 48 *8;
            _rx_frame_length = 47;
        }
        else if (modem_type == gr_modem_types::ModemTypeQPSK2K)
        {
            _bit_buf_len = 8 *8;
            _rx_frame_length = 7;
        }
        else if (modem_type == gr_modem_types::ModemType4FSK10KFM)
        {
            _bit_buf_len = 48 *8;
            _rx_frame_length = 47;
        }
        else if (modem_type == gr_modem_types::ModemType2FSK10KFM)
        {
            _bit_buf_len = 48 *8;
            _rx_frame_length = 47;
        }
        else if (modem_type == gr_modem_types::ModemType4FSK2K)
        {
            _bit_buf_len = 8 *8;
            _rx_frame_length = 7;
        }
        else if (modem_type == gr_modem_types::ModemType4FSK2KFM)
        {
            _bit_buf_len = 8 *8;
            _rx_frame_length = 7;
        }
        else if (modem_type == gr_modem_types::ModemType4FSK1KFM)
        {
            _bit_buf_len = 4 *8;
            _rx_frame_length = 4;
        }
        else if (modem_type == gr_modem_types::ModemTypeQPSKVideo)
        {
            _bit_buf_len = 3123 *8;
            _rx_frame_length = 3122;
        }
        else if(modem_type == gr_modem_types::ModemType2FSK2KFM)
        {
            _bit_buf_len = 8 *8;
            _rx_frame_length = 7;
        }
        else if(modem_type == gr_modem_types::ModemType2FSK2K)
        {
            _bit_buf_len = 8 *8;
            _rx_frame_length = 7;
        }
        else if (modem_type == gr_modem_types::ModemTypeQPSK250K)
        {
            _bit_buf_len = 1517 *8;
            _rx_frame_length = 1516;
        }
        else if (modem_type == gr_modem_types::ModemType4FSK100K)
        {
            _bit_buf_len = 623 *8;
            _rx_frame_length = 622;
        }
        else if(modem_type == gr_modem_types::ModemTypeGMSK1K)
        {
            _bit_buf_len = 4 *8;
            _rx_frame_length = 4;
        }
        else if(modem_type == gr_modem_types::ModemTypeGMSK2K)
        {
            _bit_buf_len = 8 *8;
            _rx_frame_length = 7;
        }
        else if (modem_type == gr_modem_types::ModemTypeGMSK10K)
        {
            _bit_buf_len = 48 *8;
            _rx_frame_length = 47;
        }
        else if(modem_type == gr_modem_types::ModemTypeM17)
        {
            _rx_frame_length = 46;
            _bit_buf_len = 46 *8;
        }
        delete[] _bit_buf;
        _bit_buf = new unsigned char[_bit_buf_len];
    }
}


///
/// Start of proxy methods, will be refactored
///

const QMap<std::string,QVector<int>> gr_modem::getRxGainNames() const
{
    if(_gr_demod_base)
        return _gr_demod_base->get_gain_names();
    else
    {
        QMap<std::string,QVector<int>> none;
        return none;
    }
}

const QMap<std::string,QVector<int>> gr_modem::getTxGainNames() const
{
    if(_gr_mod_base)
        return _gr_mod_base->get_gain_names();
    else
    {
        QMap<std::string,QVector<int>> none;
        return none;
    }
}

void gr_modem::startRX(int buffer_size)
{
    if(_gr_demod_base)
    {
        _gr_demod_base->start(buffer_size);
    }
}

void gr_modem::stopRX()
{
    if(_gr_demod_base)
    {
        _gr_demod_base->stop();
    }
}

void gr_modem::startTX(int buffer_size)
{
    if(_gr_mod_base)
    {
        _gr_mod_base->start(buffer_size);
    }
}

void gr_modem::stopTX()
{
    if(_gr_mod_base)
    {
        _gr_mod_base->stop();
    }
}

void gr_modem::flushSources()
{
    if(_gr_mod_base)
    {
        _gr_mod_base->flush_sources();
    }
}

double gr_modem::getFreqGUI()
{
    if(_gr_demod_base)
        return _gr_demod_base->get_freq();
    return 0;
}

void gr_modem::tune(int64_t center_freq)
{
    if(_gr_demod_base)
        _gr_demod_base->tune(center_freq);
}

void gr_modem::tuneTx(int64_t center_freq)
{
    if(_gr_mod_base)
    {
        qDebug() << "TX center frequency: " << center_freq;
        if(_limits->checkLimit(center_freq))
            _gr_mod_base->tune(center_freq);
        else
        {
            if(!_settings->tx_band_limits)
            {
                _gr_mod_base->tune(center_freq);
                _logger->log(Logger::LogLevelInfo,
                         "TX frequency is outside of configured band limits");
            }
            else
            {
                _logger->log(Logger::LogLevelWarning,
                         "Blocked attempt to set TX frequency outside of configured band limits");
            }
        }
    }
}

void gr_modem::setCarrierOffset(int64_t offset)
{
    if(_gr_demod_base)
        _gr_demod_base->set_carrier_offset(offset);
}

void gr_modem::setTxCarrierOffset(int64_t offset)
{
    qDebug() << "TX carrier offset: " << offset;
    if(_gr_mod_base)
        _gr_mod_base->set_carrier_offset(offset);
}

qint64 gr_modem::resetTxCarrierOffset()
{
    if(_gr_mod_base)
    {
        qint64 offset = _gr_mod_base->reset_carrier_offset();
        qDebug() << "TX carrier offset: " << offset;
        return offset;
    }
    return 0;
}

void gr_modem::setSampRate(int samp_rate)
{
    if(_gr_demod_base)
        _gr_demod_base->set_samp_rate(samp_rate);
    if(_gr_mod_base)
        _gr_mod_base->set_samp_rate(samp_rate);
}

void gr_modem::setFFTSize(int size)
{
    if(_gr_demod_base)
        _gr_demod_base->set_fft_size(size);
}

void gr_modem::setTxPower(float value, std::string gain_stage)
{
    if(_gr_mod_base)
        _gr_mod_base->set_power(value, gain_stage);
}

void gr_modem::setBbGain(int value)
{
    float bb_gain = (float)value / 5.0;
    if(_gr_mod_base)
        _gr_mod_base->set_bb_gain(bb_gain);
}

void gr_modem::setGain(int value)
{
    float gain = (float)value / 100.0f;
    if(_gr_demod_base)
        _gr_demod_base->set_gain(gain);
}

void gr_modem::setK(bool value)
{
    if(_gr_mod_base)
        _gr_mod_base->set_cw_k(value);
}

void gr_modem::setRxSensitivity(double value, std::string gain_stage)
{
    if(_gr_demod_base)
        _gr_demod_base->set_rx_sensitivity(value, gain_stage);
}

void gr_modem::setAgcAttack(int value)
{
    if(_gr_demod_base)
    {
        _gr_demod_base->set_agc_attack(value);
    }
}
void gr_modem::setAgcDecay(int value)
{
    if(_gr_demod_base)
    {
        _gr_demod_base->set_agc_decay(value);
    }
}

void gr_modem::setSquelch(int value)
{
    if(_gr_demod_base)
        _gr_demod_base->set_squelch(value);
}

void gr_modem::setFilterWidth(int width)
{
    if(_gr_demod_base)
        _gr_demod_base->set_filter_width(width, _modem_type_rx);
    if(_gr_mod_base)
        _gr_mod_base->set_filter_width(width, _modem_type_tx);
}

void gr_modem::setRxCTCSS(float value)
{
    if(_gr_demod_base)
        _gr_demod_base->set_ctcss(value);
}

void gr_modem::setTxCTCSS(float value)
{
    if(_gr_mod_base)
        _gr_mod_base->set_ctcss(value);
}

void gr_modem::enableGUIConst(bool value)
{
    if(_gr_demod_base)
        _gr_demod_base->enable_gui_const(value);
}

void gr_modem::enableGUIFFT(bool value)
{
    if(_gr_demod_base)
        _gr_demod_base->enable_gui_fft(value);
}

void gr_modem::enableRSSI(bool value)
{
    if(_gr_demod_base)
        _gr_demod_base->enable_rssi(value);
}

void gr_modem::calibrateRSSI(float value)
{
    if(_gr_demod_base)
        _gr_demod_base->calibrate_rssi(value);
}

void gr_modem::enableDemod(bool value)
{
    if(_gr_demod_base)
        _gr_demod_base->enable_demodulator(value);
}

///
/// End of proxy methods (for refactor)
///


void gr_modem::sendCallsign(QString callsign)
{
    std::vector<unsigned char> *send_callsign = new std::vector<unsigned char>;
    QVector<std::vector<unsigned char>*> callsign_frames;

    send_callsign->push_back((unsigned char)((FrameTypeCallsign >> 16) & 0xFF));
    send_callsign->push_back((unsigned char)((FrameTypeCallsign >> 8) & 0xFF));
    send_callsign->push_back((unsigned char)(FrameTypeCallsign & 0xFF));
    for(int i = 0;i<callsign.size();i++)
    {
        send_callsign->push_back(callsign.toStdString().c_str()[i]);
    }

    for(int i = 0;i<_tx_frame_length-callsign.size();i++)
    {
        send_callsign->push_back(0x00);
    }


    callsign_frames.append(send_callsign);
    transmit(callsign_frames);
}

void gr_modem::startTransmission(QString callsign)
{
    if(!_gr_mod_base)
    {
        return;
    }
    std::vector<unsigned char> *tx_start = new std::vector<unsigned char>;
    if(_modem_type_tx == gr_modem_types::ModemTypeM17)
    {
        m17Tx.start(callsign.toStdString(), tx_start);
        QVector<std::vector<unsigned char>*> frames;
        frames.append(tx_start);
        transmit(frames);
        return;
    }
    // set preamble to 48 bits (ramp-up included)
    for(int i = 0;i < 8;i++)
    {
        tx_start->push_back(0xAA);
    }
    QVector<std::vector<unsigned char>*> frames;
    frames.append(tx_start);
    transmit(frames);
    sendCallsign(callsign);
}

void gr_modem::endTransmission(QString callsign)
{
    std::vector<unsigned char> *tx_end = new std::vector<unsigned char>;
    if(_modem_type_tx == gr_modem_types::ModemTypeM17)
    {
        M17::payload_t dataFrame;
        memset(dataFrame.data(), 0, _tx_frame_length);
        m17Tx.send(dataFrame, tx_end, true);
        for(int i = 0;i<_tx_frame_length/2;i++)
        {
            tx_end->push_back((unsigned char)((FrameTypeM17EOT >> 8) & 0xFF));
            tx_end->push_back((unsigned char)(FrameTypeM17EOT & 0xFF));
        }
    }
    else
    {
        _frame_counter = 0;
        sendCallsign(callsign);
        tx_end->push_back((unsigned char)((FrameTypeEnd >> 16) & 0xFF));
        tx_end->push_back((unsigned char)((FrameTypeEnd >> 8) & 0xFF));
        tx_end->push_back((unsigned char)(FrameTypeEnd & 0xFF));
        for(int i = 0;i<_tx_frame_length*10;i++)
        {
            tx_end->push_back(0xAA);
        }
    }
    QVector<std::vector<unsigned char>*> frames;
    frames.append(tx_end);
    transmit(frames);
}


void gr_modem::transmitTextData(QString text, int frame_type)
{
    QStringList list;
    QVector<std::vector<unsigned char>*> frames;
    for( int k=0;k<text.length();k+=_tx_frame_length)
    {
        list.append(text.mid(k,_tx_frame_length));
    }

    for(int o = 0;o < list.length();o++)
    {
        QString chunk=list.at(o);
        unsigned char *data = new unsigned char[_tx_frame_length];
        memset(data, 0, _tx_frame_length);
        memcpy(data,chunk.toStdString().c_str(),chunk.length());
        std::vector<unsigned char> *one_frame = frame(data,_tx_frame_length, frame_type);
        frames.append(one_frame);
        delete[] data;
    }
    transmit(frames);
}

void gr_modem::transmitBinData(QByteArray bin_data, int frame_type)
{
    QVector<std::vector<unsigned char>*> frames;
    for( int k=0;k<bin_data.length();k+=_tx_frame_length)
    {
        QByteArray d = bin_data.mid(k,_tx_frame_length);
        int copy = _tx_frame_length;
        if(d.size() < _tx_frame_length)
            copy = d.size();
        char * c = d.data();
        unsigned char *data = new unsigned char[_tx_frame_length];
        memset(data, 0, _tx_frame_length);
        memcpy(data, c, copy);
        std::vector<unsigned char> *one_frame = frame(data,_tx_frame_length, frame_type);
        frames.append(one_frame);
        delete[] data;
    }
    transmit(frames);
}


void gr_modem::transmitDigitalAudio(unsigned char *data, int size)
{
    std::vector<unsigned char> *one_frame = frame(data, size, FrameTypeVoice);
    QVector<std::vector<unsigned char>*> frames;
    frames.append(one_frame);
    transmit(frames);
    delete[] data;
}


void gr_modem::transmitPCMAudio(std::vector<float> *audio_data)
{
    if(!_gr_mod_base)
    {
        audio_data->clear();
        delete audio_data;
        return;
    }
    _gr_mod_base->set_audio(audio_data);
}

void gr_modem::transmitM17Audio(unsigned char *data, int size)
{
    if(!_gr_mod_base)
    {
        delete[] data;
        return;
    }
    std::vector<unsigned char> *one_frame = new std::vector<unsigned char>;
    M17::payload_t dataFrame;
    memcpy(dataFrame.data(), data, size);
    delete[] data;
    bool lastFrame = false;
    m17Tx.send(dataFrame, one_frame, lastFrame);
    QVector<std::vector<unsigned char>*> frames;
    frames.append(one_frame);
    transmit(frames);
}

void gr_modem::transmitVideoData(unsigned char *data, int size)
{
    std::vector<unsigned char> *one_frame = frame(data, size, FrameTypeVideo);
    QVector<std::vector<unsigned char>*> frames;
    frames.append(one_frame);
    transmit(frames);
    delete[] data;
}

void gr_modem::transmitNetData(unsigned char *data, int size)
{
    QVector<std::vector<unsigned char>*> frames;
    std::vector<unsigned char> *one_frame = frame(data, size, FrameTypeIP);
    frames.append(one_frame);
    transmit(frames);
    delete[] data;
}

std::vector<unsigned char>* gr_modem::frame(unsigned char *encoded_audio, int data_size, int frame_type)
{
    std::vector<unsigned char> *data = new std::vector<unsigned char>;
    if(frame_type == FrameTypeIP && _settings->burst_ip_modem)
    {
        int preamble_size = 10; // increase ramp-up symbols to 20
        for(int i = 0;i < preamble_size;i++)
            data->push_back(0xAA);
    }
    if(frame_type == FrameTypeVoice)
    {
        if((_modem_type_tx == gr_modem_types::ModemTypeBPSK1K)
                || (_modem_type_tx == gr_modem_types::ModemType2FSK1KFM)
                || (_modem_type_tx == gr_modem_types::ModemType2FSK1K)
                || (_modem_type_tx == gr_modem_types::ModemTypeGMSK1K)
                || (_modem_type_tx == gr_modem_types::ModemType4FSK1KFM))
        {
            data->push_back((unsigned char)(FrameTypeVoice1 & 0xFF));
        }
        else
        {
            data->push_back((unsigned char)((FrameTypeVoice2 >> 8) & 0xFF));
            data->push_back((unsigned char)(FrameTypeVoice2 & 0xFF));
            data->push_back(0xAA); // reserved bits
        }
    }
    else if(frame_type == FrameTypeText)
    {
        data->push_back((unsigned char)((FrameTypeText >> 16) & 0xFF));
        data->push_back((unsigned char)((FrameTypeText >> 8) & 0xFF));
        data->push_back((unsigned char)(FrameTypeText & 0xFF));
    }
    else if(frame_type == FrameTypeVideo)
    {
        data->push_back((unsigned char)((FrameTypeVideo >> 16) & 0xFF));
        data->push_back((unsigned char)((FrameTypeVideo >> 8) & 0xFF));
        data->push_back((unsigned char)(FrameTypeVideo & 0xFF));
    }
    else if(frame_type == FrameTypeIP)
    {
        data->push_back((unsigned char)((FrameTypeIP >> 16) & 0xFF));
        data->push_back((unsigned char)((FrameTypeIP >> 8) & 0xFF));
        data->push_back((unsigned char)(FrameTypeIP & 0xFF));
    }
    else if(frame_type == FrameTypeProto)
    {
        data->push_back((unsigned char)((FrameTypeProto >> 16) & 0xFF));
        data->push_back((unsigned char)((FrameTypeProto >> 8) & 0xFF));
        data->push_back((unsigned char)(FrameTypeProto & 0xFF));
    }

    for(int i=0;i< data_size;i++)
    {
        data->push_back(encoded_audio[i]);
    }

    return data;
}

void gr_modem::transmit(QVector<std::vector<unsigned char>*> frames)
{
    if(!_gr_mod_base)
    {
        return;
    }
    std::vector<unsigned char> *all_frames = new std::vector<unsigned char>;
    for (int i=0; i<frames.size();i++)
    {
        all_frames->insert( all_frames->end(), frames.at(i)->begin(), frames.at(i)->end() );
        frames.at(i)->clear();
        delete frames.at(i);
    }
    _gr_mod_base->set_data(all_frames);

}

static void packBytes(unsigned char *pktbuf, const unsigned char *bitbuf, int bitcount)
{
    for(int i = 0; i < bitcount; i += 8)
    {
        int t = bitbuf[i+0] & 0x1;
        t = (t << 1) | (bitbuf[i+1] & 0x1);
        t = (t << 1) | (bitbuf[i+2] & 0x1);
        t = (t << 1) | (bitbuf[i+3] & 0x1);
        t = (t << 1) | (bitbuf[i+4] & 0x1);
        t = (t << 1) | (bitbuf[i+5] & 0x1);
        t = (t << 1) | (bitbuf[i+6] & 0x1);
        t = (t << 1) | (bitbuf[i+7] & 0x1);
        *pktbuf++ = t;
    }
}

void gr_modem::getFFTData(float* data, unsigned int &size)
{
    if(_gr_demod_base)
        _gr_demod_base->get_FFT_data(data, size);
}

float gr_modem::getRSSI()
{
    if(_gr_demod_base)
        return _gr_demod_base->get_rssi();
    else
    {
        return 9999.0;
    }
}

std::vector<gr_complex>* gr_modem::getConstellation()
{
    if(_gr_demod_base)
        return _gr_demod_base->get_constellation_data();
    else
    {
        return nullptr;
    }
}

bool gr_modem::demodulateAnalog()
{
    if(!_gr_demod_base)
    {
        return false;
    }
    std::vector<float> *audio_data = nullptr;
    audio_data = _gr_demod_base->getAudio();
    if(audio_data == nullptr)
        return false;
    if(audio_data->size() > 0)
    {
        emit pcmAudio(audio_data);
        return true;
    }
    else
    {
        delete audio_data;
        return false;
    }

}

bool gr_modem::demodulate()
{
    if(!_gr_demod_base)
    {
        return false;
    }
    std::vector<unsigned char> *demod_data = nullptr;
    std::vector<unsigned char> *demod_data2 = nullptr;
    std::vector<unsigned char> *data;

    if((_modem_type_rx == gr_modem_types::ModemTypeBPSK2K)
            || (_modem_type_rx == gr_modem_types::ModemType2FSK2KFM)
            || (_modem_type_rx == gr_modem_types::ModemType2FSK2K)
            || (_modem_type_rx == gr_modem_types::ModemType2FSK10KFM)
            || (_modem_type_rx == gr_modem_types::ModemTypeGMSK2K)
            || (_modem_type_rx == gr_modem_types::ModemTypeGMSK1K)
            || (_modem_type_rx == gr_modem_types::ModemTypeGMSK10K)
            || (_modem_type_rx == gr_modem_types::ModemTypeBPSK1K)
            || (_modem_type_rx == gr_modem_types::ModemTypeBPSK8)
            || (_modem_type_rx == gr_modem_types::ModemType2FSK1KFM)
            || (_modem_type_rx == gr_modem_types::ModemType2FSK1K))
    {
        demod_data = _gr_demod_base->getData(1);
        demod_data2 = _gr_demod_base->getData(2);
        if((demod_data == nullptr) || (demod_data2 == nullptr))
            return false;
    }
    else
    {
        demod_data = _gr_demod_base->getData();
        if(demod_data == nullptr)
            return false;
    }

    int v_size;
    if((_modem_type_rx == gr_modem_types::ModemTypeBPSK2K)
            || (_modem_type_rx == gr_modem_types::ModemTypeBPSK1K)
            || (_modem_type_rx == gr_modem_types::ModemTypeBPSK8)
            || (_modem_type_rx == gr_modem_types::ModemType2FSK10KFM)
            || (_modem_type_rx == gr_modem_types::ModemType2FSK2KFM)
            || (_modem_type_rx == gr_modem_types::ModemType2FSK2K)
            || (_modem_type_rx == gr_modem_types::ModemType2FSK1KFM)
            || (_modem_type_rx == gr_modem_types::ModemType2FSK1K)
            || (_modem_type_rx == gr_modem_types::ModemTypeGMSK10K)
            || (_modem_type_rx == gr_modem_types::ModemTypeGMSK2K)
            || (_modem_type_rx == gr_modem_types::ModemTypeGMSK1K))
    {
        if(demod_data->size() >= demod_data2->size())
        {
            v_size = demod_data->size();
            data = demod_data;
        }
        else
        {
            v_size = demod_data2->size();
            data = demod_data2;
        }
    }
    else
    {
        v_size = demod_data->size();
        data = demod_data;
    }

    bool data_to_process = synchronize(v_size, data);
    demod_data->clear();
    delete demod_data;
    if((_modem_type_rx == gr_modem_types::ModemTypeBPSK2K)
            || (_modem_type_rx == gr_modem_types::ModemTypeBPSK1K)
            || (_modem_type_rx == gr_modem_types::ModemTypeBPSK8)
            || (_modem_type_rx == gr_modem_types::ModemType2FSK10KFM)
            || (_modem_type_rx == gr_modem_types::ModemType2FSK2KFM)
            || (_modem_type_rx == gr_modem_types::ModemType2FSK2K)
            || (_modem_type_rx == gr_modem_types::ModemType2FSK1KFM)
            || (_modem_type_rx == gr_modem_types::ModemType2FSK1K)
            || (_modem_type_rx == gr_modem_types::ModemTypeGMSK10K)
            || (_modem_type_rx == gr_modem_types::ModemTypeGMSK2K)
            || (_modem_type_rx == gr_modem_types::ModemTypeGMSK1K))
    {
        demod_data2->clear();
        delete demod_data2;
    }
    return data_to_process;

}

bool gr_modem::synchronize(int v_size, std::vector<unsigned char> *data)
{
    bool data_to_process = false;
    for(int i=0;i < v_size;i++)
    {
        if(!_sync_found)
        {
            _current_frame_type = findSync(data->at(i));
            if(_sync_found)
            {
                _bit_buf_index = 0;
                if(_modem_sync < 32)
                    _modem_sync += 8;
                continue;
            }
            else
            {
                if(_modem_sync > 0)
                    _modem_sync -= 1;
            }
        }
        if(_sync_found)
        {
            data_to_process = true;
            _bit_buf[_bit_buf_index] =  (data->at(i)) & 0x1;
            _bit_buf_index++;
            int frame_length = _rx_frame_length;
            int bit_buf_len = _bit_buf_len;
            if((_modem_type_rx != gr_modem_types::ModemTypeBPSK1K)
                    && (_modem_type_rx != gr_modem_types::ModemType2FSK1KFM)
                    && (_modem_type_rx != gr_modem_types::ModemType2FSK1K)
                    && (_modem_type_rx != gr_modem_types::ModemType4FSK1KFM)
                    && (_modem_type_rx != gr_modem_types::ModemTypeGMSK1K)
                    && (_current_frame_type == FrameTypeVoice))
            {
                frame_length++; // reserved data
            }
            else if((_modem_type_rx != gr_modem_types::ModemTypeBPSK1K)
                    && (_modem_type_rx != gr_modem_types::ModemType2FSK1KFM)
                    && (_modem_type_rx != gr_modem_types::ModemType4FSK1KFM)
                    && (_modem_type_rx != gr_modem_types::ModemType2FSK1K)
                    && (_modem_type_rx != gr_modem_types::ModemTypeGMSK1K)
                    && (_modem_type_rx != gr_modem_types::ModemTypeM17)
                    && (_current_frame_type != FrameTypeVoice))
            {
                bit_buf_len = _bit_buf_len - 8;
            }
            if(_bit_buf_index >= bit_buf_len)
            {
                unsigned char *frame_data = new unsigned char[frame_length];
                packBytes(frame_data,_bit_buf,bit_buf_len);
                processReceivedData(frame_data, _current_frame_type);
                _sync_found = false;
                _shift_reg = 0;
                _bit_buf_index = 0;
            }
        }
    }
    return data_to_process;
}

int gr_modem::findSync(unsigned char bit)
{
    _shift_reg = (_shift_reg << 1) | (bit & 0x1);
    u_int32_t temp;
    if((_modem_type_rx == gr_modem_types::ModemTypeBPSK1K) ||
            (_modem_type_rx == gr_modem_types::ModemType2FSK1KFM) ||
            (_modem_type_rx == gr_modem_types::ModemType2FSK1K) ||
            (_modem_type_rx == gr_modem_types::ModemTypeGMSK1K) ||
            (_modem_type_rx == gr_modem_types::ModemType4FSK1KFM))
    {
        temp = _shift_reg & 0xFF;
        if (temp == FrameTypeVoice1)
        {
            _sync_found = true;
            return FrameTypeVoice;
        }
    }
    if(_modem_type_rx != gr_modem_types::ModemTypeQPSK250K &&
            _modem_type_rx != gr_modem_types::ModemTypeQPSKVideo &&
            _modem_type_rx != gr_modem_types::ModemType4FSK100K)
    {
        temp = _shift_reg & 0xFFFF;
        if(temp == FrameTypeM17Stream)
        {
            _sync_found = true;
            return FrameTypeM17Stream;
        }
        if(temp == FrameTypeM17LSF)
        {
            _sync_found = true;
            return FrameTypeM17LSF;
        }
        if(temp == FrameTypeVoice2)
        {
            _sync_found = true;
            return FrameTypeVoice;
        }
        temp = _shift_reg & 0xFFFFFF;
        if(temp == FrameTypeText)
        {
            _sync_found = true;
            return FrameTypeText;
        }
        if(temp == FrameTypeProto)
        {
            _sync_found = true;
            return FrameTypeProto;
        }

        if(temp == FrameTypeVideo)
        {
            _sync_found = true;
            return FrameTypeVideo;
        }
        if(temp == FrameTypeCallsign)
        {
            _sync_found = true;
            return FrameTypeCallsign;
        }
    }
    temp = _shift_reg & 0xFFFFFF;
    if(temp == FrameTypeIP)
    {
        _sync_found = true;
        return FrameTypeIP;
    }
    if(temp == FrameTypeVideo)
    {
        _sync_found = true;
        return FrameTypeVideo;
    }
    if(temp == FrameTypeEnd)
    {
        _sync_found = true;
        return FrameTypeEnd;
    }
    return FrameTypeNone;
}


void gr_modem::processReceivedData(unsigned char *received_data, int current_frame_type)
{
    if (current_frame_type == FrameTypeEnd)
    {
        handleStreamEnd();
    }
    else if (current_frame_type == FrameTypeText)
    {
        emit dataFrameReceived();
        _last_frame_type = FrameTypeText;
        char *text_data = new char[_rx_frame_length];
        memcpy(text_data, received_data, _rx_frame_length);
        quint8 string_length = _rx_frame_length;

        for(int ii=_rx_frame_length-1;ii>=0;ii--)
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
        QString text = QString::fromLocal8Bit(text_data,string_length);
        emit textReceived(text);
        delete[] text_data;
    }
    else if (current_frame_type == FrameTypeProto)
    {
        emit dataFrameReceived();
        _last_frame_type = FrameTypeProto;
        QByteArray data((const char*)received_data, _rx_frame_length);
        emit protoReceived(data);
    }
    else if (current_frame_type == FrameTypeCallsign)
    {
        _last_frame_type = FrameTypeCallsign;
        char *text_data = new char[8];
        memset(text_data, 0, 8);
        memcpy(text_data, received_data, 7);

        QString callsign(text_data);
        callsign = callsign.remove(QRegExp("[^a-zA-Z/\\d\\s]"));
        callsign = callsign.left(7);
        emit callsignReceived(callsign);
        delete[] text_data;
    }
    else if (current_frame_type == FrameTypeVoice)
    {
        _last_frame_type = FrameTypeVoice;
        unsigned char *codec2_data = new unsigned char[_rx_frame_length];
        memset(codec2_data,0,_rx_frame_length);
        if(((_modem_type_rx == gr_modem_types::ModemTypeBPSK1K) ||
            (_modem_type_rx == gr_modem_types::ModemType2FSK1KFM) ||
            (_modem_type_rx == gr_modem_types::ModemType2FSK1K) ||
            (_modem_type_rx == gr_modem_types::ModemTypeGMSK1K) ||
            (_modem_type_rx == gr_modem_types::ModemType4FSK1KFM)))
        {
            if(_modem_sync >= 16)
            {
                memcpy(codec2_data, received_data, _rx_frame_length);
                emit digitalAudio(codec2_data,_rx_frame_length);
            }
            else
            {
                delete[] codec2_data;
            }
        }
        else if((_modem_type_rx != gr_modem_types::ModemTypeBPSK1K) &&
                (_modem_type_rx != gr_modem_types::ModemType2FSK1KFM) &&
                (_modem_type_rx != gr_modem_types::ModemType2FSK1K) &&
                (_modem_type_rx != gr_modem_types::ModemTypeGMSK1K) &&
                (_modem_type_rx != gr_modem_types::ModemType4FSK1KFM))
        {
            memcpy(codec2_data, received_data+1, _rx_frame_length);
            emit digitalAudio(codec2_data,_rx_frame_length);
        }

    }
    else if (current_frame_type == FrameTypeVideo )
    {
        emit dataFrameReceived();
        _last_frame_type = FrameTypeVideo;
        unsigned char *video_data = new unsigned char[_rx_frame_length];
        memcpy(video_data, received_data, _rx_frame_length);
        emit videoData(video_data,_rx_frame_length);
    }
    else if (current_frame_type == FrameTypeIP )
    {
        _last_frame_type = FrameTypeIP;
        unsigned char *net_data = new unsigned char[_rx_frame_length];
        memcpy(net_data, received_data, _rx_frame_length);
        emit netData(net_data,_rx_frame_length);
        // poke repeater here
    }
    else if ((current_frame_type == FrameTypeM17Stream)
             || (current_frame_type == FrameTypeM17LSF))
    {

        M17::frame_t frame;
        frame[0] = (current_frame_type >> 8) & 0xFF;
        frame[1] = (current_frame_type) & 0xFF;
        memcpy(frame.data() + 2, received_data, _rx_frame_length);
        auto type = decoder.decodeFrame(frame);

        if(type == M17::M17FrameType::LINK_SETUP)
        {
            _last_frame_type = FrameTypeM17LSF;
            M17::M17LinkSetupFrame lsf = decoder.getLsf();
            bool valid_frame = lsf.valid();
            if(valid_frame)
            {
                std::string m17_source = lsf.getSource();
                std::string m17_destination = lsf.getDestination();
                QString callsign = QString::fromStdString(m17_source);
                callsign = callsign.remove(QRegExp("[^a-zA-Z/\\d\\s]"));
                callsign = callsign.left(7);
                emit callsignReceived(callsign);
            }
            decoder.reset();
        }

        else if((type == M17::M17FrameType::STREAM))
        {
            M17::M17StreamFrame sf = decoder.getStreamFrame();
            _last_frame_type = FrameTypeM17Stream;
            unsigned char *codec2_data = new unsigned char[16];
            memcpy(codec2_data, sf.payload().data(), 16);

            emit digitalAudio(codec2_data,16);
            /** TODO
            if(sf.isLastFrame())
            {
                emit endAudioTransmission();
                emit receiveEnd();
                qDebug() << "Last frame rcvd";
                _m17_decoder_locked = false;
            }
            */
        }
    }
    delete[] received_data;
}

void gr_modem::handleStreamEnd()
{
    if(_last_frame_type == FrameTypeText)
    {
        emit textReceived( QString("\n"));
    }
    emit endAudioTransmission();
    emit receiveEnd();
}
