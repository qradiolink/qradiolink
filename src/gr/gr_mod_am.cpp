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

#include "gr_mod_am.h"

gr_mod_am_sptr make_gr_mod_am(int sps, int samp_rate, int carrier_freq,
                                          int filter_width)
{
    return gnuradio::get_initial_sptr(new gr_mod_am(sps, samp_rate, carrier_freq,
                                                      filter_width));
}

gr_mod_am::gr_mod_am(int sps, int samp_rate, int carrier_freq,
                                 int filter_width) :
    gr::hier_block2 ("gr_mod_am",
                      gr::io_signature::make (1, 1, sizeof (float)),
                      gr::io_signature::make (1, 1, sizeof (gr_complex)))
{


    _samp_rate =samp_rate;
    _sps = sps;
    float target_samp_rate = 8000.0;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;

    _signal_source = gr::analog::sig_source_f::make(target_samp_rate,gr::analog::GR_COS_WAVE, 0, 0.5);
    _rail = gr::analog::rail_ff::make(-0.98, 0.98);
    _add = gr::blocks::add_ff::make();
    _audio_amplify = gr::blocks::multiply_const_ff::make(0.95,1);
    _agc = gr::analog::agc2_ff::make(1e-2, 1e-4, 1, 1);
    _agc->set_max_gain(1.0);
    _audio_filter = gr::filter::fft_filter_fff::make(
                1,gr::filter::firdes::band_pass_2(
                    1, 8000, 300, 3000, 200, 60, gr::fft::window::WIN_HAMMING));
    _float_to_complex = gr::blocks::float_to_complex::make();
    std::vector<float> interp_taps = gr::filter::firdes::low_pass(_sps, _samp_rate,
                                                        _filter_width, _filter_width);
    _feed_forward_agc = gr::analog::feedforward_agc_cc::make(1024,1);
    _resampler = gr::filter::rational_resampler_ccf::make(_sps, 1, interp_taps);
    _amplify = gr::blocks::multiply_const_cc::make(0.5,1);
    _bb_gain = gr::blocks::multiply_const_cc::make(1,1);
    _filter = gr::filter::fft_filter_ccc::make(1,gr::filter::firdes::complex_band_pass_2(
            1, _samp_rate, -_filter_width, _filter_width, 1200, 120, gr::fft::window::WIN_BLACKMAN_HARRIS));



    connect(self(),0,_agc,0);
    connect(_agc,0,_rail,0);
    connect(_rail,0,_audio_amplify,0);
    connect(_audio_amplify,0,_audio_filter,0);
    connect(_audio_filter,0,_add,0);
    connect(_signal_source,0,_add,1);
    connect(_add,0,_float_to_complex,0);
    connect(_float_to_complex,0,_resampler,0);
    connect(_resampler,0,_amplify,0);
    connect(_amplify,0,_bb_gain,0);
    connect(_bb_gain,0,_filter,0);
    connect(_filter,0,self(),0);
}

void gr_mod_am::set_filter_width(int filter_width)
{
    _filter_width = filter_width;
    std::vector<float> interp_taps = gr::filter::firdes::low_pass(_sps, _samp_rate,
                                                        _filter_width, _filter_width);
    std::vector<gr_complex> filter_taps = gr::filter::firdes::complex_band_pass_2(
            1, _samp_rate, -_filter_width, _filter_width, 1200, 120, gr::fft::window::WIN_BLACKMAN_HARRIS);

    _resampler->set_taps(interp_taps);
    _filter->set_taps(filter_taps);
}

void gr_mod_am::set_bb_gain(float value)
{
    _bb_gain->set_k(value);
}


