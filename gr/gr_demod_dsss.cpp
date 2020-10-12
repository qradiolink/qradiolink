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

#include "gr_demod_dsss.h"

gr_demod_dsss_sptr make_gr_demod_dsss(int sps, int samp_rate, int carrier_freq,
                                          int filter_width)
{
    std::vector<int> signature;
    signature.push_back(sizeof (gr_complex));
    signature.push_back(sizeof (gr_complex));
    signature.push_back(sizeof (char));
    signature.push_back(sizeof (char));
    return gnuradio::get_initial_sptr(new gr_demod_dsss(signature, sps, samp_rate, carrier_freq,
                                                      filter_width));
}



gr_demod_dsss::gr_demod_dsss(std::vector<int>signature, int sps, int samp_rate, int carrier_freq,
                                 int filter_width) :
    gr::hier_block2 ("gr_demod_dsss",
                      gr::io_signature::make (1, 1, sizeof (gr_complex)),
                      gr::io_signature::makev (4, 4, signature))
{
    _if_samp_rate = 20000;
    _target_samp_rate = 5200;
    _samples_per_symbol = sps;
    _samp_rate =samp_rate;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;
    static const int barker_13[] = {1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 0, 1};
    std::vector<int> dsss_code (barker_13, barker_13 + sizeof(barker_13) / sizeof(barker_13[0]) );

    std::vector<int> polys;
    polys.push_back(109);
    polys.push_back(79);
    float gain_omega, gain_mu, omega_rel_limit;
    gain_omega = 0.005;
    gain_mu = 0.05;
    omega_rel_limit = 0.005;


    std::vector<float> taps = gr::filter::firdes::low_pass(1, _samp_rate, _if_samp_rate/2, _if_samp_rate/2,
                                                           gr::filter::firdes::WIN_BLACKMAN_HARRIS);
    _resampler = gr::filter::rational_resampler_base_ccf::make(1, 50, taps);
    _resampler->set_thread_priority(99);

    std::vector<float> taps_if = gr::filter::firdes::low_pass(1, _if_samp_rate, _target_samp_rate/2, _target_samp_rate/2,
                                                           gr::filter::firdes::WIN_BLACKMAN_HARRIS);
    _resampler_if = gr::filter::rational_resampler_base_ccf::make(13, 50, taps_if);

    _agc = gr::analog::agc2_cc::make(1e-1, 1e-1, 1, 10);
    _filter = gr::filter::fft_filter_ccf::make(1, gr::filter::firdes::low_pass(
                            1, _target_samp_rate, _filter_width,1200,gr::filter::firdes::WIN_BLACKMAN_HARRIS) );
    _costas_loop = gr::digital::costas_loop_cc::make(2*M_PI/100,2);
    _costas_freq = gr::digital::costas_loop_cc::make(M_PI/200,2,true);
    _dsss_decoder = gr::dsss::dsss_decoder_cc::make(dsss_code, _samples_per_symbol);
    _complex_to_real = gr::blocks::complex_to_real::make();
    _clock_recovery = gr::digital::clock_recovery_mm_cc::make(1,
                gain_omega*gain_omega, 0.5, gain_mu, omega_rel_limit);

    _multiply_const_fec = gr::blocks::multiply_const_ff::make(64);
    _float_to_uchar = gr::blocks::float_to_uchar::make();
    _add_const_fec = gr::blocks::add_const_ff::make(128.0);

    gr::fec::code::cc_decoder::sptr decoder = gr::fec::code::cc_decoder::make(80, 7, 2, polys);
    gr::fec::code::cc_decoder::sptr decoder2 = gr::fec::code::cc_decoder::make(80, 7, 2, polys);
    _cc_decoder = gr::fec::decoder::make(decoder, 1, 1);
    _cc_decoder2 = gr::fec::decoder::make(decoder2, 1, 1);

    _ccsds_decoder = gr::fec::decode_ccsds_27_fb::make();
    _ccsds_decoder2 = gr::fec::decode_ccsds_27_fb::make();
    _packed_to_unpacked = gr::blocks::packed_to_unpacked_bb::make(1, gr::GR_MSB_FIRST);
    _packed_to_unpacked2 = gr::blocks::packed_to_unpacked_bb::make(1, gr::GR_MSB_FIRST);

    _descrambler = gr::digital::descrambler_bb::make(0x8A, 0x7F ,7);
    _delay = gr::blocks::delay::make(1,1);
    _descrambler2 = gr::digital::descrambler_bb::make(0x8A, 0x7F ,7);



    connect(self(),0,_resampler,0);
    connect(_resampler,0,_resampler_if,0);
    connect(_resampler_if,0,_costas_freq,0);
    connect(_costas_freq,0,_filter,0);
    connect(_filter,0,_agc,0);
    connect(_filter,0,self(),0);
    connect(_agc,0,_dsss_decoder,0);
    connect(_dsss_decoder,0,_clock_recovery,0);
    connect(_clock_recovery,0,_costas_loop,0);
    connect(_costas_loop,0,_complex_to_real,0);
    connect(_costas_loop,0,self(),1);
    connect(_complex_to_real,0,_multiply_const_fec,0);
    connect(_multiply_const_fec,0,_add_const_fec,0);
    connect(_add_const_fec,0,_float_to_uchar,0);
    connect(_float_to_uchar,0,_cc_decoder,0);
    connect(_cc_decoder,0,_descrambler,0);
    //connect(_ccsds_decoder,0,_packed_to_unpacked,0);
    //connect(_packed_to_unpacked,0,_descrambler,0);
    connect(_descrambler,0,self(),2);
    connect(_float_to_uchar,0,_delay,0);
    connect(_delay,0,_cc_decoder2,0);
    connect(_cc_decoder2,0,_descrambler2,0);
    //connect(_complex_to_real,0,_delay,0);
    //connect(_delay,0,_ccsds_decoder2,0);
    //connect(_ccsds_decoder2,0,_packed_to_unpacked2,0);
    //connect(_packed_to_unpacked2,0,_descrambler2,0);
    connect(_descrambler2,0,self(),3);

}


