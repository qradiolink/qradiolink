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

#include "gr_mod_base.h"

gr_mod_base::gr_mod_base(BurstTimer *burst_timer, QObject *parent, float device_frequency, float rf_gain,
                           std::string device_args, std::string device_antenna, int freq_corr, int mmdvm_channels,
                         int mmdvm_channel_separation) :
    QObject(parent)
{
    _device_frequency = device_frequency;
    _top_block = gr::make_top_block("modulator");
    _freq_correction = freq_corr;
    _mmdvm_channels = mmdvm_channels;
    _mode = 9999;
    _samp_rate = 1000000;
    _tx_gain = rf_gain;
    _use_tdma = false;
    _byte_source = make_gr_byte_source();
    _audio_source = make_gr_audio_source();

    _carrier_offset = 0;
    _preserve_carrier_offset = _carrier_offset;

    _rotator = gr::blocks::rotator_cc::make(2*M_PI*_carrier_offset/1000000);


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
        std::cout << "Using native LimeSDR sink" << std::endl;
        burst_timer->set_enabled(true);
        _limesdr_sink = gr::limesdr::sink::make(serial.toStdString(), 0, "", "burst_length");
        _limesdr_sink->set_center_freq(_device_frequency - _carrier_offset);
        _limesdr_sink->set_sample_rate(_samp_rate);
        _limesdr_sink->set_digital_filter(_samp_rate, 0);
        _limesdr_sink->set_antenna(255);
        set_bandwidth_specific();
        _limesdr_sink->calibrate(_samp_rate);
        _limesdr_sink->set_gain(int(rf_gain * 73.0f));
        _limesdr_sink->set_buffer_size(_samp_rate / 10);
        _top_block->connect(_rotator,0,_limesdr_sink,0);
        _use_tdma = true;
    }
    else if(_uhd_specific)
    {
        std::cout << "Using native USRP sink" << std::endl;
        burst_timer->set_enabled(true);
        uhd::stream_args_t stream_args("fc32", "sc16");
        stream_args.channels = {0};
        //stream_args.args["spp"] = "1000"; // 1000 samples per packet
        std::string dev_string;
        if(serial.size() > 1)
            dev_string = QString("serial=%1").arg(serial).toStdString();
        else
            dev_string = "uhd=0";
        uhd::device_addr_t device_addr(dev_string);
        _uhd_sink = gr::uhd::usrp_sink::make(device_addr, stream_args);
        _uhd_sink->set_center_freq(_device_frequency - _carrier_offset);
        _uhd_sink->set_samp_rate(_samp_rate);
        _uhd_sink->set_bandwidth(_samp_rate, 0);
        _uhd_sink->set_antenna(device_antenna);
        set_bandwidth_specific();
        _uhd_gain_range = _uhd_sink->get_gain_range();
        _gain_names = _uhd_sink->get_gain_names();
        if (!_uhd_gain_range.empty())
        {
            double gain =  _uhd_gain_range.start() + rf_gain*(_uhd_gain_range.stop()-_uhd_gain_range.start());
            _uhd_sink->set_gain(gain);
        }
        else
        {
            _uhd_sink->set_gain(rf_gain);
        }
        _uhd_sink->set_start_time(uhd::time_spec_t(0.0d));
        _top_block->connect(_rotator,0,_uhd_sink,0);
        _use_tdma = true;
    }
    else
    {
        _lime_specific = false;
        burst_timer->set_enabled(false);
        _osmosdr_sink = osmosdr::sink::make(device_args);
        //set_bandwidth_specific();
        _osmosdr_sink->set_sample_rate(_samp_rate);
        _osmosdr_sink->set_bandwidth(_samp_rate);
        _osmosdr_sink->set_antenna(device_antenna);
        _osmosdr_sink->set_center_freq(_device_frequency - _carrier_offset);
        //_osmosdr_sink->set_freq_corr(freq_corr);
        _gain_range = _osmosdr_sink->get_gain_range();
        _gain_names = _osmosdr_sink->get_gain_names();
        if (!_gain_range.empty())
        {
            double gain =  _gain_range.start() + rf_gain*(_gain_range.stop()-_gain_range.start());
            _osmosdr_sink->set_gain(gain);
        }
        _top_block->connect(_rotator,0,_osmosdr_sink,0);
    }



    _signal_source = gr::analog::sig_source_f::make(8000, gr::analog::GR_SIN_WAVE, 600, 0.001, 1);

    _mmdvm_source = make_gr_mmdvm_source(burst_timer, 1, false, _use_tdma);


    int tw = std::min(_samp_rate/4, 1500000);
    _resampler = gr::filter::rational_resampler_base_ccf::make(1, 1,
            gr::filter::firdes::low_pass(1, _samp_rate, 500000, tw, gr::filter::firdes::WIN_HAMMING));

    _2fsk_2k_fm = make_gr_mod_2fsk(25, 1000000, 1700, 4000, true); // 4000 for non FM, 2700 for FM
    _2fsk_1k_fm = make_gr_mod_2fsk(50, 1000000, 1700, 2500, true);
    _2fsk_2k = make_gr_mod_2fsk(25, 1000000, 1700, 4000, false);
    _2fsk_1k = make_gr_mod_2fsk(50, 1000000, 1700, 2000, false);
    _2fsk_10k = make_gr_mod_2fsk(5, 1000000, 1700, 25000, true);
    _gmsk_2k = make_gr_mod_gmsk(50, 1000000, 1700, 4000);
    _gmsk_1k = make_gr_mod_gmsk(100, 1000000, 1700, 2000);
    _gmsk_10k = make_gr_mod_gmsk(10, 1000000, 1700, 20000);
    _4fsk_2k = make_gr_mod_4fsk(25, 1000000, 1700, 4000, false);
    _4fsk_2k_fm = make_gr_mod_4fsk(25, 1000000, 1700, 3500, true);
    _4fsk_1k_fm = make_gr_mod_4fsk(50, 1000000, 1700, 2000, true);
    _4fsk_10k_fm = make_gr_mod_4fsk(5, 1000000, 1700, 20000, true);
    _am = make_gr_mod_am(125,1000000, 1700, 5000);
    _bpsk_1k = make_gr_mod_bpsk(500, 1000000, 1700, 1500);
    _bpsk_2k = make_gr_mod_bpsk(250, 1000000, 1700, 2800);
    _bpsk_dsss_8 = make_gr_mod_dsss(25, 1000000, 1700, 200);
    _fm_2500 = make_gr_mod_nbfm(20, 1000000, 1700, 2500);
    _fm_5000 = make_gr_mod_nbfm(20, 1000000, 1700, 5000);
    _qpsk_2k = make_gr_mod_qpsk(500, 1000000, 1700, 1300);
    _qpsk_10k = make_gr_mod_qpsk(100, 1000000, 1700, 6500);
    _qpsk_250k = make_gr_mod_qpsk(4, 1000000, 1700, 160000);
    _qpsk_video = make_gr_mod_qpsk(4, 1000000, 1700, 160000);
    _4fsk_96k = make_gr_mod_4fsk(2, 1000000, 1700, 125000, true);
    _usb = make_gr_mod_ssb(125, 1000000, 1700, 2700, 0);
    _lsb = make_gr_mod_ssb(125, 1000000, 1700, 2700, 1);
    _usb_cw = make_gr_mod_ssb(125, 1000000, 1700, 1000, 0);
    _freedv_tx1600_usb = make_gr_mod_freedv(125, 1000000, 1700, 2500, 200,
                                                gr::vocoder::freedv_api::MODE_1600, 0);

    ///int version = atoi(gr::minor_version().c_str());

    _freedv_tx700C_usb = make_gr_mod_freedv(125, 1000000, 1700, 2700, 300,
                                                    gr::vocoder::freedv_api::MODE_700C, 0);
    _freedv_tx700D_usb = make_gr_mod_freedv(125, 1000000, 1700, 2700, 300,
                                                    gr::vocoder::freedv_api::MODE_700D, 0);

    _freedv_tx800XA_usb = make_gr_mod_freedv(125, 1000000, 1700, 2700, 200,
                                                 gr::vocoder::freedv_api::MODE_800XA, 0);

    _freedv_tx1600_lsb = make_gr_mod_freedv(125, 1000000, 1700, 2700, 200,
                                                gr::vocoder::freedv_api::MODE_1600, 1);

    _freedv_tx700C_lsb = make_gr_mod_freedv(125, 1000000, 1700, 2700, 300,
                                                    gr::vocoder::freedv_api::MODE_700C, 1);
    _freedv_tx700D_lsb = make_gr_mod_freedv(125, 1000000, 1700, 2700, 300,
                                                    gr::vocoder::freedv_api::MODE_700D, 1);

    _freedv_tx800XA_lsb = make_gr_mod_freedv(125, 1000000, 1700, 2700, 200,
                                                 gr::vocoder::freedv_api::MODE_800XA, 1);
    _mmdvm_mod = make_gr_mod_mmdvm();
    _mmdvm_mod_multi = make_gr_mod_mmdvm_multi2(burst_timer, _mmdvm_channels, mmdvm_channel_separation, _use_tdma);
    _m17_mod = make_gr_mod_m17();
}


void gr_mod_base::set_samp_rate(int samp_rate)
{
    _top_block->lock();
    _samp_rate = samp_rate;

    int interpolation = (int) _samp_rate / 1000000;


    if(_samp_rate >= 2000000)
    {
        try
        {
            if(_lime_specific)
                _top_block->disconnect(_rotator,0, _limesdr_sink,0);
            else if(_uhd_specific)
                _top_block->disconnect(_rotator,0, _uhd_sink,0);
            else
                _top_block->disconnect(_rotator,0, _osmosdr_sink,0);
        }
        catch(std::invalid_argument &e)
        {

        }
        try
        {
            _top_block->disconnect(_rotator,0, _resampler,0);
            if(_lime_specific)
                _top_block->disconnect(_resampler,0, _limesdr_sink,0);
            else if(_uhd_specific)
                _top_block->disconnect(_resampler,0, _uhd_sink,0);
            else
                _top_block->disconnect(_resampler,0, _osmosdr_sink,0);
        }
        catch(std::invalid_argument &e)
        {

        }
        _resampler.reset();
        _resampler = gr::filter::rational_resampler_base_ccf::make(interpolation, 1,
                gr::filter::firdes::low_pass(interpolation, _samp_rate, 480000, 20000, gr::filter::firdes::WIN_BLACKMAN_HARRIS));
        _resampler->set_thread_priority(75);
        _top_block->connect(_rotator,0, _resampler,0);
        if(_lime_specific)
            _top_block->connect(_resampler,0, _limesdr_sink,0);
        else if(_uhd_specific)
            _top_block->connect(_resampler,0, _uhd_sink,0);
        else
            _top_block->connect(_resampler,0, _osmosdr_sink,0);
    }
    else
    {
        try
        {
            _top_block->disconnect(_rotator,0, _resampler,0);
            if(_lime_specific)
                _top_block->disconnect(_resampler,0, _limesdr_sink,0);
            else if(_uhd_specific)
                _top_block->disconnect(_resampler,0, _uhd_sink,0);
            else
                _top_block->disconnect(_resampler,0, _osmosdr_sink,0);
        }
        catch(std::invalid_argument &e)
        {
            if(_lime_specific)
                _top_block->disconnect(_rotator,0, _limesdr_sink,0);
            else if(_uhd_specific)
                _top_block->disconnect(_rotator,0, _uhd_sink,0);
            else
                _top_block->disconnect(_rotator,0, _osmosdr_sink,0);
        }
        try
        {
            if(_lime_specific)
                _top_block->connect(_rotator,0, _limesdr_sink,0);
            else if(_uhd_specific)
                _top_block->connect(_rotator,0, _uhd_sink,0);
            else
                _top_block->connect(_rotator,0, _osmosdr_sink,0);
        }
        catch(std::invalid_argument &e)
        {
        }
    }
    if(_lime_specific)
    {
        _top_block->lock();
        _limesdr_sink->set_center_freq(_device_frequency - _carrier_offset);
        _limesdr_sink->set_sample_rate(_samp_rate);
        _limesdr_sink->set_buffer_size(_samp_rate / 10);
        _limesdr_sink->set_digital_filter(_samp_rate, 0);
        _limesdr_sink->calibrate(_samp_rate);
        _top_block->unlock();
    }
    else if(_uhd_specific)
    {
        _top_block->lock();
        _uhd_sink->set_center_freq(_device_frequency - _carrier_offset);
        _uhd_sink->set_samp_rate(_samp_rate);
        _uhd_sink->set_bandwidth(_samp_rate, 0);
        _top_block->unlock();
    }
    else
    {
        _osmosdr_sink->set_center_freq(_device_frequency - _carrier_offset);
        _osmosdr_sink->set_sample_rate(_samp_rate);
    }
    set_bandwidth_specific();
    _top_block->unlock();

}

const QMap<std::string,QVector<int>> gr_mod_base::get_gain_names() const
{
    QMap<std::string,QVector<int>> gain_names;
    if(_lime_specific)
        return gain_names;
    else if(_uhd_specific)
    {
        for(unsigned int i=0;i<_gain_names.size();i++)
        {
            QVector<int> gains;
            uhd::gain_range_t gain_range = _uhd_sink->get_gain_range(_gain_names.at(i));
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
        osmosdr::gain_range_t gain_range = _osmosdr_sink->get_gain_range(_gain_names.at(i));
        int gain_min = gain_range.start();
        int gain_max = gain_range.stop();
        gains.push_back(gain_min);
        gains.push_back(gain_max);
        gain_names[_gain_names.at(i)] = gains;
    }
    return gain_names;
}

void gr_mod_base::set_mode(int mode)
{
    _top_block->lock();
    _audio_source->flush();
    _byte_source->flush();

    switch(_mode)
    {
    case gr_modem_types::ModemType2FSK2KFM:
        _top_block->disconnect(_byte_source,0,_2fsk_2k_fm,0);
        _top_block->disconnect(_2fsk_2k_fm,0,_rotator,0);
        break;
    case gr_modem_types::ModemType2FSK1KFM:
        _top_block->disconnect(_byte_source,0,_2fsk_1k_fm,0);
        _top_block->disconnect(_2fsk_1k_fm,0,_rotator,0);
        break;
    case gr_modem_types::ModemType2FSK2K:
        _top_block->disconnect(_byte_source,0,_2fsk_2k,0);
        _top_block->disconnect(_2fsk_2k,0,_rotator,0);
        break;
    case gr_modem_types::ModemType2FSK1K:
        _top_block->disconnect(_byte_source,0,_2fsk_1k,0);
        _top_block->disconnect(_2fsk_1k,0,_rotator,0);
        break;
    case gr_modem_types::ModemType2FSK10KFM:
        _top_block->disconnect(_byte_source,0,_2fsk_10k,0);
        _top_block->disconnect(_2fsk_10k,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeGMSK2K:
        _top_block->disconnect(_byte_source,0,_gmsk_2k,0);
        _top_block->disconnect(_gmsk_2k,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeGMSK1K:
        _top_block->disconnect(_byte_source,0,_gmsk_1k,0);
        _top_block->disconnect(_gmsk_1k,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeGMSK10K:
        _top_block->disconnect(_byte_source,0,_gmsk_10k,0);
        _top_block->disconnect(_gmsk_10k,0,_rotator,0);
        break;
    case gr_modem_types::ModemType4FSK2K:
        _top_block->disconnect(_byte_source,0,_4fsk_2k,0);
        _top_block->disconnect(_4fsk_2k,0,_rotator,0);
        break;
    case gr_modem_types::ModemType4FSK2KFM:
        _top_block->disconnect(_byte_source,0,_4fsk_2k_fm,0);
        _top_block->disconnect(_4fsk_2k_fm,0,_rotator,0);
        break;
    case gr_modem_types::ModemType4FSK1KFM:
        _top_block->disconnect(_byte_source,0,_4fsk_1k_fm,0);
        _top_block->disconnect(_4fsk_1k_fm,0,_rotator,0);
        break;
    case gr_modem_types::ModemType4FSK10KFM:
        _top_block->disconnect(_byte_source,0,_4fsk_10k_fm,0);
        _top_block->disconnect(_4fsk_10k_fm,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeAM5000:
        _top_block->disconnect(_audio_source,0,_am,0);
        _top_block->disconnect(_am,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeBPSK1K:
        _top_block->disconnect(_byte_source,0,_bpsk_1k,0);
        _top_block->disconnect(_bpsk_1k,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeBPSK2K:
        _top_block->disconnect(_byte_source,0,_bpsk_2k,0);
        _top_block->disconnect(_bpsk_2k,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeBPSK8:
        _top_block->disconnect(_byte_source,0,_bpsk_dsss_8,0);
        _top_block->disconnect(_bpsk_dsss_8,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeNBFM2500:
        _top_block->disconnect(_audio_source,0,_fm_2500,0);
        _top_block->disconnect(_fm_2500,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeNBFM5000:
        _top_block->disconnect(_audio_source,0,_fm_5000,0);
        _top_block->disconnect(_fm_5000,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeQPSK2K:
        _top_block->disconnect(_byte_source,0,_qpsk_2k,0);
        _top_block->disconnect(_qpsk_2k,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeQPSK20K:
        _top_block->disconnect(_byte_source,0,_qpsk_10k,0);
        _top_block->disconnect(_qpsk_10k,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeQPSK250K:
        set_carrier_offset(_carrier_offset);
        set_center_freq(_device_frequency - _carrier_offset);
        _top_block->disconnect(_byte_source,0,_qpsk_250k,0);
        _top_block->disconnect(_qpsk_250k,0,_rotator,0);
        break;
    case gr_modem_types::ModemType4FSK100K:
        set_carrier_offset(_carrier_offset);
        set_center_freq(_device_frequency - _carrier_offset);
        _top_block->disconnect(_byte_source,0,_4fsk_96k,0);
        _top_block->disconnect(_4fsk_96k,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeQPSKVideo:
        _top_block->disconnect(_byte_source,0,_qpsk_video,0);
        _top_block->disconnect(_qpsk_video,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeUSB2500:
        _top_block->disconnect(_audio_source,0,_usb,0);
        _top_block->disconnect(_usb,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeLSB2500:
        _top_block->disconnect(_audio_source,0,_lsb,0);
        _top_block->disconnect(_lsb,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeCW600USB:
        _top_block->disconnect(_signal_source,0,_usb_cw,0);
        _top_block->disconnect(_usb_cw,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeFREEDV1600USB:
        _top_block->disconnect(_audio_source,0,_freedv_tx1600_usb,0);
        _top_block->disconnect(_freedv_tx1600_usb,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeFREEDV700CUSB:
        _top_block->disconnect(_audio_source,0,_freedv_tx700C_usb,0);
        _top_block->disconnect(_freedv_tx700C_usb,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeFREEDV700DUSB:
        _top_block->disconnect(_audio_source,0,_freedv_tx700D_usb,0);
        _top_block->disconnect(_freedv_tx700D_usb,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeFREEDV800XAUSB:
        _top_block->disconnect(_audio_source,0,_freedv_tx800XA_usb,0);
        _top_block->disconnect(_freedv_tx800XA_usb,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeFREEDV1600LSB:
        _top_block->disconnect(_audio_source,0,_freedv_tx1600_lsb,0);
        _top_block->disconnect(_freedv_tx1600_lsb,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeFREEDV700CLSB:
        _top_block->disconnect(_audio_source,0,_freedv_tx700C_lsb,0);
        _top_block->disconnect(_freedv_tx700C_lsb,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeFREEDV700DLSB:
        _top_block->disconnect(_audio_source,0,_freedv_tx700D_lsb,0);
        _top_block->disconnect(_freedv_tx700D_lsb,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeFREEDV800XALSB:
        _top_block->disconnect(_audio_source,0,_freedv_tx800XA_lsb,0);
        _top_block->disconnect(_freedv_tx800XA_lsb,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeMMDVM:
        set_carrier_offset(_carrier_offset);
        set_center_freq(_device_frequency - _carrier_offset);
        _top_block->disconnect(_mmdvm_source,0,_mmdvm_mod,0);
        _top_block->disconnect(_mmdvm_mod,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeMMDVMmulti:
        set_carrier_offset(_carrier_offset);
        set_center_freq(_device_frequency - _carrier_offset);
        _top_block->disconnect(_mmdvm_mod_multi,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeM17:
        set_center_freq(_device_frequency - _carrier_offset);
        _top_block->disconnect(_byte_source,0,_m17_mod,0);
        _top_block->disconnect(_m17_mod,0,_rotator,0);
        break;
    default:
        break;
    }

    switch(mode)
    {
    case gr_modem_types::ModemType2FSK2KFM:
        set_carrier_offset(_carrier_offset);
        set_center_freq(_device_frequency - _carrier_offset);
        _top_block->connect(_byte_source,0,_2fsk_2k_fm,0);
        _top_block->connect(_2fsk_2k_fm,0,_rotator,0);
        break;
    case gr_modem_types::ModemType2FSK1KFM:
        set_carrier_offset(_carrier_offset);
        set_center_freq(_device_frequency - _carrier_offset);
        _top_block->connect(_byte_source,0,_2fsk_1k_fm,0);
        _top_block->connect(_2fsk_1k_fm,0,_rotator,0);
        break;
    case gr_modem_types::ModemType2FSK2K:
        set_carrier_offset(_carrier_offset);
        set_center_freq(_device_frequency - _carrier_offset);
        _top_block->connect(_byte_source,0,_2fsk_2k,0);
        _top_block->connect(_2fsk_2k,0,_rotator,0);
        break;
    case gr_modem_types::ModemType2FSK1K:
        set_carrier_offset(_carrier_offset);
        set_center_freq(_device_frequency - _carrier_offset);
        _top_block->connect(_byte_source,0,_2fsk_1k,0);
        _top_block->connect(_2fsk_1k,0,_rotator,0);
        break;
    case gr_modem_types::ModemType2FSK10KFM:
        set_carrier_offset(_carrier_offset);
        set_center_freq(_device_frequency - _carrier_offset);
        _top_block->connect(_byte_source,0,_2fsk_10k,0);
        _top_block->connect(_2fsk_10k,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeGMSK2K:
        set_carrier_offset(_carrier_offset);
        set_center_freq(_device_frequency - _carrier_offset);
        _top_block->connect(_byte_source,0,_gmsk_2k,0);
        _top_block->connect(_gmsk_2k,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeGMSK1K:
        set_carrier_offset(_carrier_offset);
        set_center_freq(_device_frequency - _carrier_offset);
        _top_block->connect(_byte_source,0,_gmsk_1k,0);
        _top_block->connect(_gmsk_1k,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeGMSK10K:
        set_carrier_offset(_carrier_offset);
        set_center_freq(_device_frequency - _carrier_offset);
        _top_block->connect(_byte_source,0,_gmsk_10k,0);
        _top_block->connect(_gmsk_10k,0,_rotator,0);
        break;
    case gr_modem_types::ModemType4FSK2K:
        set_carrier_offset(_carrier_offset);
        set_center_freq(_device_frequency - _carrier_offset);
        _top_block->connect(_byte_source,0,_4fsk_2k,0);
        _top_block->connect(_4fsk_2k,0,_rotator,0);
        break;
    case gr_modem_types::ModemType4FSK2KFM:
        set_carrier_offset(_carrier_offset);
        set_center_freq(_device_frequency - _carrier_offset);
        _top_block->connect(_byte_source,0,_4fsk_2k_fm,0);
        _top_block->connect(_4fsk_2k_fm,0,_rotator,0);
        break;
    case gr_modem_types::ModemType4FSK1KFM:
        set_carrier_offset(_carrier_offset);
        set_center_freq(_device_frequency - _carrier_offset);
        _top_block->connect(_byte_source,0,_4fsk_1k_fm,0);
        _top_block->connect(_4fsk_1k_fm,0,_rotator,0);
        break;
    case gr_modem_types::ModemType4FSK10KFM:
        set_carrier_offset(_carrier_offset);
        set_center_freq(_device_frequency - _carrier_offset);
        _top_block->connect(_byte_source,0,_4fsk_10k_fm,0);
        _top_block->connect(_4fsk_10k_fm,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeAM5000:
        set_carrier_offset(_carrier_offset);
        set_center_freq(_device_frequency - _carrier_offset);
        _top_block->connect(_audio_source,0,_am,0);
        _top_block->connect(_am,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeBPSK1K:
        set_carrier_offset(_carrier_offset);
        set_center_freq(_device_frequency - _carrier_offset);
        _top_block->connect(_byte_source,0,_bpsk_1k,0);
        _top_block->connect(_bpsk_1k,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeBPSK2K:
        set_carrier_offset(_carrier_offset);
        set_center_freq(_device_frequency - _carrier_offset);
        _top_block->connect(_byte_source,0,_bpsk_2k,0);
        _top_block->connect(_bpsk_2k,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeBPSK8:
        set_carrier_offset(_carrier_offset);
        set_center_freq(_device_frequency - _carrier_offset);
        _top_block->connect(_byte_source,0,_bpsk_dsss_8,0);
        _top_block->connect(_bpsk_dsss_8,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeNBFM2500:
        set_carrier_offset(_carrier_offset);
        set_center_freq(_device_frequency - _carrier_offset);
        _top_block->connect(_audio_source,0,_fm_2500,0);
        _top_block->connect(_fm_2500,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeNBFM5000:
        set_carrier_offset(_carrier_offset);
        set_center_freq(_device_frequency - _carrier_offset);
        _top_block->connect(_audio_source,0,_fm_5000,0);
        _top_block->connect(_fm_5000,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeQPSK2K:
        set_carrier_offset(_carrier_offset);
        set_center_freq(_device_frequency - _carrier_offset);
        _top_block->connect(_byte_source,0,_qpsk_2k,0);
        _top_block->connect(_qpsk_2k,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeQPSK20K:
        set_carrier_offset(_carrier_offset);
        set_center_freq(_device_frequency - _carrier_offset);
        _top_block->connect(_byte_source,0,_qpsk_10k,0);
        _top_block->connect(_qpsk_10k,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeQPSK250K:
        set_carrier_offset(_carrier_offset);
        set_center_freq(_device_frequency - _carrier_offset);
        _top_block->connect(_byte_source,0,_qpsk_250k,0);
        _top_block->connect(_qpsk_250k,0,_rotator,0);
        break;
    case gr_modem_types::ModemType4FSK100K:
        set_carrier_offset(_carrier_offset);
        set_center_freq(_device_frequency - _carrier_offset);
        _top_block->connect(_byte_source,0,_4fsk_96k,0);
        _top_block->connect(_4fsk_96k,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeQPSKVideo:
        set_carrier_offset(_carrier_offset);
        set_center_freq(_device_frequency - _carrier_offset);
        _top_block->connect(_byte_source,0,_qpsk_video,0);
        _top_block->connect(_qpsk_video,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeUSB2500:
        set_carrier_offset(_carrier_offset);
        set_center_freq(_device_frequency - _carrier_offset);
        _top_block->connect(_audio_source,0,_usb,0);
        _top_block->connect(_usb,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeLSB2500:
        set_carrier_offset(_carrier_offset);
        set_center_freq(_device_frequency - _carrier_offset);
        _top_block->connect(_audio_source,0,_lsb,0);
        _top_block->connect(_lsb,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeCW600USB:
        set_carrier_offset(_carrier_offset);
        set_center_freq(_device_frequency - _carrier_offset);
        _top_block->connect(_signal_source,0,_usb_cw,0);
        _top_block->connect(_usb_cw,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeFREEDV1600USB:
        set_carrier_offset(_carrier_offset);
        set_center_freq(_device_frequency - _carrier_offset);
        _top_block->connect(_audio_source,0,_freedv_tx1600_usb,0);
        _top_block->connect(_freedv_tx1600_usb,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeFREEDV700CUSB:
        set_carrier_offset(_carrier_offset);
        set_center_freq(_device_frequency - _carrier_offset);
        _top_block->connect(_audio_source,0,_freedv_tx700C_usb,0);
        _top_block->connect(_freedv_tx700C_usb,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeFREEDV700DUSB:
        set_carrier_offset(_carrier_offset);
        set_center_freq(_device_frequency - _carrier_offset);
        _top_block->connect(_audio_source,0,_freedv_tx700D_usb,0);
        _top_block->connect(_freedv_tx700D_usb,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeFREEDV800XAUSB:
        set_carrier_offset(_carrier_offset);
        set_center_freq(_device_frequency - _carrier_offset);
        _top_block->connect(_audio_source,0,_freedv_tx800XA_usb,0);
        _top_block->connect(_freedv_tx800XA_usb,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeFREEDV1600LSB:
        set_carrier_offset(_carrier_offset);
        set_center_freq(_device_frequency - _carrier_offset);
        _top_block->connect(_audio_source,0,_freedv_tx1600_lsb,0);
        _top_block->connect(_freedv_tx1600_lsb,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeFREEDV700CLSB:
        set_carrier_offset(_carrier_offset);
        set_center_freq(_device_frequency - _carrier_offset);
        _top_block->connect(_audio_source,0,_freedv_tx700C_lsb,0);
        _top_block->connect(_freedv_tx700C_lsb,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeFREEDV700DLSB:
        set_carrier_offset(_carrier_offset);
        set_center_freq(_device_frequency - _carrier_offset);
        _top_block->connect(_audio_source,0,_freedv_tx700D_lsb,0);
        _top_block->connect(_freedv_tx700D_lsb,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeFREEDV800XALSB:
        set_carrier_offset(_carrier_offset);
        set_center_freq(_device_frequency - _carrier_offset);
        _top_block->connect(_audio_source,0,_freedv_tx800XA_lsb,0);
        _top_block->connect(_freedv_tx800XA_lsb,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeMMDVM:
        set_carrier_offset(_carrier_offset, MMDVM_SAMPLE_RATE);
        set_center_freq(_device_frequency - _carrier_offset);
        _top_block->connect(_mmdvm_source,0,_mmdvm_mod,0);
        _top_block->connect(_mmdvm_mod,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeMMDVMmulti:
        set_carrier_offset(0, MMDVM_SAMPLE_RATE);
        set_center_freq(_device_frequency - _carrier_offset);
        _top_block->connect(_mmdvm_mod_multi,0,_rotator,0);
        break;
    case gr_modem_types::ModemTypeM17:
        set_carrier_offset(_carrier_offset);
        set_center_freq(_device_frequency - _carrier_offset);
        _top_block->connect(_byte_source,0,_m17_mod,0);
        _top_block->connect(_m17_mod,0,_rotator,0);
        break;
    default:
        break;
    }

    _mode = mode;

    _top_block->unlock();
}

void gr_mod_base::start(int buffer_size)
{
    _audio_source->flush();
    _byte_source->flush();
    if(buffer_size)
        _top_block->start(buffer_size);
    else // automatic
        _top_block->start();
}

void gr_mod_base::stop()
{
    _audio_source->flush();
    _byte_source->flush();
    _top_block->stop();
    _top_block->wait();
}

int gr_mod_base::set_data(std::vector<u_int8_t> *data)
{
    return _byte_source->set_data(data);
}

int gr_mod_base::set_audio(std::vector<float> *data)
{
    return _audio_source->set_data(data);

}

void gr_mod_base::set_carrier_offset(int64_t carrier_offset, int64_t sample_rate)
{
     // preserve current doppler tracking
    _preserve_carrier_offset = _carrier_offset;
    _carrier_offset = carrier_offset;
    _rotator->set_phase_inc(2*M_PI*float(_carrier_offset)/float(sample_rate));
}

int64_t gr_mod_base::reset_carrier_offset()
{
    _carrier_offset = _preserve_carrier_offset;
    _rotator->set_phase_inc(2*M_PI*_carrier_offset/1000000.0f);
    return _carrier_offset;
}

void gr_mod_base::tune(int64_t center_freq)
{
    int64_t steps = center_freq / 1000000;
    _device_frequency = double(center_freq) + double(steps * _freq_correction);
    double tx_freq = _device_frequency - _carrier_offset;
    if(_lime_specific)
    {
        _limesdr_sink->set_center_freq(tx_freq);
    }
    else if(_uhd_specific)
    {
        _uhd_sink->set_center_freq(tx_freq);
    }
    else
        _osmosdr_sink->set_center_freq(tx_freq);
    set_bandwidth_specific();
}

void gr_mod_base::set_power(float value, std::string gain_stage)
{
    if(value > 1.0f) value = 1.0f;
    _tx_gain = value;
    if(_lime_specific)
    {
        _limesdr_sink->set_gain(int(value * 73.0f));
    }
    else if(_uhd_specific)
    {
        if (!_uhd_gain_range.empty() && (gain_stage.size() < 1))
        {
            double gain =  std::floor(double(_uhd_gain_range.start()) +
                    double(value)*(double(_uhd_gain_range.stop())-double(_uhd_gain_range.start())));
            _uhd_sink->set_gain(gain);
        }
        else
        {
            _uhd_sink->set_gain(value, gain_stage);
        }
    }
    else
    {
        if (!_gain_range.empty() && (gain_stage.size() < 1))
        {
            double gain =  std::floor(double(_gain_range.start()) +
                    double(value)*(double(_gain_range.stop())-double(_gain_range.start())));
            _osmosdr_sink->set_gain(gain);
        }
        else
        {
            _osmosdr_sink->set_gain_mode(false);
            _osmosdr_sink->set_gain(value, gain_stage);
        }
    }
}


void gr_mod_base::set_ctcss(float value)
{
    _top_block->lock();
    _fm_2500->set_ctcss(value);
    _fm_5000->set_ctcss(value);
    _top_block->unlock();
}

void gr_mod_base::set_filter_width(int filter_width, int mode)
{
    switch(mode)
    {
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
    case gr_modem_types::ModemTypeCW600USB:
        _usb_cw->set_filter_width(filter_width);
        break;
    default:
        break;
    }
}

void gr_mod_base::set_bb_gain(float value)
{
    _2fsk_2k_fm->set_bb_gain(value);
    _2fsk_1k_fm->set_bb_gain(value);
    _2fsk_2k->set_bb_gain(value);
    _2fsk_1k->set_bb_gain(value);
    _2fsk_10k->set_bb_gain(value);
    _gmsk_2k->set_bb_gain(value);
    _gmsk_1k->set_bb_gain(value);
    _gmsk_10k->set_bb_gain(value);
    _4fsk_2k->set_bb_gain(value);
    _4fsk_2k_fm->set_bb_gain(value);
    _4fsk_1k_fm->set_bb_gain(value);
    _4fsk_10k_fm->set_bb_gain(value);
    _am->set_bb_gain(value);
    _bpsk_1k->set_bb_gain(value);
    _bpsk_2k->set_bb_gain(value);
    _bpsk_dsss_8->set_bb_gain(value);
    _fm_2500->set_bb_gain(value);
    _fm_5000->set_bb_gain(value);
    _qpsk_2k->set_bb_gain(value);
    _qpsk_10k->set_bb_gain(value);
    _qpsk_250k->set_bb_gain(value);
    _qpsk_video->set_bb_gain(value);
    _4fsk_96k->set_bb_gain(value);
    _usb->set_bb_gain(value);
    _lsb->set_bb_gain(value);
    _usb_cw->set_bb_gain(value);
    _freedv_tx1600_usb->set_bb_gain(value);
    _freedv_tx1600_lsb->set_bb_gain(value);
    _freedv_tx700C_usb->set_bb_gain(value);
    _freedv_tx700C_lsb->set_bb_gain(value);
    _freedv_tx800XA_usb->set_bb_gain(value);
    _freedv_tx800XA_lsb->set_bb_gain(value);
    _mmdvm_mod->set_bb_gain(value);
    _mmdvm_mod_multi->set_bb_gain(value);
    _m17_mod->set_bb_gain(value);
}

void gr_mod_base::set_cw_k(bool value)
{
    double a;
    if(value)
        a = 0.98;
    else
        a = 0.001;
    _signal_source->set_amplitude(a);
}


void gr_mod_base::flush_sources()
{
    _top_block->lock();
    _audio_source->flush();
    _byte_source->flush();
    _top_block->unlock();
}

void gr_mod_base::set_bandwidth_specific()
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
        _limesdr_sink->set_bandwidth(_osmo_filter_bw);
    else if(_uhd_specific)
        _uhd_sink->set_bandwidth(_osmo_filter_bw);
    else
        _osmosdr_sink->set_bandwidth(_osmo_filter_bw);
}

void gr_mod_base::set_center_freq(double freq)
{
    if(_lime_specific)
    {
        _limesdr_sink->set_center_freq(freq);
    }
    else if(_uhd_specific)
    {
        _uhd_sink->set_center_freq(freq);
    }
    else
        _osmosdr_sink->set_center_freq(freq);
}


