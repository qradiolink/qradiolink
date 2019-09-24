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

#include "gr_mod_freedv.h"

gr_mod_freedv_sdr_sptr make_gr_mod_freedv_sdr(int sps, int samp_rate, int carrier_freq,
                                          int filter_width, int mode, int sb)
{
    return gnuradio::get_initial_sptr(new gr_mod_freedv_sdr(sps, samp_rate, carrier_freq,
                                                      filter_width, mode, sb));
}

gr_mod_freedv_sdr::gr_mod_freedv_sdr(int sps, int samp_rate, int carrier_freq,
                                 int filter_width, int mode, int sb) :
    gr::hier_block2 ("gr_mod_freedv_sdr",
                      gr::io_signature::make (1, 1, sizeof (float)),
                      gr::io_signature::make (1, 1, sizeof (gr_complex)))
{


    _samp_rate =samp_rate;
    float target_samp_rate = 8000.0;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;
    _audio_gain = gr::blocks::multiply_const_ff::make(0.15);
    _agc = gr::analog::agc2_ff::make(1e-1, 1e-3, 0.95, 1); // FIXME: this seems to hurt audio a bit
    _float_to_short = gr::blocks::float_to_short::make(1, 32765);
    _short_to_float = gr::blocks::short_to_float::make(1, 32765);
    _freedv = gr::vocoder::freedv_tx_ss::make(mode);
    _audio_filter = gr::filter::fft_filter_fff::make(
                1,gr::filter::firdes::band_pass(
                    1, target_samp_rate, 200, 3500, 350, gr::filter::firdes::WIN_BLACKMAN_HARRIS));

    _float_to_complex = gr::blocks::float_to_complex::make();
    std::vector<float> interp_taps = gr::filter::firdes::low_pass(125, _samp_rate,
                                                        _filter_width, 1200);

    _resampler = gr::filter::rational_resampler_base_ccf::make(125,1, interp_taps);
    _feed_forward_agc = gr::analog::feedforward_agc_cc::make(512,0.95);
    _amplify = gr::blocks::multiply_const_cc::make(0.7f,1);
    _bb_gain = gr::blocks::multiply_const_cc::make(1,1);
    if(sb == 0)
    {
        _filter = gr::filter::fft_filter_ccc::make(
                    1,gr::filter::firdes::complex_band_pass_2(
                        1, target_samp_rate, 200, _filter_width, 250, 120,
                        gr::filter::firdes::WIN_BLACKMAN_HARRIS));
    }
    else
    {
        _filter = gr::filter::fft_filter_ccc::make(
                    1,gr::filter::firdes::complex_band_pass_2(
                        1, target_samp_rate, -_filter_width, -200, 250, 120,
                        gr::filter::firdes::WIN_BLACKMAN_HARRIS));
    }



    connect(self(),0,_audio_filter,0);
    connect(_audio_filter,0,_agc,0);
    connect(_agc,0,_audio_gain,0);
    connect(_audio_gain,0,_float_to_short,0);
    connect(_float_to_short,0,_freedv,0);
    connect(_freedv,0,_short_to_float,0);
    connect(_short_to_float,0,_float_to_complex,0);

    connect(_float_to_complex,0,_filter,0);
    connect(_filter,0,_feed_forward_agc,0);
    connect(_feed_forward_agc,0,_resampler,0);
    connect(_resampler,0,_amplify,0);
    connect(_amplify,0,_bb_gain,0);
    connect(_bb_gain,0,self(),0);
}

void gr_mod_freedv_sdr::set_bb_gain(int value)
{
    _bb_gain->set_k(value);
}


