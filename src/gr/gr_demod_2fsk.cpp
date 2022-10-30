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

#include "gr_demod_2fsk.h"

gr_demod_2fsk_sptr make_gr_demod_2fsk(int sps, int samp_rate, int carrier_freq,
                                          int filter_width, bool fm)
{
    std::vector<int> signature;
    signature.push_back(sizeof (gr_complex));
    signature.push_back(sizeof (gr_complex));
    signature.push_back(sizeof (char));
    signature.push_back(sizeof (char));
    return gnuradio::get_initial_sptr(new gr_demod_2fsk(signature, sps, samp_rate, carrier_freq,
                                                      filter_width, fm));
}



gr_demod_2fsk::gr_demod_2fsk(std::vector<int>signature, int sps, int samp_rate, int carrier_freq,
                                 int filter_width, bool fm) :
    gr::hier_block2 ("gr_demod_2fsk",
                      gr::io_signature::make (1, 1, sizeof (gr_complex)),
                      gr::io_signature::makev (4, 4, signature))
{
    int decim, interp, nfilts;
    if(sps == 10)
    {
        _target_samp_rate = 10000;
        _samples_per_symbol = sps / 2;
        decim = 100;
        interp = 1;
        nfilts = 35  * _samples_per_symbol;
    }
    else if(sps >= 5)
    {
        _target_samp_rate = 20000;
        _samples_per_symbol = sps;
        decim = 50;
        interp = 1;
        nfilts = 35  * _samples_per_symbol;
    }
    else
    {
        _target_samp_rate = 40000;
        _samples_per_symbol = sps*2;
        decim = 25;
        interp = 1;
        nfilts = 125  * _samples_per_symbol;
    }
    int spacing = 2;
    if(fm)
        spacing = 1;
    if((nfilts % 2) == 0)
        nfilts += 1;

    _samp_rate =samp_rate;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;

    std::vector<int> map;
    map.push_back(0);
    map.push_back(1);

    std::vector<int> polys;
    polys.push_back(109);
    polys.push_back(79);

    std::vector<float> taps = gr::filter::firdes::low_pass(1, _samp_rate, _target_samp_rate/2, _target_samp_rate/2,
                                                           gr::fft::window::WIN_BLACKMAN_HARRIS);
    std::vector<float> symbol_filter_taps = gr::filter::firdes::low_pass(1.0,
                                 _target_samp_rate, _target_samp_rate/_samples_per_symbol,
                                                                         _target_samp_rate/_samples_per_symbol,
                                                                         gr::fft::window::WIN_HAMMING);
    _resampler = gr::filter::rational_resampler_ccf::make(interp, decim, taps);
    _resampler->set_thread_priority(99);
    _fll = gr::digital::fll_band_edge_cc::make(_samples_per_symbol, 0.1, 16, 24*M_PI/100);
    _filter = gr::filter::fft_filter_ccf::make(1, gr::filter::firdes::low_pass(
                                1, _target_samp_rate, _filter_width,_filter_width/2,gr::fft::window::WIN_BLACKMAN_HARRIS) );

    _upper_filter = gr::filter::fft_filter_ccc::make(1, gr::filter::firdes::complex_band_pass(
                                1, _target_samp_rate, -_filter_width,0,_filter_width,gr::fft::window::WIN_BLACKMAN_HARRIS) );
    _lower_filter = gr::filter::fft_filter_ccc::make(1, gr::filter::firdes::complex_band_pass(
                                1, _target_samp_rate, 0,_filter_width,_filter_width,gr::fft::window::WIN_BLACKMAN_HARRIS) );
    _mag_lower = gr::blocks::complex_to_mag::make();
    _mag_upper = gr::blocks::complex_to_mag::make();
    _divide = gr::blocks::divide_ff::make();
    _add = gr::blocks::add_const_ff::make(-1);
    _rail = gr::analog::rail_ff::make(0,2);
    _float_to_complex = gr::blocks::float_to_complex::make();
    _symbol_filter = gr::filter::fft_filter_fff::make(1,symbol_filter_taps);

    float symbol_rate ((float)_target_samp_rate / (float)_samples_per_symbol);
    float sps_deviation = 200.0f / symbol_rate;
    _symbol_sync = gr::digital::symbol_sync_ff::make(gr::digital::TED_MOD_MUELLER_AND_MULLER, _samples_per_symbol,
                                                    2 * M_PI / (symbol_rate / 10), 1.0, 0.2869, sps_deviation, 1,
                                                     gr::digital::constellation_bpsk::make());

    _freq_demod = gr::analog::quadrature_demod_cf::make(_samples_per_symbol/(spacing * M_PI/2));
    _shaping_filter = gr::filter::fft_filter_fff::make(
                1, gr::filter::firdes::root_raised_cosine(1,_target_samp_rate,
                                    _target_samp_rate/_samples_per_symbol,0.2,nfilts));
    _multiply_const_fec = gr::blocks::multiply_const_ff::make(128);
    _float_to_uchar = gr::blocks::float_to_uchar::make();
    _add_const_fec = gr::blocks::add_const_ff::make(128.0);

    gr::fec::code::cc_decoder::sptr decoder = gr::fec::code::cc_decoder::make(80, 7, 2, polys);
    gr::fec::code::cc_decoder::sptr decoder2 = gr::fec::code::cc_decoder::make(80, 7, 2, polys);
    _cc_decoder = gr::fec::decoder::make(decoder, 1, 1);
    _cc_decoder2 = gr::fec::decoder::make(decoder2, 1, 1);

    _delay = gr::blocks::delay::make(1,1);
    _descrambler = gr::digital::descrambler_bb::make(0x8A, 0x7F ,7);
    _descrambler2 = gr::digital::descrambler_bb::make(0x8A, 0x7F ,7);


    connect(self(),0,_resampler,0);
    connect(_resampler,0,_fll,0);
    connect(_fll,0,_filter,0);
    connect(_filter,0,self(),0);
    if(fm)
    {
        connect(_filter,0,_freq_demod,0);
        connect(_freq_demod,0,_shaping_filter,0);
        connect(_shaping_filter,0,_symbol_sync,0);
    }
    else
    {
        connect(_filter,0,_lower_filter,0);
        connect(_filter,0,_upper_filter,0);
        connect(_lower_filter,0,_mag_lower,0);
        connect(_upper_filter,0,_mag_upper,0);
        connect(_mag_lower,0,_divide,1);
        connect(_mag_upper,0,_divide,0);
        connect(_divide,0,_rail,0);
        connect(_rail,0,_add,0);
        connect(_add,0,_symbol_filter,0);
        connect(_symbol_filter,0,_symbol_sync,0);
    }
    connect(_symbol_sync,0,_float_to_complex,0);
    connect(_float_to_complex,0,self(),1);
    connect(_symbol_sync,0,_multiply_const_fec,0);
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



