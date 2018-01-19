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


    std::vector<float> taps = gr::filter::firdes::low_pass(1, _samp_rate, _filter_width, 12000);
    _resampler = gr::filter::rational_resampler_base_ccf::make(1, 50, taps);
    _agc = gr::analog::agc2_cc::make(10, 1e-1, 1, 1);
    _filter = gr::filter::fft_filter_ccf::make(1, gr::filter::firdes::low_pass(
                            1, _target_samp_rate, _filter_width,1200,gr::filter::firdes::WIN_HAMMING) );
    float gain_mu = 0.025;
    _clock_recovery = gr::digital::clock_recovery_mm_cc::make(_samples_per_symbol, 0.025*gain_mu*gain_mu, 0, gain_mu,
                                                              0.001);
    _costas_loop = gr::digital::costas_loop_cc::make(2*M_PI/100,2);
    _equalizer = gr::digital::cma_equalizer_cc::make(8,1,0.00005,1);
    _fll = gr::digital::fll_band_edge_cc::make(sps, 0.35, 32, 2*M_PI/100);
    _shaping_filter = gr::filter::fft_filter_ccf::make(
                1, gr::filter::firdes::root_raised_cosine(1,_target_samp_rate,_target_samp_rate/_samples_per_symbol,0.35,32));
    _complex_to_real = gr::blocks::complex_to_real::make();
    _packed_to_unpacked = gr::blocks::packed_to_unpacked_bb::make(1,gr::GR_MSB_FIRST);
    _packed_to_unpacked2 = gr::blocks::packed_to_unpacked_bb::make(1,gr::GR_MSB_FIRST);

    _cc_decoder = gr::fec::decode_ccsds_27_fb::make();
    _cc_decoder2 = gr::fec::decode_ccsds_27_fb::make();

    _multiply_const_fec = gr::blocks::multiply_const_ff::make(0.5);

    _descrambler = gr::digital::descrambler_bb::make(0x8A, 0x7F ,7);
    _delay = gr::blocks::delay::make(4,1);
    _descrambler2 = gr::digital::descrambler_bb::make(0x8A, 0x7F ,7);



    connect(self(),0,_resampler,0);
    connect(_resampler,0,_filter,0);
    connect(_filter,0,self(),0);
    connect(_filter,0,_fll,0);
    connect(_fll,0,_agc,0);
    //connect(_shaping_filter,0,_agc,0);
    connect(_agc,0,_clock_recovery,0);
    connect(_clock_recovery,0,_costas_loop,0);
    connect(_costas_loop,0,_equalizer,0);
    connect(_equalizer,0,_complex_to_real,0);
    connect(_equalizer,0,self(),1);
    connect(_complex_to_real,0,_multiply_const_fec,0);
    connect(_multiply_const_fec,0,_cc_decoder,0);
    connect(_cc_decoder,0,_packed_to_unpacked,0);
    connect(_packed_to_unpacked,0,_descrambler,0);
    connect(_descrambler,0,self(),2);
    connect(_multiply_const_fec,0,_delay,0);
    connect(_delay,0,_cc_decoder2,0);
    connect(_cc_decoder2,0,_packed_to_unpacked2,0);
    connect(_packed_to_unpacked2,0,_descrambler2,0);
    connect(_descrambler2,0,self(),3);

}


