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

#include "gr_demod_bpsk_sdr.h"

gr_demod_bpsk_sdr_sptr make_gr_demod_bpsk_sdr(int sps, int samp_rate, int carrier_freq,
                                          int filter_width)
{
    std::vector<int> signature;
    signature.push_back(sizeof (gr_complex));
    signature.push_back(sizeof (gr_complex));
    signature.push_back(sizeof (char));
    signature.push_back(sizeof (char));
    return gnuradio::get_initial_sptr(new gr_demod_bpsk_sdr(signature, sps, samp_rate, carrier_freq,
                                                      filter_width));
}



gr_demod_bpsk_sdr::gr_demod_bpsk_sdr(std::vector<int>signature, int sps, int samp_rate, int carrier_freq,
                                 int filter_width) :
    gr::hier_block2 ("gr_demod_bpsk_sdr",
                      gr::io_signature::make (1, 1, sizeof (gr_complex)),
                      gr::io_signature::makev (4, 4, signature))
{

    _target_samp_rate = 20000;
    _samples_per_symbol = sps/25;
    _samp_rate =samp_rate;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;

    std::vector<int> polys;
    polys.push_back(109);
    polys.push_back(79);


    std::vector<float> taps = gr::filter::firdes::low_pass(1, _samp_rate, _target_samp_rate/2, _target_samp_rate/2,
                                                           gr::filter::firdes::WIN_BLACKMAN_HARRIS);
    _resampler = gr::filter::rational_resampler_base_ccf::make(1, 50, taps);
    _resampler->set_thread_priority(99);
    _agc = gr::analog::agc2_cc::make(1e-1, 1e-1, 1, 10);
    _filter = gr::filter::fft_filter_ccf::make(1, gr::filter::firdes::low_pass(
                            1, _target_samp_rate, _filter_width,1200,gr::filter::firdes::WIN_BLACKMAN_HARRIS) );
    float gain_mu = 0.025;
    _clock_recovery = gr::digital::clock_recovery_mm_cc::make(_samples_per_symbol, 0.025*gain_mu*gain_mu, 0, gain_mu,
                                                              0.001);
    _costas_loop = gr::digital::costas_loop_cc::make(2*M_PI/100,2);
    _equalizer = gr::digital::cma_equalizer_cc::make(8,1,0.00005,1);
    _fll = gr::digital::fll_band_edge_cc::make(_samples_per_symbol, 0.35, 32, 24*M_PI/100);
    _shaping_filter = gr::filter::fft_filter_ccf::make(
                1, gr::filter::firdes::root_raised_cosine(_samples_per_symbol,_samples_per_symbol,1,0.35,15 * _samples_per_symbol));
    _complex_to_real = gr::blocks::complex_to_real::make();

    _multiply_const_fec = gr::blocks::multiply_const_ff::make(64);
    _float_to_uchar = gr::blocks::float_to_uchar::make();
    _add_const_fec = gr::blocks::add_const_ff::make(128.0);

    gr::fec::code::cc_decoder::sptr decoder = gr::fec::code::cc_decoder::make(80, 7, 2, polys);
    gr::fec::code::cc_decoder::sptr decoder2 = gr::fec::code::cc_decoder::make(80, 7, 2, polys);
    _cc_decoder = gr::fec::decoder::make(decoder, 1, 1);
    _cc_decoder2 = gr::fec::decoder::make(decoder2, 1, 1);

    _descrambler = gr::digital::descrambler_bb::make(0x8A, 0x7F ,7);
    _delay = gr::blocks::delay::make(1,1);
    _descrambler2 = gr::digital::descrambler_bb::make(0x8A, 0x7F ,7);



    connect(self(),0,_resampler,0);
    connect(_resampler,0,_fll,0);
    connect(_fll,0,_shaping_filter,0);
    connect(_shaping_filter,0,self(),0);
    connect(_shaping_filter,0,_agc,0);
    connect(_agc,0,_clock_recovery,0);
    connect(_clock_recovery,0,_equalizer,0);
    connect(_equalizer,0,_costas_loop,0);
    connect(_costas_loop,0,_complex_to_real,0);
    connect(_costas_loop,0,self(),1);
    connect(_complex_to_real,0,_multiply_const_fec,0);
    connect(_multiply_const_fec,0,_add_const_fec,0);
    connect(_add_const_fec,0,_float_to_uchar,0);
    connect(_float_to_uchar,0,_cc_decoder,0);
    connect(_cc_decoder,0,_descrambler,0);
    connect(_descrambler,0,self(),2);
    connect(_float_to_uchar,0,_delay,0);
    connect(_delay,0,_cc_decoder2,0);
    connect(_cc_decoder2,0,_descrambler2,0);
    connect(_descrambler2,0,self(),3);

}


