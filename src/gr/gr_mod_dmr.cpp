// Written by Adrian Musceac YO8RZZ , started October 2024.
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

#include "gr_mod_dmr.h"

gr_mod_dmr_sptr make_gr_mod_dmr(int sps, int samp_rate, int carrier_freq,
                                          int filter_width)
{
    return gnuradio::get_initial_sptr(new gr_mod_dmr(sps, samp_rate, carrier_freq,
                                                      filter_width));
}

gr_mod_dmr::gr_mod_dmr(int sps, int samp_rate, int carrier_freq,
                                 int filter_width) :
    gr::hier_block2 ("gr_mod_dmr",
                      gr::io_signature::make (1, 1, sizeof (char)),
                      gr::io_signature::make (1, 1, sizeof (gr_complex)))
{
    _samp_rate =samp_rate;
    _sps = sps;
    _samples_per_symbol = 5;
    float if_samp_rate = 24000;
    float symbol_rate = ((float)if_samp_rate / (float)_samples_per_symbol);
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;
    std::vector<float> constellation;
    constellation.push_back(-1.5);
    constellation.push_back(-0.5);
    constellation.push_back(0.5);
    constellation.push_back(1.5);

    std::vector<int> map;
    map.push_back(2);
    map.push_back(3);
    map.push_back(1);
    map.push_back(0);

    _packed_to_unpacked = gr::blocks::packed_to_unpacked_bb::make(1,gr::GR_MSB_FIRST);
    _packer = gr::blocks::pack_k_bits_bb::make(2);
    _map = gr::digital::map_bb::make(map);

    _chunks_to_symbols = gr::digital::chunks_to_symbols_bf::make(constellation);
    std::vector<float> first_resampler_taps = gr::filter::firdes::root_raised_cosine(_samples_per_symbol,
          if_samp_rate, symbol_rate, 0.2, 25 * _samples_per_symbol);
    unsigned int first_resampler_delay = (first_resampler_taps.size() - 1) / 2;
    _first_resampler = gr::filter::rational_resampler_fff::make(_samples_per_symbol, 1, first_resampler_taps);
    //_first_resampler->declare_sample_delay(first_resampler_delay / _samples_per_symbol);
    _scale_pulses = gr::blocks::multiply_const_ff::make(0.66666666, 1);

    _fm_modulator = gr::analog::frequency_modulator_fc::make((M_PI*symbol_rate*0.85)/if_samp_rate);
    _zero_idle = make_gr_zero_idle_bursts(first_resampler_delay);

    std::vector<float> interp_taps = gr::filter::firdes::low_pass_2(_sps, _samp_rate * 3,
                        _filter_width, 2000, 60, gr::fft::window::WIN_BLACKMAN_HARRIS);
    unsigned int interp_delay = (interp_taps.size() - 1) / (2);
    _resampler = gr::filter::rational_resampler_ccf::make(_sps, 3, interp_taps);
    //_resampler->declare_sample_delay(interp_delay * _sps);
    _amplify = gr::blocks::multiply_const_cc::make(0.9,1);
    _bb_gain = gr::blocks::multiply_const_cc::make(1,1);
    _filter = gr::filter::fft_filter_ccf::make(1,gr::filter::firdes::low_pass(
                1, if_samp_rate, _filter_width, _filter_width, gr::fft::window::WIN_BLACKMAN_HARRIS));


    connect(self(),0,_packed_to_unpacked,0);
    connect(_packed_to_unpacked,0,_packer,0);
    connect(_packer,0,_map,0);
    connect(_map,0,_chunks_to_symbols,0);
    connect(_chunks_to_symbols,0,_first_resampler,0);
    connect(_first_resampler,0,_scale_pulses,0);
    connect(_scale_pulses,0,_fm_modulator,0);
    connect(_fm_modulator,0,_zero_idle,0);
    connect(_zero_idle,0,_amplify,0);
    connect(_amplify,0,_bb_gain,0);
    connect(_bb_gain,0,_resampler,0);
    connect(_resampler,0,self(),0);
}


void gr_mod_dmr::set_bb_gain(float value)
{
    _bb_gain->set_k(value);
}


