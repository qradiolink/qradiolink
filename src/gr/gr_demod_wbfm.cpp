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

#include "gr_demod_wbfm.h"

gr_demod_wbfm_sptr make_gr_demod_wbfm(int sps, int samp_rate, int carrier_freq,
                                          int filter_width)
{
    std::vector<int> signature;
    signature.push_back(sizeof (gr_complex));
    signature.push_back(sizeof (float));
    return gnuradio::get_initial_sptr(new gr_demod_wbfm(signature, sps, samp_rate, carrier_freq,
                                                      filter_width));
}



gr_demod_wbfm::gr_demod_wbfm(std::vector<int>signature, int sps, int samp_rate, int carrier_freq,
                                 int filter_width) :
    gr::hier_block2 ("gr_demod_wbfm",
                      gr::io_signature::make (1, 1, sizeof (gr_complex)),
                      gr::io_signature::makev (2, 2, signature))
{
    (void) sps;
    _target_samp_rate = 200000;

    _samp_rate = samp_rate;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;
    gr::calculate_deemph_taps(8000, 50e-6, _ataps, _btaps);

    _de_emph_filter = gr::filter::iir_filter_ffd::make(_btaps, _ataps, false);

    std::vector<float> taps = gr::filter::firdes::low_pass(1, _samp_rate, _target_samp_rate/2,
                                    _target_samp_rate/2, gr::fft::window::WIN_BLACKMAN_HARRIS);
    std::vector<float> audio_taps = gr::filter::firdes::low_pass(1, _target_samp_rate, 4000, 2000,
                                                        gr::fft::window::WIN_BLACKMAN_HARRIS);
    _resampler = gr::filter::rational_resampler_ccf::make(1,5,taps);
    _audio_resampler = gr::filter::rational_resampler_fff::make(1,25, audio_taps);

    _filter = gr::filter::fft_filter_ccf::make(1, gr::filter::firdes::low_pass_2(
            1, _target_samp_rate, _filter_width, 600, 90, gr::fft::window::WIN_BLACKMAN_HARRIS) );

    _fm_demod = gr::analog::quadrature_demod_cf::make(_target_samp_rate/(2*M_PI* _filter_width));
    _squelch = gr::analog::pwr_squelch_cc::make(-140,0.01,0,true);
    _amplify = gr::blocks::multiply_const_ff::make(0.9);


    connect(self(),0,_resampler,0);
    connect(_resampler,0,_filter,0);
    connect(_filter,0,self(),0);
    connect(_filter,0,_squelch,0);
    connect(_squelch,0,_fm_demod,0);
    connect(_fm_demod,0,_amplify,0);
    connect(_amplify,0,_de_emph_filter,0);
    connect(_de_emph_filter,0,_audio_resampler,0);
    connect(_audio_resampler,0,self(),1);


}


void gr_demod_wbfm::set_filter_width(int filter_width)
{
    _filter_width = filter_width;
    std::vector<float> filter_taps = gr::filter::firdes::low_pass(
            1, _target_samp_rate, _filter_width,1200,gr::fft::window::WIN_BLACKMAN_HARRIS);

    _filter->set_taps(filter_taps);
    _fm_demod->set_gain(_target_samp_rate/(2*M_PI* _filter_width));
}


void gr_demod_wbfm::set_squelch(int value)
{
    _squelch->set_threshold(value);
}
