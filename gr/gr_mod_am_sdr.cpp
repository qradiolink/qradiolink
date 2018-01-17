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

#include "gr_mod_am_sdr.h"

gr_mod_am_sdr_sptr make_gr_mod_am_sdr(int sps, int samp_rate, int carrier_freq,
                                          int filter_width)
{
    return gnuradio::get_initial_sptr(new gr_mod_am_sdr(sps, samp_rate, carrier_freq,
                                                      filter_width));
}

gr_mod_am_sdr::gr_mod_am_sdr(int sps, int samp_rate, int carrier_freq,
                                 int filter_width) :
    gr::hier_block2 ("gr_mod_am_sdr",
                      gr::io_signature::make (1, 1, sizeof (float)),
                      gr::io_signature::make (1, 1, sizeof (gr_complex)))
{


    _samp_rate =samp_rate;
    float target_samp_rate = 8000.0;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;

    _signal_source = gr::analog::sig_source_f::make(target_samp_rate,gr::analog::GR_COS_WAVE, 0, 0.55);
    _audio_amplify = gr::blocks::multiply_const_ff::make(3,1);
    _multiply = gr::blocks::multiply_cc::make();
    _audio_filter = gr::filter::fft_filter_fff::make(
                1,gr::filter::firdes::low_pass(
                    1, target_samp_rate, _filter_width, 1200, gr::filter::firdes::WIN_HAMMING));
    _float_to_complex = gr::blocks::float_to_complex::make();
    std::vector<float> interp_taps = gr::filter::firdes::low_pass(1, target_samp_rate,
                                                        _filter_width, _filter_width);
    float rerate = (float)_samp_rate/target_samp_rate;
    _resampler = gr::filter::pfb_arb_resampler_ccf::make(rerate, interp_taps, 16);
    _amplify = gr::blocks::multiply_const_cc::make(20,1);
    _filter = gr::filter::fft_filter_ccc::make(
                1,gr::filter::firdes::complex_band_pass_2(
                    1, _samp_rate, -_filter_width, _filter_width, 1200, 120, gr::filter::firdes::WIN_BLACKMAN_HARRIS));



    connect(self(),0,_audio_amplify,0);
    connect(_audio_amplify,0,_audio_filter,0);
    connect(_audio_filter,0,_float_to_complex,0);
    connect(_signal_source,0,_float_to_complex,1);
    connect(_float_to_complex,0,_resampler,0);
    connect(_resampler,0,_amplify,0);
    connect(_amplify,0,_filter,0);

    connect(_filter,0,self(),0);
}


