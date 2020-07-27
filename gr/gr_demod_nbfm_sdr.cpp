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

#include "gr_demod_nbfm_sdr.h"

gr_demod_nbfm_sdr_sptr make_gr_demod_nbfm_sdr(int sps, int samp_rate, int carrier_freq,
                                          int filter_width)
{
    std::vector<int> signature;
    signature.push_back(sizeof (gr_complex));
    signature.push_back(sizeof (float));
    return gnuradio::get_initial_sptr(new gr_demod_nbfm_sdr(signature, sps, samp_rate, carrier_freq,
                                                      filter_width));
}



gr_demod_nbfm_sdr::gr_demod_nbfm_sdr(std::vector<int>signature, int sps, int samp_rate, int carrier_freq,
                                 int filter_width) :
    gr::hier_block2 ("gr_demod_nbfm_sdr",
                      gr::io_signature::make (1, 1, sizeof (gr_complex)),
                      gr::io_signature::makev (2, 2, signature))
{
    (void) sps;
    _target_samp_rate = 20000;

    _samp_rate = samp_rate;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;
    calculate_deemph_taps(_target_samp_rate, 44e-6);

    /* unused
    static const float coeff[] = {0.06306464970111847, 0.4777590036392212, 0.9183526635169983,
                                  0.4777590036392212, 0.06306464970111847};
    std::vector<float> deemph_taps(coeff, coeff + sizeof(coeff) / sizeof(coeff[0]) );
    */

    _de_emph_filter = gr::filter::iir_filter_ffd::make(_ataps, _btaps, false);

    std::vector<float> taps = gr::filter::firdes::low_pass(50, _samp_rate, _target_samp_rate/2,
                                _target_samp_rate/2, gr::filter::firdes::WIN_BLACKMAN_HARRIS);
    std::vector<float> audio_taps = gr::filter::firdes::low_pass_2(2, 2*_target_samp_rate, 3600, 250, 60,
                                                    gr::filter::firdes::WIN_BLACKMAN_HARRIS);
    _resampler = gr::filter::rational_resampler_base_ccf::make(1,50, taps);
    _audio_resampler = gr::filter::rational_resampler_base_fff::make(2,5, audio_taps);

    _filter = gr::filter::fft_filter_ccf::make(1, gr::filter::firdes::low_pass(
                    1, _target_samp_rate, _filter_width,1200,gr::filter::firdes::WIN_BLACKMAN_HARRIS) );

    _fm_demod = gr::analog::quadrature_demod_cf::make(_target_samp_rate/(4*M_PI* _filter_width));
    _squelch = gr::analog::pwr_squelch_cc::make(-140,0.01,0,true);
    _ctcss = gr::analog::ctcss_squelch_ff::make(8000,88.5,0.02,4000,0,true);
    _amplify = gr::blocks::multiply_const_ff::make(2.0);
    _float_to_short = gr::blocks::float_to_short::make();


    connect(self(),0,_resampler,0);

    connect(_resampler,0,_filter,0);
    connect(_filter,0,self(),0);
    connect(_filter,0,_squelch,0);
    connect(_squelch,0,_fm_demod,0);
    connect(_fm_demod,0,_de_emph_filter,0);
    connect(_de_emph_filter,0,_audio_resampler,0);
    connect(_audio_resampler,0,_amplify,0);
    connect(_amplify,0,self(),1);

}

void gr_demod_nbfm_sdr::calculate_deemph_taps(int sample_rate, double tau)
{
    // code from GNUradio gr-analog/python/analog/fm_emph.py
    // Digital corner frequency
    /**
        #
        # Copyright 2005,2007,2012 Free Software Foundation, Inc.
        #
        # This file is part of GNU Radio
        #
        # SPDX-License-Identifier: GPL-3.0-or-later
        #
        #
    */
    double w_c = 1.0 / tau;

    // Prewarped analog corner frequency
    double w_ca = 2.0 * double(sample_rate) * tanf(w_c / (2.0 * double(sample_rate)));

    // Resulting digital pole, zero, and gain term from the bilinear
    // transformation of H(s) = w_ca / (s + w_ca) to
    // H(z) = b0 (1 - z1 z^-1)/(1 - p1 z^-1)
    double k = -w_ca / (2.0 * double(sample_rate));
    double z1 = -1.0;
    double p1 = (1.0 + k) / (1.0 - k);
    double b0 = -k / (1.0 - k);

    _btaps = { b0 * 1.0, b0 * -z1 };
    _ataps = {      1.0,      -p1 };

    // Since H(s = 0) = 1.0, then H(z = 1) = 1.0 and has 0 dB gain at DC


}


void gr_demod_nbfm_sdr::set_filter_width(int filter_width)
{
    _filter_width = filter_width;
    std::vector<float> filter_taps = gr::filter::firdes::low_pass(
                    1, _target_samp_rate, _filter_width,1200,gr::filter::firdes::WIN_BLACKMAN_HARRIS);

    _filter->set_taps(filter_taps);
    _fm_demod->set_gain(_target_samp_rate/(4*M_PI* _filter_width));
}

void gr_demod_nbfm_sdr::set_squelch(int value)
{
    _squelch->set_threshold(value);
}

void gr_demod_nbfm_sdr::set_ctcss(float value)
{
    if(value == 0)
    {
        try {
            disconnect(_de_emph_filter,0,_ctcss,0);
            disconnect(_ctcss,0,_amplify,0);
            connect(_de_emph_filter,0,_amplify,0);
        }
        catch(std::invalid_argument &e)
        {

        }
    }
    else
    {
        _ctcss->set_frequency(value);
        try {
            disconnect(_de_emph_filter,0,_amplify,0);
            connect(_de_emph_filter,0,_ctcss,0);
            connect(_ctcss,0,_amplify,0);
        }
        catch(std::invalid_argument &e)
        {

        }
    }
}
