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

#include "gr_demod_mmdvm.h"

gr_demod_mmdvm_sptr make_gr_demod_mmdvm(int sps, int samp_rate, int carrier_freq,
                                          int filter_width)
{
    std::vector<int> signature;
    signature.push_back(sizeof (gr_complex));
    signature.push_back(sizeof (float));
    signature.push_back(sizeof (short));
    return gnuradio::get_initial_sptr(new gr_demod_mmdvm(signature, sps, samp_rate, carrier_freq,
                                                      filter_width));
}



gr_demod_mmdvm::gr_demod_mmdvm(std::vector<int>signature, int sps, int samp_rate, int carrier_freq,
                                 int filter_width) :
    gr::hier_block2 ("gr_demod_mmdvm",
                      gr::io_signature::make (1, 1, sizeof (gr_complex)),
                      gr::io_signature::makev (3, 3, signature))
{
    (void) sps;
    _target_samp_rate = 24000;
    _samp_rate = samp_rate;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;

    std::vector<float> taps = gr::filter::firdes::low_pass(3, 3*_samp_rate, _filter_width,
                                _filter_width, gr::filter::firdes::WIN_BLACKMAN_HARRIS);
    _resampler = gr::filter::rational_resampler_base_ccf::make(3,125, taps);

    _filter = gr::filter::fft_filter_ccf::make(1, gr::filter::firdes::low_pass_2(
            1, _target_samp_rate, _filter_width, _filter_width, 90 ,gr::filter::firdes::WIN_BLACKMAN_HARRIS) );

    _fm_demod = gr::analog::quadrature_demod_cf::make(float(_target_samp_rate)/(4*M_PI* float(_filter_width)));
    std::vector<float> audio_taps = gr::filter::firdes::low_pass(1, _target_samp_rate, 3500, 500,
                                                    gr::filter::firdes::WIN_BLACKMAN_HARRIS);
    _audio_resampler = gr::filter::rational_resampler_base_fff::make(1, 3, audio_taps);
    _squelch = gr::analog::pwr_squelch_cc::make(-140,0.01,0,true);
    _level_control = gr::blocks::multiply_const_ff::make(0.7);
    _float_to_short = gr::blocks::float_to_short::make(1, 32767.0);


    connect(self(),0,_resampler,0);

    connect(_resampler,0,_filter,0);
    connect(_filter,0,self(),0);
    connect(_filter,0,_squelch,0);
    connect(_squelch,0,_fm_demod,0);
    connect(_fm_demod,0,_level_control,0);
    connect(_level_control,0,_float_to_short,0);
    connect(_level_control,0,_audio_resampler,0);
    connect(_audio_resampler,0,self(),1);
    connect(_float_to_short,0,self(),2);


}


void gr_demod_mmdvm::set_squelch(int value)
{
    _squelch->set_threshold(value);
}
