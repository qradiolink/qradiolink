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

#include "gr_demod_freedv.h"

gr_demod_freedv_sptr make_gr_demod_freedv(int sps, int samp_rate, int carrier_freq,
                                          int filter_width)
{
    std::vector<int> signature;
    signature.push_back(sizeof (gr_complex));
    signature.push_back(sizeof (float));
    return gnuradio::get_initial_sptr(new gr_demod_freedv(signature, sps, samp_rate, carrier_freq,
                                                      filter_width));
}



gr_demod_freedv::gr_demod_freedv(std::vector<int>signature, int sps, int samp_rate, int carrier_freq,
                                 int filter_width) :
    gr::hier_block2 ("gr_demod_freedv",
                      gr::io_signature::make (1, 1, sizeof (gr_complex)),
                      gr::io_signature::makev (2, 2, signature))
{
    _target_samp_rate = 8000;
    _samp_rate = samp_rate;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;

    std::vector<float> taps = gr::filter::firdes::low_pass(1, _samp_rate, _filter_width, _filter_width,
                                                           gr::filter::firdes::WIN_BLACKMAN_HARRIS);
    _resampler = gr::filter::rational_resampler_base_ccf::make(1,125,taps);

    _filter = gr::filter::fft_filter_ccc::make(125, gr::filter::firdes::complex_band_pass(
                            1, _target_samp_rate, 200, _filter_width,600,gr::filter::firdes::WIN_BLACKMAN_HARRIS) );
    _feed_forward_agc = gr::analog::feedforward_agc_cc::make(16,1);
    _agc = gr::analog::agc2_cc::make(1, 1e-3, 1, 1);
    _complex_to_real = gr::blocks::complex_to_real::make();
    _float_to_short = gr::blocks::float_to_short::make(1, 0.1);
    _freedv = gr::vocoder::freedv_rx_ss::make();
    _short_to_float = gr::blocks::short_to_float::make();
    _audio_gain = gr::blocks::multiply_const_ff::make(0.9);


    connect(self(),0,_resampler,0);

    connect(_resampler,0,_filter,0);
    connect(_filter,0,self(),0);
    connect(_filter,0,_complex_to_real,0);
    //connect(_agc,0,_complex_to_real,0);
    connect(_complex_to_real,0, _float_to_short,0);
    connect(_float_to_short, 0, _freedv, 0);
    connect(_freedv, 0 , _short_to_float, 0);
    connect(_short_to_float,0,_audio_gain,0);
    connect(_audio_gain,0,self(),1);

}


void gr_demod_freedv::set_squelch(int value)
{
    _freedv->set_squelch_thresh(value);
}
