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

#include "gr_mod_mmdvm_multi.h"

gr_mod_mmdvm_multi_sptr make_gr_mod_mmdvm_multi(BurstTimer *burst_timer, int sps, int samp_rate, int carrier_freq,
                                          int filter_width)
{
    return gnuradio::get_initial_sptr(new gr_mod_mmdvm_multi(burst_timer, sps, samp_rate, carrier_freq,
                                                      filter_width));
}

gr_mod_mmdvm_multi::gr_mod_mmdvm_multi(BurstTimer *burst_timer, int sps, int samp_rate, int carrier_freq,
                                 int filter_width) :
    gr::hier_block2 ("gr_mod_mmdvm_multi",
                      gr::io_signature::make (0, 0, sizeof (short)),
                      gr::io_signature::make (1, 1, sizeof (gr_complex)))
{

    _samp_rate =samp_rate;
    _sps = sps;
    float target_samp_rate = 24000;
    float intermediate_samp_rate = 200000;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;
    int resamp_filter_width = 60000;
    int resamp_filter_slope = 10000;
    float carrier_offset2 = 25000;
    float carrier_offset3 = 50000;


    _short_to_float1 = gr::blocks::short_to_float::make(1, 32767.0);
    _short_to_float2 = gr::blocks::short_to_float::make(1, 32767.0);
    _short_to_float3 = gr::blocks::short_to_float::make(1, 32767.0);
    _fm_modulator1 = gr::analog::frequency_modulator_fc::make(4*M_PI*_filter_width/target_samp_rate);
    _fm_modulator2 = gr::analog::frequency_modulator_fc::make(4*M_PI*_filter_width/target_samp_rate);
    _fm_modulator3 = gr::analog::frequency_modulator_fc::make(4*M_PI*_filter_width/target_samp_rate);
    _audio_amplify1 = gr::blocks::multiply_const_ff::make(0.9,1);
    _audio_amplify2 = gr::blocks::multiply_const_ff::make(0.9,1);
    _audio_amplify3 = gr::blocks::multiply_const_ff::make(0.9,1);

    std::vector<float> intermediate_interp_taps = gr::filter::firdes::low_pass_2(3, 3*intermediate_samp_rate,
                        _filter_width, _filter_width, 60, gr::filter::firdes::WIN_BLACKMAN_HARRIS);
    std::vector<float> interp_taps = gr::filter::firdes::low_pass_2(5, _samp_rate,
                        resamp_filter_width, resamp_filter_slope, 60, gr::filter::firdes::WIN_BLACKMAN_HARRIS);
    _resampler1 = gr::filter::rational_resampler_base_ccf::make(25, 3, intermediate_interp_taps);
    _resampler2 = gr::filter::rational_resampler_base_ccf::make(25, 3, intermediate_interp_taps);
    _resampler3 = gr::filter::rational_resampler_base_ccf::make(25, 3, intermediate_interp_taps);
    _final_resampler = gr::filter::rational_resampler_base_ccf::make(5, 1, interp_taps);
    _amplify1 = gr::blocks::multiply_const_cc::make(0.8,1);
    _amplify2 = gr::blocks::multiply_const_cc::make(0.8,1);
    _amplify3 = gr::blocks::multiply_const_cc::make(0.8,1);
    _bb_gain = gr::blocks::multiply_const_cc::make(1,1);
    _filter1 = gr::filter::fft_filter_ccf::make(1,gr::filter::firdes::low_pass_2(
                1, target_samp_rate, _filter_width, _filter_width/2, 90, gr::filter::firdes::WIN_BLACKMAN_HARRIS));
    _filter2 = gr::filter::fft_filter_ccf::make(1,gr::filter::firdes::low_pass_2(
                1, target_samp_rate, _filter_width, _filter_width/2, 90, gr::filter::firdes::WIN_BLACKMAN_HARRIS));
    _filter3 = gr::filter::fft_filter_ccf::make(1,gr::filter::firdes::low_pass_2(
                1, target_samp_rate, _filter_width, _filter_width/2, 90, gr::filter::firdes::WIN_BLACKMAN_HARRIS));
    _rotator2 = gr::blocks::rotator_cc::make(2*M_PI*carrier_offset2/intermediate_samp_rate);
    _rotator3 = gr::blocks::rotator_cc::make(2*M_PI*carrier_offset3/intermediate_samp_rate);
    _add = gr::blocks::add_cc::make();
    _divide_level = gr::blocks::multiply_const_cc::make(0.33);
    _mmdvm_source1 = make_gr_mmdvm_source(burst_timer, 1);
    _mmdvm_source2 = make_gr_mmdvm_source(burst_timer, 2);
    _mmdvm_source3 = make_gr_mmdvm_source(burst_timer, 3);



    connect(_mmdvm_source1,0,_short_to_float1,0);
    connect(_mmdvm_source2,0,_short_to_float2,0);
    connect(_mmdvm_source3,0,_short_to_float3,0);
    connect(_short_to_float1,0,_audio_amplify1,0);
    connect(_short_to_float2,0,_audio_amplify2,0);
    connect(_short_to_float3,0,_audio_amplify3,0);
    connect(_audio_amplify1,0,_fm_modulator1,0);
    connect(_audio_amplify2,0,_fm_modulator2,0);
    connect(_audio_amplify3,0,_fm_modulator3,0);
    connect(_fm_modulator1,0,_filter1,0);
    connect(_fm_modulator2,0,_filter2,0);
    connect(_fm_modulator3,0,_filter3,0);
    connect(_filter1,0,_amplify1,0);
    connect(_filter2,0,_amplify2,0);
    connect(_filter3,0,_amplify3,0);
    connect(_amplify1,0,_resampler1,0);
    connect(_amplify2,0,_resampler2,0);
    connect(_amplify3,0,_resampler3,0);
    connect(_resampler2,0,_rotator2,0);
    connect(_resampler3,0,_rotator3,0);
    connect(_resampler1,0,_add,0);
    connect(_rotator2,0,_add,1);
    connect(_rotator3,0,_add,2);
    connect(_add,0,_divide_level,0);
    connect(_divide_level,0,_bb_gain,0);
    connect(_bb_gain,0,_final_resampler,0);
    connect(_final_resampler,0,self(),0);
}



void gr_mod_mmdvm_multi::set_bb_gain(float value)
{
    _bb_gain->set_k(value);
}


