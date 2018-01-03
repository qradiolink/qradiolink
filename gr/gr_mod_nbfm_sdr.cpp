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

#include "gr_mod_nbfm_sdr.h"

gr_mod_nbfm_sdr_sptr make_gr_mod_nbfm_sdr(int sps, int samp_rate, int carrier_freq,
                                          int filter_width)
{
    return gnuradio::get_initial_sptr(new gr_mod_nbfm_sdr(sps, samp_rate, carrier_freq,
                                                      filter_width));
}

gr_mod_nbfm_sdr::gr_mod_nbfm_sdr(int sps, int samp_rate, int carrier_freq,
                                 int filter_width) :
    gr::hier_block2 ("gr_mod_nbfm_sdr",
                      gr::io_signature::make (1, 1, sizeof (float)),
                      gr::io_signature::make (1, 1, sizeof (gr_complex)))
{

    _samp_rate =samp_rate;
    float target_samp_rate = 8000;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;


    _fm_modulator = gr::analog::frequency_modulator_fc::make(4*M_PI*_filter_width/target_samp_rate);

    _audio_amplify = gr::blocks::multiply_const_ff::make(0.9,1);
    _audio_filter = gr::filter::fft_filter_fff::make(
                1,gr::filter::firdes::band_pass(
                    1, target_samp_rate, 300, _filter_width, 600, gr::filter::firdes::WIN_HAMMING));

    static const float coeff[] =  {-0.026316914707422256, -0.2512197494506836, 1.5501943826675415,
                                   -0.2512197494506836, -0.026316914707422256};
    std::vector<float> iir_taps(coeff, coeff + sizeof(coeff) / sizeof(coeff[0]) );
    _emphasis_filter = gr::filter::fft_filter_fff::make(1,iir_taps);

    _tone_source = gr::analog::sig_source_f::make(target_samp_rate,gr::analog::GR_COS_WAVE,88.5,0.1);
    _add = gr::blocks::add_ff::make();

    std::vector<float> interp_taps = gr::filter::firdes::low_pass(1, target_samp_rate,
                                                        _filter_width, 2000);
    float rerate = (float)_samp_rate/target_samp_rate;
    _resampler = gr::filter::pfb_arb_resampler_ccf::make(rerate, interp_taps, 32);
    _amplify = gr::blocks::multiply_const_cc::make(10,1);
    _filter = gr::filter::fft_filter_ccf::make(
                1,gr::filter::firdes::low_pass(
                    1, _samp_rate, _filter_width, 1200, gr::filter::firdes::WIN_HAMMING));


    connect(self(),0,_audio_filter,0);
    connect(_audio_filter,0,_audio_amplify,0);
    connect(_audio_amplify,0,_emphasis_filter,0);
    connect(_emphasis_filter,0,_fm_modulator,0);
    connect(_fm_modulator,0,_resampler,0);
    connect(_resampler,0,_amplify,0);
    connect(_amplify,0,_filter,0);

    connect(_filter,0,self(),0);
}


void gr_mod_nbfm_sdr::set_ctcss(float value)
{
    if(value == 0)
    {
        try {
            disconnect(_emphasis_filter,0,_add,0);
            disconnect(_add,0,_fm_modulator,0);
            disconnect(_tone_source,0,_add,1);
            connect(_emphasis_filter,0,_fm_modulator,0);
        }
        catch(std::invalid_argument e)
        {
        }
    }
    else
    {
        _tone_source->set_frequency(value);
        try {
            disconnect(_emphasis_filter,0,_fm_modulator,0);
            connect(_emphasis_filter,0,_add,0);
            connect(_add,0,_fm_modulator,0);
            connect(_tone_source,0,_add,1);
        }
        catch(std::invalid_argument e)
        {
        }
    }
}
