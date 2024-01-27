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

gr_demod_mmdvm_multi_sptr make_gr_demod_mmdvm_multi(BurstTimer *burst_timer, int num_channels,
                                                    int channel_separation, bool use_tdma,
                                                    int sps, int samp_rate, int carrier_freq,
                                                    int filter_width)
{

    return gnuradio::get_initial_sptr(new gr_demod_mmdvm_multi(burst_timer, num_channels, channel_separation,
                                                               use_tdma, sps, samp_rate, carrier_freq,
                                                      filter_width));
}



gr_demod_mmdvm_multi::gr_demod_mmdvm_multi(BurstTimer *burst_timer, int num_channels, int channel_separation,
                                           bool use_tdma,
                                           int sps, int samp_rate, int carrier_freq,
                                            int filter_width) :
    gr::hier_block2 ("gr_demod_mmdvm_multi",
                      gr::io_signature::make (1, 1, sizeof (gr_complex)),
                      gr::io_signature::make (0, 0, sizeof (short)))
{
    (void) sps;
    if(num_channels > MAX_MMDVM_CHANNELS)
        num_channels = MAX_MMDVM_CHANNELS;
    _samp_rate = samp_rate;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;
    _samp_rate =samp_rate;
    _num_channels = num_channels;
    _use_tdma = use_tdma;
    float target_samp_rate = 24000;
    float intermediate_samp_rate = 240000;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;
    float fm_demod_width = 12500.0f;
    int c_n_b = num_channels;
    if(c_n_b > 4)
        c_n_b = 4;
    int resamp_filter_width = c_n_b * channel_separation;
    int resamp_filter_slope = 25000;
    float carrier_offset = float(-channel_separation);


    std::vector<float> taps = gr::filter::firdes::low_pass(1, _samp_rate, resamp_filter_width,
                                resamp_filter_slope, gr::fft::window::WIN_BLACKMAN_HARRIS);
    std::vector<float> intermediate_interp_taps = gr::filter::firdes::low_pass(1, intermediate_samp_rate,
                        _filter_width, 3500, gr::fft::window::WIN_BLACKMAN_HARRIS);


    for(int i = 0;i < _num_channels;i++)
    {
        _resampler[i] = gr::filter::rational_resampler_ccf::make(1, 10, intermediate_interp_taps);
    }
    for(int i = 0;i < _num_channels;i++)
    {
        _filter[i] = gr::filter::fft_filter_ccf::make(1,gr::filter::firdes::low_pass(
                1, target_samp_rate, _filter_width, 3500, gr::fft::window::WIN_BLACKMAN_HARRIS));
    }
    for(int i = 0;i < _num_channels;i++)
    {
        _fm_demod[i] = gr::analog::quadrature_demod_cf::make(float(target_samp_rate)/(2*M_PI* float(fm_demod_width)));
    }
    for(int i = 0;i < _num_channels;i++)
    {
        _level_control[i] = gr::blocks::multiply_const_ff::make(1.0);
    }
    for(int i = 0;i < _num_channels;i++)
    {
        _float_to_short[i] = gr::blocks::float_to_short::make(1, 32767.0);
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
        _rssi_tag_block[i] = make_rssi_tag_block();
    }

    _mmdvm_sink = make_gr_mmdvm_sink(burst_timer, num_channels, true, _use_tdma);
    _first_resampler = gr::filter::rational_resampler_ccf::make(1, 5, taps);


    //connect(self(),0,_first_resampler,0);
    for(int i = 0;i < num_channels;i++)
    {
        if(i == 0)
        {
            connect(self(),0,_resampler[i],0);
        }
        else
        {
            connect(self(),0,_rotator[i],0);
            connect(_rotator[i],0,_resampler[i],0);
        }
        connect(_resampler[i],0,_filter[i],0);
        connect(_filter[i],0,_rssi_tag_block[i],0);
        connect(_rssi_tag_block[i],0,_fm_demod[i],0);
        connect(_fm_demod[i],0,_level_control[i],0);
        connect(_level_control[i],0,_float_to_short[i],0);
        connect(_float_to_short[i],0,_mmdvm_sink, i);
    }
}

void gr_demod_mmdvm_multi::calibrate_rssi(float level)
{
    for(int i = 0;i < _num_channels;i++)
    {
        _rssi_tag_block[i]->calibrate_rssi(level);
    }
}



