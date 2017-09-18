// Written by Adrian Musceac YO8RZZ at gmail dot com, started March 2016.
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

gr_modem::gr_modem(Settings *settings, gr::qtgui::const_sink_c::sptr const_gui,
                   gr::qtgui::number_sink::sptr rssi_gui, QObject *parent) :
    QObject(parent)
{
    _modem_type = gr_modem_types::ModemTypeBPSK2000;
    _frame_length = 7;
    _settings = settings;
    _transmitting = false;
    _frame_counter = 0;
    _shift_reg = 0;
    _last_frame_type = FrameTypeNone;
    //_gr_mod_gmsk = new gr_mod_gmsk(0,24,48000,1600,1200,1);
    //_gr_mod_gmsk->start();
    //_gr_demod_gmsk = new gr_demod_gmsk(0,24,48000,1600,1200,1);
    //_gr_demod_gmsk->start();
    //_gr_mod_bpsk = new gr_mod_bpsk(0,24,48000,1700,1200,1);
    //_gr_mod_bpsk->start();
    //_gr_demod_bpsk = new gr_demod_bpsk(0,24,48000,1700,1200,1);
    //_gr_demod_bpsk->start();
    _bit_buf_len = 7 *8;
    _bit_buf_index = 0;
    _sync_found = false;
    _stream_ended = false;
    _current_frame_type = FrameTypeNone;
    _const_gui = const_gui;
    _rssi_gui = rssi_gui;
    _frequency_found =0;
    _requested_frequency_hz = 434025000;
}

gr_modem::~gr_modem()
{
    //_gr_demod_gmsk->stop();
    //_gr_mod_gmsk->stop();
    //_gr_demod_bpsk->stop();
    //_gr_mod_bpsk->stop();
    //deinitRX();
    //deinitTX();
    //delete _gr_mod_gmsk;
    //delete _gr_demod_gmsk;
    //delete _gr_mod_bpsk;
    //delete _gr_demod_bpsk;
}

void gr_modem::initTX(int modem_type)
{
    _modem_type = modem_type;
    if(modem_type == gr_modem_types::ModemTypeBPSK2000)
    {
        _gr_mod_bpsk_sdr = new gr_mod_bpsk_sdr(0, 125, 250000, 1700, 1200, 1, _requested_frequency_hz, 50);
        _frame_length = 7;
        //_gr_mod_bpsk_sdr->start();
    }
    else if(modem_type == gr_modem_types::ModemTypeQPSK20000)
    {
        _gr_mod_qpsk_sdr = new gr_mod_qpsk_sdr(0, 50, 250000, 1700, 4000, 1, _requested_frequency_hz, 50);
        _frame_length = 47;
        //_gr_mod_qpsk_sdr->start();
    }
    else if(modem_type == gr_modem_types::ModemTypeQPSK2000)
    {
        _gr_mod_qpsk_sdr = new gr_mod_qpsk_sdr(0, 250, 250000, 1700, 800, 1, _requested_frequency_hz, 50);
        _frame_length = 7;
        //_gr_mod_qpsk_sdr->start();
    }
    else if(modem_type == gr_modem_types::ModemType4FSK20000)
    {
        _gr_mod_4fsk_sdr = new gr_mod_4fsk_sdr(0, 50, 250000, 1700, 8500, 1, _requested_frequency_hz, 50);
        _frame_length = 47;
        //_gr_mod_qpsk_sdr->start();
    }
    else if(modem_type == gr_modem_types::ModemType4FSK2000)
    {
        _gr_mod_4fsk_sdr = new gr_mod_4fsk_sdr(0, 250, 250000, 1700, 1700, 1, _requested_frequency_hz, 50);
        _frame_length = 7;
        //_gr_mod_qpsk_sdr->start();
    }

    else if(modem_type == gr_modem_types::ModemTypeNBFM2500)
    {
        _gr_mod_nbfm_sdr = new gr_mod_nbfm_sdr(0,250000, 1700, 2500, 1, _requested_frequency_hz, 50);
        //_gr_mod_qpsk_sdr->start();
    }
    else if(modem_type == gr_modem_types::ModemTypeSSB2500)
    {
        _gr_mod_ssb_sdr = new gr_mod_ssb_sdr(0,250000, 1700, 2500, 1, _requested_frequency_hz, 50);
        //_gr_mod_qpsk_sdr->start();
    }
    else if(modem_type == gr_modem_types::ModemTypeQPSKVideo)
    {
        _gr_mod_qpsk_sdr = new gr_mod_qpsk_sdr(0, 2, 1920000, 1700, 480000, 1, _requested_frequency_hz, 50);
        _frame_length = 7997;
        //_gr_mod_qpsk_sdr->start();
    }

}

void gr_modem::initRX(int modem_type)
{
    _modem_type = modem_type;
    if(modem_type == gr_modem_types::ModemTypeBPSK2000)
    {
        _gr_demod_bpsk_sdr = new gr_demod_bpsk_sdr(
                    _const_gui, _rssi_gui, 0,125,1000000,1700,1200,1, _requested_frequency_hz, 50);
        _bit_buf_len = 7 *8;
        _frame_length = 7;
        _bit_buf = new unsigned char[_bit_buf_len];
        //_gr_demod_bpsk_sdr->start();
    }
    else if (modem_type == gr_modem_types::ModemTypeQPSK20000)
    {
        _gr_demod_qpsk_sdr = new gr_demod_qpsk_sdr(
                    _const_gui,_rssi_gui, 0,50,1000000,20000,1700,4000,1, _requested_frequency_hz, 50);
        _bit_buf_len = 47 *8;
        _frame_length = 47;
        _bit_buf = new unsigned char[_bit_buf_len];
        //_gr_demod_qpsk_sdr->start();
    }
    else if (modem_type == gr_modem_types::ModemTypeQPSK2000)
    {
        _gr_demod_qpsk_sdr = new gr_demod_qpsk_sdr(
                    _const_gui,_rssi_gui, 0,250,1000000,20000,1700,900,1, _requested_frequency_hz, 50);
        _bit_buf_len = 7 *8;
        _frame_length = 7;
        _bit_buf = new unsigned char[_bit_buf_len];
        //_gr_demod_qpsk_sdr->start();
    }
    else if (modem_type == gr_modem_types::ModemType4FSK20000)
    {
        _gr_demod_4fsk_sdr = new gr_demod_4fsk_sdr(
                    _const_gui,_rssi_gui, 0,50,1000000,1700,8500,1, _requested_frequency_hz, 50);
        _bit_buf_len = 47 *8;
        _frame_length = 47;
        _bit_buf = new unsigned char[_bit_buf_len];
        //_gr_demod_4fsk_sdr->start();
    }
    else if (modem_type == gr_modem_types::ModemType4FSK2000)
    {
        _gr_demod_4fsk_sdr = new gr_demod_4fsk_sdr(
                    _const_gui,_rssi_gui, 0,250,1000000,1700,1700,1, _requested_frequency_hz, 50);
        _bit_buf_len = 7 *8;
        _frame_length = 7;
        _bit_buf = new unsigned char[_bit_buf_len];
        _gr_demod_4fsk_sdr->start();
    }
    else if (modem_type == gr_modem_types::ModemTypeNBFM2500)
    {
        _gr_demod_nbfm_sdr = new gr_demod_nbfm_sdr(
                    _const_gui,_rssi_gui, 0, 1000000,1700,2500,1, _requested_frequency_hz, 50);
        //_gr_demod_nbfm_sdr->start();
    }
    else if (modem_type == gr_modem_types::ModemTypeSSB2500)
    {
        _gr_demod_ssb_sdr = new gr_demod_ssb_sdr(
                    _const_gui,_rssi_gui, 0, 1000000,1700,2500,1, _requested_frequency_hz, 50);
        //_gr_demod_ssb_sdr->start();
    }
    else if (modem_type == gr_modem_types::ModemTypeQPSKVideo)
    {
        _gr_demod_qpsk_sdr = new gr_demod_qpsk_sdr(
                    _const_gui,_rssi_gui, 0,2,2000000,1474800,1700,240000,1, _requested_frequency_hz, 50);
        _bit_buf_len = 7997 *8;
        _frame_length = 7997;
        _bit_buf = new unsigned char[_bit_buf_len];
        //_gr_demod_qpsk_sdr->start();
    }
}

void gr_modem::deinitTX(int modem_type)
{
    _modem_type = modem_type;
    if(modem_type == gr_modem_types::ModemTypeBPSK2000)
    {
        _gr_mod_bpsk_sdr->stop();
        delete _gr_mod_bpsk_sdr;
        _gr_mod_bpsk_sdr =0;
    }
    else if((modem_type == gr_modem_types::ModemTypeQPSK20000)
            || (modem_type == gr_modem_types::ModemTypeQPSK2000)
            || (modem_type == gr_modem_types::ModemTypeQPSKVideo))
    {
        _gr_mod_qpsk_sdr->stop();
        delete _gr_mod_qpsk_sdr;
        _gr_mod_qpsk_sdr =0;
    }
    else if((modem_type == gr_modem_types::ModemType4FSK20000)
            || (modem_type == gr_modem_types::ModemType4FSK2000))
    {
        _gr_mod_4fsk_sdr->stop();
        delete _gr_mod_4fsk_sdr;
        _gr_mod_4fsk_sdr =0;
    }
    else if (modem_type == gr_modem_types::ModemTypeNBFM2500)
    {
        _gr_mod_nbfm_sdr->stop();
        delete _gr_mod_nbfm_sdr;
        _gr_mod_nbfm_sdr =0;
    }
    else if (modem_type == gr_modem_types::ModemTypeSSB2500)
    {
        _gr_mod_ssb_sdr->stop();
        delete _gr_mod_ssb_sdr;
        _gr_mod_ssb_sdr =0;
    }

}

void gr_modem::deinitRX(int modem_type)
{
    _modem_type = modem_type;
    if(modem_type == gr_modem_types::ModemTypeBPSK2000)
    {
        _gr_demod_bpsk_sdr->stop();
        delete _gr_demod_bpsk_sdr;
        _gr_demod_bpsk_sdr =0;
    }
    else if((modem_type == gr_modem_types::ModemTypeQPSK20000)
            || (modem_type == gr_modem_types::ModemTypeQPSK2000)
            || (modem_type == gr_modem_types::ModemTypeQPSKVideo))
    {
        _gr_demod_qpsk_sdr->stop();
        delete _gr_demod_qpsk_sdr;
        _gr_demod_qpsk_sdr =0;
    }
    else if((modem_type == gr_modem_types::ModemType4FSK20000)
            || (modem_type == gr_modem_types::ModemType4FSK2000))
    {
        _gr_demod_4fsk_sdr->stop();
        delete _gr_demod_4fsk_sdr;
        _gr_demod_4fsk_sdr =0;
    }
    else if(modem_type == gr_modem_types::ModemTypeNBFM2500)
    {
        _gr_demod_nbfm_sdr->stop();
        delete _gr_demod_nbfm_sdr;
        _gr_demod_nbfm_sdr =0;
    }
    else if(modem_type == gr_modem_types::ModemTypeSSB2500)
    {
        _gr_demod_ssb_sdr->stop();
        delete _gr_demod_ssb_sdr;
        _gr_demod_ssb_sdr =0;
    }
}

void gr_modem::startRX()
{
    if(_modem_type == gr_modem_types::ModemTypeBPSK2000)
    {
        _gr_demod_bpsk_sdr->start();
    }
    else if((_modem_type == gr_modem_types::ModemTypeQPSK20000)
            || (_modem_type == gr_modem_types::ModemTypeQPSK2000)
            || (_modem_type == gr_modem_types::ModemTypeQPSKVideo))
    {
        _gr_demod_qpsk_sdr->start();
    }
    else if((_modem_type == gr_modem_types::ModemType4FSK20000)
            || (_modem_type == gr_modem_types::ModemType4FSK2000))
    {
        _gr_demod_4fsk_sdr->start();
    }
    else if(_modem_type == gr_modem_types::ModemTypeNBFM2500)
    {
        _gr_demod_nbfm_sdr->start();
    }
    else if(_modem_type == gr_modem_types::ModemTypeSSB2500)
    {
        _gr_demod_ssb_sdr->start();
    }
}

void gr_modem::stopRX()
{
    if(_modem_type == gr_modem_types::ModemTypeBPSK2000)
    {
        _gr_demod_bpsk_sdr->stop();
    }
    else if((_modem_type == gr_modem_types::ModemTypeQPSK20000)
            || (_modem_type == gr_modem_types::ModemTypeQPSK2000)
            || (_modem_type == gr_modem_types::ModemTypeQPSKVideo))
    {
        _gr_demod_qpsk_sdr->stop();
    }
    else if((_modem_type == gr_modem_types::ModemType4FSK20000)
            || (_modem_type == gr_modem_types::ModemType4FSK2000))
    {
        _gr_demod_4fsk_sdr->stop();
    }
    else if(_modem_type == gr_modem_types::ModemTypeNBFM2500)
    {
        _gr_demod_nbfm_sdr->stop();
    }
    else if(_modem_type == gr_modem_types::ModemTypeSSB2500)
    {
        _gr_demod_ssb_sdr->stop();
    }
}

void gr_modem::startTX()
{
    if(_modem_type == gr_modem_types::ModemTypeBPSK2000)
    {
        _gr_mod_bpsk_sdr->start();
    }
    else if((_modem_type == gr_modem_types::ModemTypeQPSK20000)
            || (_modem_type == gr_modem_types::ModemTypeQPSK2000)
            || (_modem_type == gr_modem_types::ModemTypeQPSKVideo))
    {
        _gr_mod_qpsk_sdr->start();
    }
    else if((_modem_type == gr_modem_types::ModemType4FSK20000)
            || (_modem_type == gr_modem_types::ModemType4FSK2000))
    {
        _gr_mod_4fsk_sdr->start();
    }
    else if(_modem_type == gr_modem_types::ModemTypeNBFM2500)
    {
        _gr_mod_nbfm_sdr->start();
    }
    else if(_modem_type == gr_modem_types::ModemTypeSSB2500)
    {
        _gr_mod_ssb_sdr->start();
    }
}

void gr_modem::stopTX()
{
    if(_modem_type == gr_modem_types::ModemTypeBPSK2000)
    {

        _gr_mod_bpsk_sdr->stop();
    }
    else if((_modem_type == gr_modem_types::ModemTypeQPSK20000)
            || (_modem_type == gr_modem_types::ModemTypeQPSK2000)
            || (_modem_type == gr_modem_types::ModemTypeQPSKVideo))
    {
        _gr_mod_qpsk_sdr->stop();
    }
    else if((_modem_type == gr_modem_types::ModemType4FSK20000)
            || (_modem_type == gr_modem_types::ModemType4FSK2000))
    {
        _gr_mod_4fsk_sdr->stop();
    }
    else if(_modem_type == gr_modem_types::ModemTypeNBFM2500)
    {
        _gr_mod_nbfm_sdr->stop();
    }
    else if(_modem_type == gr_modem_types::ModemTypeSSB2500)
    {
        _gr_mod_ssb_sdr->stop();
    }
}

void gr_modem::tune(long center_freq, bool sync)
{
    if(_gr_demod_bpsk_sdr)
        _gr_demod_bpsk_sdr->tune(center_freq);
    if(_gr_demod_qpsk_sdr)
        _gr_demod_qpsk_sdr->tune(center_freq);
    if(_gr_demod_4fsk_sdr)
        _gr_demod_4fsk_sdr->tune(center_freq);
    if(_gr_demod_nbfm_sdr)
        _gr_demod_nbfm_sdr->tune(center_freq);
    if(_gr_demod_ssb_sdr)
        _gr_demod_ssb_sdr->tune(center_freq);


    if(!sync)
    {
        _requested_frequency_hz = center_freq;
        if(_gr_mod_bpsk_sdr)
            _gr_mod_bpsk_sdr->tune(center_freq);
        if(_gr_mod_qpsk_sdr)
            _gr_mod_qpsk_sdr->tune(center_freq);
        if(_gr_mod_4fsk_sdr)
            _gr_mod_4fsk_sdr->tune(center_freq);
        if(_gr_mod_nbfm_sdr)
            _gr_mod_nbfm_sdr->tune(center_freq);
        if(_gr_mod_nbfm_sdr)
            _gr_mod_ssb_sdr->tune(center_freq);
    }
}



void gr_modem::setTxPower(int value)
{
    if(_gr_mod_bpsk_sdr)
        _gr_mod_bpsk_sdr->set_power(value);
    if(_gr_mod_qpsk_sdr)
        _gr_mod_qpsk_sdr->set_power(value);
    if(_gr_mod_4fsk_sdr)
        _gr_mod_4fsk_sdr->set_power(value);
    if(_gr_mod_nbfm_sdr)
        _gr_mod_nbfm_sdr->set_power(value);
    if(_gr_mod_ssb_sdr)
        _gr_mod_ssb_sdr->set_power(value);
}

void gr_modem::startTransmission()
{
    _transmitting = true;
    for(int i = 0;i<3;i++)
    {
        std::vector<unsigned char> *tx_start = new std::vector<unsigned char>;
        tx_start->push_back(0x8C);
        tx_start->push_back(0x8C);
        tx_start->push_back(0x8C);
        tx_start->push_back(0x8C);
        tx_start->push_back(0x8C);
        tx_start->push_back(0x8C);
        tx_start->push_back(0x8C);
        tx_start->push_back(0x8C);
        tx_start->push_back(0x8C);
        tx_start->push_back(0x8C);
        QVector<std::vector<unsigned char>*> frames;
        frames.append(tx_start);
        transmit(frames);
    }
}

void gr_modem::endTransmission()
{
    _frame_counter = 0;
    _transmitting = false;
    std::vector<unsigned char> *tx_end = new std::vector<unsigned char>;
    tx_end->push_back(0x4C);
    tx_end->push_back(0x8A);
    tx_end->push_back(0x2B);
    tx_end->push_back(0x4C);
    tx_end->push_back(0x8A);
    tx_end->push_back(0x2B);
    tx_end->push_back(0x4C);
    tx_end->push_back(0x8A);
    tx_end->push_back(0x2B);
    tx_end->push_back(0x00);
    QVector<std::vector<unsigned char>*> frames;
    frames.append(tx_end);
    transmit(frames);
}

void gr_modem::processC2Data(unsigned char *data, int size)
{
    std::vector<unsigned char> *one_frame = frame(data, size, FrameTypeVoice);
    QVector<std::vector<unsigned char>*> frames;
    frames.append(one_frame);
    transmit(frames);
    delete[] data;
}

void gr_modem::processVideoData(unsigned char *data, int size)
{
    std::vector<unsigned char> *one_frame = frame(data, size, FrameTypeVideo);
    QVector<std::vector<unsigned char>*> frames;
    frames.append(one_frame);
    transmit(frames);
    delete[] data;
}

void gr_modem::transmit(QVector<std::vector<unsigned char>*> frames)
{
    std::vector<unsigned char> *all_frames = new std::vector<unsigned char>;
    for (int i=0; i<frames.size();i++)
    {
        all_frames->insert( all_frames->end(), frames.at(i)->begin(), frames.at(i)->end() );
        frames.at(i)->clear();
        delete frames.at(i);
    }
    int ret = 1;
    while(ret)
    {
        usleep(1);
        if(_modem_type == gr_modem_types::ModemTypeBPSK2000)
            ret = _gr_mod_bpsk_sdr->setData(all_frames);
        else if((_modem_type == gr_modem_types::ModemTypeQPSK20000)
                || (_modem_type == gr_modem_types::ModemTypeQPSK2000)
                || (_modem_type == gr_modem_types::ModemTypeQPSKVideo))
            ret = _gr_mod_qpsk_sdr->setData(all_frames);
        else if((_modem_type == gr_modem_types::ModemType4FSK20000)
                || (_modem_type == gr_modem_types::ModemType4FSK2000))
            ret = _gr_mod_4fsk_sdr->setData(all_frames);
    }

}

std::vector<unsigned char>* gr_modem::frame(unsigned char *encoded_audio, int data_size, int frame_type)
{
    std::vector<unsigned char> *data = new std::vector<unsigned char>;
    data->push_back(0xAA); // frame start
    if(frame_type == FrameTypeVoice)
    {
        data->push_back(0xED);
        data->push_back(0x89);
    }
    else if(frame_type == FrameTypeText)
    {
        data->push_back(0x89);
        data->push_back(0xED);
    }
    else if(frame_type == FrameTypeVideo)
    {
        data->push_back(0x98);
        data->push_back(0xDE);
    }
    for(int i=0;i< data_size;i++)
    {
        data->push_back(encoded_audio[i]);
    }

    return data;

}

void gr_modem::textData(QString text)
{
    QStringList list;
    QVector<std::vector<unsigned char>*> frames;
    for( int k=0;k<text.length();k+=_frame_length)
    {
        list.append(text.mid(k,_frame_length));
    }

    startTransmission();
    for(int o = 0;o < list.length();o++)
    {
        QString chunk=list.at(o);
        unsigned char *data = new unsigned char[_frame_length];
        memset(data, 0, _frame_length);
        memcpy(data,chunk.toStdString().c_str(),chunk.length());
        std::vector<unsigned char> *one_frame = frame(data,_frame_length, FrameTypeText);

        frames.append(one_frame);

        delete[] data;
    }
    transmit(frames);
    endTransmission();
}

static void packBytes(unsigned char *pktbuf, const unsigned char *bitbuf, int bitcount)
{
  for(int i = 0; i < bitcount; i += 8) {
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

void gr_modem::demodulate()
{
    std::vector<unsigned char> *demod_data;
    if(_modem_type == gr_modem_types::ModemTypeBPSK2000)
        demod_data = _gr_demod_bpsk_sdr->getData();
    else if((_modem_type == gr_modem_types::ModemTypeQPSK20000)
            || (_modem_type == gr_modem_types::ModemTypeQPSK2000)
            || (_modem_type == gr_modem_types::ModemTypeQPSKVideo))
        demod_data = _gr_demod_qpsk_sdr->getData();
    else if((_modem_type == gr_modem_types::ModemType4FSK20000)
            || (_modem_type == gr_modem_types::ModemType4FSK2000))
        demod_data = _gr_demod_4fsk_sdr->getData();

    int v_size = demod_data->size();
    if(v_size<1)
    {
        usleep(10);
        delete demod_data;
        return;
    }

    for(int i=0;i < v_size;i++)
    {
        if(!_sync_found)
        {
            _current_frame_type = findSync(demod_data->at(i));
            if(_sync_found)
            {
                _bit_buf_index = 0;
                continue;
            }
            if(_stream_ended)
            {
                _stream_ended = false;
                handleStreamEnd();
                continue;
            }
            if(_frequency_found > 0)
                _frequency_found--; // substract one bit
        }
        if(_sync_found)
        {
            if(_frequency_found < 255)
                _frequency_found += 1; // 80 bits + counter
            _bit_buf[_bit_buf_index] =  (demod_data->at(i)) & 0x1;
            _bit_buf_index++;
            if(_bit_buf_index >= _bit_buf_len)
            {
                unsigned char *frame_data = new unsigned char[_frame_length];
                packBytes(frame_data,_bit_buf,_bit_buf_index);
                processReceivedData(frame_data, _current_frame_type);
                _sync_found = false;
                _shift_reg = 0;
                _bit_buf_index = 0;
            }
        }


    }
    demod_data->clear();
    delete demod_data;

}


int gr_modem::findSync(unsigned char bit)
{


    _shift_reg = (_shift_reg << 1) | (bit & 0x1);
    u_int32_t temp = _shift_reg & 0xFFFFFF;

    if((temp == 0xAA89ED))
    {
        _sync_found = true;
        return FrameTypeText;
    }
    if((temp == 0xAAED89))
    {
        _sync_found = true;
        return FrameTypeVoice;
    }
    if((temp == 0xAA98DE))
    {
        _sync_found = true;
        return FrameTypeVideo;
    }
    temp = _shift_reg & 0xFFFFFFFF;
    if((temp == 0x4C8A2B4C))
    {
        _stream_ended = true;
        return FrameTypeEnd;
    }
    return FrameTypeNone;
}


void gr_modem::processReceivedData(unsigned char *received_data, int current_frame_type)
{
    if (current_frame_type == FrameTypeText)
    {
        emit dataFrameReceived();
        _last_frame_type = FrameTypeText;
        char *text_data = new char[_frame_length];
        memset(text_data, 0, _frame_length);
        memcpy(text_data, received_data, _frame_length);
        quint8 string_length = _frame_length;

        for(int ii=_frame_length-1;ii>=0;ii--)
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
    else if (current_frame_type == FrameTypeVoice )
    {
        emit audioFrameReceived();
        _last_frame_type = FrameTypeVoice;
        unsigned char *codec2_data = new unsigned char[_frame_length];
        memcpy(codec2_data, received_data, _frame_length);
        emit codec2Audio(codec2_data,_frame_length);
    }
    else if (current_frame_type == FrameTypeVideo )
    {
        emit audioFrameReceived();
        _last_frame_type = FrameTypeVideo;
        unsigned char *video_data = new unsigned char[_frame_length];
        memcpy(video_data, received_data, _frame_length);
        emit jpegVideo(video_data,_frame_length);
    }
    delete[] received_data;
}

void gr_modem::handleStreamEnd()
{
    if(_last_frame_type == FrameTypeText)
    {
        emit textReceived( QString("\n"));
    }
    else if(_last_frame_type == FrameTypeVoice)
    {
        // do stuff
    }
    else if(_last_frame_type == FrameTypeVideo)
    {
        // do stuff
    }
    emit receiveEnd();
}
