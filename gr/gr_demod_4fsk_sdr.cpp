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

#include "gr_demod_4fsk_sdr.h"

gr_demod_4fsk_sdr::gr_demod_4fsk_sdr(gr::qtgui::sink_c::sptr fft_gui, gr::qtgui::const_sink_c::sptr const_gui,
                                     gr::qtgui::number_sink::sptr rssi_gui, QObject *parent,
                                     int sps, int samp_rate, int carrier_freq,
                                     int filter_width, float mod_index, float device_frequency, float rf_gain) :
    QObject(parent)
{
    _target_samp_rate = 40000;
    _rssi = rssi_gui;
    const std::string device_args = "rtl=0";
    const std::string device_antenna = "TX/RX";
    _device_frequency = device_frequency;
    _samples_per_symbol = sps*4/25;
    _samp_rate =samp_rate;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;
    _modulation_index = mod_index;
    _top_block = gr::make_top_block("4fsk demodulator sdr");

    std::vector<int> map;
    map.push_back(0);
    map.push_back(1);
    map.push_back(2);
    map.push_back(3);

    std::vector<unsigned int> const_map;
    const_map.push_back(0);
    const_map.push_back(1);
    const_map.push_back(2);
    const_map.push_back(3);

    std::vector<int> pre_diff_code;

    std::vector<gr_complex> constellation_points;
    constellation_points.push_back(-1.5+0j);
    constellation_points.push_back(-0.5+0j);
    constellation_points.push_back(0.5+0j);
    constellation_points.push_back(1.5+0j);


    unsigned int flt_size = 32;
    gr::digital::constellation_expl_rect::sptr constellation = gr::digital::constellation_expl_rect::make(
                constellation_points,pre_diff_code,2,4,1,1,1,const_map);

    std::vector<float> taps = gr::filter::firdes::low_pass(flt_size, _samp_rate, _filter_width, 1000);
    std::vector<float> symbol_filter_taps = gr::filter::firdes::low_pass(1.0,
                                 _target_samp_rate, _target_samp_rate*0.75/_samples_per_symbol, _target_samp_rate*0.25/_samples_per_symbol);
    _resampler = gr::filter::rational_resampler_base_ccf::make(1, 25, taps);
    _signal_source = gr::analog::sig_source_c::make(_samp_rate,gr::analog::GR_COS_WAVE,-25000,1);
    _multiply = gr::blocks::multiply_cc::make();
    //_freq_transl_filter = gr::filter::freq_xlating_fir_filter_ccf::make(
    //            1,gr::filter::firdes::low_pass(
    //                1, _target_samp_rate, 2*_filter_width, 250000, gr::filter::firdes::WIN_HAMMING), 25000,
    //            _target_samp_rate);
    _filter = gr::filter::fft_filter_ccf::make(1, gr::filter::firdes::low_pass(
                                1, _target_samp_rate, _filter_width,600,gr::filter::firdes::WIN_HAMMING) );
    _freq_demod = gr::analog::quadrature_demod_cf::make(sps/(2*M_PI/2));
    _float_to_complex = gr::blocks::float_to_complex::make();
    _symbol_filter = gr::filter::fft_filter_ccf::make(1,symbol_filter_taps);
    float gain_mu = 0.025;
    _clock_recovery = gr::digital::clock_recovery_mm_cc::make(_samples_per_symbol, 0.025*gain_mu*gain_mu, 0.5, gain_mu,
                                                              0.015);
    _multiply_symbols = gr::blocks::multiply_const_cc::make(0.15);
    _diff_decoder = gr::digital::diff_decoder_bb::make(4);
    _map = gr::digital::map_bb::make(map);
    _unpack = gr::blocks::unpack_k_bits_bb::make(2);
    _descrambler = gr::digital::descrambler_bb::make(0x8A, 0x7F ,7);
    _constellation_receiver = gr::digital::constellation_decoder_cb::make(constellation);
    _vector_sink = make_gr_vector_sink();

    _mag_squared = gr::blocks::complex_to_mag_squared::make();
    _single_pole_filter = gr::filter::single_pole_iir_filter_ff::make(0.04);
    _log10 = gr::blocks::nlog10_ff::make();
    _multiply_const_ff = gr::blocks::multiply_const_ff::make(10);
    _moving_average = gr::blocks::moving_average_ff::make(25000,1,2000);
    _add_const = gr::blocks::add_const_ff::make(-110);

    _osmosdr_source = osmosdr::source::make(device_args);
    _osmosdr_source->set_center_freq(_device_frequency-25000);
    _osmosdr_source->set_sample_rate(_samp_rate);
    _osmosdr_source->set_freq_corr(39);
    _osmosdr_source->set_gain_mode(false);
    _osmosdr_source->set_dc_offset_mode(0);
    _osmosdr_source->set_iq_balance_mode(0);
    _osmosdr_source->set_antenna(device_antenna);
    osmosdr::gain_range_t range = _osmosdr_source->get_gain_range();
    if (!range.empty())
    {
        double gain =  range.start() + rf_gain*(range.stop()-range.start());
        _osmosdr_source->set_gain(gain);
    }
    else
    {
        _osmosdr_source->set_gain_mode(true);
    }
    _constellation = const_gui;
    _fft_gui = fft_gui;
    _top_block->connect(_osmosdr_source,0,_multiply,0);
    _top_block->connect(_signal_source,0,_multiply,1);
    _top_block->connect(_multiply,0,_fft_gui,0);
    _top_block->connect(_multiply,0,_resampler,0);
    _top_block->connect(_resampler,0,_filter,0);
    _top_block->connect(_filter,0,_freq_demod,0);
    _top_block->connect(_freq_demod,0,_float_to_complex,0);
    _top_block->connect(_float_to_complex,0,_symbol_filter,0);
    _top_block->connect(_symbol_filter,0,_clock_recovery,0);
    _top_block->connect(_clock_recovery,0,_multiply_symbols,0);
    _top_block->connect(_multiply_symbols,0,_constellation,0);
    _top_block->connect(_multiply_symbols,0,_constellation_receiver,0);
    _top_block->connect(_constellation_receiver,0,_map,0);
    _top_block->connect(_map,0,_diff_decoder,0);
    _top_block->connect(_diff_decoder,0,_unpack,0);
    _top_block->connect(_unpack,0,_descrambler,0);
    _top_block->connect(_descrambler,0,_vector_sink,0);

    _top_block->connect(_filter,0,_mag_squared,0);
    _top_block->connect(_mag_squared,0,_moving_average,0);
    _top_block->connect(_moving_average,0,_single_pole_filter,0);
    _top_block->connect(_single_pole_filter,0,_log10,0);
    _top_block->connect(_log10,0,_multiply_const_ff,0);
    _top_block->connect(_multiply_const_ff,0,_add_const,0);
    _top_block->connect(_add_const,0,_rssi,0);
}

void gr_demod_4fsk_sdr::start()
{
    _top_block->start();
}

void gr_demod_4fsk_sdr::stop()
{
    _top_block->stop();
    _top_block->wait();
}

std::vector<unsigned char>* gr_demod_4fsk_sdr::getData()
{
    std::vector<unsigned char> *data = _vector_sink->get_data();
    return data;
}

void gr_demod_4fsk_sdr::tune(long center_freq)
{
    _device_frequency = center_freq;
    _osmosdr_source->set_center_freq(_device_frequency-25000);
}

void gr_demod_4fsk_sdr::set_rx_sensitivity(int value)
{
    osmosdr::gain_range_t range = _osmosdr_source->get_gain_range();
    if (!range.empty())
    {
        double gain =  range.start() + value*(range.stop()-range.start());
        _osmosdr_source->set_gain(gain);
    }
}


