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

#include "gr_demod_ssb_sdr.h"

gr_demod_ssb_sdr_sptr make_gr_demod_ssb_sdr(int sps, int samp_rate, int carrier_freq,
                                          int filter_width)
{
    std::vector<int> signature;
    signature.push_back(sizeof (gr_complex));
    signature.push_back(sizeof (float));
    return gnuradio::get_initial_sptr(new gr_demod_ssb_sdr(signature, sps, samp_rate, carrier_freq,
                                                      filter_width));
}



gr_demod_ssb_sdr::gr_demod_ssb_sdr(std::vector<int>signature, int sps, int samp_rate, int carrier_freq,
                                 int filter_width) :
    gr::hier_block2 ("gr_demod_ssb_sdr",
                      gr::io_signature::make (1, 1, sizeof (gr_complex)),
                      gr::io_signature::makev (2, 2, signature))
{
    _target_samp_rate = 8000;
    _samp_rate = samp_rate;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;

    std::vector<float> taps = gr::filter::firdes::low_pass(125, _samp_rate, _filter_width, _filter_width,
                                                           gr::filter::firdes::WIN_BLACKMAN_HARRIS);
    _resampler = gr::filter::rational_resampler_base_ccf::make(1,125,taps);

    _filter_usb = gr::filter::fft_filter_ccc::make(1, gr::filter::firdes::complex_band_pass(
                            1, _target_samp_rate, 200, _filter_width,600,gr::filter::firdes::WIN_BLACKMAN_HARRIS) );
    _filter_lsb = gr::filter::fft_filter_ccc::make(1, gr::filter::firdes::complex_band_pass(
                            1, _target_samp_rate, -_filter_width, -20,600,gr::filter::firdes::WIN_BLACKMAN_HARRIS) );
    _squelch = gr::analog::pwr_squelch_cc::make(-140,0.01,0,true);
    _feed_forward_agc = gr::analog::feedforward_agc_cc::make(32,1);
    _agc = gr::analog::agc2_cc::make(1e-2, 1e-4, 1, 3);
    _rail = gr::analog::rail_ff::make(-1.0, 1.0);
    _complex_to_real = gr::blocks::complex_to_real::make();
    _audio_gain = gr::blocks::multiply_const_ff::make(0.8);


    connect(self(),0,_resampler,0);
    if(!sps)
    {
        connect(_resampler,0,_filter_usb,0);
        connect(_filter_usb,0,self(),0);
        connect(_filter_usb,0,_squelch,0);
    }
    else
    {
        connect(_resampler,0,_filter_lsb,0);
        connect(_filter_lsb,0,self(),0);
        connect(_filter_lsb,0,_squelch,0);
    }
    connect(_squelch,0,_agc,0);
    connect(_agc,0,_complex_to_real,0);
    //connect(_feed_forward_agc,0,_complex_to_real,0);
    //connect(_agc,0,_complex_to_real,0);
    connect(_complex_to_real,0,_rail,0);
    connect(_rail,0,_audio_gain,0);
    connect(_audio_gain,0,self(),1);

}


void gr_demod_ssb_sdr::set_squelch(int value)
{
    _squelch->set_threshold(value);
}
