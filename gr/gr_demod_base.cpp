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



gr_demod_base::gr_demod_base(QObject *parent, float device_frequency,
                             float rf_gain, std::string device_args, std::string device_antenna,
                              int freq_corr) :
    QObject(parent)
{
    _locked = false;
    _msg_nr = 0;
    _demod_running = false;
    _device_frequency = device_frequency;
    _top_block = gr::make_top_block("demodulator");
    _mode = 9999;
    _carrier_offset = 0;
    _samp_rate = 1000000;

    _audio_sink = make_gr_audio_sink();
    _vector_sink = make_gr_vector_sink();

    _message_sink = gr::blocks::message_debug::make();

    _rssi_valve = gr::blocks::copy::make(8);
    _rssi_valve->set_enabled(false);
    _rssi = gr::blocks::probe_signal_f::make();
    _constellation = make_gr_const_sink();
    _const_valve = gr::blocks::copy::make(8);
    _const_valve->set_enabled(false);
    // FIXME: this block is using CPU just because I'm lazy
    _demod_valve = gr::blocks::copy::make(8);
    _demod_valve->set_enabled(true);
    _mag_squared = gr::blocks::complex_to_mag_squared::make();
    _single_pole_filter = gr::filter::single_pole_iir_filter_ff::make(0.04);
    _log10 = gr::blocks::nlog10_ff::make();
    _multiply_const_ff = gr::blocks::multiply_const_ff::make(10);
    _agc2 = gr::analog::agc2_ff::make(100, 100, 1, 1);
    _moving_average = gr::blocks::moving_average_ff::make(2000,1,2000);
    _add_const = gr::blocks::add_const_ff::make(-80);
    _rotator = gr::blocks::rotator_cc::make(2*M_PI/1000000);
    std::vector<float> taps;
    int tw = std::min(_samp_rate/4, 1500000);
    taps = gr::filter::firdes::low_pass(1, _samp_rate, 500000, tw, gr::filter::firdes::WIN_HAMMING);
    _resampler = gr::filter::rational_resampler_base_ccf::make(1, 1, taps);

    // FIXME: LimeSDR bandwidth set to higher value for lower freq
    _lime_specific = false;
    QString device(device_args.c_str());
    if(device.contains("driver=lime", Qt::CaseInsensitive))
    {
        _lime_specific = true;
    }
    _osmosdr_source = osmosdr::source::make(device_args);
    _osmosdr_source->set_center_freq(_device_frequency);
    set_bandwidth_specific();
    _osmosdr_source->set_sample_rate(1000000);
    _osmosdr_source->set_freq_corr(freq_corr);
    _osmosdr_source->set_gain_mode(true);
    _osmosdr_source->set_dc_offset_mode(2);
    _osmosdr_source->set_iq_balance_mode(0);
    _osmosdr_source->set_antenna(device_antenna);
    _gain_range = _osmosdr_source->get_gain_range();
    _gain_names = _osmosdr_source->get_gain_names();
    if (!_gain_range.empty())
    {
        double gain =  (double)_gain_range.start() + rf_gain*((double)_gain_range.stop()- (double)_gain_range.start());
        _osmosdr_source->set_gain_mode(false);
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
        _osmosdr_source->set_gain_mode(true);
    }

    _fft_sink = make_rx_fft_c(32768, gr::filter::firdes::WIN_BLACKMAN_HARRIS);

    _deframer1 = make_gr_deframer_bb(1);
    _deframer2 = make_gr_deframer_bb(1);

    _deframer_700_1 = make_gr_deframer_bb(2);
    _deframer_700_2 = make_gr_deframer_bb(2);

    _deframer1_10k = make_gr_deframer_bb(3);
    _deframer2_10k = make_gr_deframer_bb(3);


    _top_block->connect(_osmosdr_source,0,_rotator,0);
    _top_block->connect(_rotator,0,_demod_valve,0);
    _top_block->connect(_osmosdr_source,0,_fft_sink,0);


    _top_block->connect(_rssi_valve,0,_mag_squared,0);
    _top_block->connect(_mag_squared,0,_moving_average,0);
    _top_block->connect(_moving_average,0,_single_pole_filter,0);
    _top_block->connect(_single_pole_filter,0,_log10,0);
    _top_block->connect(_log10,0,_multiply_const_ff,0);
    _top_block->connect(_multiply_const_ff,0,_add_const,0);
    _top_block->connect(_add_const,0,_rssi,0);




    _2fsk = make_gr_demod_2fsk_sdr(125,1000000,1700,4000); // 4000 for non FM demod, 2700 for FM
    _2fsk_10k = make_gr_demod_2fsk_sdr(25,1000000,1700,13500, true);
    _4fsk_2k = make_gr_demod_4fsk_sdr(125,1000000,1700,4000);
    _4fsk_10k = make_gr_demod_4fsk_sdr(25,1000000,1700,20000);
    _am = make_gr_demod_am_sdr(0, 1000000,1700,4000);
    _bpsk_1k = make_gr_demod_bpsk_sdr(250,1000000,1700,1300);
    _bpsk_2k = make_gr_demod_bpsk_sdr(125,1000000,1700,2400);
    _fm_2500 = make_gr_demod_nbfm_sdr(0, 1000000,1700,2500);
    _fm_5000 = make_gr_demod_nbfm_sdr(0, 1000000,1700,5000);
    _qpsk_2k = make_gr_demod_qpsk_sdr(125,1000000,1700,1300);
    _qpsk_10k = make_gr_demod_qpsk_sdr(25,1000000,1700,6500);
    _qpsk_250k = make_gr_demod_qpsk_sdr(2,1000000,1700,160000);
    _qpsk_video = make_gr_demod_qpsk_sdr(2,1000000,1700,160000);
    _usb = make_gr_demod_ssb_sdr(0, 1000000,1700,2500);
    _lsb = make_gr_demod_ssb_sdr(1, 1000000,1700,2500);
    _wfm = make_gr_demod_wbfm_sdr(0, 1000000,1700,75000);
    _freedv_rx1600_usb = make_gr_demod_freedv(125, 1000000, 1700, 2500, gr::vocoder::freedv_api::MODE_1600, 0);

    int version = atoi(gr::minor_version().c_str());
    if(version >= 13)
        _freedv_rx700C_usb = make_gr_demod_freedv(125, 1000000, 1700, 2500, gr::vocoder::freedv_api::MODE_700C, 0);
    else
        _freedv_rx700C_usb = make_gr_demod_freedv(125, 1000000, 1700, 2500, gr::vocoder::freedv_api::MODE_700, 0);
    _freedv_rx800XA_usb = make_gr_demod_freedv(125, 1000000, 1700, 2500, gr::vocoder::freedv_api::MODE_800XA, 0);

    _freedv_rx1600_lsb = make_gr_demod_freedv(125, 1000000, 1700, 2500, gr::vocoder::freedv_api::MODE_1600, 1);

    if(version >= 13)
        _freedv_rx700C_lsb = make_gr_demod_freedv(125, 1000000, 1700, 2500, gr::vocoder::freedv_api::MODE_700C, 1);
    else
        _freedv_rx700C_lsb = make_gr_demod_freedv(125, 1000000, 1700, 2500, gr::vocoder::freedv_api::MODE_700, 1);
    _freedv_rx800XA_lsb = make_gr_demod_freedv(125, 1000000, 1700, 2500, gr::vocoder::freedv_api::MODE_800XA, 1);

}

gr_demod_base::~gr_demod_base()
{
    _osmosdr_source.reset();
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
    _vector_sink->flush();
    if(disconnect)
    {
        switch(_mode)
        {
        case gr_modem_types::ModemType2FSK2000:
            _top_block->disconnect(_demod_valve,0,_2fsk,0);
            _top_block->disconnect(_2fsk,0,_rssi_valve,0);
            _top_block->disconnect(_2fsk,1,_const_valve,0);
            _top_block->disconnect(_const_valve,0,_constellation,0);
            _top_block->disconnect(_2fsk,2,_deframer1,0);
            _top_block->disconnect(_2fsk,3,_deframer2,0);
            break;
        case gr_modem_types::ModemType2FSK20000:
            _top_block->disconnect(_demod_valve,0,_2fsk_10k,0);
            _top_block->disconnect(_2fsk_10k,0,_rssi_valve,0);
            _top_block->disconnect(_2fsk_10k,1,_const_valve,0);
            _top_block->disconnect(_const_valve,0,_constellation,0);
            _top_block->disconnect(_2fsk_10k,2,_deframer1_10k,0);
            _top_block->disconnect(_2fsk_10k,3,_deframer2_10k,0);
            break;
        case gr_modem_types::ModemType4FSK2000:
            _top_block->disconnect(_demod_valve,0,_4fsk_2k,0);
            _top_block->disconnect(_4fsk_2k,0,_rssi_valve,0);
            _top_block->disconnect(_4fsk_2k,1,_const_valve,0);
            _top_block->disconnect(_const_valve,0,_constellation,0);
            _top_block->disconnect(_4fsk_2k,2,_vector_sink,0);
            break;
        case gr_modem_types::ModemType4FSK20000:
            _top_block->disconnect(_demod_valve,0,_4fsk_10k,0);
            _top_block->disconnect(_4fsk_10k,0,_rssi_valve,0);
            _top_block->disconnect(_4fsk_10k,1,_const_valve,0);
            _top_block->disconnect(_const_valve,0,_constellation,0);
            _top_block->disconnect(_4fsk_10k,2,_vector_sink,0);
            break;
        case gr_modem_types::ModemTypeAM5000:
            _top_block->disconnect(_demod_valve,0,_am,0);
            _top_block->disconnect(_am,0,_rssi_valve,0);
            _top_block->disconnect(_am,1,_audio_sink,0);
            break;
        case gr_modem_types::ModemTypeBPSK1000:
            _top_block->disconnect(_demod_valve,0,_bpsk_1k,0);
            _top_block->disconnect(_bpsk_1k,0,_rssi_valve,0);
            _top_block->disconnect(_bpsk_1k,1,_const_valve,0);
            _top_block->disconnect(_const_valve,0,_constellation,0);
            _top_block->disconnect(_bpsk_1k,2,_deframer_700_1,0);
            _top_block->disconnect(_bpsk_1k,3,_deframer_700_2,0);
            break;
        case gr_modem_types::ModemTypeBPSK2000:
            _top_block->disconnect(_demod_valve,0,_bpsk_2k,0);
            _top_block->disconnect(_bpsk_2k,0,_rssi_valve,0);
            _top_block->disconnect(_bpsk_2k,1,_const_valve,0);
            _top_block->disconnect(_const_valve,0,_constellation,0);
            _top_block->disconnect(_bpsk_2k,2,_deframer1,0);
            _top_block->disconnect(_bpsk_2k,3,_deframer2,0);
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
        case gr_modem_types::ModemTypeQPSK2000:
            _top_block->disconnect(_demod_valve,0,_qpsk_2k,0);
            _top_block->disconnect(_qpsk_2k,0,_rssi_valve,0);
            _top_block->disconnect(_qpsk_2k,1,_const_valve,0);
            _top_block->disconnect(_const_valve,0,_constellation,0);
            _top_block->disconnect(_qpsk_2k,2,_vector_sink,0);
            break;
        case gr_modem_types::ModemTypeQPSK20000:
            _top_block->disconnect(_demod_valve,0,_qpsk_10k,0);
            _top_block->disconnect(_qpsk_10k,0,_rssi_valve,0);
            _top_block->disconnect(_qpsk_10k,1,_const_valve,0);
            _top_block->disconnect(_const_valve,0,_constellation,0);
            _top_block->disconnect(_qpsk_10k,2,_vector_sink,0);
            break;
        case gr_modem_types::ModemTypeQPSK250000:
            _top_block->disconnect(_demod_valve,0,_qpsk_250k,0);
            _top_block->disconnect(_qpsk_250k,0,_rssi_valve,0);
            _top_block->disconnect(_qpsk_250k,1,_const_valve,0);
            _top_block->disconnect(_const_valve,0,_constellation,0);
            _top_block->disconnect(_qpsk_250k,2,_vector_sink,0);
            //_carrier_offset = 25000;
            //_osmosdr_source->set_center_freq(_device_frequency - _carrier_offset);
            break;
        case gr_modem_types::ModemTypeQPSKVideo:
            _top_block->disconnect(_demod_valve,0,_qpsk_video,0);
            _top_block->disconnect(_qpsk_video,0,_rssi_valve,0);
            _top_block->disconnect(_qpsk_video,1,_const_valve,0);
            _top_block->disconnect(_const_valve,0,_constellation,0);
            _top_block->disconnect(_qpsk_video,2,_vector_sink,0);
            //_carrier_offset = 25000;
            //_osmosdr_source->set_center_freq(_device_frequency - _carrier_offset);
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
        case gr_modem_types::ModemTypeFREEDV700DUSB:
            _top_block->disconnect(_demod_valve,0,_freedv_rx700C_usb,0);
            _top_block->disconnect(_freedv_rx700C_usb,0,_rssi_valve,0);
            _top_block->disconnect(_freedv_rx700C_usb,1,_audio_sink,0);
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
        case gr_modem_types::ModemTypeFREEDV700DLSB:
            _top_block->disconnect(_demod_valve,0,_freedv_rx700C_lsb,0);
            _top_block->disconnect(_freedv_rx700C_lsb,0,_rssi_valve,0);
            _top_block->disconnect(_freedv_rx700C_lsb,1,_audio_sink,0);
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
            //_carrier_offset = 25000;
            //_osmosdr_source->set_center_freq(_device_frequency - _carrier_offset);
            break;
        default:
            break;
        }
    }

    if(connect)
    {
        switch(mode)
        {
        case gr_modem_types::ModemType2FSK2000:
            _top_block->connect(_demod_valve,0,_2fsk,0);
            _top_block->connect(_2fsk,0,_rssi_valve,0);
            _top_block->connect(_2fsk,1,_const_valve,0);
            _top_block->connect(_const_valve,0,_constellation,0);
            _top_block->connect(_2fsk,2,_deframer1,0);
            _top_block->connect(_2fsk,3,_deframer2,0);
            break;
        case gr_modem_types::ModemType2FSK20000:
            _top_block->connect(_demod_valve,0,_2fsk_10k,0);
            _top_block->connect(_2fsk_10k,0,_rssi_valve,0);
            _top_block->connect(_2fsk_10k,1,_const_valve,0);
            _top_block->connect(_const_valve,0,_constellation,0);
            _top_block->connect(_2fsk_10k,2,_deframer1_10k,0);
            _top_block->connect(_2fsk_10k,3,_deframer2_10k,0);
            break;
        case gr_modem_types::ModemType4FSK2000:
            _top_block->connect(_demod_valve,0,_4fsk_2k,0);
            _top_block->connect(_4fsk_2k,0,_rssi_valve,0);
            _top_block->connect(_4fsk_2k,1,_const_valve,0);
            _top_block->connect(_const_valve,0,_constellation,0);
            _top_block->connect(_4fsk_2k,2,_vector_sink,0);
            break;
        case gr_modem_types::ModemType4FSK20000:
            _top_block->connect(_demod_valve,0,_4fsk_10k,0);
            _top_block->connect(_4fsk_10k,0,_rssi_valve,0);
            _top_block->connect(_4fsk_10k,1,_const_valve,0);
            _top_block->connect(_const_valve,0,_constellation,0);
            _top_block->connect(_4fsk_10k,2,_vector_sink,0);
            break;
        case gr_modem_types::ModemTypeAM5000:
            _top_block->connect(_demod_valve,0,_am,0);
            _top_block->connect(_am,0,_rssi_valve,0);
            _top_block->connect(_am,1,_audio_sink,0);
            break;
        case gr_modem_types::ModemTypeBPSK1000:
            _top_block->connect(_demod_valve,0,_bpsk_1k,0);
            _top_block->connect(_bpsk_1k,0,_rssi_valve,0);
            _top_block->connect(_bpsk_1k,1,_const_valve,0);
            _top_block->connect(_const_valve,0,_constellation,0);
            _top_block->connect(_bpsk_1k,2,_deframer_700_1,0);
            _top_block->connect(_bpsk_1k,3,_deframer_700_2,0);
            break;
        case gr_modem_types::ModemTypeBPSK2000:
            _top_block->connect(_demod_valve,0,_bpsk_2k,0);
            _top_block->connect(_bpsk_2k,0,_rssi_valve,0);
            _top_block->connect(_bpsk_2k,1,_const_valve,0);
            _top_block->connect(_const_valve,0,_constellation,0);
            _top_block->connect(_bpsk_2k,2,_deframer1,0);
            _top_block->connect(_bpsk_2k,3,_deframer2,0);
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
        case gr_modem_types::ModemTypeQPSK2000:
            _top_block->connect(_demod_valve,0,_qpsk_2k,0);
            _top_block->connect(_qpsk_2k,0,_rssi_valve,0);
            _top_block->connect(_qpsk_2k,1,_const_valve,0);
            _top_block->connect(_const_valve,0,_constellation,0);
            _top_block->connect(_qpsk_2k,2,_vector_sink,0);
            break;
        case gr_modem_types::ModemTypeQPSK20000:
            _top_block->connect(_demod_valve,0,_qpsk_10k,0);
            _top_block->connect(_qpsk_10k,0,_rssi_valve,0);
            _top_block->connect(_qpsk_10k,1,_const_valve,0);
            _top_block->connect(_const_valve,0,_constellation,0);
            _top_block->connect(_qpsk_10k,2,_vector_sink,0);
            break;
        case gr_modem_types::ModemTypeQPSK250000:
            //_carrier_offset = 250000;
            //_osmosdr_source->set_center_freq(_device_frequency - _carrier_offset);
            _top_block->connect(_demod_valve,0,_qpsk_250k,0);
            _top_block->connect(_qpsk_250k,0,_rssi_valve,0);
            _top_block->connect(_qpsk_250k,1,_const_valve,0);
            _top_block->connect(_const_valve,0,_constellation,0);
            _top_block->connect(_qpsk_250k,2,_vector_sink,0);
            break;
        case gr_modem_types::ModemTypeQPSKVideo:
            //_carrier_offset = 250000;
            //_osmosdr_source->set_center_freq(_device_frequency - _carrier_offset);
            _top_block->connect(_demod_valve,0,_qpsk_video,0);
            _top_block->connect(_qpsk_video,0,_rssi_valve,0);
            _top_block->connect(_qpsk_video,1,_const_valve,0);
            _top_block->connect(_const_valve,0,_constellation,0);
            _top_block->connect(_qpsk_video,2,_vector_sink,0);
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
        case gr_modem_types::ModemTypeFREEDV700DUSB:
            _top_block->connect(_demod_valve,0,_freedv_rx700C_usb,0);
            _top_block->connect(_freedv_rx700C_usb,0,_rssi_valve,0);
            _top_block->connect(_freedv_rx700C_usb,1,_audio_sink,0);
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
        case gr_modem_types::ModemTypeFREEDV700DLSB:
            _top_block->connect(_demod_valve,0,_freedv_rx700C_lsb,0);
            _top_block->connect(_freedv_rx700C_lsb,0,_rssi_valve,0);
            _top_block->connect(_freedv_rx700C_lsb,1,_audio_sink,0);
            break;
        case gr_modem_types::ModemTypeFREEDV800XALSB:
            _top_block->connect(_demod_valve,0,_freedv_rx800XA_lsb,0);
            _top_block->connect(_freedv_rx800XA_lsb,0,_rssi_valve,0);
            _top_block->connect(_freedv_rx800XA_lsb,1,_audio_sink,0);
            break;
        case gr_modem_types::ModemTypeWBFM:
            //_carrier_offset = 250000;
            //_osmosdr_source->set_center_freq(_device_frequency - _carrier_offset);
            _top_block->connect(_demod_valve,0,_wfm,0);
            _top_block->connect(_wfm,0,_rssi_valve,0);
            _top_block->connect(_wfm,1,_audio_sink,0);
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

void gr_demod_base::start()
{
    _audio_sink->flush();
    _vector_sink->flush();
    _top_block->start();
}

void gr_demod_base::stop()
{
    _top_block->stop();
    _audio_sink->flush();
    _vector_sink->flush();
    _top_block->wait();
}

std::vector<unsigned char>* gr_demod_base::getData(int nr)
{
    if(!_demod_running)
    {
        std::vector<unsigned char> *dummy = new std::vector<unsigned char>;
        return dummy;
    }
    std::vector<unsigned char> *data;
    if(nr == 1)
    {
        switch(_mode)
        {
        case gr_modem_types::ModemType2FSK2000:
            data = _deframer1->get_data();
            break;
        case gr_modem_types::ModemType2FSK20000:
            data = _deframer1_10k->get_data();
            break;
        case gr_modem_types::ModemTypeBPSK1000:
            data = _deframer_700_1->get_data();
            break;
        case gr_modem_types::ModemTypeBPSK2000:
            data = _deframer1->get_data();
            break;
        }
    }
    else if(nr == 2)
    {
        switch(_mode)
        {
        case gr_modem_types::ModemType2FSK2000:
            data = _deframer2->get_data();
            break;
        case gr_modem_types::ModemType2FSK20000:
            data = _deframer2_10k->get_data();
            break;
        case gr_modem_types::ModemTypeBPSK1000:
            data = _deframer_700_2->get_data();
            break;
        case gr_modem_types::ModemTypeBPSK2000:
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
        std::vector<unsigned char> *dummy = new std::vector<unsigned char>;
        return dummy;
    }
    std::vector<unsigned char> *data = _vector_sink->get_data();
    return data;
}

std::vector<float>* gr_demod_base::getAudio()
{
    if(!_demod_running)
    {
        std::vector<float> *dummy = new std::vector<float>;
        return dummy;
    }
    std::vector<float> *data = _audio_sink->get_data();
    return data;
}

void gr_demod_base::getFFTData(float *fft_data,  unsigned int &fftSize)
{
    if(!_demod_running)
    {
        return;
    }
    _fft_sink->get_fft_data(fft_data, fftSize);
    return;
}


void gr_demod_base::tune(long center_freq)
{
    _device_frequency = center_freq;
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

void gr_demod_base::set_rx_sensitivity(double value)
{
    if (!_gain_range.empty())
    {

        double gain =  floor((double)_gain_range.start() + value*((double)_gain_range.stop()- (double)_gain_range.start()));
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

void gr_demod_base::enable_demodulator(bool value)
{
    _demod_valve->set_enabled(value);
}

void gr_demod_base::set_squelch(int value)
{
    _fm_2500->set_squelch(value);
    _fm_5000->set_squelch(value);
    _am->set_squelch(value);
    _usb->set_squelch(value);
    _lsb->set_squelch(value);
    _freedv_rx1600_usb->set_squelch(value);
    _wfm->set_squelch(value);
}

void gr_demod_base::set_ctcss(float value)
{
    _top_block->lock();
    _fm_2500->set_ctcss(value);
    _fm_5000->set_ctcss(value);
    _top_block->unlock();
}

void gr_demod_base::set_carrier_offset(long carrier_offset)
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
        std::vector<gr_complex> *dummy = new std::vector<gr_complex>;
        return dummy;
    }
    std::vector<gr_complex> *data = _constellation->get_data();
    return data;
}

void gr_demod_base::set_samp_rate(int samp_rate)
{
    _top_block->lock();
    _locked = true;
    _samp_rate = samp_rate;

    int decimation = (int) _samp_rate / 1000000;


    if(_samp_rate != 1000000)
    {
        try
        {
            _top_block->disconnect(_rotator,0, _demod_valve,0);
        }
        catch(std::invalid_argument e)
        {

        }
        try
        {
            _top_block->disconnect(_rotator,0, _resampler,0);
            _top_block->disconnect(_resampler,0, _demod_valve,0);
        }
        catch(std::invalid_argument e)
        {

        }
        _resampler.reset();
        std::vector<float> taps;
        int tw = std::min(_samp_rate/4, 1500000);
        taps = gr::filter::firdes::low_pass(1, _samp_rate, 480000, 100000, gr::filter::firdes::WIN_BLACKMAN_HARRIS);

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
        catch(std::invalid_argument e)
        {
            _top_block->disconnect(_rotator,0, _demod_valve,0);
        }
        try
        {
            _top_block->connect(_rotator,0, _demod_valve,0);
        }
        catch(std::invalid_argument e)
        {
        }
    }


    _rotator->set_phase_inc(2*M_PI*-_carrier_offset/_samp_rate);
    _osmosdr_source->set_center_freq(_device_frequency);
    _osmosdr_source->set_sample_rate(_samp_rate);
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
    _osmosdr_source->set_bandwidth(_osmo_filter_bw);
}
