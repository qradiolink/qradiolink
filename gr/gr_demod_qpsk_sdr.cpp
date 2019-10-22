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

#include "gr_demod_qpsk_sdr.h"
#include <complex>

gr_demod_qpsk_sdr_sptr make_gr_demod_qpsk_sdr(int sps, int samp_rate, int carrier_freq,
                                          int filter_width)
{
    std::vector<int> signature;
    signature.push_back(sizeof (gr_complex));
    signature.push_back(sizeof (gr_complex));
    signature.push_back(sizeof (char));
    return gnuradio::get_initial_sptr(new gr_demod_qpsk_sdr(signature, sps, samp_rate, carrier_freq,
                                                      filter_width));
}



gr_demod_qpsk_sdr::gr_demod_qpsk_sdr(std::vector<int>signature, int sps, int samp_rate, int carrier_freq,
                                 int filter_width) :
    gr::hier_block2 ("gr_demod_qpsk_sdr",
                      gr::io_signature::make (1, 1, sizeof (gr_complex)),
                      gr::io_signature::makev (3, 3, signature))
{

    int decimation;
    int interpolation;
    if(sps > 4 && sps < 125)
    {
        interpolation = 1;
        decimation = 25;
        _samples_per_symbol = sps*4/25;
        _target_samp_rate = 40000;
    }
    else if(sps >= 125)
    {
        interpolation = 1;
        decimation = 100;
        _samples_per_symbol = sps/25;
        _target_samp_rate = 10000;
    }
    else
    {
        interpolation = 1;
        decimation = 2;
        _samples_per_symbol = sps;
        _target_samp_rate = 500000;
    }
    _samp_rate =samp_rate;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;
    int filter_slope = 1200;
    if(_target_samp_rate > 100000)
        filter_slope = 15000;

    std::vector<int> map;
    map.push_back(0);
    map.push_back(1);
    map.push_back(3);
    map.push_back(2);

    std::vector<int> polys;
    polys.push_back(109);
    polys.push_back(79);


    unsigned int flt_size = 32;
    /*
    gr::digital::constellation_expl_rect::sptr constellation = gr::digital::constellation_expl_rect::make(
                constellation->points(),pre_diff_code,4,2,2,1,1,const_map);
    */

    std::vector<float> taps = gr::filter::firdes::low_pass(1, _samp_rate, _target_samp_rate/2, _target_samp_rate/2,
                                                           gr::filter::firdes::WIN_BLACKMAN_HARRIS);

    _resampler = gr::filter::rational_resampler_base_ccf::make(interpolation, decimation, taps);
    _resampler->set_thread_priority(99);
    _agc = gr::analog::agc2_cc::make(1e-1, 1e-1, 1, 10);
    _filter = gr::filter::fft_filter_ccf::make(1, gr::filter::firdes::low_pass(
                                1, _target_samp_rate, _filter_width, filter_slope,gr::filter::firdes::WIN_BLACKMAN_HARRIS) );
    float gain_mu, omega_rel_limit;
    int filt_length, fll_bw;
    fll_bw = 24;
    if(sps <= 4)
    {
        gain_mu = 0.005;
        omega_rel_limit = 0.001;
        filt_length = 35;
        fll_bw = 2;
    }
    else if(sps >= 125)
    {
        gain_mu = 0.005;
        omega_rel_limit = 0.001;
        filt_length = 21;
        fll_bw = 12;
    }
    else
    {
        gain_mu = 0.001;
        omega_rel_limit = 0.001;
        filt_length = 19;
    }

    _fll = gr::digital::fll_band_edge_cc::make(_samples_per_symbol, 0.35, 32, fll_bw*M_PI/100);
    _shaping_filter = gr::filter::fft_filter_ccf::make(
                1, gr::filter::firdes::root_raised_cosine(1,_target_samp_rate,_target_samp_rate/_samples_per_symbol,0.35,filt_length * _samples_per_symbol));
    _clock_recovery = gr::digital::clock_recovery_mm_cc::make(_samples_per_symbol, 0.025*gain_mu*gain_mu, 0, gain_mu,
                                                              omega_rel_limit);
    std::vector<float> pfb_taps = gr::filter::firdes::root_raised_cosine(_samples_per_symbol,_samples_per_symbol, 1, 0.35, 11 * _samples_per_symbol);
    _clock_sync = gr::digital::pfb_clock_sync_ccf::make(_samples_per_symbol,2*M_PI/100,pfb_taps,flt_size, 0, 1.01, 8);
    _costas_loop = gr::digital::costas_loop_cc::make(2*M_PI/200,4,true);
    _equalizer = gr::digital::cma_equalizer_cc::make(8,2,0.0005,1);

    _diff_phasor = gr::digital::diff_phasor_cc::make();
    const std::complex<float> i(0, 1);
    const std::complex<float> rot(-3 * M_PI/4, 0);
    _rotate_const =  gr::blocks::multiply_const_cc::make(std::exp(i * rot));
    _complex_to_float = gr::blocks::complex_to_float::make();
    _interleave = gr::blocks::interleave::make(4);
    _multiply_const_fec = gr::blocks::multiply_const_ff::make(48);
    _float_to_uchar = gr::blocks::float_to_uchar::make();
    _add_const_fec = gr::blocks::add_const_ff::make(128.0);
    gr::fec::code::cc_decoder::sptr decoder = gr::fec::code::cc_decoder::make(80, 7, 2, polys);
    _decode_ccsds = gr::fec::decoder::make(decoder, 1, 1);
    _descrambler = gr::digital::descrambler_bb::make(0x8A, 0x7F ,7);


    connect(self(),0,_resampler,0);
    connect(_resampler,0,_fll,0);
    connect(_fll,0,_shaping_filter,0);
    connect(_shaping_filter,0,self(),0);
    connect(_shaping_filter,0,_agc,0);
    connect(_agc,0,_clock_recovery,0);
    connect(_clock_recovery,0,_equalizer,0);
    connect(_equalizer,0,_costas_loop,0);
    connect(_costas_loop,0,_diff_phasor,0);
    connect(_diff_phasor,0,_rotate_const,0);
    connect(_rotate_const,0,self(),1);
    connect(_rotate_const,0,_complex_to_float,0);
    connect(_complex_to_float,0,_interleave,0);
    connect(_complex_to_float,1,_interleave,1);
    connect(_interleave,0,_multiply_const_fec,0);
    connect(_multiply_const_fec,0,_add_const_fec,0);
    connect(_add_const_fec,0,_float_to_uchar,0);
    connect(_float_to_uchar,0,_decode_ccsds,0);
    connect(_decode_ccsds,0,_descrambler,0);
    connect(_descrambler,0,self(),2);

}
