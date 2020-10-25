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

#include "gr_mod_ssb.h"

gr_mod_ssb_sptr make_gr_mod_ssb(int sps, int samp_rate, int carrier_freq,
                                          int filter_width, int sb)
{
    return gnuradio::get_initial_sptr(new gr_mod_ssb(sps, samp_rate, carrier_freq,
                                                      filter_width, sb));
}

gr_mod_ssb::gr_mod_ssb(int sps, int samp_rate, int carrier_freq,
                                 int filter_width, int sb) :
    gr::hier_block2 ("gr_mod_ssb_sdr",
                      gr::io_signature::make (1, 1, sizeof (float)),
                      gr::io_signature::make (1, 1, sizeof (gr_complex)))
{


    _samp_rate =samp_rate;
    _sps = sps;
    float target_samp_rate = 8000.0;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;

    _agc = gr::analog::agc2_ff::make(1, 1e-3, 0.5, 1);
    _agc->set_max_gain(100);
    _rail = gr::analog::rail_ff::make(-0.6, 0.6);
    _audio_filter = gr::filter::fft_filter_fff::make(
                1,gr::filter::firdes::band_pass_2(
                    1, target_samp_rate, 300, _filter_width, 200, 60, gr::filter::firdes::WIN_BLACKMAN_HARRIS));
    _float_to_complex = gr::blocks::float_to_complex::make();
    std::vector<float> interp_taps = gr::filter::firdes::low_pass_2(_sps, _samp_rate,
                        _filter_width, _filter_width, 60, gr::filter::firdes::WIN_BLACKMAN_HARRIS);

    _resampler = gr::filter::rational_resampler_base_ccf::make(_sps,1, interp_taps);
    _feed_forward_agc = gr::analog::feedforward_agc_cc::make(640,0.5);
    _amplify = gr::blocks::multiply_const_cc::make(1.8,1);
    _bb_gain = gr::blocks::multiply_const_cc::make(1,1);
    _filter_usb = gr::filter::fft_filter_ccc::make(1,gr::filter::firdes::complex_band_pass_2(
            1, target_samp_rate, 200, _filter_width, 200, 60, gr::filter::firdes::WIN_BLACKMAN_HARRIS));
    _filter_lsb = gr::filter::fft_filter_ccc::make(1,gr::filter::firdes::complex_band_pass_2(
            1, target_samp_rate, -_filter_width, -200, 200, 60, gr::filter::firdes::WIN_BLACKMAN_HARRIS));



    connect(self(),0,_agc,0);
    connect(_agc,0,_rail,0);
    connect(_rail,0,_audio_filter,0);
    connect(_audio_filter,0,_float_to_complex,0);
    if(!sb)
    {
        connect(_float_to_complex,0,_filter_usb,0);
        connect(_filter_usb,0,_amplify,0);
    }
    else
    {
        connect(_float_to_complex,0,_filter_lsb,0);
        connect(_filter_lsb,0,_amplify,0);
    }

    connect(_amplify,0,_bb_gain,0);
    //connect(_feed_forward_agc,0,_bb_gain,0);
    connect(_bb_gain,0,_resampler,0);
    connect(_resampler,0,self(),0);

}


void gr_mod_ssb::set_filter_width(int filter_width)
{
    _filter_width = filter_width;
    float target_samp_rate = 8000.0;
    std::vector<float> interp_taps = gr::filter::firdes::low_pass_2(_sps, _samp_rate,
                    _filter_width, _filter_width,60, gr::filter::firdes::WIN_BLACKMAN_HARRIS);

    std::vector<gr_complex> filter_usb_taps = gr::filter::firdes::complex_band_pass_2(
            1, target_samp_rate, 300, _filter_width, 250, 60, gr::filter::firdes::WIN_BLACKMAN_HARRIS);
    std::vector<gr_complex> filter_lsb_taps = gr::filter::firdes::complex_band_pass_2(
            1, target_samp_rate, -_filter_width, -300, 250, 60, gr::filter::firdes::WIN_BLACKMAN_HARRIS);

    _resampler->set_taps(interp_taps);
    _filter_usb->set_taps(filter_usb_taps);
    _filter_lsb->set_taps(filter_lsb_taps);
}

void gr_mod_ssb::set_bb_gain(float value)
{
    _bb_gain->set_k(value);
}

