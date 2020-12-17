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

#include "gr_demod_am.h"

gr_demod_am_sptr make_gr_demod_am(int sps, int samp_rate, int carrier_freq,
                                          int filter_width)
{
    std::vector<int> signature;
    signature.push_back(sizeof (gr_complex));
    signature.push_back(sizeof (float));
    return gnuradio::get_initial_sptr(new gr_demod_am(signature, sps, samp_rate, carrier_freq,
                                                      filter_width));
}



gr_demod_am::gr_demod_am(std::vector<int>signature, int sps, int samp_rate, int carrier_freq,
                                 int filter_width) :
    gr::hier_block2 ("gr_demod_am_sdr",
                      gr::io_signature::make (1, 1, sizeof (gr_complex)),
                      gr::io_signature::makev (2, 2, signature))
{
    (void) sps;
    _target_samp_rate = 20000;

    _samp_rate = samp_rate;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;

    std::vector<float> taps = gr::filter::firdes::low_pass(1, _samp_rate, _target_samp_rate/2, _target_samp_rate/2,
                                                           gr::filter::firdes::WIN_BLACKMAN_HARRIS);
    std::vector<float> audio_taps = gr::filter::firdes::low_pass(2, 2*_target_samp_rate, 3600, 600,
                                                                 gr::filter::firdes::WIN_BLACKMAN_HARRIS);
    _resampler = gr::filter::rational_resampler_base_ccf::make(1,50,taps);
    _audio_resampler = gr::filter::rational_resampler_base_fff::make(2,5, audio_taps);
    _filter = gr::filter::fft_filter_ccc::make(1, gr::filter::firdes::complex_band_pass_2(
                            1, _target_samp_rate, -_filter_width, _filter_width, 200, 90, gr::filter::firdes::WIN_BLACKMAN_HARRIS) );
    _squelch = gr::analog::pwr_squelch_cc::make(-140,0.01,0,true);
    _agc = gr::analog::agc2_ff::make(1e-1, 1e-1, 1.0, 1.0);
    _complex_to_mag = gr::blocks::complex_to_mag::make();
    std::vector<double> fft;
    fft.push_back(1);
    fft.push_back(-1);
    std::vector<double> ffd;
    ffd.push_back(0);
    ffd.push_back(0.9999);
    _iir_filter = gr::filter::iir_filter_ffd::make(fft,ffd);
    _audio_gain = gr::blocks::multiply_const_ff::make(0.99);
    _audio_filter = gr::filter::fft_filter_fff::make(
                1,gr::filter::firdes::low_pass(
                    1, 8000, 3600, 300, gr::filter::firdes::WIN_BLACKMAN_HARRIS));



    connect(self(),0,_resampler,0);
    connect(_resampler,0,_filter,0);
    connect(_filter,0,self(),0);
    connect(_filter,0,_squelch,0);
    connect(_squelch,0,_complex_to_mag,0);
    connect(_complex_to_mag,0,_agc,0);
    connect(_agc,0,_iir_filter,0);
    connect(_iir_filter,0,_audio_gain,0);
    connect(_audio_gain,0,_audio_resampler,0);
    connect(_audio_resampler,0,_audio_filter,0);
    connect(_audio_filter,0,self(),1);


}

void gr_demod_am::set_filter_width(int filter_width)
{
    _filter_width = filter_width;
    std::vector<gr_complex> filter_taps = gr::filter::firdes::complex_band_pass(
                1, _target_samp_rate, -_filter_width, _filter_width,1200,gr::filter::firdes::WIN_BLACKMAN_HARRIS);

    _filter->set_taps(filter_taps);
}

void gr_demod_am::set_agc_attack(float value)
{
    _agc->set_attack_rate(value);
}

void gr_demod_am::set_agc_decay(float value)
{
    _agc->set_decay_rate(value);
}

void gr_demod_am::set_squelch(int value)
{
    _squelch->set_threshold(value);
}
