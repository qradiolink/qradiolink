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

#include "gr_demod_mmdvm_multi2.h"

gr_demod_mmdvm_multi2_sptr make_gr_demod_mmdvm_multi2(BurstTimer *burst_timer, int num_channels,
                                                    int channel_separation, bool use_tdma,
                                                    int sps, int samp_rate, int carrier_freq,
                                                    int filter_width)
{

    return gnuradio::get_initial_sptr(new gr_demod_mmdvm_multi2(burst_timer, num_channels, channel_separation,
                                                               use_tdma, sps, samp_rate, carrier_freq,
                                                      filter_width));
}



gr_demod_mmdvm_multi2::gr_demod_mmdvm_multi2(BurstTimer *burst_timer, int num_channels, int channel_separation,
                                           bool use_tdma,
                                           int sps, int samp_rate, int carrier_freq,
                                            int filter_width) :
    gr::hier_block2 ("gr_demod_mmdvm_multi2",
                      gr::io_signature::make (1, 1, sizeof (gr_complex)),
                      gr::io_signature::make (0, 0, sizeof (short)))
{
    (void) sps, channel_separation;
    int min_c = std::min(num_channels, 4);
    if(num_channels > MAX_MMDVM_CHANNELS)
        num_channels = MAX_MMDVM_CHANNELS;
    _samp_rate = samp_rate;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;
    _samp_rate =samp_rate;
    _num_channels = num_channels;
    _use_tdma = use_tdma;
    float target_samp_rate = 24000;
    float intermediate_samp_rate = 600000.0f;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;
    float fm_demod_width = 12500.0f;


    std::vector<float> taps = gr::filter::firdes::low_pass_2(1, _samp_rate, _filter_width,
                                2000, 60, gr::fft::window::WIN_BLACKMAN_HARRIS);
    std::vector<float> intermediate_interp_taps = gr::filter::firdes::low_pass_2(1, intermediate_samp_rate,
                        _filter_width, 2000, 60, gr::fft::window::WIN_BLACKMAN_HARRIS);
    std::vector<float> filter_taps = gr::filter::firdes::low_pass_2(
                1, target_samp_rate, _filter_width, 2000, 60, gr::fft::window::WIN_BLACKMAN_HARRIS);
    //unsigned int resampler_delay = (intermediate_interp_taps.size() - 1) / 2;
    //unsigned int filter_delay = (filter_taps.size() - 1) / 2;
    //unsigned int channelizer_delay = (taps.size() - 1) / 2;

    for(int i = 0;i < _num_channels;i++)
    {
        _resampler[i] = gr::filter::rational_resampler_ccf::make(24, 25, intermediate_interp_taps);
        //_resampler[i]->declare_sample_delay(resampler_delay);
    }
    for(int i = 0;i < _num_channels;i++)
    {
        _filter[i] = gr::filter::fft_filter_ccf::make(1, filter_taps);
        //_filter[i]->declare_sample_delay(filter_delay);
    }
    for(int i = 0;i < _num_channels;i++)
    {
        _fm_demod[i] = gr::analog::quadrature_demod_cf::make(float(target_samp_rate)/(2*M_PI* float(fm_demod_width)));
    }
    for(int i = 0;i < _num_channels;i++)
    {
        _level_control[i] = gr::blocks::multiply_const_ff::make(1.0);
    }
    for(int i = 0;i < 10 - _num_channels;i++)
    {
        _null_sink[i] = gr::blocks::null_sink::make(sizeof(gr_complex));
    }
    for(int i = 0;i < _num_channels;i++)
    {
        _float_to_short[i] = gr::blocks::float_to_short::make(1, 32767.0);
    }
    for(int i = 0;i < _num_channels;i++)
    {
        _rssi_tag_block[i] = make_rssi_tag_block();
    }
    _channelizer = gr::filter::pfb_channelizer_ccf::make(10, taps, 1.0f);
    //_channelizer->declare_sample_delay(channelizer_delay);
    _channelizer->set_tag_propagation_policy(gr::block::TPP_ALL_TO_ALL);
    _stream_to_streams = gr::blocks::stream_to_streams::make(sizeof(gr_complex), 10);

    _mmdvm_sink = make_gr_mmdvm_sink(burst_timer, num_channels, true, _use_tdma);


    connect(self(),0,_stream_to_streams,0);
    for(int i=0;i<10;i++)
    {
        connect(_stream_to_streams, i, _channelizer, i);
    }
    uint32_t m = 1;
    for(int i = 0;i < _num_channels;i++)
    {
        //            5  6  7  8  9  0  1  2  3  4  5
        if(i <= 3)
        {
            connect(_channelizer,i,_resampler[i],0);
        }
        else if(i > 3)
        {
            connect(_channelizer,10 - m,_resampler[i],0);
            m++;
        }

        connect(_resampler[i],0,_filter[i],0);
        connect(_filter[i],0,_rssi_tag_block[i],0);
        connect(_rssi_tag_block[i],0,_fm_demod[i],0);
        connect(_fm_demod[i],0,_level_control[i],0);
        connect(_level_control[i],0,_float_to_short[i],0);
        connect(_float_to_short[i],0,_mmdvm_sink, i);
    }
    for(int i=0;i < 10 - _num_channels;i++)
    {
        connect(_channelizer, min_c + i, _null_sink[i], 0);
    }
}

void gr_demod_mmdvm_multi2::calibrate_rssi(float level)
{
    for(int i = 0;i < _num_channels;i++)
    {
        _rssi_tag_block[i]->calibrate_rssi(level);
    }
}



