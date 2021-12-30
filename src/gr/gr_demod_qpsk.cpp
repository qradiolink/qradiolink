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

#include "gr_demod_qpsk.h"
#include <complex>

gr_demod_qpsk_sptr make_gr_demod_qpsk(int sps, int samp_rate, int carrier_freq,
                                          int filter_width)
{
    std::vector<int> signature;
    signature.push_back(sizeof (gr_complex));
    signature.push_back(sizeof (gr_complex));
    signature.push_back(sizeof (char));
    return gnuradio::get_initial_sptr(new gr_demod_qpsk(signature, sps, samp_rate, carrier_freq,
                                                      filter_width));
}



gr_demod_qpsk::gr_demod_qpsk(std::vector<int>signature, int sps, int samp_rate, int carrier_freq,
                                 int filter_width) :
    gr::hier_block2 ("gr_demod_qpsk",
                      gr::io_signature::make (1, 1, sizeof (gr_complex)),
                      gr::io_signature::makev (3, 3, signature))
{

    int decimation;
    int interpolation;
    float costas_bw;
    int fll_bw;
    fll_bw = 2;
    costas_bw = M_PI/200;


    ////////////////////////////

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
        _samples_per_symbol = sps; // FIXME: this value should not be hardcoded
        _target_samp_rate = 500000;
        costas_bw = M_PI/400;
    }
    _samp_rate =samp_rate;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;

    std::vector<int> map;
    map.push_back(0);
    map.push_back(1);
    map.push_back(3);
    map.push_back(2);

    std::vector<int> polys;
    polys.push_back(109);
    polys.push_back(79);


    /*
    gr::digital::constellation_expl_rect::sptr constellation = gr::digital::constellation_expl_rect::make(
                constellation->points(),pre_diff_code,4,2,2,1,1,const_map);
    */

    std::vector<float> taps = gr::filter::firdes::low_pass_2(interpolation, _samp_rate * interpolation, _target_samp_rate/2,
                            _target_samp_rate/10, 60, gr::filter::firdes::WIN_BLACKMAN_HARRIS);

    _resampler = gr::filter::rational_resampler_base_ccf::make(interpolation, decimation, taps);
    _resampler->set_thread_priority(99);
    _agc = gr::analog::agc2_cc::make(1, 1e-1, 1.0, 1.0);

    _fll = gr::digital::fll_band_edge_cc::make(_samples_per_symbol, 0.35, 32, fll_bw*M_PI/100);
    std::vector<float> rrc_taps = gr::filter::firdes::root_raised_cosine(_samples_per_symbol,
                                                                         _samples_per_symbol,1,0.35, 11 * _samples_per_symbol);
    _shaping_filter = gr::filter::fft_filter_ccf::make(
                1, rrc_taps);

    float symbol_rate = (float)_target_samp_rate / (float)_samples_per_symbol;
    float sps_deviation = 200.0f / symbol_rate;
    _symbol_sync = gr::digital::symbol_sync_cc::make(gr::digital::TED_MOD_MUELLER_AND_MULLER, _samples_per_symbol,
                                                    2* M_PI * 0.001, 1.0, 1.0, sps_deviation, 1,
                                                     gr::digital::constellation_dqpsk::make(), gr::digital::IR_MMSE_8TAP);
    _costas_pll = gr::digital::costas_loop_cc::make(M_PI/200/_samples_per_symbol,4,true);

    _costas_loop = gr::digital::costas_loop_cc::make(costas_bw,4,true);
    _equalizer = gr::digital::cma_equalizer_cc::make(8,1,0.005,2);

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
    if(sps > 4)
    {
        connect(_resampler,0,_fll,0);
        connect(_fll,0,_shaping_filter,0);
    }
    else
    {
        connect(_resampler,0,_shaping_filter,0);
    }

    connect(_shaping_filter,0,_agc,0);
    connect(_shaping_filter,0,self(),0);
    connect(_agc,0,_costas_pll,0);
    connect(_costas_pll,0,_symbol_sync,0);
    connect(_symbol_sync,0,_costas_loop,0);
    //connect(_equalizer,0,_costas_loop,0);
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
