// Written by Adrian Musceac YO8RZZ , started September 2022.
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

#include "gr_mod_m17.h"

gr_mod_m17_sptr make_gr_mod_m17(int sps, int samp_rate, int carrier_freq,
                                          int filter_width)
{
    return gnuradio::get_initial_sptr(new gr_mod_m17(sps, samp_rate, carrier_freq,
                                                      filter_width));
}

gr_mod_m17::gr_mod_m17(int sps, int samp_rate, int carrier_freq,
                                 int filter_width) :
    gr::hier_block2 ("gr_mod_m17",
                      gr::io_signature::make (1, 1, sizeof (float)),
                      gr::io_signature::make (1, 1, sizeof (gr_complex)))
{

    _samp_rate =samp_rate;
    _sps = 125;
    float if_samp_rate = 48000;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;

    _fm_modulator = gr::analog::frequency_modulator_fc::make(4*M_PI*_filter_width/if_samp_rate);
    _audio_amplify = gr::blocks::multiply_const_ff::make(0.25, 1);

    std::vector<float> interp_taps = gr::filter::firdes::low_pass_2(_sps, _samp_rate * 6,
                        _filter_width, _filter_width, 90, gr::filter::firdes::WIN_BLACKMAN_HARRIS);
    _resampler = gr::filter::rational_resampler_base_ccf::make(_sps, 6, interp_taps);
    _amplify = gr::blocks::multiply_const_cc::make(0.8,1);
    _bb_gain = gr::blocks::multiply_const_cc::make(1,1);
    _filter = gr::filter::fft_filter_ccf::make(1,gr::filter::firdes::low_pass_2(
                1, if_samp_rate, _filter_width, _filter_width, 90, gr::filter::firdes::WIN_BLACKMAN_HARRIS));


    connect(self(),0,_audio_amplify,0);
    connect(_audio_amplify,0,_fm_modulator,0);
    connect(_fm_modulator,0,_filter,0);
    connect(_filter,0,_amplify,0);
    connect(_amplify,0,_bb_gain,0);
    connect(_bb_gain,0,_resampler,0);
    connect(_resampler,0,self(),0);
}


void gr_mod_m17::set_filter_width(int filter_width)
{
    _filter_width = filter_width;
    float if_samp_rate = 48000;
    std::vector<float> filter_taps = gr::filter::firdes::low_pass_2(
                1, if_samp_rate, _filter_width, _filter_width, 60, gr::filter::firdes::WIN_BLACKMAN_HARRIS);
    std::vector<float> interp_taps = gr::filter::firdes::low_pass_2(_sps, _samp_rate * 6,
                    _filter_width, _filter_width, 60, gr::filter::firdes::WIN_BLACKMAN_HARRIS);

    _filter->set_taps(filter_taps);
    _resampler->set_taps(interp_taps);
    _fm_modulator->set_sensitivity(4*M_PI*_filter_width/if_samp_rate);
}

void gr_mod_m17::set_bb_gain(float value)
{
    _bb_gain->set_k(value);
}

