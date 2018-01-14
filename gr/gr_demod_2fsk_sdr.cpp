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

#include "gr_demod_2fsk_sdr.h"

gr_demod_2fsk_sdr_sptr make_gr_demod_2fsk_sdr(int sps, int samp_rate, int carrier_freq,
                                          int filter_width)
{
    std::vector<int> signature;
    signature.push_back(sizeof (gr_complex));
    signature.push_back(sizeof (gr_complex));
    return gnuradio::get_initial_sptr(new gr_demod_2fsk_sdr(signature, sps, samp_rate, carrier_freq,
                                                      filter_width));
}



gr_demod_2fsk_sdr::gr_demod_2fsk_sdr(std::vector<int>signature, int sps, int samp_rate, int carrier_freq,
                                 int filter_width) :
    gr::hier_block2 ("gr_demod_2fsk_sdr",
                      gr::io_signature::make (1, 1, sizeof (gr_complex)),
                      gr::io_signature::makev (2, 2, signature))
{

    _target_samp_rate = 40000;

    _samples_per_symbol = sps*2/25;
    _samp_rate =samp_rate;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;

    std::vector<int> map;
    map.push_back(0);
    map.push_back(1);

    std::vector<float> taps = gr::filter::firdes::low_pass(32, _samp_rate, _filter_width, 12000);
    std::vector<float> symbol_filter_taps = gr::filter::firdes::low_pass(1.0,
                                 _target_samp_rate, _target_samp_rate*0.9/_samples_per_symbol, _target_samp_rate*0.1/_samples_per_symbol);
    _resampler = gr::filter::rational_resampler_base_ccf::make(1, 25, taps);
    //_freq_transl_filter = gr::filter::freq_xlating_fir_filter_ccf::make(
    //            1,gr::filter::firdes::low_pass(
    //                1, _target_samp_rate, 2*_filter_width, 250000, gr::filter::firdes::WIN_HAMMING), 25000,
    //            _target_samp_rate);
    _filter = gr::filter::fft_filter_ccf::make(1, gr::filter::firdes::low_pass(
                                1, _target_samp_rate, _filter_width,1200,gr::filter::firdes::WIN_HAMMING) );

    _upper_filter = gr::filter::fft_filter_ccc::make(1, gr::filter::firdes::complex_band_pass(
                                1, _target_samp_rate, -_filter_width,0,600,gr::filter::firdes::WIN_HAMMING) );
    _lower_filter = gr::filter::fft_filter_ccc::make(1, gr::filter::firdes::complex_band_pass(
                                1, _target_samp_rate, 0,_filter_width,600,gr::filter::firdes::WIN_HAMMING) );
    _mag_squared_lower = gr::blocks::complex_to_mag_squared::make();
    _mag_squared_upper = gr::blocks::complex_to_mag_squared::make();
    _divide = gr::blocks::divide_ff::make();
    _add = gr::blocks::add_const_ff::make(-0.5);
    _threshhold = gr::blocks::threshold_ff::make(0.99999,1.00001);
    _float_to_complex = gr::blocks::float_to_complex::make();
    _symbol_filter = gr::filter::fft_filter_ccf::make(1,symbol_filter_taps);
    float gain_mu = 0.025;
    _clock_recovery = gr::digital::clock_recovery_mm_cc::make(_samples_per_symbol, 0.025*gain_mu*gain_mu, 0.5, gain_mu,
                                                              0.0005);

    _multiply_const_fec = gr::blocks::multiply_const_ff::make(0.5);
    _add_const_fec = gr::blocks::add_const_ff::make(0.0);


    gr::fec::decode_ccsds_27_fb::sptr _cc_decoder = gr::fec::decode_ccsds_27_fb::make();
    gr::fec::decode_ccsds_27_fb::sptr _cc_decoder2 = gr::fec::decode_ccsds_27_fb::make();


    _complex_to_real = gr::blocks::complex_to_real::make();
    _binary_slicer = gr::digital::binary_slicer_fb::make();



    _packed_to_unpacked = gr::blocks::packed_to_unpacked_bb::make(1,gr::GR_MSB_FIRST);
    _packed_to_unpacked2 = gr::blocks::packed_to_unpacked_bb::make(1,gr::GR_MSB_FIRST);
    _delay = gr::blocks::delay::make(4,1);
    _descrambler = gr::digital::descrambler_bb::make(0x8A, 0x7F ,7);
    _descrambler2 = gr::digital::descrambler_bb::make(0x8A, 0x7F ,7);
    _deframer1 = make_gr_deframer_bb(1);
    _deframer2 = make_gr_deframer_bb(1);


    connect(self(),0,_resampler,0);
    connect(_resampler,0,_filter,0);
    connect(_filter,0,self(),0);
    connect(_filter,0,_lower_filter,0);
    connect(_filter,0,_upper_filter,0);
    connect(_lower_filter,0,_mag_squared_lower,0);
    connect(_upper_filter,0,_mag_squared_upper,0);
    connect(_mag_squared_lower,0,_divide,1);
    connect(_mag_squared_upper,0,_divide,0);
    connect(_divide,0,_threshhold,0);
    connect(_threshhold,0,_add,0);
    connect(_add,0,_float_to_complex,0);
    connect(_float_to_complex,0,_symbol_filter,0);
    connect(_symbol_filter,0,_clock_recovery,0);
    connect(_clock_recovery,0,self(),1);

    connect(_clock_recovery,0,_complex_to_real,0);
    //_top_block->connect(_complex_to_real,0,_binary_slicer,0);
    connect(_complex_to_real,0,_multiply_const_fec,0);
    connect(_multiply_const_fec,0,_add_const_fec,0);
    connect(_add_const_fec,0,_cc_decoder,0);
    connect(_cc_decoder,0,_packed_to_unpacked,0);
    connect(_packed_to_unpacked,0,_descrambler,0);
    connect(_descrambler,0,_deframer1,0);

    connect(_add_const_fec,0,_delay,0);
    connect(_delay,0,_cc_decoder2,0);
    connect(_cc_decoder2,0,_packed_to_unpacked2,0);
    connect(_packed_to_unpacked2,0,_descrambler2,0);
    connect(_descrambler2,0,_deframer2,0);

}

std::vector<unsigned char>* gr_demod_2fsk_sdr::getFrame1()
{
    std::vector<unsigned char> *data = _deframer1->get_data();
    return data;
}

std::vector<unsigned char>* gr_demod_2fsk_sdr::getFrame2()
{
    std::vector<unsigned char> *data = _deframer2->get_data();
    return data;
}


