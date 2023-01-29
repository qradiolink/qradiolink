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

gr_mod_mmdvm_multi_sptr make_gr_mod_mmdvm_multi(BurstTimer *burst_timer, int num_channels, int channel_separation,
                                                bool use_tdma,
                                                int sps, int samp_rate, int carrier_freq,
                                                int filter_width)
{
    return gnuradio::get_initial_sptr(new gr_mod_mmdvm_multi(burst_timer, num_channels, channel_separation, use_tdma,
                                                             sps, samp_rate, carrier_freq,
                                                      filter_width));
}

gr_mod_mmdvm_multi::gr_mod_mmdvm_multi(BurstTimer *burst_timer, int num_channels, int channel_separation, bool use_tdma,
                                       int sps, int samp_rate, int carrier_freq,
                                 int filter_width) :
    gr::hier_block2 ("gr_mod_mmdvm_multi",
                      gr::io_signature::make (0, 0, sizeof (short)),
                      gr::io_signature::make (1, 1, sizeof (gr_complex)))
{
    if(num_channels > MAX_MMDVM_CHANNELS)
        num_channels = MAX_MMDVM_CHANNELS;
    _samp_rate =samp_rate;
    _sps = sps;
    _num_channels = num_channels;
    _use_tdma = use_tdma;
    float target_samp_rate = 24000.0f;
    float intermediate_samp_rate = 240000.0f;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;
    int c_n_b = num_channels;
    if(c_n_b > 4)
        c_n_b = 4;
    int resamp_filter_width = c_n_b * channel_separation;
    int resamp_filter_slope = 25000;
    float carrier_offset = float(channel_separation);

    std::vector<float> intermediate_interp_taps = gr::filter::firdes::low_pass(10, intermediate_samp_rate,
                        _filter_width, 3500, gr::filter::firdes::WIN_BLACKMAN_HARRIS);
    std::vector<float> interp_taps = gr::filter::firdes::low_pass_2(2, _samp_rate,
                        resamp_filter_width, resamp_filter_slope, 60, gr::filter::firdes::WIN_BLACKMAN_HARRIS);


    for(int i = 0;i < _num_channels;i++)
    {
        _short_to_float[i] = gr::blocks::short_to_float::make(1, 32767.0);
    }
    for(int i = 0;i < _num_channels;i++)
    {
        _fm_modulator[i] = gr::analog::frequency_modulator_fc::make(2*M_PI*12500.0f/target_samp_rate);
    }
    for(int i = 0;i < _num_channels;i++)
    {
        _audio_amplify[i] = gr::blocks::multiply_const_ff::make(1.0,1);
    }
    for(int i = 0;i < _num_channels;i++)
    {
        _resampler[i] = gr::filter::rational_resampler_base_ccf::make(10, 1, intermediate_interp_taps);
    }
    for(int i = 0;i < _num_channels;i++)
    {
        _amplify[i] = gr::blocks::multiply_const_cc::make(0.8,1);
    }
    for(int i = 0;i < _num_channels;i++)
    {
        _filter[i] = gr::filter::fft_filter_ccf::make(1,gr::filter::firdes::low_pass(
                1, target_samp_rate, _filter_width, 3500, gr::filter::firdes::WIN_BLACKMAN_HARRIS));
    }
    for(int i = 0;i < _num_channels;i++)
    {
        int ct = i;
        if(i > 3)
            ct = 3 - i;
        _rotator[i] = gr::blocks::rotator_cc::make(2*M_PI * carrier_offset * ct / intermediate_samp_rate);
    }
    for(int i = 0;i < _num_channels;i++)
    {
        _zero_idle[i] = make_gr_zero_idle_bursts();
    }

    _add = gr::blocks::add_cc::make();
    _divide_level = gr::blocks::multiply_const_cc::make(1.0f / float(num_channels));
    _mmdvm_source = make_gr_mmdvm_source(burst_timer, num_channels, true, _use_tdma);
    _final_resampler = gr::filter::rational_resampler_base_ccf::make(2, 1, interp_taps);
    _bb_gain = gr::blocks::multiply_const_cc::make(1,1);


    for(int i = 0;i < num_channels; i++)
    {
        connect(_mmdvm_source, i, _short_to_float[i],0);
        connect(_short_to_float[i], 0, _audio_amplify[i],0);
        connect(_audio_amplify[i], 0, _fm_modulator[i],0);
        connect(_fm_modulator[i], 0, _zero_idle[i],0);
        connect(_zero_idle[i], 0, _filter[i],0);
        connect(_filter[i], 0, _amplify[i],0);
        connect(_amplify[i], 0, _resampler[i],0);
        if(i == 0)
        {
            connect(_resampler[i], 0, _add, i);
        }
        else
        {
            connect(_resampler[i], 0, _rotator[i],0);
            connect(_rotator[i], 0, _add, i);
        }
    }

    connect(_add,0,_divide_level,0);
    connect(_divide_level,0,_bb_gain,0);
    connect(_bb_gain,0,_final_resampler,0);
    connect(_final_resampler,0,self(),0);
}



void gr_mod_mmdvm_multi::set_bb_gain(float value)
{
    _bb_gain->set_k(value);
}


