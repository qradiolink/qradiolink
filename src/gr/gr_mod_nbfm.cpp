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

#include "gr_mod_nbfm.h"

gr_mod_nbfm_sptr make_gr_mod_nbfm(int sps, int samp_rate, int carrier_freq,
                                          int filter_width)
{
    return gnuradio::get_initial_sptr(new gr_mod_nbfm(sps, samp_rate, carrier_freq,
                                                      filter_width));
}

gr_mod_nbfm::gr_mod_nbfm(int sps, int samp_rate, int carrier_freq,
                                 int filter_width) :
    gr::hier_block2 ("gr_mod_nbfm",
                      gr::io_signature::make (1, 1, sizeof (float)),
                      gr::io_signature::make (1, 1, sizeof (gr_complex)))
{

    _samp_rate =samp_rate;
    _sps = sps;
    float target_samp_rate = 8000;
    float if_samp_rate = 50000;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;
    gr::calculate_preemph_taps(8000, 50e-6, _ataps, _btaps);

    _fm_modulator = gr::analog::frequency_modulator_fc::make(4*M_PI*_filter_width/if_samp_rate);
    _audio_amplify = gr::blocks::multiply_const_ff::make(0.99,1);
    _audio_filter = gr::filter::fft_filter_fff::make(
                1,gr::filter::firdes::band_pass_2(
                    1, target_samp_rate, 300, 3500, 200, 90, gr::fft::window::WIN_BLACKMAN_HARRIS));

    _pre_emph_filter = gr::filter::iir_filter_ffd::make(_btaps, _ataps, false);

    std::vector<float> if_taps = gr::filter::firdes::low_pass_2(25, if_samp_rate * 4,
                        _filter_width, 1200, 90, gr::fft::window::WIN_BLACKMAN_HARRIS);
    _if_resampler = gr::filter::rational_resampler_fff::make(25,4, if_taps);

    _tone_source = gr::analog::sig_source_f::make(target_samp_rate,gr::analog::GR_COS_WAVE,88.5,0.15);
    _add = gr::blocks::add_ff::make();

    std::vector<float> interp_taps = gr::filter::firdes::low_pass_2(_sps, _samp_rate,
                        _filter_width, 1200, 90, gr::fft::window::WIN_BLACKMAN_HARRIS);
    _resampler = gr::filter::rational_resampler_ccf::make(_sps,1, interp_taps);
    _amplify = gr::blocks::multiply_const_cc::make(0.8,1);
    _bb_gain = gr::blocks::multiply_const_cc::make(1,1);
    _filter = gr::filter::fft_filter_ccf::make(1,gr::filter::firdes::low_pass_2(
                1, if_samp_rate, _filter_width, 1200, 90, gr::fft::window::WIN_BLACKMAN_HARRIS));


    connect(self(),0,_audio_filter,0);
    connect(_audio_filter,0,_audio_amplify,0);
    connect(_audio_amplify,0,_pre_emph_filter,0);
    connect(_pre_emph_filter,0,_if_resampler,0);
    connect(_if_resampler,0,_fm_modulator,0);
    connect(_fm_modulator,0,_filter,0);
    connect(_filter,0,_amplify,0);
    connect(_amplify,0,_bb_gain,0);
    connect(_bb_gain,0,_resampler,0);
    connect(_resampler,0,self(),0);
}


void gr_mod_nbfm::set_filter_width(int filter_width)
{
    _filter_width = filter_width;
    float if_samp_rate = 50000;
    std::vector<float> if_taps = gr::filter::firdes::low_pass_2(25, if_samp_rate * 4,
                    _filter_width, _filter_width, 60, gr::fft::window::WIN_BLACKMAN_HARRIS);
    std::vector<float> filter_taps = gr::filter::firdes::low_pass_2(
                1, if_samp_rate, _filter_width, 1200, 60, gr::fft::window::WIN_BLACKMAN_HARRIS);
    std::vector<float> interp_taps = gr::filter::firdes::low_pass_2(_sps, _samp_rate,
                    _filter_width, _filter_width, 60, gr::fft::window::WIN_BLACKMAN_HARRIS);

    _if_resampler->set_taps(if_taps);
    _filter->set_taps(filter_taps);
    _resampler->set_taps(interp_taps);
    _fm_modulator->set_sensitivity(4*M_PI*_filter_width/if_samp_rate);
}

void gr_mod_nbfm::set_bb_gain(float value)
{
    _bb_gain->set_k(value);
}


void gr_mod_nbfm::set_ctcss(float value)
{
    if(value == 0)
    {
        _audio_amplify->set_k(0.98);
        try {
            disconnect(_audio_amplify,0,_add,0);
            disconnect(_add,0,_pre_emph_filter,0);
            disconnect(_tone_source,0,_add,1);
            connect(_audio_amplify,0,_pre_emph_filter,0);
        }
        catch(std::invalid_argument &e)
        {
        }
    }
    else
    {
        _audio_amplify->set_k(0.85);
        _tone_source->set_frequency(value);
        try {
            disconnect(_audio_amplify,0,_pre_emph_filter,0);
            connect(_audio_amplify,0,_add,0);
            connect(_add,0,_pre_emph_filter,0);
            connect(_tone_source,0,_add,1);
        }
        catch(std::invalid_argument &e)
        {
        }
    }
}
