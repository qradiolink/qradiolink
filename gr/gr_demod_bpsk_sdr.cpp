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

#include "gr_demod_bpsk_sdr.h"

gr_demod_bpsk_sdr::gr_demod_bpsk_sdr(gr::qtgui::sink_c::sptr fft_gui, gr::qtgui::const_sink_c::sptr const_gui,
                                     gr::qtgui::number_sink::sptr rssi_gui, QObject *parent, int sps, int samp_rate, int carrier_freq,
                                     int filter_width, float mod_index, float device_frequency, float rf_gain,
                                     std::string device_args, std::string device_antenna, int freq_corr) :
    QObject(parent)
{
    _target_samp_rate = 20000;
    _rssi = rssi_gui;
    _device_frequency = device_frequency;
    _samples_per_symbol = sps/25;
    _samp_rate =samp_rate;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;
    _modulation_index = mod_index;
    _top_block = gr::make_top_block("bpsk demodulator sdr");

    float rerate = (float)_target_samp_rate/(float)_samp_rate;

    std::vector<int> polys;
    polys.push_back(109);
    polys.push_back(79);

    unsigned int flt_size = 32;

    std::vector<float> taps = gr::filter::firdes::low_pass(flt_size, _samp_rate, _filter_width, 1200);
    //_resampler = gr::filter::pfb_arb_resampler_ccf::make(rerate, taps, flt_size);
    _resampler = gr::filter::rational_resampler_base_ccf::make(1, 50, taps);
    _agc = gr::analog::agc2_cc::make(0.6e-1, 1e-3, 1, 1);
    _signal_source = gr::analog::sig_source_c::make(_samp_rate,gr::analog::GR_COS_WAVE,-25000,1);
    _multiply = gr::blocks::multiply_cc::make();
    _freq_transl_filter = gr::filter::freq_xlating_fir_filter_ccf::make(
                1,gr::filter::firdes::low_pass(
                    1, _target_samp_rate, 2*_filter_width ,250000, gr::filter::firdes::WIN_HAMMING), 25000,
                _target_samp_rate);
    _filter = gr::filter::fft_filter_ccf::make(1, gr::filter::firdes::low_pass(
                            1, _target_samp_rate, _filter_width,600,gr::filter::firdes::WIN_HAMMING) );
    float gain_mu = 0.025;
    _clock_recovery = gr::digital::clock_recovery_mm_cc::make(_samples_per_symbol, 0.025*gain_mu*gain_mu, 0.5, 0.175,
                                                              0.015);
    _costas_loop = gr::digital::costas_loop_cc::make(0.0628,2);
    _equalizer = gr::digital::cma_equalizer_cc::make(8,1,0.00005,1);
    _fll = gr::digital::fll_band_edge_cc::make(sps, 0.35, 32, 0.000628);
    _complex_to_real = gr::blocks::complex_to_real::make();
    _binary_slicer = gr::digital::binary_slicer_fb::make();
    _diff_decoder = gr::digital::diff_decoder_bb::make(2);
    gr::fec::code::cc_decoder::sptr cc_decoder = gr::fec::code::cc_decoder::make(100,7,2,polys,0,-1,CC_STREAMING);
    _fec_decoder = gr::fec::decoder::make(cc_decoder,1,1);
    _multiply_const_fec = gr::blocks::multiply_const_ff::make(48.0);
    _float_to_uchar = gr::blocks::float_to_uchar::make();
    _add_const_fec = gr::blocks::add_const_ff::make(128.0);
    _descrambler = gr::digital::descrambler_bb::make(0x8A, 0x7F ,7);
    _unpacked_to_packed = gr::blocks::unpacked_to_packed_bb::make(1,gr::GR_MSB_FIRST);
    _vector_sink = make_gr_vector_sink();

    _mag_squared = gr::blocks::complex_to_mag_squared::make();
    _single_pole_filter = gr::filter::single_pole_iir_filter_ff::make(0.04);
    _log10 = gr::blocks::nlog10_ff::make();
    _multiply_const_ff = gr::blocks::multiply_const_ff::make(10);
    _agc2 = gr::analog::agc2_ff::make(0.6e-1, 1e-3, 1, 1);
    _moving_average = gr::blocks::moving_average_ff::make(25000,1,2000);
    _add_const = gr::blocks::add_const_ff::make(-110);


    _osmosdr_source = osmosdr::source::make(device_args);
    _osmosdr_source->set_center_freq(_device_frequency-25000);
    _osmosdr_source->set_sample_rate(_samp_rate);
    _osmosdr_source->set_freq_corr(freq_corr);
    _osmosdr_source->set_gain_mode(false);
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

    const std::string name = "const";
    _constellation = const_gui;
    _fft_gui = fft_gui;
    _top_block->connect(_osmosdr_source,0,_multiply,0);
    _top_block->connect(_signal_source,0,_multiply,1);
    _top_block->connect(_multiply,0,_resampler,0);
    _top_block->connect(_multiply,0,_fft_gui,0);
    _top_block->connect(_resampler,0,_filter,0);
    _top_block->connect(_filter,0,_agc,0);
    _top_block->connect(_agc,0,_clock_recovery,0);
    //_top_block->connect(_fll,0,_clock_recovery,0);
    _top_block->connect(_clock_recovery,0,_equalizer,0);
    _top_block->connect(_equalizer,0,_costas_loop,0);
    _top_block->connect(_costas_loop,0,_complex_to_real,0);
    _top_block->connect(_costas_loop,0,_constellation,0);
    _top_block->connect(_complex_to_real,0,_multiply_const_fec,0);
    _top_block->connect(_multiply_const_fec,0,_add_const_fec,0);
    _top_block->connect(_add_const_fec,0,_float_to_uchar,0);
    _top_block->connect(_float_to_uchar,0,_fec_decoder,0);
    _top_block->connect(_fec_decoder,0,_diff_decoder,0);
    _top_block->connect(_diff_decoder,0,_descrambler,0);
    _top_block->connect(_descrambler,0,_vector_sink,0);

    _top_block->connect(_filter,0,_mag_squared,0);
    _top_block->connect(_mag_squared,0,_moving_average,0);
    _top_block->connect(_moving_average,0,_single_pole_filter,0);
    _top_block->connect(_single_pole_filter,0,_log10,0);
    _top_block->connect(_log10,0,_multiply_const_ff,0);
    _top_block->connect(_multiply_const_ff,0,_add_const,0);
    _top_block->connect(_add_const,0,_rssi,0);

}

void gr_demod_bpsk_sdr::start()
{
    _top_block->start();
}

void gr_demod_bpsk_sdr::stop()
{
    _top_block->stop();
    _top_block->wait();
}

std::vector<unsigned char>* gr_demod_bpsk_sdr::getData()
{
    std::vector<unsigned char> *data = _vector_sink->get_data();
    return data;
}

void gr_demod_bpsk_sdr::tune(long center_freq)
{
    _device_frequency = center_freq;
    _osmosdr_source->set_center_freq(_device_frequency-25000);
}

void gr_demod_bpsk_sdr::set_rx_sensitivity(float value)
{
    osmosdr::gain_range_t range = _osmosdr_source->get_gain_range();
    if (!range.empty())
    {

        double gain =  range.start() + value*(range.stop()-range.start());
        _osmosdr_source->set_gain(gain);
    }
}
