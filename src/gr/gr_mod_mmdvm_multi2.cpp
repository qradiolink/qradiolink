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

#include "gr_mod_mmdvm_multi2.h"

gr_mod_mmdvm_multi2_sptr make_gr_mod_mmdvm_multi2(BurstTimer *burst_timer, int num_channels, int channel_separation,
                                                bool use_tdma,
                                                int sps, int samp_rate, int carrier_freq,
                                                int filter_width)
{
    return gnuradio::get_initial_sptr(new gr_mod_mmdvm_multi2(burst_timer, num_channels, channel_separation, use_tdma,
                                                             sps, samp_rate, carrier_freq,
                                                      filter_width));
}

gr_mod_mmdvm_multi2::gr_mod_mmdvm_multi2(BurstTimer *burst_timer, int num_channels, int channel_separation, bool use_tdma,
                                       int sps, int samp_rate, int carrier_freq,
                                 int filter_width) :
    gr::hier_block2 ("gr_mod_mmdvm_multi2",
                      gr::io_signature::make (0, 0, sizeof (short)),
                      gr::io_signature::make (1, 1, sizeof (gr_complex)))
{
    (void) channel_separation;
    if(num_channels > MAX_MMDVM_CHANNELS)
        num_channels = MAX_MMDVM_CHANNELS;
    _samp_rate =samp_rate;
    _sps = sps;
    int min_c = std::min(num_channels, 4);
    _num_channels = num_channels;
    _use_tdma = use_tdma;
    float target_samp_rate = 24000.0f;
    float intermediate_samp_rate = 600000.0f;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;

    std::vector<float> intermediate_interp_taps = gr::filter::firdes::low_pass_2(25, intermediate_samp_rate,
                        _filter_width, 2000, 60, gr::fft::window::WIN_BLACKMAN_HARRIS);
    std::vector<float> filter_taps = gr::filter::firdes::low_pass_2(
                1, target_samp_rate, _filter_width, 2000, 60, gr::fft::window::WIN_BLACKMAN_HARRIS);
    //unsigned int resampler_delay = (intermediate_interp_taps.size() - 1) / 2;
    //unsigned int filter_delay = (filter_taps.size() -1) / 2;

    for(int i = 0;i < 10 - _num_channels;i++)
    {
        _null_source[i] = gr::blocks::null_source::make(sizeof(gr_complex));
    }
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
        _resampler[i] = gr::filter::rational_resampler_ccf::make(25, 24, intermediate_interp_taps);
        //_resampler[i]->declare_sample_delay(resampler_delay);
    }
    for(int i = 0;i < _num_channels;i++)
    {
        _amplify[i] = gr::blocks::multiply_const_cc::make(0.8,1);
    }
    for(int i = 0;i < _num_channels;i++)
    {
        _filter[i] = gr::filter::fft_filter_ccf::make(1, filter_taps);
        //_filter[i]->declare_sample_delay(filter_delay);
    }
    for(int i = 0;i < MAX_MMDVM_CHANNELS;i++)
    {
        _zero_idle[i] = make_gr_zero_idle_bursts();
    }
    std::vector<float> synth_taps = gr::filter::firdes::low_pass_2(10, _samp_rate,
                                            _filter_width, 2000, 60, gr::fft::window::WIN_BLACKMAN_HARRIS);
    _synthesizer = gr::filter::pfb_synthesizer_ccf::make(10, synth_taps, false);
    //unsigned int pfb_delay = (synth_taps.size() - 1) / 2;
    //_synthesizer->declare_sample_delay(pfb_delay);
    _divide_level = gr::blocks::multiply_const_cc::make(1.0f / float(num_channels));
    _mmdvm_source = make_gr_mmdvm_source(burst_timer, num_channels, true, _use_tdma);
    _bb_gain = gr::blocks::multiply_const_cc::make(1,1);

    uint32_t m = 1;
    for(int i = 0;i < _num_channels; i++)
    {
        connect(_mmdvm_source, i, _short_to_float[i],0);
        connect(_short_to_float[i], 0, _audio_amplify[i],0);
        connect(_audio_amplify[i], 0, _fm_modulator[i],0);
        connect(_fm_modulator[i], 0, _filter[i],0);
        connect(_filter[i], 0, _amplify[i],0);
        connect(_amplify[i], 0, _resampler[i],0);
        connect(_resampler[i], 0, _zero_idle[i],0);
        //              6  7  8  9  0  1  2  3  4  5
        if(i<=3)
        {
            connect(_zero_idle[i], 0, _synthesizer, i);
        }
        else if(i > 3)
        {
            connect(_zero_idle[i], 0, _synthesizer, 10 - m);
            m++;
        }
    }
    for(int i=0;i < 10 - _num_channels;i++)
    {
        connect(_null_source[i], 0, _synthesizer, min_c + i);
    }


    connect(_synthesizer,0,_divide_level,0);
    connect(_divide_level,0,_bb_gain,0);
    connect(_bb_gain,0,self(),0);
}



void gr_mod_mmdvm_multi2::set_bb_gain(float value)
{
    _bb_gain->set_k(value);
}


