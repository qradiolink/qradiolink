// Written by Adrian Musceac YO8RZZ , started July 2021.
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

#include "gr_mod_mmdvm.h"

gr_mod_mmdvm_sptr make_gr_mod_mmdvm(int sps, int samp_rate, int carrier_freq,
                                          int filter_width)
{
    return gnuradio::get_initial_sptr(new gr_mod_mmdvm(sps, samp_rate, carrier_freq,
                                                      filter_width));
}

gr_mod_mmdvm::gr_mod_mmdvm(int sps, int samp_rate, int carrier_freq,
                                 int filter_width) :
    gr::hier_block2 ("gr_mod_mmdvm",
                      gr::io_signature::make (1, 1, sizeof (short)),
                      gr::io_signature::make (1, 1, sizeof (gr_complex)))
{

    _samp_rate =samp_rate;
    _sps = sps;
    float target_samp_rate = 24000.0f;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;

    _short_to_float = gr::blocks::short_to_float::make(1, 32767.0);
    _fm_modulator = gr::analog::frequency_modulator_fc::make(2*M_PI*12500.0f/target_samp_rate);
    _audio_amplify = gr::blocks::multiply_const_ff::make(1.0,1);


    std::vector<float> interp_taps = gr::filter::firdes::low_pass_2(250, 250 * target_samp_rate,
                        _filter_width, 2000, 90, gr::filter::firdes::WIN_BLACKMAN_HARRIS);
    _resampler = gr::filter::rational_resampler_base_ccf::make(250,24, interp_taps);
    _amplify = gr::blocks::multiply_const_cc::make(0.8,1);
    _bb_gain = gr::blocks::multiply_const_cc::make(1,1);
    _filter = gr::filter::fft_filter_ccf::make(1,gr::filter::firdes::low_pass_2(
                1, target_samp_rate, _filter_width, 2000, 90, gr::filter::firdes::WIN_BLACKMAN_HARRIS));
    _zero_idle_bursts = make_gr_zero_idle_bursts();


    connect(self(),0,_short_to_float,0);
    connect(_short_to_float,0,_audio_amplify,0);
    connect(_audio_amplify,0,_fm_modulator,0);
    connect(_fm_modulator,0,_zero_idle_bursts,0);
    connect(_zero_idle_bursts,0,_filter,0);
    connect(_filter,0,_amplify,0);
    connect(_amplify,0,_bb_gain,0);
    connect(_bb_gain,0,_resampler,0);
    connect(_resampler,0,self(),0);
}



void gr_mod_mmdvm::set_bb_gain(float value)
{
    _bb_gain->set_k(value);
}


