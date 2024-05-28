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

#include "gr_demod_base.h"
#include <uhd/stream.hpp>
#include <iostream>

static const unsigned int INTERNAL_DEFAULT_SAMPLE_RATE = 1000000;

gr_demod_base::gr_demod_base(BurstTimer *burst_timer, QObject *parent, float device_frequency,
                             float rf_gain, std::string device_args, std::string device_antenna,
                              int freq_corr, int mmdvm_channels, int mmdvm_channel_separation) :
    QObject(parent)
{
    _locked = false;
    _msg_nr = 0;
    _demod_running = false;
    _device_frequency = device_frequency;
    _top_block = gr::make_top_block("demodulator");
    _mode = 9999;
    _carrier_offset = 0;
    _samp_rate = INTERNAL_DEFAULT_SAMPLE_RATE;
    _use_tdma = false;
    _time_domain_enabled = false;
    _freq_correction = freq_corr;
    _mmdvm_channels = mmdvm_channels;

    _audio_sink = make_gr_audio_sink();
    _bit_sink = make_gr_bit_sink();

    _message_sink = gr::blocks::message_debug::make();

    _rssi_valve = gr::blocks::copy::make(8);
    _rssi_valve->set_enabled(false);
    _rssi = gr::blocks::probe_signal_f::make();
    _constellation = make_gr_const_sink();
    _const_valve = gr::blocks::copy::make(8);
    _const_valve->set_enabled(false);
    _demod_valve = gr::blocks::copy::make(8);
    _demod_valve->set_enabled(true);
    _rssi_block = make_rssi_block();
    _sample_sink = make_gr_sample_sink();

    _rotator = gr::blocks::rotator_cc::make(2*M_PI/INTERNAL_DEFAULT_SAMPLE_RATE);
    int tw = std::min(_samp_rate/4, 1500000);
    _resampler = gr::filter::rational_resampler_base_ccf::make(1, 1,
                gr::filter::firdes::low_pass(1, _samp_rate, 500000, tw, gr::filter::firdes::WIN_HAMMING));
    _resampler_time_domain = gr::filter::rational_resampler_base_ccf::make(1, 10,
                gr::filter::firdes::low_pass(1, INTERNAL_DEFAULT_SAMPLE_RATE, 50000, 25000, gr::filter::firdes::WIN_HAMMING));

    // FIXME: LimeSDR bandwidth set to higher value for lower freq
    _lime_specific = false;
    _uhd_specific = false;
    QString serial = "";
    QString device(device_args.c_str());
    if(device.contains("driver=lime", Qt::CaseInsensitive))
    {
        _lime_specific = true;
        QStringList args = device.split(",");
        for(int i = 0; i < args.size();i++)
        {
            QStringList values = args.at(i).split("=");
            if((values.size() > 1) && (values.at(0) == "serial"))
            {
                serial = values.at(1);
            }
        }
    }
    if(device.contains("driver=uhd", Qt::CaseInsensitive))
    {
        _uhd_specific = true;
        QStringList args = device.split(",");
        for(int i = 0; i < args.size();i++)
        {
            QStringList values = args.at(i).split("=");
            if((values.size() > 1) && (values.at(0) == "serial"))
            {
                serial = values.at(1);
            }
        }
    }
    if(_lime_specific && serial.size() > 0)
    {
        std::cout << "Using LimeSDR native source" << std::endl;
        _use_tdma = true;
        _limesdr_source = gr::limesdr::source::make(serial.toStdString(), 0, "");
        _limesdr_source->set_center_freq(_device_frequency);
        _limesdr_source->set_sample_rate(INTERNAL_DEFAULT_SAMPLE_RATE);
        _limesdr_source->set_antenna(255);
        _limesdr_source->set_buffer_size(_samp_rate / 10);
        set_bandwidth_specific();
        _limesdr_source->set_gain(int(rf_gain * 70.0f));
    }
    else if(_uhd_specific)
    {
        std::cout << "Using USRP native source" << std::endl;
        _use_tdma = true;
         uhd::stream_args_t stream_args("fc32", "sc16");
         stream_args.channels = {0};
         //stream_args.args["spp"] = "1000"; // 1000 samples per packet
         std::string dev_string;
         if(serial.size() > 1)
             dev_string = QString("serial=%1").arg(serial).toStdString();
         else
             dev_string = "uhd=0";
         uhd::device_addr_t device_addr(dev_string);
        _uhd_source = gr::uhd::usrp_source::make(device_addr,stream_args);
        _uhd_source->set_center_freq(_device_frequency);
        _uhd_source->set_samp_rate(INTERNAL_DEFAULT_SAMPLE_RATE);
        _uhd_source->set_antenna(device_antenna);
        set_bandwidth_specific();
        _uhd_gain_range = _uhd_source->get_gain_range();
        _gain_names = _uhd_source->get_gain_names();
        if (!_uhd_gain_range.empty())
        {
            double gain =  (double)_uhd_gain_range.start() + rf_gain*(
                        (double)_uhd_gain_range.stop()- (double)_uhd_gain_range.start());
            _uhd_source->set_gain(gain);
        }
        else
        {
            _uhd_source->set_gain(rf_gain);
        }
    }
    else
    {
        _lime_specific = false;
        _osmosdr_source = osmosdr::source::make(device_args);
        _osmosdr_source->set_center_freq(_device_frequency);
        set_bandwidth_specific();
        _osmosdr_source->set_sample_rate(INTERNAL_DEFAULT_SAMPLE_RATE);
        //_osmosdr_source->set_freq_corr(freq_corr);
        _osmosdr_source->set_gain_mode(true);
        _osmosdr_source->set_dc_offset_mode(2);
        _osmosdr_source->set_iq_balance_mode(0);
        _osmosdr_source->set_antenna(device_antenna);
        _gain_range = _osmosdr_source->get_gain_range();
        _gain_names = _osmosdr_source->get_gain_names();
        if (!_gain_range.empty())
        {
            double gain =  (double)_gain_range.start() + rf_gain*(
                        (double)_gain_range.stop()- (double)_gain_range.start());
            _osmosdr_source->set_gain_mode(false);
            _osmosdr_source->set_gain(gain);
        }
        else
        {
            _osmosdr_source->set_gain_mode(true);
        }
    }

    _fft_sink = make_rx_fft_c(32768, gr::filter::firdes::WIN_BLACKMAN_HARRIS);
    _mmdvm_sink = make_gr_mmdvm_sink(burst_timer, 1, false, _use_tdma);

    _deframer1 = make_gr_deframer_bb(1);
    _deframer2 = make_gr_deframer_bb(1);

    _deframer_700_1 = make_gr_deframer_bb(2);
    _deframer_700_2 = make_gr_deframer_bb(2);

    _deframer1_10k = make_gr_deframer_bb(3);
    _deframer2_10k = make_gr_deframer_bb(3);

    _top_block->connect(_rotator,0,_demod_valve,0);

    if(_lime_specific)
    {
        _top_block->connect(_limesdr_source,0,_rotator,0);
        _top_block->connect(_limesdr_source,0,_fft_sink,0);
    }
    else if(_uhd_specific)
    {
        _top_block->connect(_uhd_source,0,_rotator,0);
        _top_block->connect(_uhd_source,0,_fft_sink,0);
    }
    else
    {
        _top_block->connect(_osmosdr_source,0,_rotator,0);
        _top_block->connect(_osmosdr_source,0,_fft_sink,0);
    }


    _top_block->connect(_rssi_valve,0,_rssi_block,0);
    _top_block->connect(_rssi_block,0,_rssi,0);


    _2fsk_2k_fm = make_gr_demod_2fsk(5,INTERNAL_DEFAULT_SAMPLE_RATE,1700,4000, true); // 4000 for non FM, 2700 for FM
    _2fsk_1k_fm = make_gr_demod_2fsk(10,INTERNAL_DEFAULT_SAMPLE_RATE,1700,2500, true);
    _2fsk_2k = make_gr_demod_2fsk(5,INTERNAL_DEFAULT_SAMPLE_RATE,1700,4000, false);
    _2fsk_1k = make_gr_demod_2fsk(10,INTERNAL_DEFAULT_SAMPLE_RATE,1700,2000, false);
    _2fsk_10k = make_gr_demod_2fsk(1,INTERNAL_DEFAULT_SAMPLE_RATE,1700,25000, true);
    _gmsk_2k = make_gr_demod_gmsk(5,INTERNAL_DEFAULT_SAMPLE_RATE,1700,4000);
    _gmsk_1k = make_gr_demod_gmsk(10,INTERNAL_DEFAULT_SAMPLE_RATE,1700,2000);
    _gmsk_10k = make_gr_demod_gmsk(1,INTERNAL_DEFAULT_SAMPLE_RATE,1700,20000);
    _4fsk_2k = make_gr_demod_4fsk(5,INTERNAL_DEFAULT_SAMPLE_RATE,1700,4000, false);
    _4fsk_2k_fm = make_gr_demod_4fsk(5,INTERNAL_DEFAULT_SAMPLE_RATE,1700,3000, true);
    _4fsk_1k_fm = make_gr_demod_4fsk(10,INTERNAL_DEFAULT_SAMPLE_RATE,1700,2000, true);
    _4fsk_10k_fm = make_gr_demod_4fsk(1,INTERNAL_DEFAULT_SAMPLE_RATE,1700,20000, true);
    _am = make_gr_demod_am(125, INTERNAL_DEFAULT_SAMPLE_RATE,1700,5000);
    _bpsk_1k = make_gr_demod_bpsk(10,INTERNAL_DEFAULT_SAMPLE_RATE,1700,1300);
    _bpsk_2k = make_gr_demod_bpsk(5,INTERNAL_DEFAULT_SAMPLE_RATE,1700,2400);
    _bpsk_dsss_8 = make_gr_demod_dsss(25,INTERNAL_DEFAULT_SAMPLE_RATE,1700,150);
    _fm_2500 = make_gr_demod_nbfm(125, INTERNAL_DEFAULT_SAMPLE_RATE,1700,2500);
    _fm_5000 = make_gr_demod_nbfm(125, INTERNAL_DEFAULT_SAMPLE_RATE,1700,5000);
    _qpsk_2k = make_gr_demod_qpsk(125,INTERNAL_DEFAULT_SAMPLE_RATE,1700,1300);
    _qpsk_10k = make_gr_demod_qpsk(25,INTERNAL_DEFAULT_SAMPLE_RATE,1700,6500);
    _qpsk_250k = make_gr_demod_qpsk(2,INTERNAL_DEFAULT_SAMPLE_RATE,1700,160000);
    _qpsk_video = make_gr_demod_qpsk(2,INTERNAL_DEFAULT_SAMPLE_RATE,1700,160000);
    _4fsk_96k = make_gr_demod_4fsk(2,INTERNAL_DEFAULT_SAMPLE_RATE,1700,125000, true);
    _usb = make_gr_demod_ssb(125, INTERNAL_DEFAULT_SAMPLE_RATE,1700,2700,0);
    _lsb = make_gr_demod_ssb(125, INTERNAL_DEFAULT_SAMPLE_RATE,1700,2700,1);
    _wfm = make_gr_demod_wbfm(125, INTERNAL_DEFAULT_SAMPLE_RATE,1700,75000);
    _freedv_rx1600_usb = make_gr_demod_freedv(125, INTERNAL_DEFAULT_SAMPLE_RATE, 1700, 2500, 200,
                                              gr::vocoder::freedv_api::MODE_1600, 0);

    ///int version = atoi(gr::minor_version().c_str());
    _freedv_rx700C_usb = make_gr_demod_freedv(125, INTERNAL_DEFAULT_SAMPLE_RATE, 1700, 2400, 600,
                                                  gr::vocoder::freedv_api::MODE_700C, 0);
    _freedv_rx700D_usb = make_gr_demod_freedv(125, INTERNAL_DEFAULT_SAMPLE_RATE, 1700, 2400, 600,
                                                  gr::vocoder::freedv_api::MODE_700D, 0);

    _freedv_rx800XA_usb = make_gr_demod_freedv(125, INTERNAL_DEFAULT_SAMPLE_RATE, 1700, 2500, 0,
                                               gr::vocoder::freedv_api::MODE_800XA, 0);

    _freedv_rx1600_lsb = make_gr_demod_freedv(125, INTERNAL_DEFAULT_SAMPLE_RATE, 1700, 2500,200,
                                              gr::vocoder::freedv_api::MODE_1600, 1);

    _freedv_rx700C_lsb = make_gr_demod_freedv(125, INTERNAL_DEFAULT_SAMPLE_RATE, 1700, 2400, 600,
                                                  gr::vocoder::freedv_api::MODE_700C, 1);
    _freedv_rx700D_lsb = make_gr_demod_freedv(125, INTERNAL_DEFAULT_SAMPLE_RATE, 1700, 2400, 600,
                                                  gr::vocoder::freedv_api::MODE_700D, 1);
    _freedv_rx800XA_lsb = make_gr_demod_freedv(125, INTERNAL_DEFAULT_SAMPLE_RATE, 1700, 2500, 0,
                                               gr::vocoder::freedv_api::MODE_800XA, 1);
    _mmdvm_demod = make_gr_demod_mmdvm();
    _mmdvm_demod_multi = make_gr_demod_mmdvm_multi(burst_timer, _mmdvm_channels, mmdvm_channel_separation, _use_tdma);
    _m17_demod = make_gr_demod_m17();
}

gr_demod_base::~gr_demod_base()
{
    _top_block->disconnect_all();
    if(_lime_specific)
        _limesdr_source.reset();
    else if(_uhd_specific)
        _uhd_source.reset();
    else
        _osmosdr_source.reset();
}

const QMap<std::string,QVector<int>> gr_demod_base::get_gain_names() const
{
    QMap<std::string,QVector<int>> gain_names;
    if(_lime_specific)
        return gain_names;
    if(_uhd_specific)
    {
        for(unsigned int i=0;i<_gain_names.size();i++)
        {
            QVector<int> gains;
            uhd::gain_range_t gain_range = _uhd_source->get_gain_range(_gain_names.at(i));
            int gain_min = gain_range.start();
            int gain_max = gain_range.stop();
            gains.push_back(gain_min);
            gains.push_back(gain_max);
            gain_names[_gain_names.at(i)] = gains;
        }
        return gain_names;
    }
    for(unsigned int i=0;i<_gain_names.size();i++)
    {
        QVector<int> gains;
        osmosdr::gain_range_t gain_range = _osmosdr_source->get_gain_range(_gain_names.at(i));
        int gain_min = gain_range.start();
        int gain_max = gain_range.stop();
        gains.push_back(gain_min);
        gains.push_back(gain_max);
        gain_names[_gain_names.at(i)] = gains;
    }
    return gain_names;
}

void gr_demod_base::set_mode(int mode, bool disconnect, bool connect)
{
    _demod_running = false;
    if(!_locked)
        _top_block->lock();

    _deframer_700_1->flush();
    _deframer_700_2->flush();
    _deframer1->flush();
    _deframer2->flush();
    _audio_sink->flush();
    _bit_sink->flush();
    if(disconnect)
    {
        switch(_mode)
        {
        case gr_modem_types::ModemType2FSK2KFM:
            _top_block->disconnect(_demod_valve,0,_2fsk_2k_fm,0);
            _top_block->disconnect(_2fsk_2k_fm,0,_rssi_valve,0);
            _top_block->disconnect(_2fsk_2k_fm,1,_const_valve,0);
            _top_block->disconnect(_const_valve,0,_constellation,0);
            _top_block->disconnect(_2fsk_2k_fm,2,_deframer1,0);
            _top_block->disconnect(_2fsk_2k_fm,3,_deframer2,0);
            break;
        case gr_modem_types::ModemType2FSK1KFM:
            _top_block->disconnect(_demod_valve,0,_2fsk_1k_fm,0);
            _top_block->disconnect(_2fsk_1k_fm,0,_rssi_valve,0);
            _top_block->disconnect(_2fsk_1k_fm,1,_const_valve,0);
            _top_block->disconnect(_const_valve,0,_constellation,0);
            _top_block->disconnect(_2fsk_1k_fm,2,_deframer_700_1,0);
            _top_block->disconnect(_2fsk_1k_fm,3,_deframer_700_2,0);
            break;
        case gr_modem_types::ModemType2FSK2K:
            _top_block->disconnect(_demod_valve,0,_2fsk_2k,0);
            _top_block->disconnect(_2fsk_2k,0,_rssi_valve,0);
            _top_block->disconnect(_2fsk_2k,1,_const_valve,0);
            _top_block->disconnect(_const_valve,0,_constellation,0);
            _top_block->disconnect(_2fsk_2k,2,_deframer1,0);
            _top_block->disconnect(_2fsk_2k,3,_deframer2,0);
            break;
        case gr_modem_types::ModemType2FSK1K:
            _top_block->disconnect(_demod_valve,0,_2fsk_1k,0);
            _top_block->disconnect(_2fsk_1k,0,_rssi_valve,0);
            _top_block->disconnect(_2fsk_1k,1,_const_valve,0);
            _top_block->disconnect(_const_valve,0,_constellation,0);
            _top_block->disconnect(_2fsk_1k,2,_deframer_700_1,0);
            _top_block->disconnect(_2fsk_1k,3,_deframer_700_2,0);
            break;
        case gr_modem_types::ModemType2FSK10KFM:
            _top_block->disconnect(_demod_valve,0,_2fsk_10k,0);
            _top_block->disconnect(_2fsk_10k,0,_rssi_valve,0);
            _top_block->disconnect(_2fsk_10k,1,_const_valve,0);
            _top_block->disconnect(_const_valve,0,_constellation,0);
            _top_block->disconnect(_2fsk_10k,2,_deframer1_10k,0);
            _top_block->disconnect(_2fsk_10k,3,_deframer2_10k,0);
            break;
        case gr_modem_types::ModemTypeGMSK2K:
            _top_block->disconnect(_demod_valve,0,_gmsk_2k,0);
            _top_block->disconnect(_gmsk_2k,0,_rssi_valve,0);
            _top_block->disconnect(_gmsk_2k,1,_const_valve,0);
            _top_block->disconnect(_const_valve,0,_constellation,0);
            _top_block->disconnect(_gmsk_2k,2,_deframer1,0);
            _top_block->disconnect(_gmsk_2k,3,_deframer2,0);
            break;
        case gr_modem_types::ModemTypeGMSK1K:
            _top_block->disconnect(_demod_valve,0,_gmsk_1k,0);
            _top_block->disconnect(_gmsk_1k,0,_rssi_valve,0);
            _top_block->disconnect(_gmsk_1k,1,_const_valve,0);
            _top_block->disconnect(_const_valve,0,_constellation,0);
            _top_block->disconnect(_gmsk_1k,2,_deframer_700_1,0);
            _top_block->disconnect(_gmsk_1k,3,_deframer_700_2,0);
            break;
        case gr_modem_types::ModemTypeGMSK10K:
            _top_block->disconnect(_demod_valve,0,_gmsk_10k,0);
            _top_block->disconnect(_gmsk_10k,0,_rssi_valve,0);
            _top_block->disconnect(_gmsk_10k,1,_const_valve,0);
            _top_block->disconnect(_const_valve,0,_constellation,0);
            _top_block->disconnect(_gmsk_10k,2,_deframer1_10k,0);
            _top_block->disconnect(_gmsk_10k,3,_deframer2_10k,0);
            break;
        case gr_modem_types::ModemType4FSK2K:
            _top_block->disconnect(_demod_valve,0,_4fsk_2k,0);
            _top_block->disconnect(_4fsk_2k,0,_rssi_valve,0);
            _top_block->disconnect(_4fsk_2k,1,_const_valve,0);
            _top_block->disconnect(_const_valve,0,_constellation,0);
            _top_block->disconnect(_4fsk_2k,2,_bit_sink,0);
            break;
        case gr_modem_types::ModemType4FSK2KFM:
            _top_block->disconnect(_demod_valve,0,_4fsk_2k_fm,0);
            _top_block->disconnect(_4fsk_2k_fm,0,_rssi_valve,0);
            _top_block->disconnect(_4fsk_2k_fm,1,_const_valve,0);
            _top_block->disconnect(_const_valve,0,_constellation,0);
            _top_block->disconnect(_4fsk_2k_fm,2,_bit_sink,0);
            break;
        case gr_modem_types::ModemType4FSK1KFM:
            _top_block->disconnect(_demod_valve,0,_4fsk_1k_fm,0);
            _top_block->disconnect(_4fsk_1k_fm,0,_rssi_valve,0);
            _top_block->disconnect(_4fsk_1k_fm,1,_const_valve,0);
            _top_block->disconnect(_const_valve,0,_constellation,0);
            _top_block->disconnect(_4fsk_1k_fm,2,_bit_sink,0);
            break;
        case gr_modem_types::ModemType4FSK10KFM:
            _top_block->disconnect(_demod_valve,0,_4fsk_10k_fm,0);
            _top_block->disconnect(_4fsk_10k_fm,0,_rssi_valve,0);
            _top_block->disconnect(_4fsk_10k_fm,1,_const_valve,0);
            _top_block->disconnect(_const_valve,0,_constellation,0);
            _top_block->disconnect(_4fsk_10k_fm,2,_bit_sink,0);
            break;
        case gr_modem_types::ModemTypeAM5000:
            _top_block->disconnect(_demod_valve,0,_am,0);
            _top_block->disconnect(_am,0,_rssi_valve,0);
            _top_block->disconnect(_am,1,_audio_sink,0);
            break;
        case gr_modem_types::ModemTypeBPSK1K:
            _top_block->disconnect(_demod_valve,0,_bpsk_1k,0);
            _top_block->disconnect(_bpsk_1k,0,_rssi_valve,0);
            _top_block->disconnect(_bpsk_1k,1,_const_valve,0);
            _top_block->disconnect(_const_valve,0,_constellation,0);
            _top_block->disconnect(_bpsk_1k,2,_deframer_700_1,0);
            _top_block->disconnect(_bpsk_1k,3,_deframer_700_2,0);
            break;
        case gr_modem_types::ModemTypeBPSK2K:
            _top_block->disconnect(_demod_valve,0,_bpsk_2k,0);
            _top_block->disconnect(_bpsk_2k,0,_rssi_valve,0);
            _top_block->disconnect(_bpsk_2k,1,_const_valve,0);
            _top_block->disconnect(_const_valve,0,_constellation,0);
            _top_block->disconnect(_bpsk_2k,2,_deframer1,0);
            _top_block->disconnect(_bpsk_2k,3,_deframer2,0);
            break;
        case gr_modem_types::ModemTypeBPSK8:
            _top_block->disconnect(_demod_valve,0,_bpsk_dsss_8,0);
            _top_block->disconnect(_bpsk_dsss_8,0,_rssi_valve,0);
            _top_block->disconnect(_bpsk_dsss_8,1,_const_valve,0);
            _top_block->disconnect(_const_valve,0,_constellation,0);
            _top_block->disconnect(_bpsk_dsss_8,2,_deframer1,0);
            _top_block->disconnect(_bpsk_dsss_8,3,_deframer2,0);
            break;
        case gr_modem_types::ModemTypeNBFM2500:
            _top_block->disconnect(_demod_valve,0,_fm_2500,0);
            _top_block->disconnect(_fm_2500,0,_rssi_valve,0);
            _top_block->disconnect(_fm_2500,1,_audio_sink,0);
            break;
        case gr_modem_types::ModemTypeNBFM5000:
            _top_block->disconnect(_demod_valve,0,_fm_5000,0);
            _top_block->disconnect(_fm_5000,0,_rssi_valve,0);
            _top_block->disconnect(_fm_5000,1,_audio_sink,0);
            break;
        case gr_modem_types::ModemTypeQPSK2K:
            _top_block->disconnect(_demod_valve,0,_qpsk_2k,0);
            _top_block->disconnect(_qpsk_2k,0,_rssi_valve,0);
            _top_block->disconnect(_qpsk_2k,1,_const_valve,0);
            _top_block->disconnect(_const_valve,0,_constellation,0);
            _top_block->disconnect(_qpsk_2k,2,_bit_sink,0);
            break;
        case gr_modem_types::ModemTypeQPSK20K:
            _top_block->disconnect(_demod_valve,0,_qpsk_10k,0);
            _top_block->disconnect(_qpsk_10k,0,_rssi_valve,0);
            _top_block->disconnect(_qpsk_10k,1,_const_valve,0);
            _top_block->disconnect(_const_valve,0,_constellation,0);
            _top_block->disconnect(_qpsk_10k,2,_bit_sink,0);
            break;
        case gr_modem_types::ModemTypeQPSK250K:
            _top_block->disconnect(_demod_valve,0,_qpsk_250k,0);
            _top_block->disconnect(_qpsk_250k,0,_rssi_valve,0);
            _top_block->disconnect(_qpsk_250k,1,_const_valve,0);
            _top_block->disconnect(_const_valve,0,_constellation,0);
            _top_block->disconnect(_qpsk_250k,2,_bit_sink,0);
            //_carrier_offset = 25000;
            //_osmosdr_source->set_center_freq(_device_frequency - _carrier_offset);
            break;
        case gr_modem_types::ModemType4FSK100K:
            _top_block->disconnect(_demod_valve,0,_4fsk_96k,0);
            _top_block->disconnect(_4fsk_96k,0,_rssi_valve,0);
            _top_block->disconnect(_4fsk_96k,1,_const_valve,0);
            _top_block->disconnect(_const_valve,0,_constellation,0);
            _top_block->disconnect(_4fsk_96k,2,_bit_sink,0);
            break;
        case gr_modem_types::ModemTypeQPSKVideo:
            _top_block->disconnect(_demod_valve,0,_qpsk_video,0);
            _top_block->disconnect(_qpsk_video,0,_rssi_valve,0);
            _top_block->disconnect(_qpsk_video,1,_const_valve,0);
            _top_block->disconnect(_const_valve,0,_constellation,0);
            _top_block->disconnect(_qpsk_video,2,_bit_sink,0);
            break;
        case gr_modem_types::ModemTypeUSB2500:
            _top_block->disconnect(_demod_valve,0,_usb,0);
            _top_block->disconnect(_usb,0,_rssi_valve,0);
            _top_block->disconnect(_usb,1,_audio_sink,0);
            break;
        case gr_modem_types::ModemTypeLSB2500:
            _top_block->disconnect(_demod_valve,0,_lsb,0);
            _top_block->disconnect(_lsb,0,_rssi_valve,0);
            _top_block->disconnect(_lsb,1,_audio_sink,0);
            break;
        case gr_modem_types::ModemTypeFREEDV1600USB:
            _top_block->disconnect(_demod_valve,0,_freedv_rx1600_usb,0);
            _top_block->disconnect(_freedv_rx1600_usb,0,_rssi_valve,0);
            _top_block->disconnect(_freedv_rx1600_usb,1,_audio_sink,0);
            break;
        case gr_modem_types::ModemTypeFREEDV700CUSB:
            _top_block->disconnect(_demod_valve,0,_freedv_rx700C_usb,0);
            _top_block->disconnect(_freedv_rx700C_usb,0,_rssi_valve,0);
            _top_block->disconnect(_freedv_rx700C_usb,1,_audio_sink,0);
            break;
        case gr_modem_types::ModemTypeFREEDV700DUSB:
            _top_block->disconnect(_demod_valve,0,_freedv_rx700D_usb,0);
            _top_block->disconnect(_freedv_rx700D_usb,0,_rssi_valve,0);
            _top_block->disconnect(_freedv_rx700D_usb,1,_audio_sink,0);
            break;
        case gr_modem_types::ModemTypeFREEDV800XAUSB:
            _top_block->disconnect(_demod_valve,0,_freedv_rx800XA_usb,0);
            _top_block->disconnect(_freedv_rx800XA_usb,0,_rssi_valve,0);
            _top_block->disconnect(_freedv_rx800XA_usb,1,_audio_sink,0);
            break;
        case gr_modem_types::ModemTypeFREEDV1600LSB:
            _top_block->disconnect(_demod_valve,0,_freedv_rx1600_lsb,0);
            _top_block->disconnect(_freedv_rx1600_lsb,0,_rssi_valve,0);
            _top_block->disconnect(_freedv_rx1600_lsb,1,_audio_sink,0);
            break;
        case gr_modem_types::ModemTypeFREEDV700CLSB:
            _top_block->disconnect(_demod_valve,0,_freedv_rx700C_lsb,0);
            _top_block->disconnect(_freedv_rx700C_lsb,0,_rssi_valve,0);
            _top_block->disconnect(_freedv_rx700C_lsb,1,_audio_sink,0);
            break;
        case gr_modem_types::ModemTypeFREEDV700DLSB:
            _top_block->disconnect(_demod_valve,0,_freedv_rx700D_lsb,0);
            _top_block->disconnect(_freedv_rx700D_lsb,0,_rssi_valve,0);
            _top_block->disconnect(_freedv_rx700D_lsb,1,_audio_sink,0);
            break;
        case gr_modem_types::ModemTypeFREEDV800XALSB:
            _top_block->disconnect(_demod_valve,0,_freedv_rx800XA_lsb,0);
            _top_block->disconnect(_freedv_rx800XA_lsb,0,_rssi_valve,0);
            _top_block->disconnect(_freedv_rx800XA_lsb,1,_audio_sink,0);
            break;
        case gr_modem_types::ModemTypeWBFM:
            _top_block->disconnect(_demod_valve,0,_wfm,0);
            _top_block->disconnect(_wfm,0,_rssi_valve,0);
            _top_block->disconnect(_wfm,1,_audio_sink,0);
            break;
        case gr_modem_types::ModemTypeMMDVM:
            _top_block->disconnect(_demod_valve,0,_mmdvm_demod,0);
            _top_block->disconnect(_demod_valve,0,_rssi_valve,0);
            _top_block->disconnect(_mmdvm_demod,0,_mmdvm_sink,0);
            break;
        case gr_modem_types::ModemTypeMMDVMmulti:
            _top_block->disconnect(_demod_valve,0,_mmdvm_demod_multi,0);
            _top_block->disconnect(_demod_valve,0,_rssi_valve,0);
            break;
        case gr_modem_types::ModemTypeM17:
            _top_block->disconnect(_demod_valve,0,_m17_demod,0);
            _top_block->disconnect(_m17_demod,0,_rssi_valve,0);
            _top_block->disconnect(_m17_demod,1,_const_valve,0);
            _top_block->disconnect(_const_valve,0,_constellation,0);
            _top_block->disconnect(_m17_demod,2,_bit_sink,0);
            break;
        default:
            break;
        }
    }

    if(connect)
    {
        switch(mode)
        {
        case gr_modem_types::ModemType2FSK2KFM:
            _top_block->connect(_demod_valve,0,_2fsk_2k_fm,0);
            _top_block->connect(_2fsk_2k_fm,0,_rssi_valve,0);
            _top_block->connect(_2fsk_2k_fm,1,_const_valve,0);
            _top_block->connect(_const_valve,0,_constellation,0);
            _top_block->connect(_2fsk_2k_fm,2,_deframer1,0);
            _top_block->connect(_2fsk_2k_fm,3,_deframer2,0);
            break;
        case gr_modem_types::ModemType2FSK1KFM:
            _top_block->connect(_demod_valve,0,_2fsk_1k_fm,0);
            _top_block->connect(_2fsk_1k_fm,0,_rssi_valve,0);
            _top_block->connect(_2fsk_1k_fm,1,_const_valve,0);
            _top_block->connect(_const_valve,0,_constellation,0);
            _top_block->connect(_2fsk_1k_fm,2,_deframer_700_1,0);
            _top_block->connect(_2fsk_1k_fm,3,_deframer_700_2,0);
            break;
        case gr_modem_types::ModemType2FSK2K:
            _top_block->connect(_demod_valve,0,_2fsk_2k,0);
            _top_block->connect(_2fsk_2k,0,_rssi_valve,0);
            _top_block->connect(_2fsk_2k,1,_const_valve,0);
            _top_block->connect(_const_valve,0,_constellation,0);
            _top_block->connect(_2fsk_2k,2,_deframer1,0);
            _top_block->connect(_2fsk_2k,3,_deframer2,0);
            break;
        case gr_modem_types::ModemType2FSK1K:
            _top_block->connect(_demod_valve,0,_2fsk_1k,0);
            _top_block->connect(_2fsk_1k,0,_rssi_valve,0);
            _top_block->connect(_2fsk_1k,1,_const_valve,0);
            _top_block->connect(_const_valve,0,_constellation,0);
            _top_block->connect(_2fsk_1k,2,_deframer_700_1,0);
            _top_block->connect(_2fsk_1k,3,_deframer_700_2,0);
            break;
        case gr_modem_types::ModemType2FSK10KFM:
            _top_block->connect(_demod_valve,0,_2fsk_10k,0);
            _top_block->connect(_2fsk_10k,0,_rssi_valve,0);
            _top_block->connect(_2fsk_10k,1,_const_valve,0);
            _top_block->connect(_const_valve,0,_constellation,0);
            _top_block->connect(_2fsk_10k,2,_deframer1_10k,0);
            _top_block->connect(_2fsk_10k,3,_deframer2_10k,0);
            break;
        case gr_modem_types::ModemTypeGMSK2K:
            _top_block->connect(_demod_valve,0,_gmsk_2k,0);
            _top_block->connect(_gmsk_2k,0,_rssi_valve,0);
            _top_block->connect(_gmsk_2k,1,_const_valve,0);
            _top_block->connect(_const_valve,0,_constellation,0);
            _top_block->connect(_gmsk_2k,2,_deframer1,0);
            _top_block->connect(_gmsk_2k,3,_deframer2,0);
            break;
        case gr_modem_types::ModemTypeGMSK1K:
            _top_block->connect(_demod_valve,0,_gmsk_1k,0);
            _top_block->connect(_gmsk_1k,0,_rssi_valve,0);
            _top_block->connect(_gmsk_1k,1,_const_valve,0);
            _top_block->connect(_const_valve,0,_constellation,0);
            _top_block->connect(_gmsk_1k,2,_deframer_700_1,0);
            _top_block->connect(_gmsk_1k,3,_deframer_700_2,0);
            break;
        case gr_modem_types::ModemTypeGMSK10K:
            _top_block->connect(_demod_valve,0,_gmsk_10k,0);
            _top_block->connect(_gmsk_10k,0,_rssi_valve,0);
            _top_block->connect(_gmsk_10k,1,_const_valve,0);
            _top_block->connect(_const_valve,0,_constellation,0);
            _top_block->connect(_gmsk_10k,2,_deframer1_10k,0);
            _top_block->connect(_gmsk_10k,3,_deframer2_10k,0);
            break;
        case gr_modem_types::ModemType4FSK2K:
            _top_block->connect(_demod_valve,0,_4fsk_2k,0);
            _top_block->connect(_4fsk_2k,0,_rssi_valve,0);
            _top_block->connect(_4fsk_2k,1,_const_valve,0);
            _top_block->connect(_const_valve,0,_constellation,0);
            _top_block->connect(_4fsk_2k,2,_bit_sink,0);
            break;
        case gr_modem_types::ModemType4FSK2KFM:
            _top_block->connect(_demod_valve,0,_4fsk_2k_fm,0);
            _top_block->connect(_4fsk_2k_fm,0,_rssi_valve,0);
            _top_block->connect(_4fsk_2k_fm,1,_const_valve,0);
            _top_block->connect(_const_valve,0,_constellation,0);
            _top_block->connect(_4fsk_2k_fm,2,_bit_sink,0);
            break;
        case gr_modem_types::ModemType4FSK1KFM:
            _top_block->connect(_demod_valve,0,_4fsk_1k_fm,0);
            _top_block->connect(_4fsk_1k_fm,0,_rssi_valve,0);
            _top_block->connect(_4fsk_1k_fm,1,_const_valve,0);
            _top_block->connect(_const_valve,0,_constellation,0);
            _top_block->connect(_4fsk_1k_fm,2,_bit_sink,0);
            break;
        case gr_modem_types::ModemType4FSK10KFM:
            _top_block->connect(_demod_valve,0,_4fsk_10k_fm,0);
            _top_block->connect(_4fsk_10k_fm,0,_rssi_valve,0);
            _top_block->connect(_4fsk_10k_fm,1,_const_valve,0);
            _top_block->connect(_const_valve,0,_constellation,0);
            _top_block->connect(_4fsk_10k_fm,2,_bit_sink,0);
            break;
        case gr_modem_types::ModemTypeAM5000:
            _top_block->connect(_demod_valve,0,_am,0);
            _top_block->connect(_am,0,_rssi_valve,0);
            _top_block->connect(_am,1,_audio_sink,0);
            break;
        case gr_modem_types::ModemTypeBPSK1K:
            _top_block->connect(_demod_valve,0,_bpsk_1k,0);
            _top_block->connect(_bpsk_1k,0,_rssi_valve,0);
            _top_block->connect(_bpsk_1k,1,_const_valve,0);
            _top_block->connect(_const_valve,0,_constellation,0);
            _top_block->connect(_bpsk_1k,2,_deframer_700_1,0);
            _top_block->connect(_bpsk_1k,3,_deframer_700_2,0);
            break;
        case gr_modem_types::ModemTypeBPSK2K:
            _top_block->connect(_demod_valve,0,_bpsk_2k,0);
            _top_block->connect(_bpsk_2k,0,_rssi_valve,0);
            _top_block->connect(_bpsk_2k,1,_const_valve,0);
            _top_block->connect(_const_valve,0,_constellation,0);
            _top_block->connect(_bpsk_2k,2,_deframer1,0);
            _top_block->connect(_bpsk_2k,3,_deframer2,0);
            break;
        case gr_modem_types::ModemTypeBPSK8:
            _top_block->connect(_demod_valve,0,_bpsk_dsss_8,0);
            _top_block->connect(_bpsk_dsss_8,0,_rssi_valve,0);
            _top_block->connect(_bpsk_dsss_8,1,_const_valve,0);
            _top_block->connect(_const_valve,0,_constellation,0);
            _top_block->connect(_bpsk_dsss_8,2,_deframer1,0);
            _top_block->connect(_bpsk_dsss_8,3,_deframer2,0);
            break;
        case gr_modem_types::ModemTypeNBFM2500:
            _top_block->connect(_demod_valve,0,_fm_2500,0);
            _top_block->connect(_fm_2500,0,_rssi_valve,0);
            _top_block->connect(_fm_2500,1,_audio_sink,0);
            break;
        case gr_modem_types::ModemTypeNBFM5000:
            _top_block->connect(_demod_valve,0,_fm_5000,0);
            _top_block->connect(_fm_5000,0,_rssi_valve,0);
            _top_block->connect(_fm_5000,1,_audio_sink,0);
            break;
        case gr_modem_types::ModemTypeQPSK2K:
            _top_block->connect(_demod_valve,0,_qpsk_2k,0);
            _top_block->connect(_qpsk_2k,0,_rssi_valve,0);
            _top_block->connect(_qpsk_2k,1,_const_valve,0);
            _top_block->connect(_const_valve,0,_constellation,0);
            _top_block->connect(_qpsk_2k,2,_bit_sink,0);
            break;
        case gr_modem_types::ModemTypeQPSK20K:
            _top_block->connect(_demod_valve,0,_qpsk_10k,0);
            _top_block->connect(_qpsk_10k,0,_rssi_valve,0);
            _top_block->connect(_qpsk_10k,1,_const_valve,0);
            _top_block->connect(_const_valve,0,_constellation,0);
            _top_block->connect(_qpsk_10k,2,_bit_sink,0);
            break;
        case gr_modem_types::ModemTypeQPSK250K:
            _top_block->connect(_demod_valve,0,_qpsk_250k,0);
            _top_block->connect(_qpsk_250k,0,_rssi_valve,0);
            _top_block->connect(_qpsk_250k,1,_const_valve,0);
            _top_block->connect(_const_valve,0,_constellation,0);
            _top_block->connect(_qpsk_250k,2,_bit_sink,0);
            break;
        case gr_modem_types::ModemType4FSK100K:
            _top_block->connect(_demod_valve,0,_4fsk_96k,0);
            _top_block->connect(_4fsk_96k,0,_rssi_valve,0);
            _top_block->connect(_4fsk_96k,1,_const_valve,0);
            _top_block->connect(_const_valve,0,_constellation,0);
            _top_block->connect(_4fsk_96k,2,_bit_sink,0);
            break;
        case gr_modem_types::ModemTypeQPSKVideo:
            _top_block->connect(_demod_valve,0,_qpsk_video,0);
            _top_block->connect(_qpsk_video,0,_rssi_valve,0);
            _top_block->connect(_qpsk_video,1,_const_valve,0);
            _top_block->connect(_const_valve,0,_constellation,0);
            _top_block->connect(_qpsk_video,2,_bit_sink,0);
            break;
        case gr_modem_types::ModemTypeUSB2500:
            _top_block->connect(_demod_valve,0,_usb,0);
            _top_block->connect(_usb,0,_rssi_valve,0);
            _top_block->connect(_usb,1,_audio_sink,0);
            break;
        case gr_modem_types::ModemTypeLSB2500:
            _top_block->connect(_demod_valve,0,_lsb,0);
            _top_block->connect(_lsb,0,_rssi_valve,0);
            _top_block->connect(_lsb,1,_audio_sink,0);
            break;
        case gr_modem_types::ModemTypeFREEDV1600USB:
            _top_block->connect(_demod_valve,0,_freedv_rx1600_usb,0);
            _top_block->connect(_freedv_rx1600_usb,0,_rssi_valve,0);
            _top_block->connect(_freedv_rx1600_usb,1,_audio_sink,0);
            break;
        case gr_modem_types::ModemTypeFREEDV700CUSB:
            _top_block->connect(_demod_valve,0,_freedv_rx700C_usb,0);
            _top_block->connect(_freedv_rx700C_usb,0,_rssi_valve,0);
            _top_block->connect(_freedv_rx700C_usb,1,_audio_sink,0);
            break;
        case gr_modem_types::ModemTypeFREEDV700DUSB:
            _top_block->connect(_demod_valve,0,_freedv_rx700D_usb,0);
            _top_block->connect(_freedv_rx700D_usb,0,_rssi_valve,0);
            _top_block->connect(_freedv_rx700D_usb,1,_audio_sink,0);
            break;
        case gr_modem_types::ModemTypeFREEDV800XAUSB:
            _top_block->connect(_demod_valve,0,_freedv_rx800XA_usb,0);
            _top_block->connect(_freedv_rx800XA_usb,0,_rssi_valve,0);
            _top_block->connect(_freedv_rx800XA_usb,1,_audio_sink,0);
            break;
        case gr_modem_types::ModemTypeFREEDV1600LSB:
            _top_block->connect(_demod_valve,0,_freedv_rx1600_lsb,0);
            _top_block->connect(_freedv_rx1600_lsb,0,_rssi_valve,0);
            _top_block->connect(_freedv_rx1600_lsb,1,_audio_sink,0);
            break;
        case gr_modem_types::ModemTypeFREEDV700CLSB:
            _top_block->connect(_demod_valve,0,_freedv_rx700C_lsb,0);
            _top_block->connect(_freedv_rx700C_lsb,0,_rssi_valve,0);
            _top_block->connect(_freedv_rx700C_lsb,1,_audio_sink,0);
            break;
        case gr_modem_types::ModemTypeFREEDV700DLSB:
            _top_block->connect(_demod_valve,0,_freedv_rx700D_lsb,0);
            _top_block->connect(_freedv_rx700D_lsb,0,_rssi_valve,0);
            _top_block->connect(_freedv_rx700D_lsb,1,_audio_sink,0);
            break;
        case gr_modem_types::ModemTypeFREEDV800XALSB:
            _top_block->connect(_demod_valve,0,_freedv_rx800XA_lsb,0);
            _top_block->connect(_freedv_rx800XA_lsb,0,_rssi_valve,0);
            _top_block->connect(_freedv_rx800XA_lsb,1,_audio_sink,0);
            break;
        case gr_modem_types::ModemTypeWBFM:
            _top_block->connect(_demod_valve,0,_wfm,0);
            _top_block->connect(_wfm,0,_rssi_valve,0);
            _top_block->connect(_wfm,1,_audio_sink,0);
            break;
        case gr_modem_types::ModemTypeMMDVM:
            _top_block->connect(_demod_valve,0,_mmdvm_demod,0);
            _top_block->connect(_demod_valve,0,_rssi_valve,0);
            _top_block->connect(_mmdvm_demod,0,_mmdvm_sink,0);
            break;
        case gr_modem_types::ModemTypeMMDVMmulti:
            _top_block->connect(_demod_valve,0,_mmdvm_demod_multi,0);
            _top_block->connect(_demod_valve,0,_rssi_valve,0);
            break;
        case gr_modem_types::ModemTypeM17:
            _top_block->connect(_demod_valve,0,_m17_demod,0);
            _top_block->connect(_m17_demod,0,_rssi_valve,0);
            _top_block->connect(_m17_demod,1,_const_valve,0);
            _top_block->connect(_const_valve,0,_constellation,0);
            _top_block->connect(_m17_demod,2,_bit_sink,0);
            break;

        default:
            break;
        }
        _mode = mode;
    }

    if(!_locked)
        _top_block->unlock();
    _demod_running = true;
}

void gr_demod_base::start(int buffer_size)
{
    _audio_sink->flush();
    _bit_sink->flush();
    if(buffer_size)
        _top_block->start(buffer_size);
    else // automatic
        _top_block->start();
}

void gr_demod_base::stop()
{
    _audio_sink->flush();
    _bit_sink->flush();
    _top_block->stop();
    _top_block->wait();
}

std::vector<unsigned char>* gr_demod_base::getData(int nr)
{
    if(!_demod_running)
    {
        return nullptr;
    }
    std::vector<unsigned char> *data = nullptr;
    if(nr == 1)
    {
        switch(_mode)
        {
        case gr_modem_types::ModemType2FSK2KFM:
            data = _deframer1->get_data();
            break;
        case gr_modem_types::ModemType2FSK1KFM:
            data = _deframer_700_1->get_data();
            break;
        case gr_modem_types::ModemType2FSK2K:
            data = _deframer1->get_data();
            break;
        case gr_modem_types::ModemType2FSK1K:
            data = _deframer_700_1->get_data();
            break;
        case gr_modem_types::ModemType2FSK10KFM:
            data = _deframer1_10k->get_data();
            break;
        case gr_modem_types::ModemTypeGMSK2K:
            data = _deframer1->get_data();
            break;
        case gr_modem_types::ModemTypeGMSK1K:
            data = _deframer_700_1->get_data();
            break;
        case gr_modem_types::ModemTypeGMSK10K:
            data = _deframer1_10k->get_data();
            break;
        case gr_modem_types::ModemTypeBPSK1K:
            data = _deframer_700_1->get_data();
            break;
        case gr_modem_types::ModemTypeBPSK2K:
            data = _deframer1->get_data();
            break;
        case gr_modem_types::ModemTypeBPSK8:
            data = _deframer1->get_data();
            break;
        }
    }
    else if(nr == 2)
    {
        switch(_mode)
        {
        case gr_modem_types::ModemType2FSK2KFM:
            data = _deframer2->get_data();
            break;
        case gr_modem_types::ModemType2FSK1KFM:
            data = _deframer_700_2->get_data();
            break;
        case gr_modem_types::ModemType2FSK2K:
            data = _deframer2->get_data();
            break;
        case gr_modem_types::ModemType2FSK1K:
            data = _deframer_700_2->get_data();
            break;
        case gr_modem_types::ModemType2FSK10KFM:
            data = _deframer2_10k->get_data();
            break;
        case gr_modem_types::ModemTypeGMSK2K:
            data = _deframer2->get_data();
            break;
        case gr_modem_types::ModemTypeGMSK1K:
            data = _deframer_700_2->get_data();
            break;
        case gr_modem_types::ModemTypeGMSK10K:
            data = _deframer2_10k->get_data();
            break;
        case gr_modem_types::ModemTypeBPSK1K:
            data = _deframer_700_2->get_data();
            break;
        case gr_modem_types::ModemTypeBPSK2K:
            data = _deframer2->get_data();
            break;
        case gr_modem_types::ModemTypeBPSK8:
            data = _deframer2->get_data();
            break;
        }
    }
    return data;
}


std::vector<unsigned char>* gr_demod_base::getData()
{
    if(!_demod_running)
    {
        return nullptr;
    }
    std::vector<unsigned char> *data = _bit_sink->get_data();
    return data;
}

std::vector<float>* gr_demod_base::getAudio()
{
    if(!_demod_running)
    {
        return nullptr;
    }
    std::vector<float> *data = _audio_sink->get_data();
    return data;
}

void gr_demod_base::get_FFT_data(float *fft_data,  unsigned int &fftSize)
{
    if(!_demod_running)
    {
        return;
    }
    _fft_sink->get_fft_data(fft_data, fftSize);
    return;
}

void gr_demod_base::get_sample_data(float *sample_data,  unsigned int &size)
{
    if(!_demod_running)
    {
        size = 0;
        return;
    }
    std::vector<gr_complex> *data = _sample_sink->get_data();
    if(data == nullptr)
    {
        size = 0;
        return;
    }
    uint i;
    for(i = 0;i<data->size();i++)
    {
        sample_data[i] = data->at(i).real();
    }
    for(uint j = 0;j<data->size();j++)
    {
        sample_data[i + j + 1] = data->at(j).imag();
    }
    size = data->size() * 2;
    data->clear();
    delete data;
    return;
}

void gr_demod_base::set_sample_window(unsigned int size)
{
    _sample_sink->set_sample_window(size);
}

void gr_demod_base::tune(int64_t center_freq)
{
    int64_t steps = center_freq / INTERNAL_DEFAULT_SAMPLE_RATE;
    _device_frequency = center_freq + steps * _freq_correction;
    if(_lime_specific)
        _limesdr_source->set_center_freq(_device_frequency);
    else if(_uhd_specific)
        _uhd_source->set_center_freq(_device_frequency);
    else
        _osmosdr_source->set_center_freq(_device_frequency);
    set_bandwidth_specific();
}

double gr_demod_base::get_freq()
{
    int n = _message_sink->num_messages();
    if(n > _msg_nr)
    {
        _msg_nr = n;
        pmt::pmt_t msg = _message_sink->get_message(n - 1);
        return pmt::to_double(pmt::cdr(msg));
    }
    else
    {
        return 0;
    }
}

void gr_demod_base::set_rx_sensitivity(double value, std::string gain_stage)
{
    if(_lime_specific)
    {
        _limesdr_source->set_gain(int(value * 70.0d));
        return;
    }
    if(_uhd_specific)
    {
        if (!_uhd_gain_range.empty())
        {
            double gain =  (double)_uhd_gain_range.start() + value*(
                        (double)_uhd_gain_range.stop()- (double)_uhd_gain_range.start());
            _uhd_source->set_gain(gain);
        }
        else
        {
            _uhd_source->set_gain(value);
        }
        return;
    }
    if (!_gain_range.empty() && (gain_stage.size() < 1))
    {

        double gain =  floor((double)_gain_range.start() + value*(
                                 (double)_gain_range.stop()- (double)_gain_range.start()));
        _osmosdr_source->set_gain_mode(false); // Pluto ??!!
        if(_gain_names.size() == 1)
        {
            _osmosdr_source->set_gain(gain, _gain_names.at(0));
        }
        else
        {
            _osmosdr_source->set_gain(gain);
        }
    }
    else
    {
        _osmosdr_source->set_gain_mode(false);
        _osmosdr_source->set_gain(value, gain_stage);
    }
}

void gr_demod_base::enable_gui_const(bool value)
{
    _const_valve->set_enabled(value);
}

void gr_demod_base::enable_rssi(bool value)
{
    _rssi_valve->set_enabled(value);
}

void gr_demod_base::enable_gui_fft(bool value)
{
    _fft_sink->set_enabled(value);
}

void gr_demod_base::enable_time_domain(bool value)
{
    _time_domain_enabled = value;
    _top_block->lock();
    _locked = true;
    if(value)
    {
        try
        {
            _top_block->connect(_demod_valve,0, _resampler_time_domain,0);
            _top_block->connect(_resampler_time_domain,0, _sample_sink,0);
        }
        catch(std::invalid_argument &e)
        {

        }
    }
    else
    {
        try
        {
            _top_block->disconnect(_demod_valve,0, _resampler_time_domain,0);
            _top_block->disconnect(_resampler_time_domain,0, _sample_sink,0);
        }
        catch(std::invalid_argument &e)
        {

        }
    }

    _sample_sink->set_enabled(value);
    _top_block->unlock();
    _locked = false;
}

void gr_demod_base::enable_demodulator(bool value)
{
    _demod_valve->set_enabled(value);
}

void gr_demod_base::set_filter_width(int filter_width, int mode)
{

    switch(mode)
    {
    case gr_modem_types::ModemTypeWBFM:
        _wfm->set_filter_width(filter_width);
        break;
    case gr_modem_types::ModemTypeAM5000:
        _am->set_filter_width(filter_width);
        break;
    case gr_modem_types::ModemTypeNBFM2500:
        _fm_2500->set_filter_width(filter_width);
        break;
    case gr_modem_types::ModemTypeNBFM5000:
        _fm_5000->set_filter_width(filter_width);
        break;
    case gr_modem_types::ModemTypeUSB2500:
        _usb->set_filter_width(filter_width);
        break;
    case gr_modem_types::ModemTypeLSB2500:
        _lsb->set_filter_width(filter_width);
        break;
    default:
        break;
    }


}

void gr_demod_base::set_squelch(int value)
{
    _fm_2500->set_squelch(value);
    _fm_5000->set_squelch(value);
    _am->set_squelch(value);
    _usb->set_squelch(value);
    _lsb->set_squelch(value);
    _freedv_rx1600_usb->set_squelch(value);
    _freedv_rx1600_lsb->set_squelch(value);
    _freedv_rx700C_usb->set_squelch(value);
    _freedv_rx700C_lsb->set_squelch(value);
    _freedv_rx700D_usb->set_squelch(value);
    _freedv_rx700D_lsb->set_squelch(value);
    _freedv_rx800XA_usb->set_squelch(value);
    _freedv_rx800XA_lsb->set_squelch(value);
    _wfm->set_squelch(value);
}

void gr_demod_base::set_gain(float value)
{
    _usb->set_gain(value);
    _lsb->set_gain(value);
}

void gr_demod_base::set_ctcss(float value)
{
    _top_block->lock();
    _fm_2500->set_ctcss(value);
    _fm_5000->set_ctcss(value);
    _top_block->unlock();
}

void gr_demod_base::set_carrier_offset(int64_t carrier_offset)
{
    _carrier_offset = carrier_offset;
    _rotator->set_phase_inc(2*M_PI*-_carrier_offset/_samp_rate);

}

void gr_demod_base::set_fft_size(int size)
{
    _top_block->lock();
    _fft_sink->set_fft_size((unsigned int)size);
    _top_block->unlock();
}

float gr_demod_base::get_rssi()
{
    return _rssi->level();
}

std::vector<gr_complex>* gr_demod_base::get_constellation_data()
{
    if(!_demod_running)
    {
        return nullptr;
    }
    std::vector<gr_complex> *data = _constellation->get_data();
    return data;
}

void gr_demod_base::set_time_sink_samp_rate(int samp_rate)
{
    if((uint)samp_rate > INTERNAL_DEFAULT_SAMPLE_RATE)
        return;
    _top_block->lock();
    _locked = true;

    int decimation = INTERNAL_DEFAULT_SAMPLE_RATE / samp_rate;
    if(_time_domain_enabled)
    {
        try
        {
            _top_block->disconnect(_demod_valve,0, _resampler_time_domain,0);
            _top_block->disconnect(_resampler_time_domain,0, _sample_sink,0);
        }
        catch(std::invalid_argument &e)
        {

        }
    }

    _resampler_time_domain.reset();
    std::vector<float> taps;
    taps = gr::filter::firdes::low_pass(1, INTERNAL_DEFAULT_SAMPLE_RATE, samp_rate/2, samp_rate/4,
                                        gr::filter::firdes::WIN_HAMMING);

    _resampler_time_domain = gr::filter::rational_resampler_base_ccf::make(1, decimation, taps);
    if(_time_domain_enabled)
    {
        try
        {
            _top_block->connect(_demod_valve,0, _resampler_time_domain,0);
            _top_block->connect(_resampler_time_domain,0, _sample_sink,0);
        }
        catch(std::invalid_argument &e)
        {

        }
    }
    _top_block->unlock();
    _locked = false;
}

void gr_demod_base::set_samp_rate(int samp_rate)
{
    _top_block->lock();
    _locked = true;
    _samp_rate = samp_rate;

    int decimation = (int) _samp_rate / INTERNAL_DEFAULT_SAMPLE_RATE;

    if(_samp_rate >= 2000000)
    {
        try
        {
            _top_block->disconnect(_rotator,0, _demod_valve,0);
        }
        catch(std::invalid_argument &e)
        {

        }
        try
        {
            _top_block->disconnect(_rotator,0, _resampler,0);
            _top_block->disconnect(_resampler,0, _demod_valve,0);
        }
        catch(std::invalid_argument &e)
        {

        }
        _resampler.reset();
        std::vector<float> taps;
        //int tw = std::min(_samp_rate/4, 1500000);
        taps = gr::filter::firdes::low_pass(1, _samp_rate, 480000, 100000,
                                            gr::filter::firdes::WIN_BLACKMAN_HARRIS);

        _resampler = gr::filter::rational_resampler_base_ccf::make(1, decimation, taps);
        _resampler->set_thread_priority(75);
        _top_block->connect(_rotator,0, _resampler,0);
        _top_block->connect(_resampler,0, _demod_valve,0);
    }
    else
    {
        try
        {
            _top_block->disconnect(_rotator,0, _resampler,0);
            _top_block->disconnect(_resampler,0, _demod_valve,0);
        }
        catch(std::invalid_argument &e)
        {
            _top_block->disconnect(_rotator,0, _demod_valve,0);
        }
        try
        {
            _top_block->connect(_rotator,0, _demod_valve,0);
        }
        catch(std::invalid_argument &e)
        {
        }
    }


    _rotator->set_phase_inc(2*M_PI*-_carrier_offset/_samp_rate);

    if(_lime_specific)
    {
        _limesdr_source->set_center_freq(_device_frequency);
        _limesdr_source->set_sample_rate(_samp_rate);
        _limesdr_source->set_digital_filter(_samp_rate, 0);
        _limesdr_source->calibrate(_samp_rate);
    }
    else if(_uhd_specific)
    {
        _uhd_source->set_center_freq(_device_frequency);
        _uhd_source->set_samp_rate(_samp_rate);
        _uhd_source->set_bandwidth(_samp_rate, 0);
    }
    else
    {
        _osmosdr_source->set_center_freq(_device_frequency);
        _osmosdr_source->set_sample_rate(_samp_rate);
    }
    set_bandwidth_specific();
    _top_block->unlock();
    _locked = false;


}

void gr_demod_base::set_bandwidth_specific()
{
    _osmo_filter_bw = (double)_samp_rate;
    if((_device_frequency < 30 * 1000 * 1000) && _lime_specific)
    {
        _osmo_filter_bw = 40.0 * 1000 * 1000;
    }
    else if(_lime_specific)
    {
        _osmo_filter_bw = (double)(std::max(1500000, _samp_rate));
    }

    if(_lime_specific)
    {
        _limesdr_source->set_bandwidth(_osmo_filter_bw);
    }
    else if(_uhd_specific)
    {
        _uhd_source->set_bandwidth(_osmo_filter_bw);
    }
    else
        _osmosdr_source->set_bandwidth(_osmo_filter_bw);
}

void gr_demod_base::calibrate_rssi(float value)
{
    _rssi_block->set_level(value);
    _mmdvm_demod->calibrate_rssi(value);
    _mmdvm_demod_multi->calibrate_rssi(value);
}

void gr_demod_base::set_agc_attack(int value)
{
    float attack;
    if(value == 0)
        attack = 1.0f;
    if(value < 0)
        attack = 1.0f / (float) -value;
    if(value > 0)
        attack = (float) value * 20.0f;
    _usb->set_agc_attack(attack);
    _lsb->set_agc_attack(attack);
    _am->set_agc_attack(attack);
    _freedv_rx1600_usb->set_agc_attack(attack);
    _freedv_rx1600_lsb->set_agc_attack(attack);
    _freedv_rx700C_usb->set_agc_attack(attack);
    _freedv_rx700C_lsb->set_agc_attack(attack);
    _freedv_rx700D_usb->set_agc_attack(attack);
    _freedv_rx700D_lsb->set_agc_attack(attack);
    _freedv_rx800XA_usb->set_agc_attack(attack);
    _freedv_rx800XA_lsb->set_agc_attack(attack);
}

void gr_demod_base::set_agc_decay(int value)
{
    float decay;
    if(value == 0)
        decay = 1.0f;
    if(value < 0)
        decay = 1.0f / (float) -value;
    if(value > 0)
        decay = (float) value;
    _usb->set_agc_decay(decay);
    _lsb->set_agc_decay(decay);
    _am->set_agc_decay(decay);
    _freedv_rx1600_usb->set_agc_decay(decay);
    _freedv_rx1600_lsb->set_agc_decay(decay);
    _freedv_rx700C_usb->set_agc_decay(decay);
    _freedv_rx700C_lsb->set_agc_decay(decay);
    _freedv_rx700D_usb->set_agc_decay(decay);
    _freedv_rx700D_lsb->set_agc_decay(decay);
    _freedv_rx800XA_usb->set_agc_decay(decay);
    _freedv_rx800XA_lsb->set_agc_decay(decay);
}
