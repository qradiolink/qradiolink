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

#include "gr_demod_mmdvm_multi.h"

gr_demod_mmdvm_multi_sptr make_gr_demod_mmdvm_multi(BurstTimer *burst_timer, int sps, int samp_rate, int carrier_freq,
                                          int filter_width)
{

    return gnuradio::get_initial_sptr(new gr_demod_mmdvm_multi(burst_timer, sps, samp_rate, carrier_freq,
                                                      filter_width));
}



gr_demod_mmdvm_multi::gr_demod_mmdvm_multi(BurstTimer *burst_timer, int sps, int samp_rate, int carrier_freq,
                                 int filter_width) :
    gr::hier_block2 ("gr_demod_mmdvm_multi",
                      gr::io_signature::make (1, 1, sizeof (gr_complex)),
                      gr::io_signature::make (0, 0, sizeof (short)))
{
    (void) sps;
    _samp_rate = samp_rate;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;
    _samp_rate =samp_rate;
    float target_samp_rate = 24000;
    float intermediate_samp_rate = 200000;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;
    int resamp_filter_width = 65000;
    int resamp_filter_slope = 10000;
    float carrier_offset2 = -25000.0f;
    float carrier_offset3 = -50000.0f;

    std::vector<float> taps = gr::filter::firdes::low_pass(1, _samp_rate, resamp_filter_width,
                                resamp_filter_slope, gr::filter::firdes::WIN_BLACKMAN_HARRIS);
    std::vector<float> intermediate_interp_taps = gr::filter::firdes::low_pass_2(3, 3 * intermediate_samp_rate,
                        _filter_width, _filter_width/2, 60, gr::filter::firdes::WIN_BLACKMAN_HARRIS);
    _first_resampler = gr::filter::rational_resampler_base_ccf::make(1, 5, taps);
    _resampler1 = gr::filter::rational_resampler_base_ccf::make(3, 25, intermediate_interp_taps);
    _resampler2 = gr::filter::rational_resampler_base_ccf::make(3, 25, intermediate_interp_taps);
    _resampler3 = gr::filter::rational_resampler_base_ccf::make(3, 25, intermediate_interp_taps);


    _filter1 = gr::filter::fft_filter_ccf::make(1,gr::filter::firdes::low_pass_2(
                1, target_samp_rate, _filter_width, _filter_width/2, 90, gr::filter::firdes::WIN_BLACKMAN_HARRIS));
    _filter2 = gr::filter::fft_filter_ccf::make(1,gr::filter::firdes::low_pass_2(
                1, target_samp_rate, _filter_width, _filter_width/2, 90, gr::filter::firdes::WIN_BLACKMAN_HARRIS));
    _filter3 = gr::filter::fft_filter_ccf::make(1,gr::filter::firdes::low_pass_2(
                1, target_samp_rate, _filter_width, _filter_width/2, 90, gr::filter::firdes::WIN_BLACKMAN_HARRIS));


    _fm_demod1 = gr::analog::quadrature_demod_cf::make(float(target_samp_rate)/(4*M_PI* float(_filter_width)));
    _fm_demod2 = gr::analog::quadrature_demod_cf::make(float(target_samp_rate)/(4*M_PI* float(_filter_width)));
    _fm_demod3 = gr::analog::quadrature_demod_cf::make(float(target_samp_rate)/(4*M_PI* float(_filter_width)));
    _level_control1 = gr::blocks::multiply_const_ff::make(0.7);
    _level_control2 = gr::blocks::multiply_const_ff::make(0.7);
    _level_control3 = gr::blocks::multiply_const_ff::make(0.7);
    _float_to_short1 = gr::blocks::float_to_short::make(1, 32767.0);
    _float_to_short2 = gr::blocks::float_to_short::make(1, 32767.0);
    _float_to_short3 = gr::blocks::float_to_short::make(1, 32767.0);
    _rotator2 = gr::blocks::rotator_cc::make(2*M_PI*carrier_offset2/intermediate_samp_rate);
    _rotator3 = gr::blocks::rotator_cc::make(2*M_PI*carrier_offset3/intermediate_samp_rate);
    _mmdvm_sink1 = make_gr_mmdvm_sink(burst_timer, 0);
    _mmdvm_sink2 = make_gr_mmdvm_sink(burst_timer, 1);
    _mmdvm_sink3 = make_gr_mmdvm_sink(burst_timer, 2);


    connect(self(),0,_first_resampler,0);
    connect(_first_resampler,0,_resampler1,0);
    connect(_first_resampler,0,_rotator2,0);
    connect(_first_resampler,0,_rotator3,0);
    connect(_rotator2,0,_resampler2,0);
    connect(_rotator3,0,_resampler3,0);
    connect(_resampler1,0,_filter1,0);
    connect(_resampler2,0,_filter2,0);
    connect(_resampler3,0,_filter3,0);
    connect(_filter1,0,_fm_demod1,0);
    connect(_filter2,0,_fm_demod2,0);
    connect(_filter3,0,_fm_demod3,0);
    connect(_fm_demod1,0,_level_control1,0);
    connect(_fm_demod2,0,_level_control2,0);
    connect(_fm_demod3,0,_level_control3,0);
    connect(_level_control1,0,_float_to_short1,0);
    connect(_level_control2,0,_float_to_short2,0);
    connect(_level_control3,0,_float_to_short3,0);
    connect(_float_to_short1,0,_mmdvm_sink1,0);
    connect(_float_to_short2,0,_mmdvm_sink2,0);
    connect(_float_to_short3,0,_mmdvm_sink3,0);
}





